/**
 ******************************************************************************
 * @file    display.c
 * @author  Edin & Gemini
 * @version V2.1.0
 * @date    18.08.2025.
 * @brief   Implementacija GUI logike i upravljanja ekranima.
 *
 * @note
 * Ovaj fajl sadrži kompletnu logiku za iscrtavanje svih ekrana definisanih
 * u `eScreen` enumu. Koristi STemWin (emWin) grafičku biblioteku za
 * kreiranje i upravljanje widgetima (dugmad, spinbox-ovi, itd.).
 *
 * Glavna servisna petlja `DISP_Service()` se poziva iz `main.c` i funkcioniše
 * kao state-machine, pozivajući odgovarajuću `Service_...Screen()` funkciju
 * u zavisnosti od trenutno aktivnog ekrana.
 *
 * Funkcija `PID_Hook()` je centralna tačka za obradu korisničkog unosa
 * (dodira na ekran) i prosljeđuje događaje odgovarajućim handlerima.
 *
 * Fajl takođe sadrži logiku za internacionalizaciju (prevođenje tekstova)
 * i upravljanje pozadinskim procesima kao što je screensaver.
 *
 ******************************************************************************
 * @attention
 *
 * (C) COPYRIGHT 2025 JUBERA d.o.o. Sarajevo
 *
 * Sva prava zadržana.
 *
 ******************************************************************************
 */

/* Provjera verzije build-a kako bi se osigurala konzistentnost sa header fajlom */
#if (__DISPH__ != FW_BUILD)
#error "display header version mismatch"
#endif

/*============================================================================*/
/* UKLJUČENI FAJLOVI (INCLUDES)                                               */
/*============================================================================*/
// --- Standardni i sistemski headeri ---
#include "main.h"
#include "display.h"
#include "stm32746g_eeprom.h"

// --- Headeri drugih modula (za pozivanje njihovih API-ja) ---
#include "thermostat.h"
#include "ventilator.h"
#include "defroster.h"
#include "curtain.h"
#include "lights.h"
#include "rs485.h"
#include "gate.h"
#include "scene.h"
#include "translations.h"

/*============================================================================*/
/* PRIVATNE DEFINICIJE I MAKROI (INTERNI)                                     */
/*============================================================================*/

/** @name Vremenske konstante za GUI
 * @{
 */
#define GUI_REFRESH_TIME                100U    ///< Svrha: Period osvježavanja GUI-ja. Vrijednost: 100 milisekundi (10 puta u sekundi).
#define DATE_TIME_REFRESH_TIME          1000U   ///< Svrha: Period osvježavanja prikaza datuma i vremena. Vrijednost: 1000 milisekundi (svake sekunde).
#define SETTINGS_MENU_ENABLE_TIME       3456U   ///< Svrha: Vrijeme držanja pritiska za ulazak u meni. Vrijednost: 3456 milisekundi (~3.5 sekunde).
#define SETTINGS_MENU_TIMEOUT           59000U  ///< Svrha: Timeout za automatski izlazak iz menija. Vrijednost: 59000 milisekundi (59 sekundi).
#define EVENT_ONOFF_TOUT                500     ///< Svrha: Maksimalno vrijeme za "kratak dodir". Vrijednost: 500 milisekundi.
#define VALUE_STEP_TOUT                 15      ///< Svrha: Brzina promjene vrijednosti kod držanja dugmeta (npr. dimovanje). Vrijednost: 15 milisekundi.
#define GHOST_WIDGET_SCAN_INTERVAL      2000    ///< Svrha: Period za skeniranje i brisanje "duhova" (zaostalih widgeta). Vrijednost: 2000 milisekundi.
#define FW_UPDATE_BUS_TIMEOUT           15000U   ///< Svrha: Vrijeme (u ms) nakon kojeg smatramo da je FW update na busu završen ako nema novih paketa.
/** @} */

/** @name Konfiguracija ekrana i prikaza
 * @{
 */
#define DISP_BRGHT_MAX                  80      ///< Svrha: Maksimalna dozvoljena vrijednost za svjetlinu ekrana. Vrijednost: 80 (na skali 1-90).
#define DISP_BRGHT_MIN                  5       ///< Svrha: Minimalna dozvoljena vrijednost za svjetlinu ekrana. Vrijednost: 5 (na skali 1-90).
#define QR_CODE_COUNT                   2       ///< Svrha: Ukupan broj QR kodova koje sistem podržava. Vrijednost: 2 (jedan za WiFi, jedan za App).
#define QR_CODE_LENGTH                  50      ///< Svrha: Maksimalna dužina stringa za QR kod. Vrijednost: 50 karaktera.
#define DRAWING_AREA_WIDTH              380     ///< Svrha: Širina glavnog područja za crtanje. Vrijednost: 380 piksela (cijeli ekran je 480px).
#define COLOR_BSIZE                     28      ///< Svrha: Veličina `clk_clrs` niza. Vrijednost: 28, mora odgovarati broju boja u nizu.
/** @} */

/** @name Definicije za ikonice svjetala
 * @note Premješteno iz lights.h, privatno za display modul.
 * @{
 */
#define LIGHT_ICON_COUNT                10       ///< Svrha: Ukupan broj različitih tipova ikonica za svjetla. Vrijednost: 2 (sijalica i ventilator).
#define LIGHT_ICON_ID_BULB              0       ///< Svrha: Jedinstveni ID za ikonicu sijalice. Vrijednost: 0.
#define LIGHT_ICON_ID_VENTILATOR        1       ///< Svrha: Jedinstveni ID za ikonicu ventilatora. Vrijednost: 1.
#define LIGHT_ICON_ID_CEILING_LED_FIXTURE 2
#define LIGHT_ICON_ID_CHANDELIER        3
#define LIGHT_ICON_ID_HANGING           4
#define LIGHT_ICON_ID_LED_STRIP         5
#define LIGHT_ICON_ID_SPOT_CONSOLE      6
#define LIGHT_ICON_ID_SPOT_SINGLE       7
#define LIGHT_ICON_ID_STAIRS            8
#define LIGHT_ICON_ID_WALL              9
/** @} */
/* Privatne definicije... */
#define PIN_MASK_DELAY 2000 // 2 sekunde prije maskiranja karaktera
#define MAX_PIN_LENGTH 8    // << NOVO: Maksimalna dužina PIN-a

/******************************************************************************
 * @brief       Definicije za raspored alfanumeričke tastature.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ove konstante se koriste za dinamičko kreiranje tastature
 * u `DSP_InitKeyboardScreen` funkciji.
 *****************************************************************************/
#define KEY_ROWS 4          // Broj redova sa karakterima
#define KEYS_PER_ROW 10     // Maksimalan broj tastera po redu
#define KEY_SHIFT_STATES 2  // Broj stanja (0 = mala slova, 1 = VELIKA SLOVA)


/* Definicije ID-jeva za PIN tastaturu */
#define ID_PINPAD_BASE      (GUI_ID_USER + 100)
#define ID_PINPAD_1         (ID_PINPAD_BASE + 1)
#define ID_PINPAD_2         (ID_PINPAD_BASE + 2)
#define ID_PINPAD_3         (ID_PINPAD_BASE + 3)
#define ID_PINPAD_4         (ID_PINPAD_BASE + 4)
#define ID_PINPAD_5         (ID_PINPAD_BASE + 5)
#define ID_PINPAD_6         (ID_PINPAD_BASE + 6)
#define ID_PINPAD_7         (ID_PINPAD_BASE + 7)
#define ID_PINPAD_8         (ID_PINPAD_BASE + 8)
#define ID_PINPAD_9         (ID_PINPAD_BASE + 9)
#define ID_PINPAD_0         (ID_PINPAD_BASE + 0)
#define ID_PINPAD_DEL       (ID_PINPAD_BASE + 10)
#define ID_PINPAD_OK        (ID_PINPAD_BASE + 11)
#define ID_PINPAD_TEXT      (ID_PINPAD_BASE + 12)

// DODATI SA OSTALIM #define-ovima
#define ID_KEYBOARD_BASE    (GUI_ID_USER + 200) // Baza za ID-jeve specijalnih tastera
#define GUI_ID_SHIFT        (ID_KEYBOARD_BASE + 0)
#define GUI_ID_SPACE        (ID_KEYBOARD_BASE + 1)
#define GUI_ID_BACKSPACE    (ID_KEYBOARD_BASE + 2)
#define GUI_ID_OKAY         (ID_KEYBOARD_BASE + 3)

// DODATI NOVI ID SA OSTALIM #define-ovima ZA TASTATURE
#define ID_BUTTON_RENAME_LIGHT (ID_KEYBOARD_BASE + 4)

/** @name Bazni ID-jevi za dinamičke widgete
 * @note Koriste se u petljama za kreiranje widgeta kao početna vrijednost.
 * @{
 */
#define ID_CurtainsRelay                0x894   ///< Svrha: Početni ID za widgete zavjesa. Vrijednost: 0x894 (Heksadecimalni broj).
#define ID_LightsModbusRelay            0x8B3   ///< Svrha: Početni ID za widgete svjetala. Vrijednost: 0x8B3 (Heksadecimalni broj).
/** @} */

/** @name ID-jevi za QR kodove
 * @{
 */
#define QR_CODE_WIFI_ID                 1       ///< Svrha: Logički ID za WiFi QR kod. Vrijednost: 1.
#define QR_CODE_APP_ID                  2       ///< Svrha: Logički ID za App QR kod. Vrijednost: 2.
/** @} */

/** @name Definicije boja
 * @note Koriste `GUI_MAKE_COLOR` makro koji pretvara 0xBBGGRR (Plava, Zelena, Crvena) format u format koji koristi GUI biblioteka.
 * @{
 */
#define CLR_DARK_BLUE                   GUI_MAKE_COLOR(0x613600)  ///< Svrha: Definicija tamno plave boje.
#define CLR_LIGHT_BLUE                  GUI_MAKE_COLOR(0xaa7d67)  ///< Svrha: Definicija svijetlo plave boje.
#define CLR_BLUE                        GUI_MAKE_COLOR(0x855a41)  ///< Svrha: Definicija plave boje.
#define CLR_LEMON                       GUI_MAKE_COLOR(0x00d6d3)  ///< Svrha: Definicija limun žute boje.

/*============================================================================*/
/* PRIVATNE STRUKTURE I DEKLARACIJE VARIJABLI                                 */
/*============================================================================*/
// DODATI OVAJ BLOK KODA NA VRH FAJLA display.c, NAKON #include SEKCIJE

/******************************************************************************
 * @brief       Struktura koja definiše kontekst za univerzalni numerički keypad.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ova struktura se koristi za dinamičku konfiguraciju keypad-a.
 *****************************************************************************/
typedef struct
{
    const char* title;          /**< Pokazivač na string koji će biti prikazan kao naslov. */
    char initial_value[12];     /**< Početna vrijednost koja se prikazuje. */
    int32_t min_val;            /**< Minimalna dozvoljena vrijednost. */
    int32_t max_val;            /**< Maksimalna dozvoljena vrijednost. */
    uint8_t max_len;            /**< Maksimalan broj karaktera za unos. */
    bool    allow_decimal;      /**< Ako je 'true', prikazuje se dugme '.'. */
    bool    allow_minus_one;    /**< Ako je 'true', prikazuje se dugme '[ ISKLJ. / OFF ]'. */
} NumpadContext_t;

/**
 * @brief Statička, privatna instanca konteksta za Numpad.
 * @note  Vidljiva samo unutar display.c. Funkcije unutar ovog fajla će
 * popunjavati ovu strukturu prije prebacivanja na ekran keypada.
 */
static NumpadContext_t g_numpad_context;

/******************************************************************************
 * @brief       Struktura koja sadrži rezultat unosa sa numeričkog keypada.
 * @author      Gemini (po specifikaciji korisnika)
 *****************************************************************************/
typedef struct
{
    char    value[12];          /**< Bafer za konačnu unesenu vrijednost. */
    bool    is_confirmed;       /**< Fleg, 'true' ako je korisnik potvrdio unos. */
    bool    is_cancelled;       /**< Fleg, 'true' ako je korisnik otkazao unos. */
} NumpadResult_t;

/**
 * @brief Statička, privatna instanca za rezultat unosa sa Numpad-a.
 * @note  Služi za internu komunikaciju između logike keypada i logike
 * ekrana koji ga je pozvao (npr. SCREEN_SETTINGS_GATE).
 */
static NumpadResult_t g_numpad_result;


/******************************************************************************
 * @brief       Struktura koja definiše kontekst za univerzalnu alfanumeričku tastaturu.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ova struktura se popunjava prije poziva funkcije
 * Display_ShowKeyboard() kako bi se tastatura prilagodila
 * specifičnoj potrebi (npr. unos imena svjetla).
 *****************************************************************************/
typedef struct
{
    const char* title;          /**< Pokazivač na string koji će biti prikazan kao naslov. */
    char initial_value[32];     /**< String koji sadrži početnu vrijednost za editovanje. */
    uint8_t max_len;            /**< Maksimalan broj karaktera koji se mogu unijeti. */
} KeyboardContext_t;

/**
 * @brief Statička, privatna instanca konteksta za alfanumeričku tastaturu.
 * @note  Vidljiva samo unutar display.c.
 */
static KeyboardContext_t g_keyboard_context;

/******************************************************************************
 * @brief       Struktura koja sadrži rezultat unosa sa alfanumeričke tastature.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Služi kao mehanizam za komunikaciju između tastature i ekrana
 * koji ju je pozvao, bez upotrebe callback funkcija.
 *****************************************************************************/
typedef struct
{
    char    value[32];          /**< Bafer u koji se upisuje konačni uneseni tekst. */
    bool    is_confirmed;       /**< Fleg, postaje 'true' kada korisnik pritisne "OK". */
    bool    is_cancelled;       /**< Fleg, postaje 'true' ako je korisnik otkazao unos. */
} KeyboardResult_t;

/**
 * @brief Statička, privatna instanca za rezultat unosa sa tastature.
 * @note  Vidljiva samo unutar display.c.
 */
static KeyboardResult_t g_keyboard_result;

/**
 * @brief Matrica koja sadrži rasporede tastera za sve podržane jezike.
 * @note  Struktura: [JEZIK][SHIFT_STANJE][RED][TASTER]
 * Ovo je "izvor istine" za iscrtavanje tastature. Funkcija
 * `DSP_InitKeyboardScreen` će na osnovu odabranog jezika i
 * shift stanja odabrati odgovarajući set karaktera za ispis.
 * Trenutno su definisani BHS, ENG (QWERTZ) i GER.
 */
static const char* key_layouts[LANGUAGE_COUNT][KEY_SHIFT_STATES][KEY_ROWS][KEYS_PER_ROW] =
{
    // =========================================================================
    // === JEZIK: BSHC (Bosanski/Srpski/Hrvatski/Crnogorski) - QWERTZ ===
    // =========================================================================
    [BSHC] = {
        {   // Stanje 0: Mala slova
            { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0" },
            { "q", "w", "e", "r", "t", "z", "u", "i", "o", "p" },
            { "a", "s", "d", "f", "g", "h", "j", "k", "l", "č" },
            { "š", "y", "x", "c", "v", "b", "n", "m", "đ", "ž" }
        },
        {   // Stanje 1: Velika slova
            { "!", "\"", "#", "$", "%", "&", "/", "(", ")", "=" },
            { "Q", "W", "E", "R", "T", "Z", "U", "I", "O", "P" },
            { "A", "S", "D", "F", "G", "H", "J", "K", "L", "Č" },
            { "Š", "Y", "X", "C", "V", "B", "N", "M", "Đ", "Ž" }
        }
    },

    // =========================================================================
    // === JEZIK: ENG (Engleski) - QWERTZ za konzistentnost ===
    // =========================================================================
    [ENG] = {
        {   // Stanje 0: Mala slova
            { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0" },
            { "q", "w", "e", "r", "t", "y", "u", "i", "o", "p" }, // QWERTY
            { "a", "s", "d", "f", "g", "h", "j", "k", "l", ";" },
            { "z", "x", "c", "v", "b", "n", "m", ",", ".", "-" }  // QWERTY
        },
        {   // Stanje 1: Velika slova
            { "!", "@", "#", "$", "%", "^", "&", "*", "(", ")" },
            { "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P" }, // QWERTY
            { "A", "S", "D", "F", "G", "H", "J", "K", "L", ":" },
            { "Z", "X", "C", "V", "B", "N", "M", "<", ">", "_" }  // QWERTY
        }
    },

    // =========================================================================
    // === JEZIK: GER (Njemački) - QWERTZ ===
    // =========================================================================
    [GER] = {
        {   // Stanje 0: Mala slova
            { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0" },
            { "q", "w", "e", "r", "t", "z", "u", "i", "o", "p" },
            { "a", "s", "d", "f", "g", "h", "j", "k", "l", "ö" },
            { "ü", "y", "x", "c", "v", "b", "n", "m", "ä", "ß" }
        },
        {   // Stanje 1: Velika slova
            { "!", "\"", "§", "$", "%", "&", "/", "(", ")", "=" },
            { "Q", "W", "E", "R", "T", "Z", "U", "I", "O", "P" },
            { "A", "S", "D", "F", "G", "H", "J", "K", "L", "Ö" },
            { "Ü", "Y", "X", "C", "V", "B", "N", "M", "Ä", "?" }
        }
    },

    // Ostali jezici trenutno koriste ENG layout kao placeholder
    [FRA] = { [0 ... KEY_SHIFT_STATES-1] = {}}, // Placeholder
    [ITA] = { [0 ... KEY_SHIFT_STATES-1] = {}}, // Placeholder
    [SPA] = { [0 ... KEY_SHIFT_STATES-1] = {}}, // Placeholder
    [RUS] = { [0 ... KEY_SHIFT_STATES-1] = {}}, // Placeholder
    [UKR] = { [0 ... KEY_SHIFT_STATES-1] = {}}, // Placeholder
    [POL] = { [0 ... KEY_SHIFT_STATES-1] = {}}, // Placeholder
    [CZE] = { [0 ... KEY_SHIFT_STATES-1] = {}}, // Placeholder
    [SLO] = { [0 ... KEY_SHIFT_STATES-1] = {}}  // Placeholder
};
/**
 * @brief Definicija opšteg tipa za zonu dodira (pravougaonik).
 * @note  Olakšava kreiranje i rad sa layout strukturama.
 */
typedef struct {
    int16_t x0, y0, x1, y1;
} TouchZone_t;

/**
 * @brief Struktura koja objedinjuje sve GUI widgete (handle-ove)
 * za jedan red u meniju za podešavanje svjetala.
 */
typedef struct
{
    SPINBOX_Handle relay;                   // Handle za spinbox widget za Modbus adresu releja.
    SPINBOX_Handle iconID;                  // Handle za spinbox widget za ID ikonice.
    SPINBOX_Handle controllerID_on;         // Handle za spinbox za Modbus adresu kontrolera koji se pali zajedno sa svjetlom.
    SPINBOX_Handle controllerID_on_delay;   // Handle za spinbox za vrijeme odgode paljenja drugog kontrolera.
    SPINBOX_Handle on_hour;                 // Handle za spinbox za sat automatskog paljenja.
    SPINBOX_Handle on_minute;               // Handle za spinbox za minutu automatskog paljenja.
    SPINBOX_Handle offTime;                 // Handle za spinbox za vrijeme automatskog gašenja.
    SPINBOX_Handle communication_type;      // Handle za spinbox za tip komunikacije (binarno, dimer, RGB).
    SPINBOX_Handle local_pin;               // Handle za spinbox za odabir lokalnog GPIO pina.
    SPINBOX_Handle sleep_time;              // Handle za spinbox za 'sleep' vrijeme.
    SPINBOX_Handle button_external;         // Handle za spinbox za mod eksternog tastera.
    CHECKBOX_Handle tiedToMainLight;        // Handle za checkbox za povezivanje sa glavnim svjetlom.
    CHECKBOX_Handle rememberBrightness;     // Handle za checkbox za pamćenje zadnje svjetline.
} LIGHT_settingsWidgets;

/**
 * @brief Pomoćna struktura koja objedinjuje sve GUI widgete (handle-ove)
 * za meni za podešavanje odmrzivača (Defroster).
 * @note Interna je za `display.c`.
 */
typedef struct
{
    SPINBOX_Handle cycleTime, activeTime, pin;
} Defroster_settingsWidgets;

/**
 * @brief Pomoćna struktura za definisanje pozicije i dimenzija widget-a.
 */
typedef struct {
    int16_t x, y, w, h;
} WidgetRect_t;

/** @brief Pomoćna struktura za definisanje horizontalne linije. */
typedef struct {
    int16_t y, x0, x1;
} HLine_t;

/**
 ******************************************************************************
 * @brief       Struktura koja sadrži konstante za iscrtavanje "hamburger" menija.
 * @author      Gemini
 * @note        Centralizuje sve dimenzije i koordinate za gornju desnu i donju
 * lijevu ikonu kako bi se izbjegli "magični brojevi" u kodu i
 * olakšalo buduće održavanje. [cite: 63, 64, 65]
 ******************************************************************************
 */
static const struct
{
    /** @brief Konstante za GORNJI DESNI meni (pozicija 1) */
    struct {
        int16_t x_start; /**< Početna X koordinata (lijevo). */
        int16_t y_start; /**< Početna Y koordinata (gore). */
        int16_t width;   /**< Širina linija. */
        int16_t y_gap;   /**< Vertikalni razmak između linija. */
    } top_right;

    /** @brief Konstante za DONJI LIJEVI meni (pozicija 2) */
    struct {
        int16_t x_start; /**< Početna X koordinata (lijevo). */
        int16_t y_start; /**< Y koordinata donje linije. */
        int16_t width;   /**< Širina linija. */
        int16_t y_gap;   /**< Negativan vertikalni razmak za crtanje prema gore. */
    } bottom_left;

    int16_t line_thickness; /**< Debljina linija za obje ikone. */
}
hamburger_menu_layout =
{
    .top_right = {
        .x_start = 400,
        .y_start = 20,
        .width   = 50,
        .y_gap   = 20
    },
    .bottom_left = {
        .x_start = 30,
        .y_start = 252,
        .width   = 50,
        .y_gap   = -20
    },
    .line_thickness = 9
};
/**
 * @brief Struktura koja sadrži konstante za globalne elemente GUI-ja.
 * @note  Trenutno sadrži samo zonu za hamburger meni, ali se može proširiti.
 */
static const struct
{
    TouchZone_t hamburger_menu_zone; /**< Zona za ulazak/povratak iz menija. */
}
global_layout =
{
    .hamburger_menu_zone = { .x0 = 400, .y0 = 0, .x1 = 480, .y1 = 80 }
};

/**
 * @brief Struktura koja sadrži konstante za raspored elemenata (layout)
 * na glavnom ekranu.
 */
static const struct
{
    int16_t circle_center_x;    /**< X koordinata centra kruga. */
    int16_t circle_center_y;    /**< Y koordinata centra kruga. */
    int16_t circle_radius_x;    /**< Horizontalni radijus kruga. */
    int16_t circle_radius_y;    /**< Vertikalni radijus kruga. */

    // NOVE KOORDINATE ZA VRIJEME I DATUM
    GUI_POINT time_pos_standard;        // Pozicija za vrijeme na standardnom ekranu
    GUI_POINT time_pos_scrnsvr;         // Pozicija za vrijeme na screensaver-u
    GUI_POINT date_pos_scrnsvr;         // Pozicija za datum na screensaver-u
}
main_screen_layout =
{
    .circle_center_x = 240,
    .circle_center_y = 136,
    .circle_radius_x = 50,
    .circle_radius_y = 50,

    // NOVE VRIJEDNOSTI
    .time_pos_standard  = { 5, 245 },   // Bivše (5, 245)
    .time_pos_scrnsvr   = { 240, 136 },  // Bivše (240, 136)
    .date_pos_scrnsvr   = { 240, 220 }   // Nova pozicija za datum
};
/**
 * @brief Struktura koja sadrži konstante za raspored elemenata (layout)
 * na prvom ekranu za odabir.
 */
static const struct
{
    TouchZone_t lights_zone;        /**< Zona za odabir menija za svjetla (kvadrant 1). */
    TouchZone_t thermostat_zone;    /**< Zona za odabir menija za termostat (kvadrant 2). */
    TouchZone_t curtains_zone;      /**< Zona za odabir menija za zavjese (kvadrant 3). */
    TouchZone_t dynamic_zone;       /**< Zona za dinamicku ikonicu (Defroster/Ventilator, kvadrant 4). */
    TouchZone_t next_button_zone;   /**< Zona za dugme "NEXT". */
}
select_screen1_layout =
{
    .lights_zone      = { .x0 = 0,   .y0 = 0,   .x1 = 190, .y1 = 136 },
    .thermostat_zone  = { .x0 = 190, .y0 = 0,   .x1 = 380, .y1 = 136 },
    .curtains_zone    = { .x0 = 0,   .y0 = 136, .x1 = 190, .y1 = 272 },
    .dynamic_zone     = { .x0 = 190, .y0 = 136, .x1 = 380, .y1 = 272 },
    .next_button_zone = { .x0 = 400, .y0 = 159, .x1 = 480, .y1 = 272 }
};
/**
 * @brief Struktura koja sadrži konstante za raspored elemenata (layout)
 * na drugom ekranu za odabir.
 */
static const struct
{
    TouchZone_t clean_zone;         /**< Zona za odabir menija za čišćenje. */
    TouchZone_t wifi_zone;          /**< Zona za odabir Wi-Fi QR koda. */
    TouchZone_t app_zone;           /**< Zona za odabir App QR koda. */
    TouchZone_t next_button_zone;   /**< Zona za dugme "NEXT". */
}
select_screen2_layout =
{
    .clean_zone       = { .x0 = 0,   .y0 = 80, .x1 = 126, .y1 = 200 }, // Prva trećina
    .wifi_zone        = { .x0 = 126, .y0 = 80, .x1 = 253, .y1 = 200 }, // Druga trećina
    .app_zone         = { .x0 = 253, .y0 = 80, .x1 = 380, .y1 = 200 }, // Treća trećina
    .next_button_zone = { .x0 = 380, .y0 = 159, .x1 = 480, .y1 = 272 }
};
/**
 * @brief Struktura koja sadrži konstante za raspored elemenata (layout)
 * na ekranu za prikaz scena.
 * @note  Centralizuje sve dimenzije i pozicije kako bi se izbjegli
 * "magični brojevi" u funkciji za iscrtavanje.
 */
static const struct
{
    int16_t items_per_row;      /**< Broj ikonica u jednom redu. */
    int16_t slot_width;         /**< Širina jednog slota za ikonicu. */
    int16_t slot_height;        /**< Visina jednog slota za ikonicu. */
    int16_t text_y_offset;      /**< Vertikalni pomak teksta u odnosu na centar ikonice. */
}
scene_screen_layout =
{
    .items_per_row   = 3,
    .slot_width      = 126,
    .slot_height     = 136,
    .text_y_offset   = 35
};

/**
 * @brief Struktura koja sadrži sve konstante za raspored elemenata (layout)
 * na ekranu termostata. Sve koordinate su na jednom mjestu radi lakšeg održavanja.
 */
static const struct
{
    TouchZone_t increase_zone;      /**< Zona za povećanje temperature (+) */
    TouchZone_t decrease_zone;      /**< Zona za smanjenje temperature (-) */
    TouchZone_t on_off_zone;        /**< Zona za paljenje/gašenje dugim pritiskom */
}
thermostat_layout =
{
    .increase_zone = { .x0 = 200, .y0 = 90, .x1 = 320, .y1 = 270 },
    .decrease_zone = { .x0 = 0,   .y0 = 90, .x1 = 120, .y1 = 270 },
    .on_off_zone   = { .x0 = 400, .y0 = 150, .x1 = 480, .y1 = 190 }
};


/**
 * @brief Struktura koja sadrži konstante za raspored elemenata (layout)
 * na ekranu za kontrolu svjetala.
 */
static const struct
{
    int16_t icon_width;     /**< Širina zone dodira za jednu ikonicu. */
    int16_t icon_height;    /**< Visina zone dodira za jednu ikonicu (uključujući tekst). */
}
lights_screen_layout =
{
    .icon_width = 80,
    .icon_height = 120
};
/**
 * @brief Struktura koja sadrži konstante za raspored elemenata (layout)
 * na ekranu za kontrolu roletni.
 */
static const struct
{
    TouchZone_t up_zone;              /**< Zona za trougao GORE. */
    TouchZone_t down_zone;            /**< Zona za trougao DOLE. */
    TouchZone_t previous_arrow_zone;  /**< Zona za strelicu PRETHODNA. */
    TouchZone_t next_arrow_zone;      /**< Zona za strelicu SLJEDEĆA. */
}
curtains_screen_layout =
{
    .up_zone             = { .x0 = 100, .y0 = 0,   .x1 = 280, .y1 = 136 }, // Gornja polovina centralnog dijela
    .down_zone           = { .x0 = 100, .y0 = 136, .x1 = 280, .y1 = 272 }, // Donja polovina centralnog dijela
    .previous_arrow_zone = { .x0 = 0,   .y0 = 192, .x1 = 80,  .y1 = 272 }, // Donji lijevi ugao
    .next_arrow_zone     = { .x0 = 320, .y0 = 192, .x1 = 380, .y1 = 272 }  // Donji desni ugao
};
/**
 * @brief Struktura koja sadrži konstante za raspored elemenata (layout)
 * na ekranu za detaljno podešavanje svjetala (dimer i RGB).
 */
static const struct
{   
    TouchZone_t rename_text_zone;       /**< Fiksna zona dodira u gornjem lijevom uglu za pokretanje izmjene naziva. */
    TouchZone_t white_square_zone;      /**< Zona za odabir bijele boje. */
    TouchZone_t brightness_slider_zone; /**< Zona za slajder svjetline. */
    TouchZone_t color_palette_zone;     /**< Zona za paletu boja. */
}
light_settings_screen_layout =
{
    .rename_text_zone       = { .x0 = 0, .y0 = 0, .x1 = 200, .y1 = 60 },
    .white_square_zone      = { .x0 = 210, .y0 = 41,  .x1 = 270, .y1 = 101 },
    .brightness_slider_zone = { .x0 = 60,  .y0 = 111, .x1 = 420, .y1 = 161 },
    .color_palette_zone     = { .x0 = 60,  .y0 = 181, .x1 = 420, .y1 = 231 }
};
/**
 * @brief Struktura koja sadrži konstante za raspored elemenata (layout)
 * na ekranu za resetovanje menija (ponaša se kao glavni prekidač).
 */
static const struct
{
    TouchZone_t main_switch_zone;  /**< Velika centralna zona koja se ponaša kao glavni prekidač. */
}
reset_menu_switches_layout =
{
    .main_switch_zone = { .x0 = 80, .y0 = 80, .x1 = 400, .y1 = 192 }
};
/**
 * @brief Struktura koja sadrži sve konstante za ISCRTAVANJE elemenata
 * na prvom ekranu za odabir (Select Screen 1).
 * @note  Ova struktura je redizajnirana da podrži dinamički "Smart Grid" raspored.
 * Sadrži parametre za iscrtavanje rasporeda sa 1, 2, 3 ili 4 ikonice,
 * čime se izbjegavaju "magični brojevi" unutar `Service_SelectScreen1` funkcije.
 */
static const struct
{
    /** @brief X pozicija krajnje desne vertikalne linije. */
    int16_t x_separator_pos;
    /** @brief Y koordinata centra za "NEXT" dugme. */
    int16_t y_next_button_center;
    /** @brief Y koordinata centra za rasporede sa jednim redom (kada su 1, 2 ili 3 ikonice aktivne). */
    int16_t y_center_single_row;
    /** @brief Y koordinata centra za gornji red u 2x2 rasporedu (kada su 4 ikonice aktivne). */
    int16_t y_center_top_row;
    /** @brief Y koordinata centra za donji red u 2x2 rasporedu (kada su 4 ikonice aktivne). */
    int16_t y_center_bottom_row;
    /** @brief Vertikalni razmak (u pikselima) između dna ikonice i vrha teksta ispod nje. */
    int16_t text_vertical_offset;
    /** @brief Početna Y koordinata (vrh) za KRATKE vertikalne separatore. */
    int16_t short_separator_y_start;
    /** @brief Krajnja Y koordinata (dno) za KRATKE vertikalne separatore. */
    int16_t short_separator_y_end;
    /** @brief Početna Y koordinata (vrh) za DUGAČKI desni separator. */
    int16_t long_separator_y_start;
    /** @brief Krajnja Y koordinata (dno) za DUGAČKI desni separator. */
    int16_t long_separator_y_end;
    /** @brief Horizontalni razmak (padding) od ivica ekrana za horizontalni separator u 2x2 rasporedu. */
    int16_t separator_x_padding;
    /** @brief Definiše kompletnu zonu dodira za "NEXT" dugme. */
    TouchZone_t next_button_zone;
}
select_screen1_drawing_layout =
{
    .x_separator_pos        = DRAWING_AREA_WIDTH,
    .y_next_button_center   = 192,
    .y_center_single_row    = 136,
    .y_center_top_row       = 68,
    .y_center_bottom_row    = 204,
    .text_vertical_offset   = 10,
    .short_separator_y_start = 60,
    .short_separator_y_end   = 212,
    .long_separator_y_start = 10,
    .long_separator_y_end   = 252,
    .separator_x_padding    = 20,
    .next_button_zone       = { .x0 = 400, .y0 = 80, .x1 = 480, .y1 = 272 }
};
/**
 * @brief Struktura koja sadrži konstante za ISCRTAVANJE i ZONE DODIRA
 * na drugom ekranu za odabir.
 * @note  Verzija 2.6.0: Zadržano originalno ime `select_screen2_drawing_layout`.
 * Redizajnirana da podrži fiksni 2x2 raspored.
 */
static const struct
{
    /** @brief Zona dodira za gornji lijevi kvadrant (Čišćenje). */
    TouchZone_t clean_zone;
    /** @brief Zona dodira za gornji desni kvadrant (WiFi). */
    TouchZone_t wifi_zone;
    /** @brief Zona dodira za donji lijevi kvadrant (App). */
    TouchZone_t app_zone;
    /** @brief Zona dodira za donji desni kvadrant (Podešavanja). */
    TouchZone_t settings_zone;
    /** @brief Zona dodira za "NEXT" dugme za rotaciju ekrana. */
    TouchZone_t next_button_zone;

    // Koordinate za iscrtavanje
    /** @brief X koordinata centra lijeve kolone. */
    int16_t x_center_left;
    /** @brief X koordinata centra desne kolone. */
    int16_t x_center_right;
    /** @brief Y koordinata centra gornjeg reda. */
    int16_t y_center_top;
    /** @brief Y koordinata centra donjeg reda. */
    int16_t y_center_bottom;
    /** @brief Vertikalni pomak teksta u odnosu na centar ikonice. */
    int16_t text_vertical_offset;
    /** @brief Početna Y koordinata za vertikalne separatore. */
    int16_t separator_y_start;
    /** @brief Krajnja Y koordinata za vertikalne separatore. */
    int16_t separator_y_end;
    /** @brief Horizontalni padding za horizontalni separator. */
    int16_t separator_x_padding;
    /** @brief X pozicija za "NEXT" dugme. */
    int16_t next_button_x_pos;
    /** @brief Y centar za "NEXT" dugme. */
    int16_t next_button_y_center;
}
select_screen2_drawing_layout =
{
    .clean_zone         = { .x0 = 0,   .y0 = 0, .x1 = 190, .y1 = 136 },
    .wifi_zone          = { .x0 = 190, .y0 = 0, .x1 = 380, .y1 = 136 },
    .app_zone           = { .x0 = 0,   .y0 = 136, .x1 = 190, .y1 = 272 },
    .settings_zone      = { .x0 = 190, .y0 = 136, .x1 = 380, .y1 = 272 },
    .next_button_zone   = { .x0 = 400, .y0 = 80, .x1 = 480, .y1 = 272 },

    .x_center_left      = 95,
    .x_center_right     = 285,
    .y_center_top       = 68,
    .y_center_bottom    = 204,
    .text_vertical_offset = 10,
    .separator_y_start  = 20,
    .separator_y_end    = 252,
    .separator_x_padding = 20,
    .next_button_x_pos  = DRAWING_AREA_WIDTH + 5,
    .next_button_y_center = 192,
};
/**
 * @brief Struktura koja sadrži konstante za ISCRTAVANJE elemenata
 * na prvom ekranu za podešavanja (Termostat i Ventilator).
 */
static const struct
{
    WidgetRect_t thst_control_pos;      /**< Pozicija i dimenzije za RADIO widget kontrole termostata. */
    WidgetRect_t fan_control_pos;       /**< Pozicija i dimenzije za RADIO widget kontrole ventilatora. */
    WidgetRect_t thst_max_sp_pos;       /**< Pozicija i dimenzije za SPINBOX max. zadate temperature. */
    WidgetRect_t thst_min_sp_pos;       /**< Pozicija i dimenzije za SPINBOX min. zadate temperature. */
    WidgetRect_t fan_diff_pos;          /**< Pozicija i dimenzije za SPINBOX diferencijala ventilatora. */
    WidgetRect_t fan_low_band_pos;      /**< Pozicija i dimenzije za SPINBOX donjeg opsega ventilatora. */
    WidgetRect_t fan_hi_band_pos;       /**< Pozicija i dimenzije za SPINBOX gornjeg opsega ventilatora. */
    WidgetRect_t thst_group_pos;        /**< Pozicija i dimenzije za SPINBOX grupe termostata. */
    WidgetRect_t thst_master_pos;       /**< Pozicija i dimenzije za CHECKBOX master moda. */
    WidgetRect_t next_button_pos;       /**< Pozicija i dimenzije za "NEXT" dugme. */
    WidgetRect_t save_button_pos;       /**< Pozicija i dimenzije za "SAVE" dugme. */

    GUI_POINT label_thst_max_sp[2];     /**< Pozicije za dvorednu labelu za max. setpoint. */
    GUI_POINT label_thst_min_sp[2];     /**< Pozicije za dvorednu labelu za min. setpoint. */
    GUI_POINT label_fan_diff[2];        /**< Pozicije za dvorednu labelu za diferencijal ventilatora. */
    GUI_POINT label_fan_low[2];         /**< Pozicije za dvorednu labelu za donji opseg ventilatora. */
    GUI_POINT label_fan_hi[2];          /**< Pozicije za dvorednu labelu za gornji opseg ventilatora. */
    GUI_POINT label_thst_ctrl_title;    /**< Pozicija za naslov "THERMOSTAT CONTROL MODE". */
    GUI_POINT label_fan_ctrl_title;     /**< Pozicija za naslov "FAN SPEED CONTROL MODE". */
    GUI_POINT label_thst_group;         /**< Pozicija za labelu "GROUP". */
}
settings_screen_1_layout =
{
    .thst_control_pos   = { 10,  20,  150, 80 },
    .fan_control_pos    = { 10,  150, 150, 80 },
    .thst_max_sp_pos    = { 110, 20,  90,  30 },
    .thst_min_sp_pos    = { 110, 70,  90,  30 },
    .fan_diff_pos       = { 110, 150, 90,  30 },
    .fan_low_band_pos   = { 110, 190, 90,  30 },
    .fan_hi_band_pos    = { 110, 230, 90,  30 },
    .thst_group_pos     = { 320, 20,  100, 40 },
    .thst_master_pos    = { 320, 70,  170, 20 },
    .next_button_pos    = { 340, 180, 130, 30 },
    .save_button_pos    = { 340, 230, 130, 30 },

    .label_thst_max_sp  = { {210, 24}, {210, 36} },
    .label_thst_min_sp  = { {210, 74}, {210, 86} },
    .label_fan_diff     = { {210, 154}, {210, 166} },
    .label_fan_low      = { {210, 194}, {210, 206} },
    .label_fan_hi       = { {210, 234}, {210, 246} },
    .label_thst_ctrl_title = { 10, 4 },
    .label_fan_ctrl_title  = { 10, 120 },
    .label_thst_group      = { 430, 37 } // 320 (x) + 100 (w) + 10 (padding)
};
/**
 * @brief Struktura koja sadrži konstante za ISCRTAVANJE elemenata
 * na drugom ekranu za podešavanja (Vrijeme, Datum, Screensaver).
 * @note  Verzija 3.0: Sadrži pozicije za SVE elemente bez izuzetka.
 */
static const struct
{
    WidgetRect_t high_brightness_pos;       /**< @brief Pozicija i dimenzije za SPINBOX visoke svjetline. */
    WidgetRect_t low_brightness_pos;        /**< @brief Pozicija i dimenzije za SPINBOX niske svjetline. */
    WidgetRect_t scrnsvr_timeout_pos;       /**< @brief Pozicija i dimenzije za SPINBOX screensaver timeout-a. */
    WidgetRect_t scrnsvr_enable_hour_pos;   /**< @brief Pozicija i dimenzije za SPINBOX sata aktivacije screensaver-a. */
    WidgetRect_t scrnsvr_disable_hour_pos;  /**< @brief Pozicija i dimenzije za SPINBOX sata deaktivacije screensaver-a. */
    WidgetRect_t hour_pos;                  /**< @brief Pozicija i dimenzije za SPINBOX sata. */
    WidgetRect_t minute_pos;                /**< @brief Pozicija i dimenzije za SPINBOX minute. */
    WidgetRect_t day_pos;                   /**< @brief Pozicija i dimenzije za SPINBOX dana. */
    WidgetRect_t month_pos;                 /**< @brief Pozicija i dimenzije za SPINBOX mjeseca. */
    WidgetRect_t year_pos;                  /**< @brief Pozicija i dimenzije za SPINBOX godine. */
    WidgetRect_t scrnsvr_color_pos;         /**< @brief Pozicija i dimenzije za SPINBOX boje sata na screensaver-u. */
    WidgetRect_t scrnsvr_checkbox_pos;      /**< @brief Pozicija i dimenzije za CHECKBOX screensaver sata. */
    WidgetRect_t weekday_dropdown_pos;      /**< @brief Pozicija i dimenzije za DROPDOWN dan u sedmici. */
    WidgetRect_t next_button_pos;           /**< @brief Pozicija i dimenzije za "NEXT" dugme. */
    WidgetRect_t save_button_pos;           /**< @brief Pozicija i dimenzije za "SAVE" dugme. */
    TouchZone_t  scrnsvr_color_preview_rect;/**< @brief Pozicija pravougaonika za pregled boje sata. */

    GUI_POINT label_backlight_title;        /**< @brief Pozicija za naslov "DISPLAY BACKLIGHT". */
    GUI_POINT label_high_brightness;        /**< @brief Pozicija za labelu "HIGH". */
    GUI_POINT label_low_brightness;         /**< @brief Pozicija za labelu "LOW". */
    GUI_POINT label_time_title;             /**< @brief Pozicija za naslov "SET TIME". */
    GUI_POINT label_hour;                   /**< @brief Pozicija za labelu "HOUR". */
    GUI_POINT label_minute;                 /**< @brief Pozicija za labelu "MINUTE". */
    GUI_POINT label_color_title;            /**< @brief Pozicija za naslov "SET COLOR". */
    GUI_POINT label_full_color;             /**< @brief Pozicija za labelu "FULL". */
    GUI_POINT label_clock_color;            /**< @brief Pozicija za labelu "CLOCK". */
    GUI_POINT label_scrnsvr_title;          /**< @brief Pozicija za naslov "SCREENSAVER OPTION". */
    GUI_POINT label_timeout;                /**< @brief Pozicija za labelu "TIMEOUT". */
    GUI_POINT label_enable_hour[2];         /**< @brief Pozicije za dvorednu labelu "ENABLE HOUR". */
    GUI_POINT label_disable_hour[2];        /**< @brief Pozicije za dvorednu labelu "DISABLE HOUR". */
    GUI_POINT label_date_title;             /**< @brief Pozicija za naslov "SET DATE". */
    GUI_POINT label_day;                    /**< @brief Pozicija za labelu "DAY". */
    GUI_POINT label_month;                  /**< @brief Pozicija za labelu "MONTH". */
    GUI_POINT label_year;                   /**< @brief Pozicija za labelu "YEAR". */

    HLine_t   line1, line2, line3, line4, line5; /**< @brief Definicije za sve dekorativne linije. */
}
settings_screen_2_layout =
{
    .high_brightness_pos      = { 10,  20,  90, 30 },
    .low_brightness_pos       = { 10,  60,  90, 30 },
    .scrnsvr_timeout_pos      = { 10,  130, 90, 30 },
    .scrnsvr_enable_hour_pos  = { 10,  170, 90, 30 },
    .scrnsvr_disable_hour_pos = { 10,  210, 90, 30 },
    .hour_pos                 = { 190, 20,  90, 30 },
    .minute_pos               = { 190, 60,  90, 30 },
    .day_pos                  = { 190, 130, 90, 30 },
    .month_pos                = { 190, 170, 90, 30 },
    .year_pos                 = { 190, 210, 90, 30 },
    .scrnsvr_color_pos        = { 340, 20,  90, 30 },
    .scrnsvr_checkbox_pos     = { 340, 70,  110, 20 },
    .weekday_dropdown_pos     = { 340, 100, 130, 100 },
    .next_button_pos          = { 340, 180, 130, 30 },
    .save_button_pos          = { 340, 230, 130, 30 },
    .scrnsvr_color_preview_rect = { 340, 51, 430, 59 },

    .label_backlight_title  = { 10,  5 },
    .label_high_brightness  = { 110, 35 },
    .label_low_brightness   = { 110, 75 },
    .label_time_title       = { 190, 5 },
    .label_hour             = { 290, 35 },
    .label_minute           = { 290, 75 },
    .label_color_title      = { 340, 5 },
    .label_full_color       = { 440, 26 },
    .label_clock_color      = { 440, 38 },
    .label_scrnsvr_title    = { 10,  115 },
    .label_timeout          = { 110, 145 },
    .label_enable_hour      = { {110, 176}, {110, 188} },
    .label_disable_hour     = { {110, 216}, {110, 228} },
    .label_date_title       = { 190, 115 },
    .label_day              = { 290, 145 },
    .label_month            = { 290, 185 },
    .label_year             = { 290, 225 },

    .line1 = { 15, 5, 160 },
    .line2 = { 15, 185, 320 },
    .line3 = { 15, 335, 475 },
    .line4 = { 125, 5, 160 },
    .line5 = { 125, 185, 320 }
};
/**
 * @brief Struktura koja sadrži konstante za ISCRTAVANJE elemenata
 * na trećem ekranu za podešavanja (Defroster, Ventilator).
 */
static const struct
{
    WidgetRect_t defroster_cycle_time_pos;      /**< @brief Pozicija i dimenzije za SPINBOX ciklusa odmrzivača. */
    WidgetRect_t defroster_active_time_pos;     /**< @brief Pozicija i dimenzije za SPINBOX aktivnog vremena odmrzivača. */
    WidgetRect_t defroster_pin_pos;             /**< @brief Pozicija i dimenzije za SPINBOX pina odmrzivača. */
    WidgetRect_t ventilator_relay_pos;          /**< @brief Pozicija i dimenzije za SPINBOX releja ventilatora. */
    WidgetRect_t ventilator_delay_on_pos;       /**< @brief Pozicija i dimenzije za SPINBOX odgode paljenja ventilatora. */
    WidgetRect_t ventilator_delay_off_pos;      /**< @brief Pozicija i dimenzije za SPINBOX odgode gašenja ventilatora. */
    WidgetRect_t ventilator_trigger1_pos;       /**< @brief Pozicija i dimenzije za SPINBOX prvog okidača ventilatora. */
    WidgetRect_t ventilator_trigger2_pos;       /**< @brief Pozicija i dimenzije za SPINBOX drugog okidača ventilatora. */
    WidgetRect_t ventilator_local_pin_pos;      /**< @brief Pozicija i dimenzije za SPINBOX lokalnog pina ventilatora. */
    WidgetRect_t select_control_pos;            /**< @brief Pozicija i dimenzije za DROPDOWN odabir kontrole. */
    WidgetRect_t next_button_pos;               /**< @brief Pozicija i dimenzije za "NEXT" dugme. */
    WidgetRect_t save_button_pos;               /**< @brief Pozicija i dimenzije za "SAVE" dugme. */

    GUI_POINT label_ventilator_title;           /**< @brief Pozicija za naslov "VENTILATOR CONTROL". */
    GUI_POINT label_defroster_title;            /**< @brief Pozicija za naslov "DEFROSTER CONTROL". */
    GUI_POINT label_select_control_title;       /**< @brief Pozicija za naslov "SELECT CONTROL 4". */

    GUI_POINT label_ventilator_relay[2];        /**< @brief Pozicije za dvorednu labelu za relej ventilatora. */
    GUI_POINT label_ventilator_delay_on[2];     /**< @brief Pozicije za dvorednu labelu za odgodu paljenja. */
    GUI_POINT label_ventilator_delay_off[2];    /**< @brief Pozicije za dvorednu labelu za odgodu gašenja. */
    GUI_POINT label_ventilator_trigger1[2];     /**< @brief Pozicije za dvorednu labelu za okidač 1. */
    GUI_POINT label_ventilator_trigger2[2];     /**< @brief Pozicije za dvorednu labelu za okidač 2. */
    GUI_POINT label_ventilator_local_pin[2];    /**< @brief Pozicije za dvorednu labelu za lokalni pin. */

    GUI_POINT label_defroster_cycle_time[2];    /**< @brief Pozicije za dvorednu labelu za vrijeme ciklusa. */
    GUI_POINT label_defroster_active_time[2];   /**< @brief Pozicije za dvorednu labelu za aktivno vrijeme. */
    GUI_POINT label_defroster_pin[2];           /**< @brief Pozicije za dvorednu labelu za pin. */

    HLine_t   line_ventilator_title;            /**< @brief Linija ispod naslova za ventilator. */
    HLine_t   line_defroster_title;             /**< @brief Linija ispod naslova za odmrzivač. */
    HLine_t   line_select_control;              /**< @brief Linija ispod odabira kontrole. */
}
settings_screen_3_layout =
{
    .defroster_cycle_time_pos   = { 200, 20,  110, 35 },
    .defroster_active_time_pos  = { 200, 60,  110, 35 },
    .defroster_pin_pos          = { 200, 100, 110, 35 },
    .ventilator_relay_pos       = { 10,  20,  110, 35 },
    .ventilator_delay_on_pos    = { 10,  60,  110, 35 },
    .ventilator_delay_off_pos   = { 10,  100, 110, 35 },
    .ventilator_trigger1_pos    = { 10,  140, 110, 35 },
    .ventilator_trigger2_pos    = { 10,  180, 110, 35 },
    .ventilator_local_pin_pos   = { 10,  220, 110, 35 },
    .select_control_pos         = { 200, 170, 110, 80 },
    .next_button_pos            = { 410, 180, 60,  30 },
    .save_button_pos            = { 410, 230, 60,  30 },

    .label_ventilator_title     = { 10, 4 },
    .label_defroster_title      = { 210, 4 },
    .label_select_control_title = { 200, 154 },

    .label_ventilator_relay     = { {130, 30},  {130, 42} },
    .label_ventilator_delay_on  = { {130, 70},  {130, 82} },
    .label_ventilator_delay_off = { {130, 110}, {130, 122} },
    .label_ventilator_trigger1  = { {130, 150}, {130, 162} },
    .label_ventilator_trigger2  = { {130, 190}, {130, 202} },
    .label_ventilator_local_pin = { {130, 230}, {130, 242} },

    .label_defroster_cycle_time = { {320, 30}, {320, 42} },
    .label_defroster_active_time= { {320, 70}, {320, 82} },
    .label_defroster_pin        = { {320, 110}, {320, 122} },

    .line_ventilator_title      = { 12, 5, 180 },
    .line_defroster_title       = { 12, 200, 375 },
    .line_select_control        = { 162, 200, 375 }
};

/**
 * @brief Struktura koja sadrži konstante za ISCRTAVANJE elemenata
 * na četvrtom ekranu za podešavanja (Roletne).
 * @note  Definiše pravila za dinamičko iscrtavanje mreže 2x2.
 */
static const struct
{
    /** @brief Početna tačka (gore-lijevo) za prvi widget u mreži. */
    GUI_POINT    grid_start_pos;
    /** @brief Širina jednog SPINBOX widgeta. */
    int16_t      widget_width;
    /** @brief Visina jednog SPINBOX widgeta. */
    int16_t      widget_height;
    /** @brief Vertikalni razmak između "UP" i "DOWN" spinbox-a za istu roletnu. */
    int16_t      y_row_spacing;
    /** @brief Vertikalni razmak između grupa widgeta za različite roletne. */
    int16_t      y_group_spacing;
    /** @brief Horizontalni razmak između prve i druge kolone widgeta. */
    int16_t      x_col_spacing;
    /** @brief Relativni pomak za ispis prve linije labele u odnosu na widget. */
    GUI_POINT    label_line1_offset;
    /** @brief Vertikalni pomak za ispis druge linije labele. */
    int16_t      label_line2_offset_y;
    /** @brief Pozicija i dimenzije za "NEXT" dugme. */
    WidgetRect_t next_button_pos;
    /** @brief Pozicija i dimenzije za "SAVE" dugme. */
    WidgetRect_t save_button_pos;
}
settings_screen_4_layout =
{
    .grid_start_pos         = { 10,  20 },
    .widget_width           = 110,
    .widget_height          = 40,
    .y_row_spacing          = 50,
    .y_group_spacing        = 100,
    .x_col_spacing          = 190,
    .label_line1_offset     = { 120, 8 },
    .label_line2_offset_y   = 12,
    .next_button_pos        = { 410, 180, 60, 30 },
    .save_button_pos        = { 410, 230, 60, 30 }
};

/**
 * @brief Struktura koja sadrži konstante za ISCRTAVANJE elemenata
 * na petom ekranu za podešavanja (detaljne postavke za svjetla).
 * @note  Definiše pravila za iscrtavanje elemenata u dvije kolone.
 */
static const struct
{
    /** @brief X pozicija za prvu kolonu widgeta i labela. */
    int16_t      col1_x;
    /** @brief X pozicija za drugu kolonu widgeta i labela. */
    int16_t      col2_x;
    /** @brief Početna Y pozicija za prvi red. */
    int16_t      start_y;
    /** @brief Vertikalni razmak (visina reda) između widgeta. */
    int16_t      y_step;
    /** @brief Dimenzije za SPINBOX widgete. */
    WidgetRect_t spinbox_size;
    /** @brief Dimenzije za prvi CHECKBOX. */
    WidgetRect_t checkbox1_size;
    /** @brief Dimenzije za drugi CHECKBOX. */
    WidgetRect_t checkbox2_size;
    /** @brief Relativni pomak za ispis prve linije labele. */
    GUI_POINT    label_line1_offset;
    /** @brief Vertikalni pomak za ispis druge linije labele. */
    int16_t      label_line2_offset_y;
    /** @brief Pozicija i dimenzije za "NEXT" dugme. */
    WidgetRect_t next_button_pos;
    /** @brief Pozicija i dimenzije za "SAVE" dugme. */
    WidgetRect_t save_button_pos;
}
settings_screen_5_layout =
{
    .col1_x                 = 10,
    .col2_x                 = 200,
    .start_y                = 5,
    .y_step                 = 43,
    .spinbox_size           = { 0, 0, 100, 40 }, // x,y se ne koriste, samo w,h
    .checkbox1_size         = { 0, 0, 130, 20 },
    .checkbox2_size         = { 0, 0, 145, 20 },
    .label_line1_offset     = { 110, 10 }, // 100 (sirina) + 10 (padding)
    .label_line2_offset_y   = 12,
    .next_button_pos        = { 410, 180, 60, 30 },
    .save_button_pos        = { 410, 230, 60, 30 }
};

/**
 * @brief Struktura koja sadrži konstante za ISCRTAVANJE elemenata
 * na šestom ekranu za podešavanja (Opšte postavke).
 */
static const struct
{
    WidgetRect_t device_id_pos;                 /**< @brief Pozicija i dimenzije za SPINBOX ID-a uređaja. */
    WidgetRect_t curtain_move_time_pos;         /**< @brief Pozicija i dimenzije za SPINBOX vremena kretanja roletni. */
    WidgetRect_t leave_scrnsvr_checkbox_pos;    /**< @brief Pozicija i dimenzije za CHECKBOX ponašanja screensaver-a. */
    WidgetRect_t night_timer_checkbox_pos;      /**< @brief Pozicija i dimenzije za CHECKBOX noćnog tajmera za svjetla. */
    WidgetRect_t enable_scenes_checkbox_pos;    /**< @brief Pozicija i dimenzije za CHECKBOX koji omogućava/onemogućava sistem scena. */
    WidgetRect_t set_defaults_button_pos;       /**< @brief Pozicija i dimenzije za "SET DEFAULTS" dugme. */
    WidgetRect_t restart_button_pos;            /**< @brief Pozicija i dimenzije za "RESTART" dugme. */
    WidgetRect_t next_button_pos;               /**< @brief Pozicija i dimenzije za "NEXT" dugme. */
    WidgetRect_t save_button_pos;               /**< @brief Pozicija i dimenzije za "SAVE" dugme. */

    GUI_POINT    device_id_label_pos[2];        /**< @brief Pozicije za dvorednu labelu za ID uređaja. */
    GUI_POINT    curtain_move_time_label_pos[2];/**< @brief Pozicije za dvorednu labelu za vrijeme kretanja roletni. */

    WidgetRect_t language_dropdown_pos;         /**< @brief Pozicija i dimenzije za DROPDOWN odabir jezika. */
    GUI_POINT    language_label_pos;            /**< @brief Pozicija za labelu "LANGUAGE". */
}
settings_screen_6_layout =
{
    .device_id_pos              = { 10, 10, 110, 40 },
    .curtain_move_time_pos      = { 10, 60, 110, 40 },
    .leave_scrnsvr_checkbox_pos = { 10, 110, 205, 20 },
    .night_timer_checkbox_pos   = { 10, 140, 170, 20 },
    .enable_scenes_checkbox_pos = { 10, 165, 240, 20 },
    .set_defaults_button_pos    = { 10, 190, 80, 30 },
    .restart_button_pos         = { 10, 230, 80, 30 },
    .next_button_pos            = { 410, 180, 60, 30 },
    .save_button_pos            = { 410, 230, 60, 30 },

    .device_id_label_pos        = { {130, 20}, {130, 32} },
    .curtain_move_time_label_pos= { {130, 70}, {130, 82} },

    .language_dropdown_pos      = { 220, 10, 110, 180 },
    .language_label_pos         = { 340, 22 } // 220 (x) + 150 (w) + 10 (padding)
};


/**
 * @brief Struktura koja sadrži konstante za ISCRTAVANJE elemenata
 * na sedmom ekranu za podešavanja (Scene Backend).
 */
static const struct
{
    WidgetRect_t enable_scenes_checkbox_pos;    /**< @brief Pozicija i dimenzije za CHECKBOX koji omogućava/onemogućava sistem scena. */
    GUI_POINT    grid_start_pos;                /**< @brief Početna tačka (gore-lijevo) za prvi widget u mreži okidača. */
    int16_t      widget_width;                  /**< @brief Širina jednog SPINBOX widgeta. */
    int16_t      widget_height;                 /**< @brief Visina jednog SPINBOX widgeta. */
    int16_t      y_spacing;                     /**< @brief Vertikalni razmak između redova. */
    int16_t      x_col_spacing;                 /**< @brief Horizontalni razmak između kolona. */
    GUI_POINT    label_offset;                  /**< @brief Relativni pomak za ispis labele. */
    WidgetRect_t next_button_pos;               /**< @brief Pozicija i dimenzije za "NEXT" dugme. */
    WidgetRect_t save_button_pos;               /**< @brief Pozicija i dimenzije za "SAVE" dugme. */
}
settings_screen_7_layout =
{
    .enable_scenes_checkbox_pos = { 10, 5, 240, 20 },
    .grid_start_pos         = { 10, 40 },
    .widget_width           = 110,
    .widget_height          = 35,
    .y_spacing              = 50,
    .x_col_spacing          = 190,
    .label_offset           = { 120, 18 },
    .next_button_pos        = { 410, 180, 60, 30 },
    .save_button_pos        = { 410, 230, 60, 30 }
};
/**
 * @brief Niz sa pokazivačima na bitmape za ikonice svjetala.
 * @note  IZMIJENJENO: Redoslijed u ovom nizu sada MORA TAČNO ODGOVARATI
 * redoslijedu u `enum IconID` definisanom u `display.h`.
 * Svaka ikonica ima par (OFF, ON).
 */
static GUI_CONST_STORAGE GUI_BITMAP* light_modbus_images[] = {
    // Indeksi 0, 1 (Odgovara ICON_BULB = 0)
    &bmSijalicaOff, &bmSijalicaOn,
    // Indeksi 2, 3 (Odgovara ICON_VENTILATOR_ICON = 1)
    &bmVENTILATOR_OFF, &bmVENTILATOR_ON,
    // Indeksi 4, 5 (Odgovara ICON_CEILING_LED_FIXTURE = 2)
    &bmicons_lights_ceiling_led_fixture_off, &bmicons_lights_ceiling_led_fixture_on,
    // Indeksi 6, 7 (Odgovara ICON_CHANDELIER = 3)
    &bmicons_lights_chandelier_off, &bmicons_lights_chandelier_on,
    // Indeksi 8, 9 (Odgovara ICON_HANGING = 4)
    &bmicons_lights_hanging_off, &bmicons_lights_hanging_on,
    // Indeksi 10, 11 (Odgovara ICON_LED_STRIP = 5)
    &bmicons_lights_led_off, &bmicons_lights_led_on,
    // Indeksi 12, 13 (Odgovara ICON_SPOT_CONSOLE = 6)
    &bmicons_lights_spot_console_off, &bmicons_lights_spot_console_on,
    // Indeksi 14, 15 (Odgovara ICON_SPOT_SINGLE = 7)
    &bmicons_lights_spot_single_off, &bmicons_lights_spot_single_on,
    // Indeksi 16, 17 (Odgovara ICON_STAIRS = 8)
    &bmicons_lights_stairs_off, &bmicons_lights_stairs_on,
    // Indeksi 18, 19 (Odgovara ICON_WALL = 9)
    &bmicons_lights_wall_off, &bmicons_lights_wall_on
};
/**
 * @brief Niz sa pokazivačima na bitmape ISKLJUČIVO za ikonice scena.
 * @note  Redoslijed u ovom nizu MORA tačno odgovarati redoslijedu enum-a
 * koji počinje sa ICON_SCENE_WIZZARD u fajlu display.h.
 */
static GUI_CONST_STORAGE GUI_BITMAP* scene_icon_images[] = {
    &bmicons_scene_wizzard,
    &bmicons_scene_morning,
    &bmicons_scene_sleep,
    &bmicons_scene_leaving,
    &bmicons_scene_homecoming,
    &bmicons_scene_movie,
    &bmicons_scene_dinner,
    &bmicons_scene_reading,
    &bmicons_scene_relaxing,
    &bmicons_scene_gathering,
    &bmicons_scene_security
};
/** @brief Niz sa definisanim bojama za sat na screensaver-u. */
static uint32_t clk_clrs[COLOR_BSIZE] = {
    GUI_GRAY, GUI_RED, GUI_BLUE, GUI_GREEN, GUI_CYAN, GUI_MAGENTA,
    GUI_YELLOW, GUI_LIGHTGRAY, GUI_LIGHTRED, GUI_LIGHTBLUE, GUI_LIGHTGREEN,
    GUI_LIGHTCYAN, GUI_LIGHTMAGENTA, GUI_LIGHTYELLOW, GUI_DARKGRAY, GUI_DARKRED,
    GUI_DARKBLUE,  GUI_DARKGREEN, GUI_DARKCYAN, GUI_DARKMAGENTA, GUI_DARKYELLOW,
    GUI_WHITE, GUI_BROWN, GUI_ORANGE, CLR_DARK_BLUE, CLR_LIGHT_BLUE, CLR_BLUE, CLR_LEMON
};

// =======================================================================
// ===        Automatsko generisanje `const` niza za "Skener"          ===
// =======================================================================
// ULOGA: Ovaj blok koda koristi X-Macro tehniku da bi automatski
// popunio `settings_static_widget_ids` niz sa ID-jevima iz `settings_widgets.def`.
//
static const uint16_t settings_static_widget_ids[] = {
    // 1. Privremeno redefinišemo makro WIDGET da vraća samo prvi argument (ID)
#define WIDGET(id, val, comment) id,
    // 2. Uključujemo našu glavnu listu
#include "settings_widgets.def"
    // 3. Odmah poništavamo našu privremenu definiciju da ne bi smetala ostatku koda
#undef WIDGET
};

/*============================================================================*/
/* HANDLE-OVI ZA GUI WIDGET-E                                                 */
/*============================================================================*/

static BUTTON_Handle   hBUTTON_Ok;                         // Handle za dugme "OK" ili "SAVE"
static BUTTON_Handle   hBUTTON_Next;                       // Handle za dugme "NEXT"
static BUTTON_Handle   hBUTTON_SET_DEFAULTS;               // Handle za dugme "SET DEFAULTS"
static BUTTON_Handle   hBUTTON_SYSRESTART;                 // Handle za dugme "RESTART"
static RADIO_Handle    hThstControl;                       // Handle za radio dugmad kontrole termostata
static RADIO_Handle    hFanControl;                        // Handle za radio dugmad kontrole ventilatora
static RADIO_Handle    hSelectControl_4;                   // Handle za radio dugmad selekcije kontrole korisničkog menija
static SPINBOX_Handle  hThstMaxSetPoint;                   // Handle za spinbox maksimalne zadane temperature
static SPINBOX_Handle  hThstMinSetPoint;                   // Handle za spinbox minimalne zadane temperature
static SPINBOX_Handle  hFanDiff;                           // Handle za spinbox diferencijala ventilatora
static SPINBOX_Handle  hFanLowBand;                        // Handle za spinbox opsega niske brzine ventilatora
static SPINBOX_Handle  hFanHiBand;                         // Handle za spinbox opsega visoke brzine ventilatora
static SPINBOX_Handle  hThstGroup;                         // Handle za spinbox grupe termostata
static CHECKBOX_Handle hThstMaster;                        // Handle za checkbox master moda termostata
static SPINBOX_Handle  hSPNBX_DisplayHighBrightness;       // Handle za spinbox visoke svjetline ekrana
static SPINBOX_Handle  hSPNBX_DisplayLowBrightness;        // Handle za spinbox niske svjetline ekrana
static SPINBOX_Handle  hSPNBX_ScrnsvrTimeout;              // Handle za spinbox screensaver timeouta
static SPINBOX_Handle  hSPNBX_ScrnsvrEnableHour;           // Handle za spinbox sata aktivacije screensavera
static SPINBOX_Handle  hSPNBX_ScrnsvrDisableHour;          // Handle za spinbox sata deaktivacije screensavera
static SPINBOX_Handle  hSPNBX_ScrnsvrClockColour;          // Handle za spinbox boje sata na screensaveru
static SPINBOX_Handle  hSPNBX_Hour;                        // Handle za spinbox sata
static SPINBOX_Handle  hSPNBX_Minute;                      // Handle za spinbox minute
static SPINBOX_Handle  hSPNBX_Day;                         // Handle za spinbox dana
static SPINBOX_Handle  hSPNBX_Month;                       // Handle za spinbox mjeseca
static SPINBOX_Handle  hSPNBX_Year;                        // Handle za spinbox godine
static CHECKBOX_Handle hCHKBX_ScrnsvrClock;                // Handle za checkbox screensaver sata
static DROPDOWN_Handle hDRPDN_WeekDay;                     // Handle za dropdown dan u sedmici
static SPINBOX_Handle  hVentilatorRelay;                   // Handle za spinbox releja ventilatora
static SPINBOX_Handle  hVentilatorDelayOn;                 // Handle za spinbox odgode paljenja ventilatora
static SPINBOX_Handle  hVentilatorDelayOff;                // Handle za spinbox odgode gašenja ventilatora
static SPINBOX_Handle  hVentilatorTriggerSource1;          // Handle za spinbox izbor prvog lokalnog okidača ventilatora
static SPINBOX_Handle  hVentilatorTriggerSource2;          // Handle za spinbox izbor drugog lokalnog okidača ventilatora
static SPINBOX_Handle  hVentilatorLocalPin;                // Handle za spinbox izbor lokalnog pina ventilatora
static SPINBOX_Handle  hCurtainsRelay[CURTAINS_SIZE * 2];  // Niz handle-ova za spinboxove releja zavjesa
static SPINBOX_Handle  hCurtainsMoveTime;                  // Handle za spinbox vremena kretanja zavjesa
static SPINBOX_Handle  hDEV_ID;                            // Handle za spinbox ID-a uređaja
static CHECKBOX_Handle hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH; // Handle za checkbox screensaver ponašanja
static CHECKBOX_Handle hCHKBX_LIGHT_NIGHT_TIMER;           // Handle za checkbox noćnog tajmera za svjetla
static CHECKBOX_Handle hCHKBX_EnableScenes;                // Handle za checkbox koji omogućava/onemogućava sistem scena.
static LIGHT_settingsWidgets lightsWidgets[LIGHTS_MODBUS_SIZE]; // Niz struktura za postavke svjetala
static Defroster_settingsWidgets defroster_settingWidgets;
static DROPDOWN_Handle hDRPDN_Language;
static BUTTON_Handle hKeypadButtons[12];
static BUTTON_Handle hButtonRenameLight;
static SPINBOX_Handle  hGateSelect;
static DROPDOWN_Handle hGateType;
static BUTTON_Handle   hGateEditButtons[9]; // 9 tastera za editovanje numeričkih vrijednosti
/**
 * @brief Handle za dugme "[ Promijeni ]" unutar čarobnjaka za scene.
 * @note Pokreće ekran za odabir izgleda i naziva scene.
 */
static BUTTON_Handle hButtonChangeAppearance;
static BUTTON_Handle hButtonDeleteScene;          /**< Dugme [Obriši] za brisanje postojeće scene. */
static BUTTON_Handle hButtonDetailedSetup;        /**< Dugme [Detaljna Podešavanja] za ulazak u "anketni" čarobnjak. */
/** @name Handle-ovi za Widgete "Anketnog" Čarobnjaka
 * @{
 */
static CHECKBOX_Handle hCheckboxSceneLights;      /**< Checkbox za uključivanje svjetala u scenu. */
static CHECKBOX_Handle hCheckboxSceneCurtains;    /**< Checkbox za uključivanje roletni u scenu. */
static CHECKBOX_Handle hCheckboxSceneThermostat;  /**< Checkbox za uključivanje termostata u scenu. */
static BUTTON_Handle hButtonWizNext;              /**< Dugme [Dalje] unutar čarobnjaka. */
static BUTTON_Handle hButtonWizBack;              /**< Dugme [Nazad] unutar čarobnjaka. */
static BUTTON_Handle hButtonWizCancel;            /**< Dugme [Otkaži] unutar čarobnjaka. */
/** @} */
static SPINBOX_Handle  hSPNBX_SceneTriggers[SCENE_MAX_TRIGGERS];  /**< Handle-ovi za spinbox-ove za adrese okidača na ekranu za podešavanje scena. */

/*============================================================================*/
/* GLOBALNE VARIJABLE NA NIVOU PROJEKTA                                       */
/*============================================================================*/

/**
 * @brief Glavni 32-bitni registar sa flegovima za kompletan display modul.
 * @note Mora biti globalan da bi drugi moduli (npr. thermostat) mogli
 * da signaliziraju promjene putem makroa (npr. MVUpdateSet()).
 */
uint32_t dispfl;

/**
 * @brief Varijable koje čuvaju trenutno stanje korisničkog interfejsa.
 * @note Moraju biti globalne jer drugi moduli (rs485, lights, curtain)
 * čitaju 'screen' i postavljaju 'shouldDrawScreen' da zatraže osvježavanje.
 */
uint8_t screen, shouldDrawScreen;

/**
 * @brief Indeks trenutno odabrane zavjese.
 * @note Mora biti globalan jer ga `curtain.c` koristi da zna kojom zavjesom
 * se manipuliše preko GUI-ja.
 */
uint8_t curtain_selected;

// Definicija globalne instance strukture za postavke
Display_EepromSettings_t g_display_settings;

/*============================================================================*/
/* STATIČKE VARIJABLE NA NIVOU MODULA                                         */
/*============================================================================*/
static uint8_t thermostatMenuState = 0;                     // Zamjena za globalni 'menu_thst'
static bool dynamicIconUpdateFlag = false;                  // Zamjena za globalni 'ctrl1'
static uint32_t rtctmr;                                     // Tajmer za periodično osvježavanje vremena.
static uint32_t thermostatOnOffTouch_timer = 0;             // Tajmer za detekciju dugog pritiska na termostatu.
static uint32_t scrnsvr_tmr;                                // Tajmer za praćenje neaktivnosti korisnika.
static uint32_t light_settingsTimerStart = 0;               // Tajmer za detekciju dugog pritiska za ulazak u podešavanja svjetla.
static uint32_t everyMinuteTimerStart = 0;                  // Tajmer koji se okida svake minute.
static uint32_t onoff_tmr = 0;                              // Tajmer za logiku on/off dodira.
static uint32_t value_step_tmr = 0;                         // Tajmer za postepenu promjenu vrijednosti (dimovanje).
static uint32_t refresh_tmr = 0;                            // Tajmer za periodično osvježavanje ekrana termostata.
static uint32_t clean_tmr = 0;                              // Tajmer za ekran za čišćenje.
static bool touch_in_menu_zone = false;                     // Fleg, true ako je dodir počeo u zoni menija.
static uint8_t menu_clean = 0;                              // Stanje menija za čišćenje.
static uint8_t menu_lc = 0;                                 // Stanje menija za odabir kontrole.
static uint8_t curtainSettingMenu = 0;                      // Trenutna stranica u meniju za podešavanje zavjesa.
static uint8_t lightsModbusSettingsMenu = 0;                // Trenutna stranica u meniju za podešavanje svjetala.
static uint8_t light_selectedIndex = LIGHTS_MODBUS_SIZE + 1;// Indeks odabranog svjetla.
static uint8_t lights_allSelected_hasRGB = 0;               // Fleg, true ako među odabranim svjetlima ima RGB.
static uint8_t settingsChanged = 0;                         // Fleg, true ako je došlo do promjene u podešavanjima.
static uint8_t thsta = 0;                                   // Fleg za promjene u podešavanjima termostata.
static uint8_t lcsta = 0;                                   // Fleg za promjene u podešavanjima svjetala.
static uint8_t btnset = 0;                                  // Fleg za detekciju dugog pritiska za ulazak u meni.
static uint8_t btninc, _btninc;                             // Flegovi za dugme "increase".
static uint8_t btndec, _btndec;                             // Flegovi za dugme "decrease".
static uint8_t old_min = 60;                                // Prethodna vrijednost minuta, za detekciju promjene.
static uint8_t old_day = 0;                                 // Prethodna vrijednost dana, za detekciju promjene.
static uint8_t qr_codes[QR_CODE_COUNT][QR_CODE_LENGTH] = {0}; // Bafer za čuvanje stringova QR kodova.
static uint8_t qr_code_draw_id = 0;                         // ID QR koda koji treba iscrtati.
static uint8_t clrtmr = 0;
static GUI_PID_STATE last_press_state; 

/** @name Statičke varijable za Alfanumeričku Tastaturu
 * @{
 */
static WM_HWIN hKeyboardButtons[KEY_ROWS * KEYS_PER_ROW]; ///< Handle-ovi za dinamički kreirane tastere sa karakterima.
static WM_HWIN hKeyboardSpecialButtons[5];              ///< Handle-ovi za specijalne tastere (Shift, Del, Space, OK, Cancel).
static char    keyboard_buffer[32];                     ///< Interni bafer za unos teksta.
static uint8_t keyboard_buffer_idx = 0;                 ///< Trenutna pozicija (indeks) u baferu za unos.
static bool    keyboard_shift_active = false;             ///< Fleg koji prati da li je Shift (ili CapsLock) aktivan.
/** @} */
// Fleg za odbrojavanje na ekranu za ciscenje.
/* Privatne varijable za PIN tastaturu */

static char pin_buffer[MAX_PIN_LENGTH + 1];
static uint8_t pin_buffer_idx = 0;
static uint32_t pin_mask_timer = 0;
static bool pin_error_active = false;
static char pin_last_char = 0;
/**
 * @brief Tajmer za detekciju dugog pritiska na naziv svjetla za pokretanje izmjene.
 * @note Pokreće se kada korisnik pritisne zonu sa tekstom u `SCREEN_LIGHT_SETTINGS`.
 * Provjerava se u `Handle_PeriodicEvents`, a resetuje u `HandleTouchReleaseEvent`.
 */
static uint32_t rename_light_timer_start = 0;
/**
 * @brief Čuva ID posljednjeg pritisnutog "EDIT" dugmeta na ekranu za podešavanje kapija.
 * @note  Koristi se za obradu rezultata vraćenog sa Numpad-a.
 */
static int active_gate_edit_button_id = 0;
/**
 * @brief Indeks kapije (0-5) koja je trenutno odabrana za podešavanje.
 */
static uint8_t settings_gate_selected_index = 0;
/**
 * @brief Čuva ID ekrana na koji se treba vratiti nakon zatvaranja Numpad-a.
 */
static eScreen numpad_return_screen = SCREEN_MAIN;
/**
 * @brief Čuva ID ekrana na koji se treba vratiti nakon zatvaranja alfanumeričke tastature.
 */
static eScreen keyboard_return_screen = SCREEN_MAIN;
/**
 * @brief Čuva ID ekrana na koji se treba vratiti nakon izlaska iz podešavanja svjetla.
 * @note  Ovo je neophodno jer se u SCREEN_LIGHT_SETTINGS može ući sa više
 * različitih ekrana (npr. SCREEN_MAIN ili SCREEN_LIGHTS).
 */
static eScreen light_settings_return_screen = SCREEN_MAIN;
/**
 * @brief Indeks scene (0-5) koja se trenutno kreira ili mijenja.
 * @note Ovu varijablu postavlja `HandlePress_SceneScreen` kada korisnik dodirne
 * neki od slotova za scene, a `DSP_InitSceneEditScreen` i `HandlePress_SceneEditScreen`
 * je koriste da znaju nad kojom scenom treba izvršiti akcije.
 */
static uint8_t scene_edit_index = 0;
/**
 * @brief Timestamp (HAL_GetTick()) kada je korisnik pritisnuo ikonicu na ekranu sa scenama.
 * @note  Koristi se za izračunavanje trajanja pritiska kako bi se razlikovao
 * kratak klik (aktivacija) od dugog pritiska (editovanje). Vrijednost 0
 * označava da pritisak nije aktivan.
 */
static uint32_t scene_press_timer_start = 0;
/**
 * @brief Indeks slota (0-5) koji je korisnik pritisnuo na ekranu sa scenama.
 * @note  Čuva se prilikom pritiska, a koristi prilikom otpuštanja.
 * Vrijednost -1 označava da nijedan slot nije pritisnut.
 */
static int8_t scene_pressed_index = -1;
/**
 * @brief Indeks trenutne stranice na ekranu za odabir izgleda scene.
 * @note  Koristi se za paginaciju ako ima više ikonica nego što može stati
 * na jedan ekran. Vrijednost 0 je prva stranica.
 */
static uint8_t scene_appearance_page = 0;
/**
 * @brief Globalni fleg unutar display modula koji prati da li je sistem u "Scene Wizard" modu.
 * @note  Ovaj fleg je ključan za kontekstualnu promjenu ponašanja ekrana.
 * Kada je `true`, ekrani kao što su SCREEN_LIGHTS ili SCREEN_THERMOSTAT
 * će prikazati "Dalje" dugme umjesto standardnog interfejsa.
 * Postavlja se na `true` pri ulasku u "anketni" čarobnjak, a na `false`
 * pri izlasku (snimanje, otkazivanje, timeout).
 */
static bool is_in_scene_wizard_mode = false;
/*============================================================================*/
/*============================================================================*/
/* PROTOTIPOVI PRIVATNIH (STATIC) FUNKCIJA                                    */
/*============================================================================*/

/**
 * @name Funkcije za Inicijalizaciju i Uništavanje Ekrana Podešavanja
 * @brief Grupa funkcija odgovornih za kreiranje (`Init`) i uništavanje (`Kill`)
 * svih GUI widgeta na pojedinačnim ekranima za podešavanja.
 * @{
 */
static void DSP_InitSet1Scrn(void);
static void DSP_InitSet2Scrn(void);
static void DSP_InitSet3Scrn(void);
static void DSP_InitSet4Scrn(void);
static void DSP_InitSet5Scrn(void);
static void DSP_InitSet6Scrn(void);
static void DSP_InitSet7Scrn(void);
static void DSP_KillSet1Scrn(void);
static void DSP_KillSet2Scrn(void);
static void DSP_KillSet3Scrn(void);
static void DSP_KillSet4Scrn(void);
static void DSP_KillSet5Scrn(void);
static void DSP_KillSet6Scrn(void);
static void DSP_KillSet7Scrn(void);
static void DSP_InitSceneEditScreen(void);
static void DSP_KillSceneEditScreen(void);
static void DSP_InitSettingsGateScreen(void);
static void DSP_KillSettingsGateScreen(void);
static void DSP_KillLightSettingsScreen(void);
static void DSP_InitSceneAppearanceScreen(void);
static void DSP_InitSceneWizDevicesScreen(void);
static void DSP_KillSceneWizDevicesScreen(void);
static void DSP_KillSceneAppearanceScreen(void);
static void DSP_KillSceneScreen(void);
static void DSP_KillSceneEditLightsScreen(void);
static void DSP_KillSceneEditCurtainsScreen(void);
static void DSP_KillSceneEditThermostatScreen(void);
static void DSP_InitSceneWizFinalizeScreen(void);
static void DSP_KillSceneWizFinalizeScreen(void);

/** @} */

/**
 * @name Servisne Funkcije za Upravljanje Ekranima
 * @brief Svaka od ovih funkcija predstavlja "state" u glavnoj state-machine
 * petlji (`DISP_Service`). Poziva se periodično dok je odgovarajući
 * ekran aktivan i sadrži logiku za taj ekran.
 * @{
 */
static void Service_MainScreen(void);
static void Service_SelectScreen1(void);
static void Service_SelectScreen2(void);
static void Service_SelectScreenLast(void);
static void Service_ThermostatScreen(void);
static void Service_ReturnToFirst(void);
static void Service_SceneScreen(void);
static void Service_CleanScreen(void);
static void Service_SettingsScreen_1(void);
static void Service_SettingsScreen_2(void);
static void Service_SettingsScreen_3(void);
static void Service_SettingsScreen_4(void);
static void Service_SettingsScreen_5(void);
static void Service_SettingsScreen_6(void);
static void Service_SettingsScreen_7(void);
static void Service_LightsScreen(void);
static void Service_GateScreen(void);
static void Service_TimerScreen(void);
static void Service_SecurityScreen(void);
static void Service_CurtainsScreen(void);
static void Service_QrCodeScreen(void);
static void Service_LightSettingsScreen(void);
static void Service_MainScreenSwitch(void);
static void Service_SceneAppearanceScreen(void);
static void Service_SceneWizDevicesScreen(void);
static void Service_SceneEditScreen(void);
static void Service_SceneEditLightsScreen(void);
static void Service_SceneEditThermostatScreen(void);
static void Service_SceneWizFinalizeScreen(void);
/** @} */

/**
 * @brief Glavna "hook" funkcija za obradu dodira na ekran.
 * @note Ovu funkciju poziva STemWin GUI biblioteka svaki put kada detektuje
 * promjenu stanja dodira. Ona je ulazna tačka za sve korisničke interakcije.
 * @param pTS Pokazivač na strukturu sa stanjem dodira (koordinate, pritisnut/otpušten).
 */
static void PID_Hook(GUI_PID_STATE * pTS);
/**
 * @brief Upravlja svim periodičnim događajima i tajmerima.
 * @note Poziva se iz `DISP_Service`. Odgovorna za logiku screensaver-a,
 * noćnog tajmera za svjetla, ažuriranje vremena i druge pozadinske zadatke.
 */
static void Handle_PeriodicEvents(void);
/**
 * @brief Dispečer za događaje pritiska na ekran.
 * @note Poziva se iz `PID_Hook` kada je ekran pritisnut. Na osnovu trenutnog
 * ekrana (`screen`), prosljeđuje događaj odgovarajućoj `HandlePress_...` funkciji.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 * @param click_flag Pokazivač na fleg koji se postavlja ako treba generisati zvučni signal.
 */
static void HandleTouchPressEvent(GUI_PID_STATE * pTS, uint8_t *click_flag);
/**
 * @brief Dispečer za događaje otpuštanja dodira sa ekrana.
 * @note Poziva se iz `PID_Hook` kada korisnik otpusti prst sa ekrana.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 */
static void HandleTouchReleaseEvent(GUI_PID_STATE * pTS);

static void HandlePress_SelectScreen1(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_SelectScreen2(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_ThermostatScreen(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_LightsScreen(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_CurtainsScreen(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_SelectScreenLast(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_LightSettingsScreen(GUI_PID_STATE * pTS);
static void HandlePress_MainScreenSwitch(GUI_PID_STATE * pTS);
static void HandleRelease_MainScreenSwitch(GUI_PID_STATE * pTS);
static void HandlePress_SceneAppearanceScreen(GUI_PID_STATE* pTS, uint8_t* click_flag);
static void HandlePress_SceneScreen(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandleRelease_SceneScreen(void);
/** @} */
/* Prototipovi novih funkcija za PIN tastaturu */
static void DSP_DrawNumpadText(void);
static void DSP_InitNumpadScreen(void);
static void Service_NumpadScreen(void);
static void DSP_KillNumpadScreen(void);
static void Display_ShowNumpad(const NumpadContext_t* context);

// NOVI PROTOTIPOVI ZA ALFANUMERIČKU TASTATURU
static void Display_ShowKeyboard(const KeyboardContext_t* context);
static void DSP_InitKeyboardScreen(void);
static void Service_KeyboardScreen(void);
static void DSP_KillKeyboardScreen(void);

static void DISPSetBrightnes(uint8_t val);
/**
 * @name Pomoćne (Utility) Funkcije
 * @brief Grupa internih funkcija za razne zadatke kao što su snimanje,
 * iscrtavanje zajedničkih elemenata, upravljanje menijima, itd.
 * @{
 */
static void DISP_Animation(void);
/**
 * @brief Snima trenutne postavke displeja u EEPROM.
 */
static void Display_Save(void);

/**
 * @brief Postavlja sve postavke displeja na sigurne fabričke vrijednosti.
 */
static void Display_SetDefault(void);

/**
 * @brief Inicijalizuje postavke displeja iz EEPROM-a pri startu sistema.
 */
static void Display_InitSettings(void);

/**
 * @brief Iscrtava i ažurira prikaz datuma i vremena na ekranu.
 */
static void DISPDateTime(void);

/**
 * @brief Iscrtava ikonu "hamburger" menija u gornjem desnom uglu.
 */
static void DrawHamburgerMenu(uint8_t position);

/**
 * @brief Detektuje i obrađuje dugi pritisak za ulazak u meni za podešavanja.
 * @param btn Fleg koji ukazuje na početak pritiska (postavljen u `PID_Hook`).
 * @retval uint8_t 1 ako je dugi pritisak detektovan, inače 0.
 */
static uint8_t DISPMenuSettings(uint8_t btn);

/**
 * @brief Provjerava status ažuriranja firmvera i prikazuje odgovarajuću poruku.
 * @retval uint8_t 1 ako je ažuriranje aktivno, inače 0.
 */
static uint8_t Service_HandleFirmwareUpdate(void);

/**
 * @brief Forsirano uništava sve widgete sa ekrana za podešavanja.
 * @note Koristi se kao "fail-safe" mehanizam za čišćenje "duhova" sa ekrana.
 */
static void ForceKillAllSettingsWidgets(void);

static bool IsBusFwUpdateActive(void);
/** @} */

/*============================================================================*/
/* IMPLEMENTACIJA JAVNIH FUNKCIJA (GRUPA 1/12)                                */
/*============================================================================*/
/**
 * @brief Inicijalizuje GUI sistem.
 * @note Poziva se jednom na početku programa iz main() funkcije.
 * Inicijalizuje STemWin, postavlja hook za touch, učitava
 * parametre iz EEPROM-a i odmah iscrtava glavni ekran.
 */
void DISP_Init(void)
{
    uint8_t len;

    Display_InitSettings();

    // Inicijalizacija STemWin grafičke biblioteke
    GUI_Init();
    // Povezivanje (hook) funkcije za obradu dodira sa GUI sistemom
    GUI_PID_SetHook(PID_Hook);
    // Omogućavanje višestrukog baferovanja za fluidnije iscrtavanje
    WM_MULTIBUF_Enable(1);
    // Postavljanje UTF-8 enkodiranja za podršku specijalnim karakterima
    GUI_UC_SetEncodeUTF8();
    // Odabir i čišćenje prvog sloja (layer 0)
    GUI_SelectLayer(0);
    GUI_Clear();
    // Odabir drugog sloja (layer 1) i postavljanje providne pozadine
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    //DISP_Animation();
    // =======================================================================
    // === KRAJ NOVE LOGIKE ===
    // =======================================================================
    // Učitavanje prvog QR koda
    EE_ReadBuffer(&len, EE_QR_CODE1, 1);
    if (len < QR_CODE_LENGTH) {
        EE_ReadBuffer(&qr_codes[0][0], EE_QR_CODE1 + 1, len);
    }

    // Učitavanje drugog QR koda
    EE_ReadBuffer(&len, EE_QR_CODE2, 1);
    if (len < QR_CODE_LENGTH) {
        EE_ReadBuffer(&qr_codes[1][0], EE_QR_CODE2 + 1, len);
    }

    // Pokretanje tajmera koji se okida svake minute
    everyMinuteTimerStart = HAL_GetTick();

    // =======================================================================
    // === POČETAK NOVE "PAMETNE" LOGIKE ODABIRA POČETNOG EKRANA ===
    // =======================================================================
    // KORAK 1: Dobijamo handle-ove za sve relevantne module.
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();
    // Ovdje ćemo dodati i gettere za Gate, Timer, Security kada njihovi moduli budu imali tu funkciju.

    // KORAK 2: Provjeravamo konfiguraciju po prioritetima.
    bool has_lights = (LIGHTS_getCount() > 0);
    bool has_thermostat = (Thermostat_GetGroup(pThst) > 0);
    bool has_curtains = (Curtains_getCount() > 0);
    // bool has_gates = (Gates_GetCount() > 0); // Primjer za budućnost

    if (has_lights) {
        // Ako ima svjetala, početni ekran je uvijek Main screen.
        screen = SCREEN_MAIN;
    } else if (has_thermostat && !has_curtains /* && !has_gates... */) {
        // Ako je JEDINA konfigurisana funkcija termostat.
        screen = SCREEN_THERMOSTAT;
    } else if (has_thermostat || has_curtains /* || has_gates... */) {
        // Ako nema svjetala, ali ima VIŠE drugih funkcija, scene su najbolji dashboard.
        screen = SCREEN_SCENE;
    } else {
        // Ako nema NIJEDNE konfigurisane glavne funkcije, provjeravamo SelectScreen1.
        // Ovdje ćemo dodati provjeru za Ventilator/Defroster.
        // Za sada, ako nema ničega, idemo na poruku za konfiguraciju.
        screen = SCREEN_CONFIGURE_DEVICE; // Privremeno, dok ne dodamo sve provjere
    }

    // Fallback ako nijedan uslov nije zadovoljen (iako ne bi trebalo da se desi)
    if (screen == 0) { // Sigurnosna provjera
        screen = SCREEN_MAIN; // Vraćamo na Main kao najsigurniju opciju
    }

    // =======================================================================
    // === KRAJ NOVE LOGIKE ===
    // =======================================================================

    // Ažurira se shouldDrawScreen za prvu petlju da bi se iscrtao odabrani ekran.
    shouldDrawScreen = 1;
}
/**
 * @brief Glavna servisna funkcija za korisnički interfejs.
 * @note Poziva se periodično iz glavne petlje programa (while(1) u main.c).
 * Odgovorna je za ažuriranje GUI-ja, obradu tajmera i pozivanje
 * specifičnih servisnih funkcija u zavisnosti od aktivnog ekrana.
 */
void DISP_Service(void)
{
    static uint32_t guitmr = 0;

    // Ažuriranje GUI-ja i obrada TS-a u fiksnim intervalima
    if ((HAL_GetTick() - guitmr) >= GUI_REFRESH_TIME) {
        guitmr = HAL_GetTick();
        GUI_Exec(); // Izvršava sve pending operacije iscrtavanja
    }

    // Provjera i prikaz poruke o ažuriranju firmvera
    if (Service_HandleFirmwareUpdate()) {
        return; // Ako je ažuriranje u toku, prekini dalje izvršavanje GUI logike
    }

    // Glavni switch za upravljanje stanjima (ekranima)
    switch (screen) {
    case SCREEN_MAIN:
        Service_MainScreen();
        break;
    case SCREEN_SELECT_1:
        Service_SelectScreen1();
        break;
    case SCREEN_SELECT_2:
        Service_SelectScreen2();
        break;
    case SCREEN_SCENE:
        Service_SceneScreen();
        break;
    case SCREEN_SCENE_EDIT:
        Service_SceneEditScreen();
        break;
    case SCREEN_SCENE_APPEARANCE:
        Service_SceneAppearanceScreen();
        break;
    case SCREEN_SCENE_WIZ_DEVICES:
        Service_SceneWizDevicesScreen();
        break;
    case SCREEN_SELECT_LAST:
        Service_SelectScreenLast();
        break;
    case SCREEN_THERMOSTAT:
        Service_ThermostatScreen();
        break;
    case SCREEN_RETURN_TO_FIRST:
        Service_ReturnToFirst();
        break;
    case SCREEN_SETTINGS_1:
        Service_SettingsScreen_1();
        break;
    case SCREEN_SETTINGS_2:
        Service_SettingsScreen_2();
        break;
    case SCREEN_SETTINGS_3:
        Service_SettingsScreen_3();
        break;
    case SCREEN_SETTINGS_4:
        Service_SettingsScreen_4();
        break;
    case SCREEN_SETTINGS_5:
        Service_SettingsScreen_5();
        break;
    case SCREEN_SETTINGS_6:
        Service_SettingsScreen_6();
        break;
    case SCREEN_SETTINGS_7:
        Service_SettingsScreen_7();
        break;
    case SCREEN_CLEAN:
        Service_CleanScreen();
        break;
    case SCREEN_NUMPAD:
        Service_NumpadScreen();
        break;
    case SCREEN_LIGHTS:
        Service_LightsScreen();
        break;
    case SCREEN_CURTAINS:
        Service_CurtainsScreen();
        break;
    case SCREEN_GATE:
        Service_GateScreen();
        break;
    case SCREEN_TIMER:
        Service_TimerScreen();
        break;
    case SCREEN_SECURITY:
        Service_SecurityScreen();
        break;
    case SCREEN_QR_CODE:
        Service_QrCodeScreen();
        break;
    case SCREEN_LIGHT_SETTINGS:
        Service_LightSettingsScreen();
        break;
    case SCREEN_RESET_MENU_SWITCHES:
        Service_MainScreenSwitch();
        break;
    default:
        // U slučaju nepoznatog stanja, resetuj flegove menija
        menu_lc = 0;
        thermostatMenuState = 0;
        break;
    }

    // Upravljanje periodičnim događajima i tajmerima (npr. screensaver)
    Handle_PeriodicEvents();

    // Provjera da li treba ući u meni za podešavanja (dugi pritisak)
    if (DISPMenuSettings(btnset) && (screen < SCREEN_SETTINGS_1)) {
        // Inicijalizuj prvi ekran podešavanja
        DSP_InitSet1Scrn();
        screen = SCREEN_SETTINGS_1;
    }
}
/**
 * @brief Prikazuje zadanu temperaturu (Set Point) na ekranu termostata.
 */
void DISPSetPoint(void)
{
    // Deklaracija lokalnih const varijabli
    const int spHPos = 200; // Horizontalna pozicija za SetPoint
    const int spVPos = 150; // Vertikalna pozicija za SetPoint

    // 1. Dobijamo handle za termostat.
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();

    // Koristi se višestruko baferovanje za iscrtavanje bez treperenja
    GUI_MULTIBUF_BeginEx(1);

    // Očisti pravougaonik gdje se nalazi vrijednost
    GUI_ClearRect(spHPos - 5, spVPos - 5, spHPos + 120, spVPos + 85);

    // Postavi boju, font i poravnanje
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_D48);
    GUI_SetTextMode(GUI_TM_NORMAL);
    GUI_SetTextAlign(GUI_TA_RIGHT);
    GUI_GotoXY(spHPos, spVPos);

    // Ispiši vrijednost zadate temperature
    GUI_DispDec(Thermostat_GetSetpoint(pThst), 2);

    // Završi operaciju iscrtavanja
    GUI_MULTIBUF_EndEx(1);
}

/**
 * @brief Resetuje tajmer za screensaver i postavlja visoku svjetlinu ekrana.
 * @note Ova funkcija se poziva nakon svakog dodira na ekran.
 */
void DISPResetScrnsvr(void)
{
    const int scrnsvrTout = 30; // 30 sekundi za izlaz iz screensavera
    // Ako je screensaver bio aktivan, vrati se na početni ekran
    if(IsScrnsvrActiv() && IsScrnsvrEnabled()) {
        screen = SCREEN_RETURN_TO_FIRST;
    }
    // Resetuj softverske flegove za screensaver
    ScrnsvrReset();
    ScrnsvrInitReset();
    // Resetuj tajmer neaktivnosti na trenutno vrijeme
    scrnsvr_tmr = HAL_GetTick();
    // Vrati default timeout vrijednost
    g_display_settings.scrnsvr_tout = scrnsvrTout;
    // Postavi visoku (normalnu) svjetlinu ekrana
    DISPSetBrightnes(g_display_settings.high_bcklght);
}

/**
 * @brief Hook funkcija za obradu događaja sa ekrana osjetljivog na dodir.
 * @note  Ovo je centralna tačka za obradu korisničkog unosa. STemWin poziva
 * ovu funkciju svaki put kada detektuje promjenu stanja dodira.
 * @param pTS Pointer na strukturu sa stanjem dodira (koordinate, pritisnut/otpušten).
 */
/**
 * @brief Hook funkcija za obradu događaja sa ekrana osjetljivog na dodir.
 * @param pTS Pointer na strukturu sa stanjem dodira (koordinate, pritisnut/otpušten).
 */
void PID_Hook(GUI_PID_STATE * pTS)
{
    static uint8_t release = 0; // Fleg koji prati da li je dodir bio pritisnut i čeka otpuštanje.
    uint8_t click = 0;          // Fleg koji signalizira da treba generisati zvučni signal (klik).

    // NOVO: Potpuna blokada dodira ako je detektovana aktivnost ažuriranja na busu.
    if (IsBusFwUpdateActive()) {
        // Ako je ažuriranje aktivno, resetujemo screensaver da ekran ostane upaljen
        // i vidljiv, ali ignorišemo svaki unos dodirom.
        DISPResetScrnsvr();
        return;
    }

    // Provjera da li je dodir registrovan na početku s nulama, i resetuj btnset.
    // NOVO: Svaka linija je objašnjena
    if(pTS->x == 0 && pTS->y == 0 && pTS->Pressed == 0) { // Provjerava da li su koordinate nula i pritisak nula
        btnset = 0; // Ako je početno stanje, resetuj fleg za dugi pritisak
        return; // Izlazak iz funkcije
    }

    // Ostatak koda ostaje nepromijenjen
    if (screen == SCREEN_CLEAN) { // Ako je ekran za čišćenje, ignoriraj dodir
        return; // Izlazak iz funkcije
    }

    // --- Obrada pritiska na ekran ---
    if (pTS->Pressed == 1U) { // Ako je pritisak registrovan
        pTS->Layer = 1U; // Događaj se odnosi na gornji sloj (layer 1)
        release = 1;     // Postavi fleg da se čeka otpuštanje

        // << IZMJENA: Provjera zone hamburger menija sada koristi `global_layout` strukturu >>
        if ((pTS->x >= global_layout.hamburger_menu_zone.x0) && (pTS->x < global_layout.hamburger_menu_zone.x1) &&
                (pTS->y >= global_layout.hamburger_menu_zone.y0) && (pTS->y < global_layout.hamburger_menu_zone.y1) &&
                (screen < SCREEN_SETTINGS_1 && screen != SCREEN_KEYBOARD_ALPHA && screen != SCREEN_SCENE_APPEARANCE) && 
                !is_in_scene_wizard_mode)
        {
            touch_in_menu_zone = true; // Postavi fleg da je dodir počeo u zoni menija
            click = 1;                 // Svaki dodir u ovoj zoni generiše zvučni signako setuje l

            // << ISPRAVKA: Centralizovano brisanje ekrana prilikom svake navigacije >>
            // Ovo osigurava da je ekran uvijek čist prije iscrtavanja novog sadržaja.
            GUI_SelectLayer(0);
            GUI_Clear();
            GUI_SelectLayer(1);
            GUI_Clear();

            // =======================================================================
            // === POČETAK IZMJENE: Nova, ispravna logika navigacije ===
            // =======================================================================
            switch(screen) {
                // Pravilo: Sa bilo kog SELECT ekrana, hamburger vodi na MAIN
                case SCREEN_SELECT_1:
                case SCREEN_SELECT_2:
                case SCREEN_SELECT_LAST:
                case SCREEN_SCENE:    
                    screen = SCREEN_MAIN;
                    break;
                
                // Pravilo: Sa pod-menija prvog nivoa, hamburger vodi na SELECT_1
                case SCREEN_THERMOSTAT:
                    thermostatMenuState = 0; // Resetuj stanje ekrana termostata
                    // Propadanje je namjerno
                case SCREEN_LIGHTS:
                case SCREEN_CURTAINS:
                    screen = SCREEN_SELECT_1;
                    break;
                
                // Pravilo: Sa pod-menija drugog nivoa, hamburger vodi na SELECT_2
                case SCREEN_GATE:
                case SCREEN_TIMER:
                case SCREEN_SECURITY:
                    screen = SCREEN_SELECT_2;
                    break;
                
                // Pravilo: Sa pod-menija trećeg nivoa, hamburger vodi na SELECT_LAST
                case SCREEN_QR_CODE:
                    menu_lc = 0; // Resetuj parametar
                    screen = SCREEN_SELECT_LAST;
                    break;
                
                // Pravilo: Sa MAIN ekrana, hamburger vodi u prvi meni
                case SCREEN_MAIN:
                    screen = SCREEN_SELECT_1;
                    break;
                
                // Specijalni slučajevi ostaju isti
                case SCREEN_LIGHT_SETTINGS:
                    DSP_KillLightSettingsScreen();
                    screen = light_settings_return_screen;
                    break;
                
                case SCREEN_NUMPAD:
                    g_numpad_result.is_cancelled = true;
                    screen = numpad_return_screen;
                    break;
                
                case SCREEN_KEYBOARD_ALPHA:
                    g_keyboard_result.is_cancelled = true;
                    screen = keyboard_return_screen;
                    break;
            }
            // =======================================================================
            // === KRAJ IZMJENE ===
            // =======================================================================
            
            shouldDrawScreen = 1; // Uvijek zatraži iscrtavanje nakon promjene ekrana
            btnset = 1; // Fleg za dugi pritisak
        } else {
            touch_in_menu_zone = false;
            HandleTouchPressEvent(pTS, &click);
        }
        if (click) {
            BuzzerOn();
            HAL_Delay(1);
            BuzzerOff();
            click = 0;
        }
    }
    else { // Ako je dodir otpušten
        if(release) {
            release = 0;
            HandleTouchReleaseEvent(pTS);
            touch_in_menu_zone = false;
        }
        // Čim korisnik podigne prst, bez obzira gdje, vraćamo osjetljivost na normalu.
        g_high_precision_mode = false;
    }
    if (pTS->Pressed == 1U) { // Ako je pritisak bio registrovan
        DISPResetScrnsvr();
    }
}

/**
 * @brief Prikazuje log poruku na ekranu (koristi se za debug).
 * @note Funkcija ispisuje poruku i pomjera prethodne poruke prema gore,
 * stvarajući efekat skrolujućeg loga.
 * @param pbuf Pointer na string koji treba prikazati.
 */
void DISP_UpdateLog(const char *pbuf)
{
    static char displog[6][128]; // Statički 2D niz za čuvanje historije loga (6 linija)
    int i = 5;

    // Očisti područje na ekranu gdje se ispisuje log
    GUI_ClearRect(120, 80, 480, 240);
    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_TOP);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_SetFont(&GUI_Font16B_1);
    GUI_SetColor(GUI_WHITE);

    // Pomjeri sve stare poruke za jedno mjesto prema gore
    do {
        ZEROFILL(displog[i], COUNTOF(displog[i]));
        strcpy(displog[i], displog[i-1]);
        GUI_DispStringAt(displog[i], 125, 200-(i*20));
    } while(--i);

    // Ispiši novu poruku na dnu loga žutom bojom
    GUI_SetColor(GUI_YELLOW);
    ZEROFILL(displog[0], COUNTOF(displog[0]));
    strcpy(displog[0], pbuf);
    GUI_DispStringAt(displog[0], 125, 200);

    // Ažuriraj ekran
    GUI_Exec();
}
/**
 * @brief Postavlja stanje menija termostata.
 * @note Ova funkcija se koristi za postavljanje internog fleg-a
 * koji prati status ekrana menija termostata.
 * @param state Novo stanje menija (npr. 0 za neaktivan, 1 za aktivan).
 * @retval None
 */
void DISP_SetThermostatMenuState(uint8_t state)
{
    thermostatMenuState = state;
}

/**
 * @brief Dobiva trenutno stanje menija termostata.
 * @note Ova funkcija vraća vrijednost internog fleg-a koji
 * prati status ekrana menija termostata.
 * @param None
 * @retval uint8_t Trenutno stanje menija.
 */
uint8_t DISP_GetThermostatMenuState(void)
{
    return thermostatMenuState;
}

/**
 * @brief Signalizira da je potrebno ažurirati dinamičku ikonu.
 * @note Ova funkcija postavlja interni fleg-a `dynamicIconUpdateFlag`
 * na 'true'. Glavna servisna funkcija će pročitati ovaj fleg i
 * ponovo iscrtati dinamičku ikonu (za defroster/ventilator).
 * @param None
 * @retval None
 */
void DISP_SignalDynamicIconUpdate(void)
{
    dynamicIconUpdateFlag = true;
}

/**
 * @brief Vraća pointer na odgovarajući string iz tabele prevoda.
 * @param t ID teksta koji treba učitati (iz TextID enum-a).
 * @return const char* Pointer na string u tabeli za trenutno odabrani jezik.
 */
const char* lng(uint8_t t)
{
    // Provjera da li je ID u validnom opsegu
    if (t > 0 && t < TEXT_COUNT) {
        // Vrati direktan pointer na string iz tabele
        return language_strings[t][g_display_settings.language];
    }
    // U slučaju nevalidnog ID-ja, vrati pointer na prazan string
    return language_strings[0][0]; // Index 0 (TXT_DUMMY) je definisan kao prazan string
}
/**
 * @brief Provjerava da li je dužina podataka za QR kod validna.
 * @param dataLength Dužina podataka.
 * @return true ako je dužina manja od definisanog maksimuma, inače false.
 */
bool QR_Code_isDataLengthShortEnough(uint8_t dataLength)
{
    return dataLength < QR_CODE_LENGTH;
}

/**
 * @brief Provjerava da li će string stati u bafer za QR kod.
 * @param data Pointer na string sa podacima.
 * @return true ako podaci staju, inače false.
 */
bool QR_Code_willDataFit(const uint8_t *data)
{
    return QR_Code_isDataLengthShortEnough(strlen((char*)data));
}

/**
 * @brief Vraća pointer na podatke za specifični QR kod.
 * @param qrCodeID ID QR koda (1 za WiFi, 2 za App).
 * @return Pointer na string sa podacima.
 */
uint8_t* QR_Code_Get(const uint8_t qrCodeID)
{
    // Provjera da li je ID validan
    if (qrCodeID > 0 && qrCodeID <= QR_CODE_COUNT) {
        return qr_codes[qrCodeID - 1]; // Vrati traženi QR kod
    }
    return qr_codes[0]; // Vrati prvi kao fallback u slučaju greške
}
/**
 * @brief Postavlja podatke za specifični QR kod.
 * @note  Funkcija provjerava da li podaci staju u predviđeni bafer prije kopiranja.
 * @param qrCodeID ID QR koda (1 za WiFi, 2 za App).
 * @param data Pointer na string sa podacima.
 */
void QR_Code_Set(const uint8_t qrCodeID, const uint8_t *data)
{
    // Provjeri da li je ID validan i da li će podaci stati u bafer
    if(QR_Code_willDataFit(data) && qrCodeID > 0 && qrCodeID <= QR_CODE_COUNT)
    {
        // Sigurno kopiraj string u odgovarajući red 2D niza
        sprintf((char*)(qr_codes[qrCodeID - 1]), "%s", (char*)data);
    }
}
/*============================================================================*/
/* IMPLEMENTACIJA STATIČKIH (PRIVATNIH) FUNKCIJA                              */
/*============================================================================*/
static void DISP_Animation(void)
{
    DISPSetBrightnes(g_display_settings.low_bcklght);
     // Deklaracija niza pokazivača na sve frejmove animacije
    GUI_CONST_STORAGE GUI_BITMAP* animation_frames[] = {
        &bmanimation_welcome_frame_05,
        &bmanimation_welcome_frame_10,
        &bmanimation_welcome_frame_15,
        &bmanimation_welcome_frame_20,
        &bmanimation_welcome_frame_25,
        &bmanimation_welcome_frame_30,
        &bmanimation_welcome_frame_35,
        &bmanimation_welcome_frame_40,
        &bmanimation_welcome_frame_45,
        &bmanimation_welcome_frame_50,
        &bmanimation_welcome_frame_55,
        &bmanimation_welcome_frame_60,
        &bmanimation_welcome_frame_65,
        &bmanimation_welcome_frame_70,
        &bmanimation_welcome_frame_75,
        &bmanimation_welcome_frame_80,
        &bmanimation_welcome_frame_85,
        &bmanimation_welcome_frame_90,
        &bmanimation_welcome_frame_95,
        &bmanimation_welcome_frame_100
    };
    
    // Definirajte vrijeme zadržavanja svakog frejma u milisekundama
    const uint32_t FRAME_DELAY_MS = 10; // 20ms za svaki frejm = 50 FPS
    
  
    // Prikaz svakog frejma u petlji
    for (int i = 0; i < (sizeof(animation_frames) / sizeof(animation_frames[0])); i++)
    {
        GUI_MULTIBUF_Begin();
        GUI_Clear();
        // Crtanje bitmapa na centar ekrana
        GUI_DrawBitmap(animation_frames[i], (LCD_GetXSize() - animation_frames[i]->XSize) / 2, (LCD_GetYSize() - animation_frames[i]->YSize) / 2);
        GUI_MULTIBUF_End();
        
        // Obavezno pozvati GUI_Exec() nakon svake operacije
        GUI_Exec();
        
        // Čekanje prije prelaska na sljedeći frejm
        HAL_Delay(FRAME_DELAY_MS);
    }
    
    HAL_Delay(1000);
    
     // =======================================================================
    // === POČETAK NOVE LOGIKE ZA ISPIS TEKSTA NA DNU EKRANA ===
    // =======================================================================

    // Postavljanje fonta i boje za tekst
    GUI_SetFont(&GUI_Font20_ASCII);
    GUI_SetColor(GUI_WHITE);

    // Tekst koji želimo animirati
    const char* text = "www.imedia.ba";

    // Izračunavanje pozicije i dimenzija teksta
    int x_center = LCD_GetXSize() / 2;
    int y_bottom = LCD_GetYSize() - GUI_GetFontDistY() - 30;
    int text_width = GUI_GetStringDistX(text);
    int x_start = x_center - (text_width / 2);

    // Postavljanje globalnog poravnanja teksta za crtanje
    GUI_SetTextAlign(GUI_TA_LEFT);

    // Petlja za animaciju
    for(int current_width = 0; current_width <= text_width; current_width += 5) {
        GUI_MULTIBUF_Begin();
        GUI_ClearRect(x_start, y_bottom, x_start + text_width, y_bottom + GUI_GetFontDistY());

        // Postavljanje pravokutnika za "kliping" (clipped rect)
        GUI_RECT clip_rect = {x_start, y_bottom, x_start + current_width, y_bottom + GUI_GetFontDistY()};
        GUI_SetClipRect(&clip_rect);

        // Iscrtavanje cijelog teksta unutar definiranog "kliping" područja
        GUI_DispStringAt(text, x_start, y_bottom);

        // Obavezno resetirajte "kliping" područje nakon svake operacije
        GUI_SetClipRect(NULL);

        GUI_MULTIBUF_End();
        GUI_Exec();
        HAL_Delay(5); // Podesite kašnjenje za brzinu animacije
    }
    
    HAL_Delay(1000);
    // =======================================================================
    // === KRAJ NOVE LOGIKE ZA ISPIS TEKSTA ===
    // =======================================================================
    
    #define ANIMATION_REPEATS 20 // Podesite broj ponavljanja animacije

    GUI_CONST_STORAGE GUI_BITMAP* animation_flame[] = {
        &bmanimation_candle_frame_1,
        &bmanimation_candle_frame_2,
        &bmanimation_candle_frame_3,
        &bmanimation_candle_frame_4
    };
        
    // Definirajte vrijeme zadržavanja svakog frejma u milisekundama
    const uint32_t FLAME_DELAY_MS = 100; // 100ms za svaki frejm
        
    // Izračunavanje dimenzija za brisanje (koristimo dimenzije prvog frejma)
    const int xPos = 118;
    const int yPos = 80;
    const int clearWidth = animation_flame[0]->XSize;
    const int clearHeight = animation_flame[0]->YSize;

    // Vanjska petlja za ponavljanje animacije
    for (int repeat = 0; repeat < ANIMATION_REPEATS; repeat++)
    {
        // Unutrašnja petlja za prikaz svakog frejma
        for (int i = 0; i < (sizeof(animation_flame) / sizeof(animation_flame[0])); i++)
        {
            GUI_MULTIBUF_Begin();
            
            // Dinamičko čišćenje područja bitmape na njenoj poziciji
            GUI_ClearRect(xPos, yPos, xPos + clearWidth, yPos + clearHeight);

            // Crtanje bitmapa
            GUI_DrawBitmap(animation_flame[i], xPos, yPos);
            GUI_MULTIBUF_End();
            
            // Obavezno pozvati GUI_Exec() nakon svake operacije
            GUI_Exec();
            
            // Čekanje prije prelaska na sljedeći frejm
            HAL_Delay(FLAME_DELAY_MS);
            DISPSetBrightnes(g_display_settings.high_bcklght);
        }
    }
    GUI_Clear();
    HAL_Delay(1000);
    DISPSetBrightnes(g_display_settings.low_bcklght);
}
/**
 * @brief Postavlja svjetlinu pozadinskog osvjetljenja.
 * @param val Vrijednost svjetline (od 1 do 90).
 */
static void DISPSetBrightnes(uint8_t val)
{
    if (val < DISP_BRGHT_MIN) val = DISP_BRGHT_MIN;
    else if (val > DISP_BRGHT_MAX) val = DISP_BRGHT_MAX;
    __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, (uint16_t)(val * 10U));
}
/**
 ******************************************************************************
 * @brief Provjerava da li je u toku ažuriranje firmvera bilo gdje na RS485 busu.
 * @author Gemini & [Vaše Ime]
 * @note  Funkcija provjerava globalni tajmer koji se resetuje svakim primljenim
 * FIRMWARE_UPDATE paketom. Ako je od zadnjeg paketa prošlo manje od
 * FW_UPDATE_BUS_TIMEOUT, smatra se da je ažuriranje i dalje aktivno.
 * @retval bool true ako je ažuriranje aktivno, inače false.
 ******************************************************************************
 */
static bool IsBusFwUpdateActive(void)
{
    // Provjera da li je tajmer inicijalizovan (da ne bi bilo lažno pozitivnih na startu)
    if (g_last_fw_packet_timestamp == 0) {
        return false;
    }
    // Provjera da li je prošlo manje vremena od definisanog timeout-a
    return ((HAL_GetTick() - g_last_fw_packet_timestamp) < FW_UPDATE_BUS_TIMEOUT);
}
/**
 * @brief  Postavlja sve postavke displeja na sigurne fabričke vrijednosti.
 * @note   Ova funkcija se poziva kada podaci u EEPROM-u nisu validni.
 * @param  None
 * @retval None
 */
static void Display_SetDefault(void)
{
    // Sigurnosno nuliranje cijele strukture
    memset(&g_display_settings, 0, sizeof(Display_EepromSettings_t));

    // Postavljanje logičkih početnih vrijednosti
    g_display_settings.low_bcklght = 5;
    g_display_settings.high_bcklght = 80;
    g_display_settings.scrnsvr_tout = 30; // 30 sekundi
    g_display_settings.scrnsvr_ena_hour = 22; // 22:00
    g_display_settings.scrnsvr_dis_hour = 7;  // 07:00
    g_display_settings.scrnsvr_clk_clr = 0;   // GUI_GRAY
    g_display_settings.scrnsvr_on_off = true;
    g_display_settings.leave_scrnsvr_on_release = false;
    g_display_settings.language = BSHC; // Početni jezik je bosanski
    g_display_settings.scenes_enabled = true;
}

/**
 * @brief  Čuva kompletnu konfiguraciju displeja u EEPROM, uključujući CRC.
 * @param  None
 * @retval None
 */
static void Display_Save(void)
{
    // Postavljanje magičnog broja kao "potpisa"
    g_display_settings.magic_number = EEPROM_MAGIC_NUMBER;
    // Privremeno nuliranje CRC-a radi ispravnog izračuna
    g_display_settings.crc = 0;
    // Izračunavanje CRC-a nad cijelom strukturom
    g_display_settings.crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&g_display_settings, sizeof(Display_EepromSettings_t));
    // Snimanje cijelog bloka podataka u EEPROM
    EE_WriteBuffer((uint8_t*)&g_display_settings, EE_DISPLAY_SETTINGS, sizeof(Display_EepromSettings_t));
}

/**
 * @brief  Inicijalizuje postavke displeja iz EEPROM-a, uz provjeru validnosti.
 * @note   Ova funkcija se poziva jednom pri startu sistema.
 * @param  None
 * @retval None
 */
static void Display_InitSettings(void)
{
    // Učitavanje cijelog konfiguracionog bloka iz EEPROM-a
    EE_ReadBuffer((uint8_t*)&g_display_settings, EE_DISPLAY_SETTINGS, sizeof(Display_EepromSettings_t));

    // Provjera "magičnog broja"
    if (g_display_settings.magic_number != EEPROM_MAGIC_NUMBER) {
        // Podaci su nevažeći, učitaj fabričke postavke
        Display_SetDefault();
        // Odmah snimi ispravne fabričke postavke
        Display_Save();
    } else {
        // Magični broj je ispravan, provjeri integritet podataka pomoću CRC-a
        uint16_t received_crc = g_display_settings.crc;
        g_display_settings.crc = 0;
        uint16_t calculated_crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&g_display_settings, sizeof(Display_EepromSettings_t));

        if (received_crc != calculated_crc) {
            // Podaci su oštećeni, učitaj sigurne fabričke postavke
            Display_SetDefault();
            Display_Save();
        }
    }

    // Nakon validacije, primijeni učitane/defaultne postavke na globalne varijable
    // koje se koriste u ostatku display.c fajla, ako je to neophodno.
    // Npr. ako `language` varijabla mora biti globalna:
    // language = g_display_settings.language;
    // Ovo omogućava da ostatak koda ne morate mijenjati odmah.
}

/**
* @brief Očisti sve duhove sa ekrana
 * @note Ova funkcija provjerava postojanje widgeta iz liste svih definisanih u settings_widgets.def
 * kada je kontrola na glavnom ekranu i displej u modu screen savera i uklanja svaki "zivi" widget
 */
static void ForceKillAllSettingsWidgets(void)
{
    WM_HWIN hWidget;
    uint16_t id_to_check;

    // --- 1. Uništavanje statičkih widgeta sa nove, pregledne liste ---
    for (uint16_t i = 0; i < (sizeof(settings_static_widget_ids) / sizeof(settings_static_widget_ids[0])); i++) {
        id_to_check = settings_static_widget_ids[i];
        if ((hWidget = WM_GetDialogItem(WM_GetDesktopWindow(), id_to_check))) {
            WM_DeleteWindow(hWidget);
        }
    }

    // --- 2. Uništavanje dinamičkih widgeta (petlje ostaju) ---
    // Zavjese
    for (uint16_t i = 0; i < (CURTAINS_SIZE * 2); i++) {
        id_to_check = ID_CurtainsRelay + i;
        if ((hWidget = WM_GetDialogItem(WM_GetDesktopWindow(), id_to_check))) {
            WM_DeleteWindow(hWidget);
        }
    }

    // Svjetla
    for (uint16_t i = 0; i < (LIGHTS_MODBUS_SIZE * 13); i++) {
        id_to_check = ID_LightsModbusRelay + i;
        if ((hWidget = WM_GetDialogItem(WM_GetDesktopWindow(), id_to_check))) {
            WM_DeleteWindow(hWidget);
        }
    }

    // --- 3. DODATO: Brisanje widgeta za tastature i kapije ---
    
    // Alfanumerička tastatura
    for (int i = 0; i < (KEY_ROWS * KEYS_PER_ROW); i++) {
        if (WM_IsWindow(hKeyboardButtons[i])) {
            WM_DeleteWindow(hKeyboardButtons[i]);
            hKeyboardButtons[i] = 0;
        }
    }
    for (int i = 0; i < 5; i++) {
        if (WM_IsWindow(hKeyboardSpecialButtons[i])) {
            WM_DeleteWindow(hKeyboardSpecialButtons[i]);
            hKeyboardSpecialButtons[i] = 0;
        }
    }

    // Dugme za preimenovanje
    if (WM_IsWindow(hButtonRenameLight)) {
        WM_DeleteWindow(hButtonRenameLight);
        hButtonRenameLight = 0;
    }
    
    // NumPad tastatura
    for (int i = 0; i < 12; i++) {
        if (WM_IsWindow(hKeypadButtons[i])) {
            WM_DeleteWindow(hKeypadButtons[i]);
            hKeypadButtons[i] = 0;
        }
    }

    // Podešavanja kapija
    if (WM_IsWindow(hGateSelect)) { WM_DeleteWindow(hGateSelect); }
    if (WM_IsWindow(hGateType)) { WM_DeleteWindow(hGateType); }
    for (int i = 0; i < 9; i++) {
        if (WM_IsWindow(hGateEditButtons[i])) {
            WM_DeleteWindow(hGateEditButtons[i]);
            hGateEditButtons[i] = 0;
        }
    }
}
/**
 * @brief       Iscrtava "hamburger" meni ikonu na predefinisanoj poziciji.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ova funkcija je refaktorisana da eliminiše dupliciranje koda i
 * "magične brojeve". Koristi centralizovane konstante iz
 * `hamburger_menu_layout` strukture. Poziva se sa različitih
 * ekrana za iscrtavanje standardnih navigacionih elemenata. [cite: 52, 53, 54]
 * @param       position Određuje lokaciju iscrtavanja: [cite: 58]
 * - 1: Gornji desni ugao (standardni meni za povratak)
 * - 2: Donji lijevi ugao (meni za ulazak u scene)
 * @retval      None [cite: 61]
 *****************************************************************************/
static void DrawHamburgerMenu(uint8_t position)
{
    int16_t xStart, yStart, width, yGap;

    if (position == 1) // Gornji desni meni
    {
        xStart = hamburger_menu_layout.top_right.x_start;
        yStart = hamburger_menu_layout.top_right.y_start;
        width  = hamburger_menu_layout.top_right.width;
        yGap   = hamburger_menu_layout.top_right.y_gap;
    }
    else if (position == 2) // Donji lijevi meni
    {
        xStart = hamburger_menu_layout.bottom_left.x_start;
        yStart = hamburger_menu_layout.bottom_left.y_start;
        width  = hamburger_menu_layout.bottom_left.width;
        yGap   = hamburger_menu_layout.bottom_left.y_gap;
    }
    else
    {
        return; // Nepoznata pozicija, ne radi ništa.
    }

    // Postavljanje parametara za iscrtavanje ostaje nepromijenjeno
    GUI_SetPenSize(hamburger_menu_layout.line_thickness);
    GUI_SetColor(clk_clrs[g_display_settings.scrnsvr_clk_clr]);

    // Iscrtaj tri linije na osnovu odabranih parametara
    GUI_DrawLine(xStart, yStart, xStart + width, yStart);
    GUI_DrawLine(xStart, yStart + yGap, xStart + width, yStart + yGap);
    GUI_DrawLine(xStart, yStart + (yGap * 2), xStart + width, yStart + (yGap * 2));
}
/**
 * @brief Provjerava status ažuriranja firmvera i prikazuje odgovarajuću poruku.
 * @note  Ova funkcija blokira ostatak GUI logike dok je ažuriranje u toku.
 * @return uint8_t 1 ako je ažuriranje aktivno, inače 0.
 */
static uint8_t Service_HandleFirmwareUpdate(void)
{
    static uint8_t fwmsg = 2; // Statički fleg za praćenje stanja iscrtavanja poruke

    // IZMJENA: Sada provjeravamo da li je ažuriranje aktivno BILO GDJE NA BUSU.
    if (IsBusFwUpdateActive()) {
        // Ako je ažuriranje aktivno, a poruka još nije iscrtana
        if (!fwmsg) {
            fwmsg = 1; // Postavi fleg da je poruka iscrtana
            GUI_MULTIBUF_BeginEx(1);
            GUI_Clear();
            GUI_SetFont(GUI_FONT_24B_1);
            GUI_SetColor(GUI_ORANGE);
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
            // Koristi lng() za prikaz poruke na odabranom jeziku
            GUI_DispStringAt(lng(TXT_UPDATE_IN_PROGRESS), 240, 135);
            GUI_MULTIBUF_EndEx(1);
            DISPResetScrnsvr();
        }
        return 1; // Vrati 1 da signalizira da je ažuriranje u toku
    }
    // Ako je ažuriranje upravo završeno (fleg je bio 1, a sada je IsBusFwUpdateActive() false)
    else if (fwmsg == 1) {
        fwmsg = 0; // Resetuj fleg
        scrnsvr_tmr = 0; // Resetuj tajmer za screensaver
        // Forsiraj ponovno iscrtavanje trenutnog ekrana da se ukloni poruka o ažuriranju
        shouldDrawScreen = 1;
    }
    // Jednokratno iscrtavanje inicijalne grafike na samom početku
    else if (fwmsg == 2) {
        fwmsg = 0;
        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        // Iscrtaj hamburger meni ikonicu
        DrawHamburgerMenu(1);
        GUI_MULTIBUF_EndEx(1);
    }
    return 0; // Ažuriranje nije aktivno
}
/**
 ******************************************************************************
 * @brief       Servisira glavni ekran.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva u petlji kada je `screen == SCREEN_MAIN`.
 * Odgovorna je za resetovanje internih flegova menija i za iscrtavanje
 * osnovnih elemenata glavnog ekrana (hamburger meni i crveni/zeleni krug
 * koji indicira stanje svjetala).
 * Optimizovana je tako da se ponovno iscrtavanje dešava samo kada je
 * to neophodno (kada se promijeni stanje svjetala ili kada se forsira).
 * **Ažurirana verzija dodaje donji meni za scene.**
 ******************************************************************************
 */
static void Service_MainScreen(void)
{
    // Statička varijabla čuva prethodno stanje svjetala. Na ovaj način,
    // ponovno iscrtavanje se dešava samo kada se stanje promijeni.
    static bool old_light_state = false;

    // Dohvatanje trenutnog stanja svjetala pozivom nove API funkcije.
    // Ovdje se vidi ljepota enkapsulacije: ne znamo KAKO lights modul
    // ovo provjerava, samo dobijamo odgovor.
    bool current_light_state = LIGHTS_isAnyLightOn();

    // Resetuje interne flagove za menije
    thermostatMenuState = 0; // Resetuje fleg za meni termostata
    menu_lc = 0; // Resetuje fleg za meni za odabir kontrole
    old_min = 60; // Forsira update vremena pri sljedećem pozivu DISPDateTime()
    rtctmr = 0; // Resetuje tajmer za RTC

    // Iscrtavanje ekrana se dešava samo ako je eksplicitno zatraženo (`shouldDrawScreen`)
    // ILI ako se stanje svjetala promijenilo od posljednje provjere.
    if (shouldDrawScreen || (current_light_state != old_light_state)) {
        shouldDrawScreen = 0; // Resetuj fleg za iscrtavanje
        old_light_state = current_light_state; // Ažuriraj staro stanje svjetala

        GUI_MULTIBUF_BeginEx(1); // Pokreće dvostruko baferovanje
        GUI_Clear(); // Čisti cijeli ekran
        DrawHamburgerMenu(1); // Crtanje hamburger menija za gornji desni meni
        // Donji lijevi meni za ulazak u scene se iscrtava samo ako su scene omogućene
        if (g_display_settings.scenes_enabled)
        {
            DrawHamburgerMenu(2); 
        }

        // Provjeri stanje svjetala povezanih sa glavnim prekidačem da bi se odredila boja kruga
        if (current_light_state) { // Ako je bilo koje svjetlo upaljeno
            GUI_SetColor(GUI_GREEN); // Postavi boju na zelenu
        } else { // Ako su sva svjetla ugašena
            GUI_SetColor(GUI_RED); // Postavi boju na crvenu
        }
        // Crtanje kruga
        GUI_DrawEllipse(main_screen_layout.circle_center_x,
                        main_screen_layout.circle_center_y,
                        main_screen_layout.circle_radius_x,
                        main_screen_layout.circle_radius_y);

        GUI_MULTIBUF_EndEx(1); // Završava dvostruko baferovanje
    }
}
/**
 ******************************************************************************
 * @brief       Servisira ekran za odabir glavnih kontrola (Svjetla, Termostat, Roletne, Dinamička ikona).
 * @author      Gemini & [Vaše Ime]
 * @note        Verzija 2.3.2: Potpuno refaktorisano da koristi "Pametnu Mrežu" (Smart Grid).
 * Ekran dinamički iscrtava samo konfigurisane module i prilagođava raspored
 * na osnovu njihovog broja (1, 2, 3 ili 4), koristeći nove fontove sa
 * punom podrškom za specijalne karaktere.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void Service_SelectScreen1(void)
{
    /**
     * @brief Struktura za čuvanje podataka o jednom dinamičkom meniju.
     */
    typedef struct {
        const GUI_BITMAP* icon;
        TextID text_id;
        eScreen target_screen;
        bool is_active; // Dodatni fleg za dinamičku ikonu
    } DynamicMenuItem;

    // Inicijalizacija handle-ova za module
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();
    Defroster_Handle* defHandle = Defroster_GetInstance();
    Ventilator_Handle* ventHandle = Ventilator_GetInstance();

    // Lista SVIH mogućih modula za ovaj ekran
    DynamicMenuItem all_modules[] = {
        { &bmSijalicaOff, TXT_LIGHTS, SCREEN_LIGHTS, false },
        { &bmTermometar, TXT_THERMOSTAT, SCREEN_THERMOSTAT, false },
        { &bmblindMedium, TXT_BLINDS, SCREEN_CURTAINS, false },
        { NULL, TXT_DUMMY, SCREEN_SELECT_1, false } // Mjesto za dinamičku ikonu
    };

    // --- KORAK 1: Detekcija aktivnih modula ---
    DynamicMenuItem active_modules[4];
    uint8_t active_modules_count = 0;

    if (LIGHTS_getCount() > 0) {
        active_modules[active_modules_count++] = all_modules[0];
    }
    if (Thermostat_GetGroup(pThst) > 0) {
        active_modules[active_modules_count++] = all_modules[1];
    }
    if (Curtains_getCount() > 0) {
        active_modules[active_modules_count++] = all_modules[2];
    }

    // Detekcija za dinamičku ikonu
    if (g_display_settings.selected_control_mode == MODE_DEFROSTER && Defroster_getPin(defHandle) > 0) {
        bool is_active = Defroster_isActive(defHandle);
        all_modules[3].icon = is_active ? &bmdefrostericoOn : &bmdefrosterico;
        all_modules[3].text_id = TXT_DEFROSTER;
        all_modules[3].is_active = is_active;
        active_modules[active_modules_count++] = all_modules[3];
    } else if (g_display_settings.selected_control_mode == MODE_VENTILATOR && (Ventilator_getRelay(ventHandle) > 0 || Ventilator_getLocalPin(ventHandle) > 0)) {
        bool is_active = Ventilator_isActive(ventHandle);
        all_modules[3].icon = is_active ? &bmVENTILATOR_ON : &bmVENTILATOR_OFF;
        all_modules[3].text_id = TXT_VENTILATOR;
        all_modules[3].is_active = is_active;
        active_modules[active_modules_count++] = all_modules[3];
    }

    // Crtamo samo ako je potrebno
    if (shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        DrawHamburgerMenu(1);

        // --- NOVA LOGIKA ZA ISCRTAVANJE SEPARATORA ---
        if (active_modules_count < 4) {
            // Crtaj dugački desni separator za 1, 2, i 3 ikonice
            GUI_DrawLine(DRAWING_AREA_WIDTH, select_screen1_drawing_layout.long_separator_y_start,
                         DRAWING_AREA_WIDTH, select_screen1_drawing_layout.long_separator_y_end);
        }

        // --- KORAK 2: "Pametna Mreža" - Iscrtavanje na osnovu broja aktivnih modula ---
        switch (active_modules_count) {
        case 1: {
            // Jedna velika ikona na sredini
            const DynamicMenuItem* item = &active_modules[0];
            int x_pos = (DRAWING_AREA_WIDTH / 2) - (item->icon->XSize / 2);
            int y_pos = (LCD_GetYSize() / 2) - (item->icon->YSize / 2) - 10;
            GUI_DrawBitmap(item->icon, x_pos, y_pos);

            // =======================================================================
            // === IZMJENA FONT-a ===
            // =======================================================================
            GUI_SetFont(&GUI_FontVerdana32_LAT);
            GUI_SetColor(GUI_ORANGE);
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER);
            GUI_DispStringAt(lng(item->text_id), DRAWING_AREA_WIDTH / 2, y_pos + item->icon->YSize + 10);
            break;
        }
        case 2: {
            // Dvije ikonice sa separatorom
            // Jedan kratki separator u sredini
            GUI_DrawLine(DRAWING_AREA_WIDTH / 2, select_screen1_drawing_layout.short_separator_y_start,
                         DRAWING_AREA_WIDTH / 2, select_screen1_drawing_layout.short_separator_y_end);

            for (int i = 0; i < 2; i++) {
                const DynamicMenuItem* item = &active_modules[i];
                int x_center = (DRAWING_AREA_WIDTH / 4) * (i == 0 ? 1 : 3);
                int x_pos = x_center - (item->icon->XSize / 2);
                int y_pos = (LCD_GetYSize() / 2) - (item->icon->YSize / 2) - 10;
                GUI_DrawBitmap(item->icon, x_pos, y_pos);

                // =======================================================================
                // === IZMJENA FONT-a ===
                // =======================================================================
                GUI_SetFont(&GUI_FontVerdana20_LAT);
                GUI_SetColor(GUI_ORANGE);
                GUI_SetTextMode(GUI_TM_TRANS);
                GUI_SetTextAlign(GUI_TA_HCENTER);
                GUI_DispStringAt(lng(item->text_id), x_center, y_pos + item->icon->YSize + 10);
            }
            break;
        }
        case 3: {
            // Tri ikonice sa separatorima
            // Dva kratka separatora
            GUI_DrawLine(DRAWING_AREA_WIDTH / 3, select_screen1_drawing_layout.short_separator_y_start,
                         DRAWING_AREA_WIDTH / 3, select_screen1_drawing_layout.short_separator_y_end);
            GUI_DrawLine((DRAWING_AREA_WIDTH / 3) * 2, select_screen1_drawing_layout.short_separator_y_start,
                         (DRAWING_AREA_WIDTH / 3) * 2, select_screen1_drawing_layout.short_separator_y_end);
            for (int i = 0; i < 3; i++) {
                const DynamicMenuItem* item = &active_modules[i];
                int x_center = (DRAWING_AREA_WIDTH / 6) * (1 + 2 * i);
                int x_pos = x_center - (item->icon->XSize / 2);
                int y_pos = (LCD_GetYSize() / 2) - (item->icon->YSize / 2) - 10;
                GUI_DrawBitmap(item->icon, x_pos, y_pos);

                // =======================================================================
                // === IZMJENA FONT-a ===
                // =======================================================================
                GUI_SetFont(&GUI_FontVerdana20_LAT);
                GUI_SetColor(GUI_ORANGE);
                GUI_SetTextMode(GUI_TM_TRANS);
                GUI_SetTextAlign(GUI_TA_HCENTER);
                GUI_DispStringAt(lng(item->text_id), x_center, y_pos + item->icon->YSize + 10);
            }
            break;
        }
        case 4:
        default: {
            // Klasična 2x2 mreža
            // Samo krstasti separator, bez dugačke desne linije
            GUI_DrawLine(DRAWING_AREA_WIDTH / 2, select_screen1_drawing_layout.long_separator_y_start,
                         DRAWING_AREA_WIDTH / 2, select_screen1_drawing_layout.long_separator_y_end);
            GUI_DrawLine(select_screen1_drawing_layout.separator_x_padding, LCD_GetYSize() / 2,
                         DRAWING_AREA_WIDTH - select_screen1_drawing_layout.separator_x_padding, LCD_GetYSize() / 2);
            for (int i = 0; i < 4; i++) {
                const DynamicMenuItem* item = &active_modules[i];
                int x_center = (DRAWING_AREA_WIDTH / 4) * (i % 2 == 0 ? 1 : 3);
                int y_center = (LCD_GetYSize() / 4) * (i < 2 ? 1 : 3);
                int x_pos = x_center - (item->icon->XSize / 2);
                int y_pos = y_center - (item->icon->YSize / 2) - 10;
                GUI_DrawBitmap(item->icon, x_pos, y_pos);

                // =======================================================================
                // === IZMJENA FONT-a ===
                // =======================================================================
                GUI_SetFont(&GUI_FontVerdana20_LAT);
                GUI_SetColor(GUI_ORANGE);
                GUI_SetTextMode(GUI_TM_TRANS);
                GUI_SetTextAlign(GUI_TA_HCENTER);
                GUI_DispStringAt(lng(item->text_id), x_center, y_pos + item->icon->YSize + 10);
            }
            break;
        }
        }

        // Iscrtaj "Next" dugme za prelazak na Screen 2
        // Iscrtavanje "Next" dugmeta (logika ostaje ista)
        if (select_screen2_layout.next_button_zone.x1 > 0) {
            const GUI_BITMAP* iconNext = &bmnext;
            /**
            * @brief Iscrtavanje "NEXT" dugmeta.
            */
            GUI_DrawBitmap(iconNext, select_screen1_drawing_layout.x_separator_pos + 5, select_screen1_drawing_layout.y_next_button_center - (iconNext->YSize / 2));
        }
        GUI_MULTIBUF_EndEx(1);
    }
    // Ažuriranje dinamičke ikone ako se njeno stanje promijenilo
    else if (dynamicIconUpdateFlag) {
        dynamicIconUpdateFlag = 0;
        shouldDrawScreen = 1; // Najlakši način je ponovno iscrtati cijeli ekran
    }
}
/**
 ******************************************************************************
 * @brief       Servisira drugi ekran za odabir (Kapija, Tajmer, Alarm, Multi-funkcija).
 * @author      Gemini & [Vaše Ime]
 * @note        Ovaj ekran služi kao drugi nivo glavnog menija. Iscrtava do
 * četiri ikonice u 2x2 mreži, dinamički prikazujući samo one module
 * koji su konfigurisani.
 ******************************************************************************
 */
static void Service_SelectScreen2(void)
{
    if (shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        DrawHamburgerMenu(1); // Meni za povratak na SELECT_1

        // Crtanje krstastog separatora za 2x2 mrežu
        GUI_DrawLine(DRAWING_AREA_WIDTH / 2, select_screen2_drawing_layout.separator_y_start,
                     DRAWING_AREA_WIDTH / 2, select_screen2_drawing_layout.separator_y_end);
        GUI_DrawLine(select_screen2_drawing_layout.separator_x_padding, LCD_GetYSize() / 2,
                     DRAWING_AREA_WIDTH - select_screen2_drawing_layout.separator_x_padding, LCD_GetYSize() / 2);

        // =======================================================================
        // === FINALNA VERZIJA: Korištenje finalnih ikonica za sve opcije ===
        // =======================================================================
        const GUI_BITMAP* icons[] = {&bmicons_menu_gate, &bmicons_menu_timers, &bmicons_scene_security, &bmicons_security_sos};
        TextID texts[] = {TXT_GATE, TXT_TIMER, TXT_SECURITY, TXT_LANGUAGE_SOS_ALL_OFF};
        
        int x_centers[] = {select_screen2_drawing_layout.x_center_left, select_screen2_drawing_layout.x_center_right, 
                           select_screen2_drawing_layout.x_center_left, select_screen2_drawing_layout.x_center_right};
        int y_centers[] = {select_screen2_drawing_layout.y_center_top, select_screen2_drawing_layout.y_center_top, 
                           select_screen2_drawing_layout.y_center_bottom, select_screen2_drawing_layout.y_center_bottom};

        for (int i = 0; i < 4; i++) {
            int x_pos = x_centers[i] - (icons[i]->XSize / 2);
            int y_pos = y_centers[i] - (icons[i]->YSize / 2) - select_screen2_drawing_layout.text_vertical_offset;
            GUI_DrawBitmap(icons[i], x_pos, y_pos);

            GUI_SetFont(&GUI_FontVerdana20_LAT);
            GUI_SetColor(GUI_ORANGE);
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER);
            GUI_DispStringAt(lng(texts[i]), x_centers[i], y_pos + icons[i]->YSize + select_screen2_drawing_layout.text_vertical_offset);
        }

        // Iscrtavanje "NEXT" dugmeta koje vodi na SELECT_LAST
        const GUI_BITMAP* iconNext = &bmnext;
        int x_pos = select_screen2_drawing_layout.next_button_x_pos;
        int y_pos = select_screen2_drawing_layout.next_button_y_center - (iconNext->YSize / 2);
        GUI_DrawBitmap(iconNext, x_pos, y_pos);

        GUI_MULTIBUF_EndEx(1);
    }
}
/**
 ******************************************************************************
 * @brief       Servisira posljednji ekran za odabir (Čišćenje, WiFi, App, Postavke).
 * @author      Gemini & [Vaše Ime]
 * [cite_start]@note Ova funkcija je preimenovana iz Service_SelectScreen2. 
 * Odgovorna je za iscrtavanje 4 fiksne ikonice na posljednjem ekranu u nizu
 * za odabir.Raspored elemenata je definisan u select_screen2_drawing_layout strukturi.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void Service_SelectScreenLast(void)
{
    if (shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        DrawHamburgerMenu(1);

        // Crtanje krstastog separatora za 2x2 mrežu
        GUI_DrawLine(DRAWING_AREA_WIDTH / 2, select_screen2_drawing_layout.separator_y_start,
                     DRAWING_AREA_WIDTH / 2, select_screen2_drawing_layout.separator_y_end);
        GUI_DrawLine(select_screen2_drawing_layout.separator_x_padding, LCD_GetYSize() / 2,
                     DRAWING_AREA_WIDTH - select_screen2_drawing_layout.separator_x_padding, LCD_GetYSize() / 2);

        // Definicija ikonica i tekstova za 4 kvadranta
        const GUI_BITMAP* icons[] = {&bmCLEAN, &bmwifi, &bmmobilePhone, &bmicons_settings};
        TextID texts[] = {TXT_CLEAN, TXT_WIFI, TXT_APP, TXT_SETTINGS};
        int x_centers[] = {select_screen2_drawing_layout.x_center_left, select_screen2_drawing_layout.x_center_right, select_screen2_drawing_layout.x_center_left, select_screen2_drawing_layout.x_center_right};
        int y_centers[] = {select_screen2_drawing_layout.y_center_top, select_screen2_drawing_layout.y_center_top, select_screen2_drawing_layout.y_center_bottom, select_screen2_drawing_layout.y_center_bottom};

        for (int i = 0; i < 4; i++) {
            int x_pos = x_centers[i] - (icons[i]->XSize / 2);
            int y_pos = y_centers[i] - (icons[i]->YSize / 2) - select_screen2_drawing_layout.text_vertical_offset;
            GUI_DrawBitmap(icons[i], x_pos, y_pos);

            // =======================================================================
            // === IZMJENA FONT-a ===
            // =======================================================================
            GUI_SetFont(&GUI_FontVerdana20_LAT);
            GUI_SetColor(GUI_ORANGE);
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER);
            GUI_DispStringAt(lng(texts[i]), x_centers[i], y_pos + icons[i]->YSize + select_screen2_drawing_layout.text_vertical_offset);
        }

        // Iscrtavanje "NEXT" dugmeta koje sada vraća na početni ekran
        const GUI_BITMAP* iconNext = &bmnext;
        int x_pos = select_screen2_drawing_layout.next_button_x_pos;
        int y_pos = select_screen2_drawing_layout.next_button_y_center - (iconNext->YSize / 2);
        GUI_DrawBitmap(iconNext, x_pos, y_pos);

        GUI_MULTIBUF_EndEx(1);
    }
}
/**
 ******************************************************************************
 * @brief       Servisira ekran sa termostatom ISKLJUČIVO unutar "Scene Wizard" moda.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija je kopija originalne `Service_ThermostatScreen`,
 * modifikovana za rad unutar čarobnjaka. Uklonjeni su hamburger meni,
 * vrijeme i status ON/OFF, a dodato je "[Next]" dugme.
 ******************************************************************************
 */
static void Service_SceneEditThermostatScreen(void)
{
   
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();

    if (thermostatMenuState == 0) {
        thermostatMenuState = 1;
        GUI_MULTIBUF_BeginEx(0);
        GUI_SelectLayer(0);
        GUI_SetColor(GUI_BLACK);
        GUI_Clear();
        GUI_BMP_Draw(&thstat, 0, 0);
        // Ne crtamo hamburger meni
        GUI_MULTIBUF_EndEx(0);
        GUI_SelectLayer(1);
        GUI_SetBkColor(GUI_TRANSPARENT);
        GUI_Clear();
        
        // Kreiraj "Next" dugme sa ikonicom
        hButtonWizNext = BUTTON_CreateEx(390, 182, 80, 80, 0, WM_CF_SHOW, 0, ID_WIZ_NEXT);
        BUTTON_SetBitmap(hButtonWizNext, BUTTON_CI_UNPRESSED, &bmnext);
        BUTTON_SetBitmap(hButtonWizNext, BUTTON_CI_PRESSED, &bmnext);

        DISPSetPoint();
        MVUpdateSet();
        menu_lc = 0;
    } else if (thermostatMenuState == 1) {
        // Logika za +/- dugmad ostaje ista
        if (btninc && !_btninc) { _btninc = 1; Thermostat_SP_Temp_Increment(pThst); THSTAT_Save(pThst); DISPSetPoint(); }
        else if (!btninc && _btninc) { _btninc = 0; }
        if (btndec && !_btndec) { _btndec = 1; Thermostat_SP_Temp_Decrement(pThst); THSTAT_Save(pThst); DISPSetPoint(); }
        else if (!btndec && _btndec) { _btndec = 0; }

//            if (IsMVUpdateActiv()) {
//                MVUpdateReset();
//                GUI_MULTIBUF_BeginEx(1);
//                // Obriši samo staru poziciju za MV Temp
//                GUI_ClearRect(410, 185, 480, 235);
//                
//                // Iscrtaj Izmjerenu Temperaturu na novoj poziciji
//                GUI_SetFont(GUI_FONT_24_1);
//                GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
//                GUI_SetColor(GUI_WHITE);
//                GUI_DispSDecAt(Thermostat_GetMeasuredTemp(pThst) / 10, 5, 245, 3);
//                GUI_DispString("°c");
//                
//                GUI_MULTIBUF_EndEx(1);
//            }
    }

    // Navigacija za "[Next]" dugme
    if (BUTTON_IsPressed(hButtonWizNext))
    {
        GUI_SelectLayer(0);
        GUI_SetColor(GUI_BLACK);
        GUI_Clear();
        GUI_SelectLayer(1);
        GUI_SetBkColor(GUI_TRANSPARENT);
        GUI_Clear();
        DSP_KillSceneEditThermostatScreen();
        is_in_scene_wizard_mode = false; // Završi wizard mod
        // TODO: Prebaci na finalni ekran za snimanje
        DSP_InitSceneEditScreen();
        screen = SCREEN_SCENE_EDIT;
        shouldDrawScreen = 0;
        return;
    }   
}
/**
 * @brief Servisira ekran termostata, uključujući iscrtavanje i obradu unosa.
 * @note Ova funkcija se poziva u petlji dok je `screen == SCREEN_THERMOSTAT`.
 * U njoj se upravlja prikazom zadane temperature, trenutne temperature, te se obrađuju
 * dodiri za podešavanje temperature i paljenje/gašenje termostata.
 */
static void Service_ThermostatScreen(void)
{
     if (is_in_scene_wizard_mode)
    {
        Service_SceneEditThermostatScreen();
    }
    else
    {
        // Dobijamo handle za termostat.
        THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();

        // Ažuriranje prikaza termostata se radi unutar jedne multibuffer transakcije.
        GUI_MULTIBUF_BeginEx(1);

        if (thermostatMenuState == 0) {
            // Ako je ovo prvi put da ulazimo na ekran, iscrtavamo kompletnu pozadinu.
            thermostatMenuState = 1;

            GUI_MULTIBUF_BeginEx(0);
            GUI_SelectLayer(0);
            GUI_SetColor(GUI_BLACK);
            GUI_Clear();
            // Iscrtavanje pozadinske bitmap slike termostata.
            GUI_BMP_Draw(&thstat, 0, 0);
            GUI_ClearRect(380, 0, 480, 100);
            // Iscrtavanje hamburger meni ikonice u gornjem desnom uglu.
            DrawHamburgerMenu(1);

            // Čišćenje specifičnih dijelova ekrana za dinamičke vrijednosti.
            GUI_ClearRect(350, 80, 480, 180);
            GUI_ClearRect(310, 180, 420, 205);
            GUI_MULTIBUF_EndEx(0);

            // Prebacivanje na drugi sloj za dinamičke elemente.
            GUI_SelectLayer(1);
            GUI_SetBkColor(GUI_TRANSPARENT);
            GUI_Clear();

            // Prikaz zadane temperature.
            DISPSetPoint();
            // Prikaz trenutnog vremena i datuma.
            DISPDateTime();
            // Postavi flag da treba ažurirati trenutnu temperaturu.
            MVUpdateSet();
            menu_lc = 0;
        } else if (thermostatMenuState == 1) {
            // Ako je ekran već iscrtan, obrađujemo samo promjene stanja.

            // Logika za povećanje/smanjenje zadane temperature.
            if (btninc && !_btninc) {
                _btninc = 1;
                Thermostat_SP_Temp_Increment(pThst);
                THSTAT_Save(pThst);
                DISPSetPoint();
            } else if (!btninc && _btninc) {
                _btninc = 0;
            }

            if (btndec && !_btndec) {
                _btndec = 1;
                Thermostat_SP_Temp_Decrement(pThst);
                THSTAT_Save(pThst);
                DISPSetPoint();
            } else if (!btndec && _btndec) {
                _btndec = 0;
            }

            // Ažuriranje prikaza trenutne temperature i statusa regulatora.
            if (IsMVUpdateActiv()) {
                MVUpdateReset();
                char dbuf_mv[8]; // Lokalni bafer za prikaz MV_Temp
                GUI_ClearRect(410, 185, 480, 235);
                GUI_ClearRect(310, 230, 480, 255);

                // Postavljanje boje na osnovu statusa regulatora.
                if(Thermostat_IsActive(pThst)) {
                    GUI_SetColor(GUI_GREEN);
                } else {
                    GUI_SetColor(GUI_RED);
                }

                GUI_SetFont(GUI_FONT_32B_1);
                GUI_GotoXY(410, 170);
                GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
                if(Thermostat_IsActive(pThst)) {
                    GUI_DispString("ON");
                } else {
                    GUI_DispString("OFF");
                }

                GUI_GotoXY(310, 242);
                GUI_SetFont(GUI_FONT_20_1);
                GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
                GUI_SetColor(GUI_WHITE);
                GUI_GotoXY(415, 220);
                GUI_SetFont(GUI_FONT_24_1);
                GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
                GUI_DispSDec(Thermostat_GetMeasuredTemp(pThst) / 10, 3);
                GUI_DispString("°c");
            }

            // Ažuriranje prikaza vremena.
            if ((HAL_GetTick() - rtctmr) >= DATE_TIME_REFRESH_TIME) {
                rtctmr = HAL_GetTick();
                if(IsRtcTimeValid()) {
                    RTC_TimeTypeDef rtctm_local;
                    RTC_DateTypeDef rtcdt_local;
                    HAL_RTC_GetTime(&hrtc, &rtctm_local, RTC_FORMAT_BCD);
                    HAL_RTC_GetDate(&hrtc, &rtcdt_local, RTC_FORMAT_BCD);
                    char dbuf_time[8];
                    HEX2STR(dbuf_time, &rtctm_local.Hours);
                    dbuf_time[2] = ':';
                    HEX2STR(&dbuf_time[3], &rtctm_local.Minutes);
                    dbuf_time[5]  = '\0';
                    GUI_SetFont(GUI_FONT_32_1);
                    GUI_SetColor(GUI_WHITE);
                    GUI_SetTextMode(GUI_TM_TRANS);
                    GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
                    GUI_GotoXY(5, 245);
                    GUI_MULTIBUF_BeginEx(1);
                    GUI_ClearRect(0, 220, 100, 270);
                    GUI_DispString(dbuf_time);
                    GUI_MULTIBUF_EndEx(1);
                }
            }
        }
        GUI_MULTIBUF_EndEx(1);

        // Detekcija dugog pritiska za paljenje/gašenje termostata.
        if(thermostatOnOffTouch_timer) {
            DISPResetScrnsvr();
            if((HAL_GetTick() - thermostatOnOffTouch_timer) > (2 * 1000)) {
                thermostatOnOffTouch_timer = 0;
                thermostatMenuState = 0;
                if(Thermostat_IsActive(pThst)) {
                    Thermostat_TurnOff(pThst);
                } else {
                    Thermostat_SetControlMode(pThst, THST_HEATING);
                }
                THSTAT_Save(pThst);
            }
        }
    }
}

/**
 * @brief Vraća sistem na početni ekran i resetuje sve relevantne flagove.
 * @note Ova funkcija se poziva kada se treba vratiti na "čisto" početno stanje.
 * Briše oba grafička sloja i resetuje sve interne varijable stanja.
 */
static void Service_ReturnToFirst(void)
{
    // === "FAIL-SAFE" MEHANIZAM ===
    ForceKillAllSettingsWidgets();

    // Očisti oba grafička sloja.
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();

    // NOVO: Svjetlina se ne mijenja automatski pri povratku na početni ekran.
    // DISPSetBrightnes(DISP_BRGHT_MIN); // Ova linija je uklonjena.

    // Postavi aktivni ekran na glavni meni.
    screen = SCREEN_MAIN;

    // Resetovanje svih flagova i brojača.
    thermostatMenuState = 0;
    menu_lc = 0;
    menu_clean = 0;
    lcsta = 0;
    thsta = 0;
    curtainSettingMenu = 0;
    lightsModbusSettingsMenu = 0;
    light_selectedIndex = LIGHTS_MODBUS_SIZE + 1;
    lights_allSelected_hasRGB = false;

    // Postavi flag za ponovno iscrtavanje ekrana.
    shouldDrawScreen = 1;
}

/**
 ******************************************************************************
 * @brief       Servisira placeholder ekran za kapiju.
 * @author      Gemini & [Vaše Ime]
 * @note        Ovo je privremena funkcija koja služi za uspostavljanje
 * navigacije. Iscrtava samo naslov ekrana i meni za povratak,
 * pripremajući teren za buduću implementaciju punih kontrola za kapiju. 
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void Service_GateScreen(void)
{
    if (shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        DrawHamburgerMenu(1); // Meni za povratak

        // Iscrtaj naslov kao placeholder
        GUI_SetFont(&GUI_FontVerdana32_LAT);
        GUI_SetColor(GUI_WHITE);
        GUI_SetTextMode(GUI_TM_TRANS);
        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt("KAPIJA", LCD_GetXSize() / 2, LCD_GetYSize() / 2);

        GUI_MULTIBUF_EndEx(1);
    }
}
/**
 ******************************************************************************
 * @brief       Servisira ekran za prikaz i aktivaciju scena.
 * @author      Gemini & [Vaše Ime]
 * @note        FINALNA VERZIJA 2.1: Logika je ažurirana. Funkcija sada
 * iscrtava samo konfigurisane scene u 3x2 mreži. Ikona za dodavanje
 * nove scene ("čarobnjak") je izdvojena i prikazuje se u donjem
 * desnom uglu, na poziciji konzistentnoj sa "Next" dugmetom, i to
 * samo ako broj konfigurisanih scena nije dostigao maksimum.
 ******************************************************************************
 */
static void Service_SceneScreen(void)
{
    if (shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        DrawHamburgerMenu(1);

        uint8_t configured_scenes_count = Scene_GetCount();
        uint8_t configured_scene_tracker = 0;

        // --- Iscrtavanje postojećih, konfigurisanih scena u mreži ---
        for (int i = 0; i < configured_scenes_count; i++)
        {
            const SceneAppearance_t* appearance = NULL;
            
            // Pronađi i-tu po redu konfigurisanu scenu
            for (int k = configured_scene_tracker; k < SCENE_MAX_COUNT; k++)
            {
                Scene_t* temp_handle = Scene_GetInstance(k);
                if (temp_handle && temp_handle->is_configured)
                {
                    if (temp_handle->appearance_id < (sizeof(scene_appearance_table) / sizeof(SceneAppearance_t))) {
                        appearance = &scene_appearance_table[temp_handle->appearance_id];
                    }
                    configured_scene_tracker = k + 1;
                    break;
                }
            }

            if (!appearance) continue;

            // Pozicioniranje u mreži ostaje isto
            int row = i / scene_screen_layout.items_per_row;
            int col = i % scene_screen_layout.items_per_row;
            int x_center = (scene_screen_layout.slot_width / 2) + (col * scene_screen_layout.slot_width);
            int y_center = (scene_screen_layout.slot_height / 2) + (row * scene_screen_layout.slot_height);
            
            // Iscrtavanje ikonice i teksta
            int scene_icon_index = appearance->icon_id - ICON_SCENE_WIZZARD;
            if (scene_icon_index >= 0 && scene_icon_index < (sizeof(scene_icon_images) / sizeof(scene_icon_images[0])))
            {
                const GUI_BITMAP* icon_to_draw = scene_icon_images[scene_icon_index];
                GUI_DrawBitmap(icon_to_draw, x_center - (icon_to_draw->XSize / 2), y_center - (icon_to_draw->YSize / 2));
            }
            
            GUI_SetFont(&GUI_FontVerdana16_LAT);
            GUI_SetColor(GUI_ORANGE);
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER);
            GUI_DispStringAt(lng(appearance->text_id), x_center, y_center + scene_screen_layout.text_y_offset);
        }

        // --- Iscrtavanje ikonice čarobnjaka odvojeno ---
        if (configured_scenes_count < SCENE_MAX_COUNT)
        {
            const GUI_BITMAP* wizard_icon = &bmicons_scene_wizzard;
            // Koristimo koordinate konzistentne sa "Next" dugmetom
            int x_pos = select_screen2_drawing_layout.next_button_x_pos;
            int y_pos = select_screen2_drawing_layout.next_button_y_center - (wizard_icon->YSize / 2);
            GUI_DrawBitmap(wizard_icon, x_pos, y_pos);
            
            // === DODATA LINIJA KODA ZA ISPIS TEKSTA ===
            GUI_SetFont(&GUI_FontVerdana16_LAT);
            GUI_SetColor(GUI_ORANGE);
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER);
            int x_text_center = x_pos + (wizard_icon->XSize / 2);
            int y_text_pos = y_pos + wizard_icon->YSize + 5; // 5 piksela razmaka
            GUI_DispStringAt(lng(TXT_SCENE_WIZZARD), x_text_center, y_text_pos);
        }

        GUI_MULTIBUF_EndEx(1);
    }
}
/**
 ******************************************************************************
 * @brief       Servisira ekran sa svjetlima ISKLJUČIVO unutar "Scene Wizard" moda.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija je kopija originalne `Service_LightsScreen` funkcije,
 * modifikovana za rad unutar čarobnjaka. Uklonjen je "hamburger" meni,
 * a dodato je "[Next]" dugme sa ikonicom (80x80) i "pametnom"
 * navigacijom na sljedeći korak u čarobnjaku.
 ******************************************************************************
 */
static void Service_SceneEditLightsScreen(void)
{
    if(shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        
        // Kreiraj "Next" dugme sa ikonicom, bez teksta, 80x80
        hButtonWizNext = BUTTON_CreateEx(400, 192, 80, 80, 0, WM_CF_SHOW, 0, ID_WIZ_NEXT);
        BUTTON_SetBitmap(hButtonWizNext, BUTTON_CI_UNPRESSED, &bmnext);
        BUTTON_SetBitmap(hButtonWizNext, BUTTON_CI_PRESSED, &bmnext);
        
        // Kompletan, neizmijenjen kod za iscrtavanje ikonica svjetala
        const GUI_FONT* fontToUse = &GUI_FontVerdana20_LAT;
        const int text_padding = 10;
        bool downgrade_font = false;
        for(uint8_t i = 0; i < LIGHTS_getCount(); ++i)
        {
            uint8_t lights_total = LIGHTS_getCount();
            uint8_t lights_in_this_row = 0;
            if (lights_total <= 3) lights_in_this_row = lights_total;
            else if (lights_total == 4) lights_in_this_row = 2;
            else if (lights_total == 5) lights_in_this_row = 3;
            else lights_in_this_row = 3;
            
            int max_width_per_icon = (DRAWING_AREA_WIDTH / lights_in_this_row) - text_padding;

            LIGHT_Handle* handle = LIGHTS_GetInstance(i);
            if (handle) {
                uint16_t selection_index = LIGHT_GetIconID(handle);
                if (selection_index < (sizeof(icon_mapping_table) / sizeof(IconMapping_t)))
                {
                    const IconMapping_t* mapping = &icon_mapping_table[selection_index];
                    GUI_SetFont(&GUI_FontVerdana20_LAT);
                    
                    if (GUI_GetStringDistX(lng(mapping->primary_text_id)) > max_width_per_icon || GUI_GetStringDistX(lng(mapping->secondary_text_id)) > max_width_per_icon)
                    {
                        downgrade_font = true;
                        break;
                    }
                }
            }
        }
        if (downgrade_font) {
            fontToUse = &GUI_FontVerdana16_LAT;
        }
        int y_row_start = (LIGHTS_Rows_getCount() > 1) ? 10 : 86;
        const int y_row_height = 130;
        uint8_t lightsInRowSum = 0;
        for(uint8_t row = 0; row < LIGHTS_Rows_getCount(); ++row) {
            uint8_t lightsInRow = LIGHTS_getCount();
            if(LIGHTS_getCount() > 3) {
                if(LIGHTS_getCount() == 4) lightsInRow = 2;
                else if(LIGHTS_getCount() == 5) lightsInRow = (row > 0) ? 2 : 3;
                else lightsInRow = 3;
            }
            uint8_t currentLightsMenuSpaceBetween = (400 - (80 * lightsInRow)) / (lightsInRow - 1 + 2);
            for(uint8_t idx_in_row = 0; idx_in_row < lightsInRow; ++idx_in_row) {
                uint8_t absolute_light_index = lightsInRowSum + idx_in_row;
                LIGHT_Handle* handle = LIGHTS_GetInstance(absolute_light_index);
                if (handle) {
                    uint16_t selection_index = LIGHT_GetIconID(handle);
                    if (selection_index < (sizeof(icon_mapping_table) / sizeof(IconMapping_t)))
                    {
                        const IconMapping_t* mapping = &icon_mapping_table[selection_index];
                        GUI_CONST_STORAGE GUI_BITMAP* icon_to_draw = light_modbus_images[(mapping->visual_icon_id * 2) + LIGHT_isActive(handle)];
                        GUI_SetFont(fontToUse);
                        const int font_height = GUI_GetFontDistY();
                        const int icon_height = icon_to_draw->YSize;
                        const int icon_width = icon_to_draw->XSize;
                        const int padding = 2;
                        const int total_block_height = font_height + padding + icon_height + padding + font_height;
                        const int y_slot_center = y_row_start + (y_row_height / 2);
                        const int y_block_start = y_slot_center - (total_block_height / 2);
                        const int x_slot_start = (currentLightsMenuSpaceBetween * (idx_in_row + 1)) + (80 * idx_in_row);
                        const int x_text_center = x_slot_start + 40;
                        const int x_icon_pos = x_text_center - (icon_width / 2);
                        const int y_primary_text_pos = y_block_start;
                        const int y_icon_pos = y_primary_text_pos + font_height + padding;
                        const int y_secondary_text_pos = y_icon_pos + icon_height + padding;
                        GUI_SetTextMode(GUI_TM_TRANS);
                        GUI_SetTextAlign(GUI_TA_HCENTER);
                        GUI_SetColor(GUI_WHITE);
                        GUI_DispStringAt(lng(mapping->primary_text_id), x_text_center, y_primary_text_pos);
                        GUI_DrawBitmap(icon_to_draw, x_icon_pos, y_icon_pos);
                        GUI_SetTextMode(GUI_TM_TRANS);
                        GUI_SetTextAlign(GUI_TA_HCENTER);
                        GUI_SetColor(GUI_ORANGE);
                        GUI_DispStringAt(lng(mapping->secondary_text_id), x_text_center, y_secondary_text_pos);
                    }
                }
            }
            lightsInRowSum += lightsInRow;
            y_row_start += y_row_height;
        }
        GUI_MULTIBUF_EndEx(1);
    }

    if (BUTTON_IsPressed(hButtonWizNext))
    {
        DSP_KillSceneEditLightsScreen();
        
        Scene_t* scene_handle = Scene_GetInstance(scene_edit_index);
        if (scene_handle)
        {
            if (scene_handle->curtains_mask) {
                screen = SCREEN_CURTAINS;
            } 
            else if (scene_handle->thermostat_mask) {
                screen = SCREEN_THERMOSTAT;
            } 
            else {
                is_in_scene_wizard_mode = false;
                DSP_InitSceneEditScreen();
                screen = SCREEN_SCENE_EDIT;
                shouldDrawScreen = 0;
                return;
            }
            shouldDrawScreen = 1;
        }
    }
}
/**
 ******************************************************************************
 * @brief       Servisira ekran sa svjetlima, sa dinamičkim odabirom fonta.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija dinamički iscrtava ikone svjetala. Sadrži "pametnu"
 * logiku koja prvo analizira dužinu svih tekstualnih labela. Na osnovu
 * dostupnog prostora (koji zavisi od broja ikonica u redu), funkcija
 * bira najveći mogući font (`Verdana20_LAT` ili `Verdana16_LAT`) koji
 * može stati, a zatim taj odabrani font primjenjuje na SVE labele na
 * ekranu radi vizuelne konzistentnosti.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void Service_LightsScreen(void)
{
    if (is_in_scene_wizard_mode)
    {
        Service_SceneEditLightsScreen();
    }
    else if(shouldDrawScreen) 
    {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        DrawHamburgerMenu(1);

        // =======================================================================
        // === FAZA 1: PRE-KALKULACIJA I ODABIR FONTA ZA CIJELI EKRAN ===
        // =======================================================================
        const GUI_FONT* fontToUse = &GUI_FontVerdana20_LAT; // Početna pretpostavka: koristimo najveći font
        const int text_padding = 10; // Sigurnosni razmak u pikselima između tekstova susjednih ikonica
        bool downgrade_font = false;

        // Petlja za PRE-KALKULACIJU (prolazimo kroz sve, ali ne iscrtavamo ništa)
        for(uint8_t i = 0; i < LIGHTS_getCount(); ++i)
        {
            // Izračunaj maksimalnu dozvoljenu širinu teksta na osnovu rasporeda
            uint8_t lights_total = LIGHTS_getCount();
            uint8_t lights_in_this_row = 0;
            if (lights_total <= 3) lights_in_this_row = lights_total;
            else if (lights_total == 4) lights_in_this_row = 2;
            else if (lights_total == 5) lights_in_this_row = 3; // Pretpostavka za prvi red
            else lights_in_this_row = 3;
            
            int max_width_per_icon = (DRAWING_AREA_WIDTH / lights_in_this_row) - text_padding;

            LIGHT_Handle* handle = LIGHTS_GetInstance(i);
            if (handle) {
                uint16_t selection_index = LIGHT_GetIconID(handle);
                if (selection_index < (sizeof(icon_mapping_table) / sizeof(IconMapping_t)))
                {
                    const IconMapping_t* mapping = &icon_mapping_table[selection_index];
                    const char* primary_text = lng(mapping->primary_text_id);
                    const char* secondary_text = lng(mapping->secondary_text_id);

                    // Mjerenje širine sa velikim fontom
                    GUI_SetFont(&GUI_FontVerdana20_LAT);
                    if (GUI_GetStringDistX(primary_text) > max_width_per_icon || GUI_GetStringDistX(secondary_text) > max_width_per_icon)
                    {
                        downgrade_font = true; // Ako je BILO KOJI tekst predugačak...
                        break; // ...odmah prekidamo provjeru, odluka je pala.
                    }
                }
            }
        }

        if (downgrade_font) {
            fontToUse = &GUI_FontVerdana16_LAT; // ...cijeli ekran će koristiti manji font.
        }

        // =======================================================================
        // === FAZA 2: ISCRTAVANJE IKONICA SA KONAČNO ODABRANIM FONTOM ===
        // =======================================================================
        int y_row_start = (LIGHTS_Rows_getCount() > 1) ? 10 : 86;
        const int y_row_height = 130;
        uint8_t lightsInRowSum = 0;

        for(uint8_t row = 0; row < LIGHTS_Rows_getCount(); ++row) {
            uint8_t lightsInRow = LIGHTS_getCount();
            if(LIGHTS_getCount() > 3) {
                if(LIGHTS_getCount() == 4) lightsInRow = 2;
                else if(LIGHTS_getCount() == 5) lightsInRow = (row > 0) ? 2 : 3;
                else lightsInRow = 3;
            }
            uint8_t currentLightsMenuSpaceBetween = (400 - (80 * lightsInRow)) / (lightsInRow - 1 + 2);

            for(uint8_t idx_in_row = 0; idx_in_row < lightsInRow; ++idx_in_row) {
                uint8_t absolute_light_index = lightsInRowSum + idx_in_row;
                LIGHT_Handle* handle = LIGHTS_GetInstance(absolute_light_index);
                if (handle) {
                    uint16_t selection_index = LIGHT_GetIconID(handle);
                    if (selection_index < (sizeof(icon_mapping_table) / sizeof(IconMapping_t)))
                    {
                        const IconMapping_t* mapping = &icon_mapping_table[selection_index];
                        GUI_CONST_STORAGE GUI_BITMAP* icon_to_draw = light_modbus_images[(mapping->visual_icon_id * 2) + LIGHT_isActive(handle)];
                        
                        // Postavljamo KONAČNO ODABRANI FONT za iscrtavanje
                        GUI_SetFont(fontToUse);
                        const int font_height = GUI_GetFontDistY();
                        const int icon_height = icon_to_draw->YSize;
                        const int icon_width = icon_to_draw->XSize;
                        const int padding = 2;
                        const int total_block_height = font_height + padding + icon_height + padding + font_height;
                        const int y_slot_center = y_row_start + (y_row_height / 2);
                        const int y_block_start = y_slot_center - (total_block_height / 2);
                        const int x_slot_start = (currentLightsMenuSpaceBetween * (idx_in_row + 1)) + (80 * idx_in_row);
                        const int x_text_center = x_slot_start + 40;
                        const int x_icon_pos = x_text_center - (icon_width / 2);
                        const int y_primary_text_pos = y_block_start;
                        const int y_icon_pos = y_primary_text_pos + font_height + padding;
                        const int y_secondary_text_pos = y_icon_pos + icon_height + padding;

                        GUI_SetTextMode(GUI_TM_TRANS);
                        GUI_SetTextAlign(GUI_TA_HCENTER);
                        
                        GUI_SetColor(GUI_WHITE);
                        GUI_DispStringAt(lng(mapping->primary_text_id), x_text_center, y_primary_text_pos);

                        GUI_DrawBitmap(icon_to_draw, x_icon_pos, y_icon_pos);

                        GUI_SetTextMode(GUI_TM_TRANS);
                        GUI_SetTextAlign(GUI_TA_HCENTER);
                        GUI_SetColor(GUI_ORANGE);
                        GUI_DispStringAt(lng(mapping->secondary_text_id), x_text_center, y_secondary_text_pos);
                    }
                }
            }
            lightsInRowSum += lightsInRow;
            y_row_start += y_row_height;
        }
        GUI_MULTIBUF_EndEx(1);
    }
}

/**
 ******************************************************************************
 * @brief       Servisira ekran sa roletnama ISKLJUČIVO unutar "Scene Wizard" moda.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija je kopija originalne `Service_CurtainsScreen` funkcije,
 * modifikovana za rad unutar čarobnjaka. Uklonjen je "hamburger" meni,
 * a dodato je "[Next]" dugme sa ikonicom (80x80) i "pametnom"
 * navigacijom na sljedeći korak u čarobnjaku.
 ******************************************************************************
 */
static void Service_SceneEditCurtainsScreen(void)
{
    if(shouldDrawScreen) {
        shouldDrawScreen = 0;
        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        
        // Kreiraj "Next" dugme sa ikonicom, bez teksta, 80x80
        hButtonWizNext = BUTTON_CreateEx(390, 182, 80, 80, 0, WM_CF_SHOW, 0, ID_WIZ_NEXT);
        BUTTON_SetBitmap(hButtonWizNext, BUTTON_CI_UNPRESSED, &bmnext);
        BUTTON_SetBitmap(hButtonWizNext, BUTTON_CI_PRESSED, &bmnext);
        
        // Kompletan, neizmijenjen kod za iscrtavanje kontrola za roletne iz originalne funkcije
        GUI_SetColor(GUI_WHITE);
        if(!Curtain_areAllSelected()) {
            GUI_SetFont(GUI_FONT_D48);
            uint8_t physical_index = 0;
            uint8_t count = 0;
            for (uint8_t i = 0; i < CURTAINS_SIZE; i++) {
                Curtain_Handle* handle = Curtain_GetInstanceByIndex(i);
                if (Curtain_hasRelays(handle)) {
                    if (count == curtain_selected) {
                        physical_index = i;
                        break;
                    }
                    count++;
                }
            }
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
            GUI_DispDecAt(physical_index + 1, 50, 50, ((physical_index + 1) < 10) ? 1 : 2);
        } else {
            GUI_SetFont(&GUI_FontVerdana32_LAT);
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
            GUI_DispStringAt(lng(TXT_ALL), 75, 40);
        }
        
        const uint16_t drawingAreaWidth = 380;
        const uint8_t triangleBaseWidth = 180;
        const uint8_t triangleHeight = 90;
        const uint16_t horizontalOffset = (drawingAreaWidth - triangleBaseWidth) / 2;
        const uint8_t yLinePosition = 136;
        const uint8_t verticalGap = 20;
        const uint8_t verticalOffsetUp = yLinePosition - triangleHeight - verticalGap;
        const uint8_t verticalOffsetDown = yLinePosition + verticalGap;

        GUI_SetColor(GUI_WHITE);
        GUI_DrawLine(horizontalOffset, yLinePosition, horizontalOffset + triangleBaseWidth, yLinePosition);

        const GUI_POINT aBlindsUp[] = { {0, triangleHeight}, {triangleBaseWidth, triangleHeight}, {triangleBaseWidth / 2, 0} };
        const GUI_POINT aBlindsDown[] = { {0, 0}, {triangleBaseWidth, 0}, {triangleBaseWidth / 2, triangleHeight} };

        bool isMovingUp, isMovingDown;
        if(Curtain_areAllSelected()) {
            isMovingUp = Curtains_isAnyCurtainMovingUp();
            isMovingDown = Curtains_isAnyCurtainMovingDown();
        } else {
            Curtain_Handle* cur = Curtain_GetByLogicalIndex(curtain_selected);
            if (cur) {
                isMovingUp = Curtain_isMovingUp(cur);
                isMovingDown = Curtain_isMovingDown(cur);
            } else {
                isMovingUp = false; isMovingDown = false;
            }
        }

        if (isMovingUp) { GUI_SetColor(GUI_RED); GUI_FillPolygon(aBlindsUp, 3, horizontalOffset, verticalOffsetUp); } 
        else { GUI_SetColor(GUI_RED); GUI_DrawPolygon(aBlindsUp, 3, horizontalOffset, verticalOffsetUp); }

        if (isMovingDown) { GUI_SetColor(GUI_BLUE); GUI_FillPolygon(aBlindsDown, 3, horizontalOffset, verticalOffsetDown); } 
        else { GUI_SetColor(GUI_BLUE); GUI_DrawPolygon(aBlindsDown, 3, horizontalOffset, verticalOffsetDown); }

        if (Curtains_getCount() > 1) {
            const uint8_t arrowSize = 50;
            const uint16_t verticalArrowCenter = 192 + (80/2);
            const uint16_t leftSpace = horizontalOffset;
            const uint16_t rightSpace = drawingAreaWidth - (horizontalOffset + triangleBaseWidth);
            const uint16_t xLeftArrow = leftSpace/2 - arrowSize/2;
            const uint16_t xRightArrow = horizontalOffset + triangleBaseWidth + (rightSpace/2) - arrowSize/2;
            const GUI_POINT leftArrow[] = { {xLeftArrow + arrowSize, verticalArrowCenter - arrowSize / 2}, {xLeftArrow, verticalArrowCenter}, {xLeftArrow + arrowSize, verticalArrowCenter + arrowSize / 2}, };
            const GUI_POINT rightArrow[] = { {xRightArrow, verticalArrowCenter - arrowSize / 2}, {xRightArrow + arrowSize, verticalArrowCenter}, {xRightArrow, verticalArrowCenter + arrowSize / 2}, };
            GUI_SetColor(GUI_WHITE);
            GUI_DrawPolygon(leftArrow, 3, 0, 0);
            GUI_DrawPolygon(rightArrow, 3, 0, 0);
        }
        GUI_MULTIBUF_EndEx(1);
    }

    // "Pametna" navigacija za "[Next]" dugme
    if (BUTTON_IsPressed(hButtonWizNext))
    {
        DSP_KillSceneEditCurtainsScreen();
        
        Scene_t* scene_handle = Scene_GetInstance(scene_edit_index);
        if (scene_handle)
        {
            if (scene_handle->thermostat_mask) {
                screen = SCREEN_THERMOSTAT;
            } 
            else {
                is_in_scene_wizard_mode = false; // Završi wizard mod
                DSP_InitSceneEditScreen();
                screen = SCREEN_SCENE_EDIT;
                shouldDrawScreen = 0;
                return;
            }
            shouldDrawScreen = 1;
        }
    }
}
/**
 ******************************************************************************
 * @brief       Servisira ekran sa zavjesama.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija dinamički iscrtava korisnički interfejs za kontrolu zavjesa.
 * Odgovorna je za crtanje trouglova za smjer kretanja, bijele linije kao vizualnog
 * separatora i navigacijskih dugmadi. Stanje trouglova (obojeno/neobojeno)
 * precizno reflektuje status odabrane zavjese ili grupe.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void Service_CurtainsScreen(void)
{
    if (is_in_scene_wizard_mode)
    {
        Service_SceneEditCurtainsScreen();
    }
    else if(shouldDrawScreen) 
    {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);

        GUI_Clear();

        // 1. CRTANJE GLOBALNIH UI ELEMENATA
        //--------------------------------------------------------------------------------
        // Iscrtavanje hamburger menija
        DrawHamburgerMenu(1);

        // Prikaz broja odabrane zavjese ili teksta "SVE".
        GUI_ClearRect(0, 0, 70, 70);
        GUI_SetColor(GUI_WHITE);

        // KLJUČNA PROMJENA: Ovdje se bira pravi font
        // << ISPRAVKA: Logika je ažurirana da koristi novi, sigurni API >>
        if(!Curtain_areAllSelected()) {
            GUI_SetFont(GUI_FONT_D48);
            uint8_t physical_index = 0;
            uint8_t count = 0;
            // Petlja pronalazi fizički indeks (0-15) na osnovu logičkog (0-broj konfigurisanih)
            for (uint8_t i = 0; i < CURTAINS_SIZE; i++) {
                Curtain_Handle* handle = Curtain_GetInstanceByIndex(i);
                if (Curtain_hasRelays(handle)) {
                    if (count == curtain_selected) {
                        physical_index = i;
                        break;
                    }
                    count++;
                }
            }
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
            GUI_DispDecAt(physical_index + 1, 50, 50, ((physical_index + 1) < 10) ? 1 : 2);
        } else {
            // =======================================================================
            // === IZMJENA FONT-a ===
            // =======================================================================
            GUI_SetFont(&GUI_FontVerdana32_LAT); // Koristi font za tekst "SVE"
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
            GUI_DispStringAt(lng(TXT_ALL), 75, 40);
        }

        //--------------------------------------------------------------------------------

        // 2. CRTANJE GLAVNOG KONTROLNog ELEMENTA (TROUGLOVA I LINIJE)
        //--------------------------------------------------------------------------------
        // Definiranje geometrije i pozicije elemenata
        const uint16_t drawingAreaWidth = 380;       // Širina radnog prostora za kontrole
        const uint8_t triangleBaseWidth = 180;      // Širina baze trougla (hipotenusa)
        const uint8_t triangleHeight = 90;          // Visina trougla
        const uint16_t horizontalOffset = (drawingAreaWidth - triangleBaseWidth) / 2; // Centriranje trouglova horizontalno

        // Vertikalne pozicije i razmaci
        const uint8_t yLinePosition = 136;          // Y-koordinata bijele linije
        const uint8_t verticalGap = 20;             // Razmak od linije do hipotenuze

        // Izračunavanje vertikalnih pomaka za trouglove
        const uint8_t verticalOffsetUp = yLinePosition - triangleHeight - verticalGap;
        const uint8_t verticalOffsetDown = yLinePosition + verticalGap;

        // Iscrtavanje bijele linije čija je dužina jednaka širini baze trougla.
        GUI_SetColor(GUI_WHITE);
        GUI_DrawLine(horizontalOffset, yLinePosition, horizontalOffset + triangleBaseWidth, yLinePosition);

        // Definiranje tačaka za gornji i donji trougao
        const GUI_POINT aBlindsUp[] = {
            {0, triangleHeight},
            {triangleBaseWidth, triangleHeight},
            {triangleBaseWidth / 2, 0}
        };
        const GUI_POINT aBlindsDown[] = {
            {0, 0},
            {triangleBaseWidth, 0},
            {triangleBaseWidth / 2, triangleHeight}
        };

        //--------------------------------------------------------------------------------

        // 3. ODREĐIVANJE STANJA I CRTANJE
        //--------------------------------------------------------------------------------
        // Provjera stanja kretanja ovisno o tome da li je odabrana jedna ili sve roletne.
        bool isMovingUp, isMovingDown;
        if(Curtain_areAllSelected()) {
            isMovingUp = Curtains_isAnyCurtainMovingUp();
            isMovingDown = Curtains_isAnyCurtainMovingDown();
        } else {
            Curtain_Handle* cur = Curtain_GetByLogicalIndex(curtain_selected);
            if (cur) {
                isMovingUp = Curtain_isMovingUp(cur);
                isMovingDown = Curtain_isMovingDown(cur);
            } else {
                isMovingUp = false;
                isMovingDown = false;
            }
        }
        // Iscrtavanje gornjeg trougla
        if (isMovingUp) {
            GUI_SetColor(GUI_RED);
            GUI_FillPolygon(aBlindsUp, 3, horizontalOffset, verticalOffsetUp);
        } else {
            GUI_SetColor(GUI_RED);
            GUI_DrawPolygon(aBlindsUp, 3, horizontalOffset, verticalOffsetUp);
        }

        // Iscrtavanje donjeg trougla
        if (isMovingDown) {
            GUI_SetColor(GUI_BLUE);
            GUI_FillPolygon(aBlindsDown, 3, horizontalOffset, verticalOffsetDown);
        } else {
            GUI_SetColor(GUI_BLUE);
            GUI_DrawPolygon(aBlindsDown, 3, horizontalOffset, verticalOffsetDown);
        }

        // 4. CRTANJE NAVIGACIJSKIH STRELICA (bez bitmapa)
        //--------------------------------------------------------------------------------
        // Ove strelice se crtaju samo ako postoji više od jedne zavjese.
        if (Curtains_getCount() > 1) {
            // Smanjena veličina strelica i izračunavanje novih pozicija za centriranje
            const uint8_t arrowSize = 50;
            const uint16_t verticalArrowCenter = 192 + (80/2); // Y-koordinata centra zone za strelice

            // Izračunavanje pozicija za strelice, centrirano u lijevom i desnom slobodnom prostoru
            const uint16_t leftSpace = horizontalOffset;
            const uint16_t rightSpace = drawingAreaWidth - (horizontalOffset + triangleBaseWidth);
            const uint16_t xLeftArrow = leftSpace/2 - arrowSize/2;
            const uint16_t xRightArrow = horizontalOffset + triangleBaseWidth + (rightSpace/2) - arrowSize/2;

            // Definiranje tačaka za lijevu strelicu
            const GUI_POINT leftArrow[] = {
                {xLeftArrow + arrowSize, verticalArrowCenter - arrowSize / 2},
                {xLeftArrow, verticalArrowCenter},
                {xLeftArrow + arrowSize, verticalArrowCenter + arrowSize / 2},
            };

            // Definiranje tačaka za desnu strelicu
            const GUI_POINT rightArrow[] = {
                {xRightArrow, verticalArrowCenter - arrowSize / 2},
                {xRightArrow + arrowSize, verticalArrowCenter},
                {xRightArrow, verticalArrowCenter + arrowSize / 2},
            };

            GUI_SetColor(GUI_WHITE);
            GUI_DrawPolygon(leftArrow, 3, 0, 0);
            GUI_DrawPolygon(rightArrow, 3, 0, 0);
        }
        //--------------------------------------------------------------------------------

        GUI_MULTIBUF_EndEx(1);
    }
}
/**
 * @brief Servisira ekran sa QR kodom.
 * @note Ova funkcija generiše i iscrtava QR kod na osnovu podataka
 * pohranjenih u `qr_codes` baferu.
 */
static void Service_QrCodeScreen(void)
{
    if(shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);

        GUI_Clear();

        // Iscrtavanje hamburger meni ikonice.
        DrawHamburgerMenu(1);

        // Generisanje i crtanje QR koda.
        GUI_HMEM hqr = GUI_QR_Create((char*)QR_Code_Get(qr_code_draw_id), 8, GUI_QR_ECLEVEL_M, 0);
        GUI_QR_INFO qrInfo;
        GUI_QR_GetInfo(hqr, &qrInfo);

        GUI_SetColor(GUI_WHITE);
        GUI_FillRect(0, 0, qrInfo.Size + 20, qrInfo.Size + 20);

        GUI_QR_Draw(hqr, 10, 10);
        GUI_QR_Delete(hqr);

        GUI_MULTIBUF_EndEx(1);
    }
}

/**
 ******************************************************************************
 * @brief       Servisira ekran za čišćenje ekrana (privremeno onemogućava dodir).
 * @author      Gemini & [Vaše Ime]
 * @note        Prikazuje tajmer sa odbrojavanjem od 60 sekundi. Tokom odbrojavanja,
 * ekran je zaključan za unos. Posljednjih 5 sekundi je vizuelno i
 * zvučno naglašeno. Nakon što tajmer istekne, automatski se vraća
 * na početni ekran. Koristi matematički proračun za precizno
 * centriranje teksta na ekranu.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void Service_CleanScreen(void)
{
    // --- 1. DEFINICIJA I IZRAČUNI SVIH KONSTANTI NA POČETKU FUNKCIJE ---
    const uint16_t xDisplayCenter = 480 / 2;     // Horizontalna sredina ekrana
    const uint16_t yDisplayCenter = 272 / 2;     // Vertikalna sredina ekrana

    // Visine fontova
    const uint8_t yFontTitleHeight = 32;        // Visina fonta GUI_FONT_32_1
    const uint8_t yFontCounterHeight = 64;      // Visina fonta GUI_FONT_D64

    // Razmaci
    const uint8_t textGap = 10;                 // Razmak između naslova i brojača

    // OPACIJA: Vertikalni ofset za pomjeranje SAMO naslova (naslova) gore/dolje
    const int16_t verticalTextOffset = -30;     // Negativna vrijednost pomjera gore, pozitivna dole

    // Izračun vertikalnih pozicija
    // Brojač je FIKSIRAN na sredini ekrana
    const uint16_t yCounterPosition = yDisplayCenter;

    // Naslov se pozicionira iznad brojača, uzimajući u obzir ofset
    const uint16_t yTitlePosition = yCounterPosition - (yFontCounterHeight / 2) - textGap - (yFontTitleHeight / 2) + verticalTextOffset;

    // Dinamički izračun granica za GUI_ClearRect (sada pokriva cijeli dinamični prostor)
    const uint16_t yClearRectStart = yTitlePosition - (yFontTitleHeight / 2) - 5; // Padding 5px
    const uint16_t yClearRectEnd = yCounterPosition + (yFontCounterHeight / 2) + 5; // Padding 5px
    // --- KRAJ DEFINICIJE KONSTANTI ---


    if (menu_clean == 0) {
        // Prvi ulazak na ekran za čišćenje, inicijalizuj stanje i tajmer.
        menu_clean = 1;
        GUI_Clear();
        clrtmr = 60; // Tajmer na 60 sekundi.
    } else if (menu_clean == 1) {
        // Odbrojavanje tajmera.
        if (HAL_GetTick() - clean_tmr >= 1000) {
            clean_tmr = HAL_GetTick();
            DISPResetScrnsvr();

            GUI_MULTIBUF_BeginEx(1);

            // Dinamičko čišćenje ekrana u zoni teksta
            GUI_ClearRect(0, yClearRectStart, 480, yClearRectEnd);

            // Promjena boje teksta i zvučni signal za posljednjih 5 sekundi.
            GUI_SetColor((clrtmr > 5) ? GUI_GREEN : GUI_RED);
            if (clrtmr <= 5) {
                BuzzerOn();
                HAL_Delay(1);
                BuzzerOff();
            }

            // --- Iscrtavanje naslova i broja na matematički određenim pozicijama ---
            // =======================================================================
            // === IZMJENA FONT-a ===
            // =======================================================================
            GUI_SetFont(&GUI_FontVerdana32_LAT);
            GUI_SetTextMode(GUI_TM_TRANS);

            GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
            GUI_DispStringAt(lng(TXT_DISPLAY_CLEAN_TIME), xDisplayCenter, yTitlePosition);

            char count_str[3];
            sprintf(count_str, "%d", clrtmr);

            GUI_SetFont(GUI_FONT_D64);
            GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
            GUI_DispStringAt(count_str, xDisplayCenter, yCounterPosition);
            // --- KRAJ ISPRAVKE ---

            GUI_MULTIBUF_EndEx(1);

            if (clrtmr) {
                --clrtmr;
            } else {
                screen = SCREEN_RETURN_TO_FIRST;
            }
        }
    }
}
/**
 ******************************************************************************
 * @brief       Servisira placeholder ekran za tajmer.
 * @author      Gemini & [Vaše Ime]
 * @note        Ovo je privremena funkcija koja služi za uspostavljanje
 * navigacije. Iscrtava samo naslov ekrana i meni za povratak,
 * pripremajući teren za buduću implementaciju podešavanja tajmera.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void Service_TimerScreen(void)
{
    if (shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        DrawHamburgerMenu(1); // Meni za povratak

        // Iscrtaj naslov kao placeholder
        GUI_SetFont(&GUI_FontVerdana32_LAT);
        GUI_SetColor(GUI_WHITE);
        GUI_SetTextMode(GUI_TM_TRANS);
        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt("TAJMER", LCD_GetXSize() / 2, LCD_GetYSize() / 2);

        GUI_MULTIBUF_EndEx(1);
    }
}
/**
 ******************************************************************************
 * @brief       Servisira placeholder ekran za alarm.
 * @author      Gemini & [Vaše Ime]
 * @note        Ovo je privremena funkcija koja služi za uspostavljanje
 * navigacije. Iscrtava samo naslov ekrana i meni za povratak,
 * pripremajući teren za buduću implementaciju kontrola alarmnog sistema.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void Service_SecurityScreen(void)
{
    if (shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        DrawHamburgerMenu(1); // Meni za povratak

        // Iscrtaj naslov kao placeholder
        GUI_SetFont(&GUI_FontVerdana32_LAT);
        GUI_SetColor(GUI_WHITE);
        GUI_SetTextMode(GUI_TM_TRANS);
        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt("ALARM", LCD_GetXSize() / 2, LCD_GetYSize() / 2);

        GUI_MULTIBUF_EndEx(1);
    }
}
/**
 * @brief  Servisira prvi ekran podešavanja (kontrola termostata i ventilatora).
 * @note   Refaktorisana verzija koja koristi isključivo javni API termostat modula.
 * @param  None
 * @retval None
 */
static void Service_SettingsScreen_1(void)
{
    // Dobijamo handle za termostat na početku funkcije.
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();

    // Detekcija promjena na widgetima.
    if (Thermostat_GetControlMode(pThst) != RADIO_GetValue(hThstControl)) {
        Thermostat_SetControlMode(pThst, RADIO_GetValue(hThstControl));
        ++thsta;
    }
    if (Thermostat_GetFanControlMode(pThst) != RADIO_GetValue(hFanControl)) {
        Thermostat_SetFanControlMode(pThst, RADIO_GetValue(hFanControl));
        ++thsta;
    }
    // === NASTAVAK REFAKTORISANJA ===
    if (Thermostat_Get_SP_Max(pThst) != SPINBOX_GetValue(hThstMaxSetPoint)) {
        // Pozivamo setter sa handle-om.
        Thermostat_Set_SP_Max(pThst, SPINBOX_GetValue(hThstMaxSetPoint));
        // Očitavamo validiranu vrijednost nazad pomoću gettera.
        SPINBOX_SetValue(hThstMaxSetPoint, Thermostat_Get_SP_Max(pThst));
        ++thsta;
    }
    if (Thermostat_Get_SP_Min(pThst) != SPINBOX_GetValue(hThstMinSetPoint)) {
        // Pozivamo setter sa handle-om.
        Thermostat_Set_SP_Min(pThst, SPINBOX_GetValue(hThstMinSetPoint));
        // Očitavamo validiranu vrijednost nazad pomoću gettera.
        SPINBOX_SetValue(hThstMinSetPoint, Thermostat_Get_SP_Min(pThst));
        ++thsta;
    }
    if (Thermostat_GetFanDifference(pThst) != SPINBOX_GetValue(hFanDiff)) {
        // Postavljamo novu vrijednost preko settera.
        Thermostat_SetFanDifference(pThst, SPINBOX_GetValue(hFanDiff));
        ++thsta;
    }
    if (Thermostat_GetFanLowBand(pThst) != SPINBOX_GetValue(hFanLowBand)) {
        // Postavljamo novu vrijednost preko settera.
        Thermostat_SetFanLowBand(pThst, SPINBOX_GetValue(hFanLowBand));
        ++thsta;
    }
    if (Thermostat_GetFanHighBand(pThst) != SPINBOX_GetValue(hFanHiBand)) {
        // Postavljamo novu vrijednost preko settera.
        Thermostat_SetFanHighBand(pThst, SPINBOX_GetValue(hFanHiBand));
        ++thsta;
    }
    if (Thermostat_GetGroup(pThst) != SPINBOX_GetValue(hThstGroup)) {
        // Postavljamo novu vrijednost preko settera.
        Thermostat_SetGroup(pThst, SPINBOX_GetValue(hThstGroup));
        thsta = 1;
    }
    if (Thermostat_IsMaster(pThst) != CHECKBOX_IsChecked(hThstMaster)) {
        // Postavljamo novu vrijednost preko settera.
        Thermostat_SetMaster(pThst, CHECKBOX_IsChecked(hThstMaster));
        thsta = 1;
    }

    // Obrada pritiska na dugmad "SAVE" ili "NEXT"
    if (BUTTON_IsPressed(hBUTTON_Ok)) {
        if (thsta) {
            // ISPRAVKA: Pozivamo THSTAT_Save sa handle-om.
            // Fleg hasInfoChanged se sada postavlja unutar settera.
            THSTAT_Save(pThst);
        }
        thsta = 0;
        DSP_KillSet1Scrn(); // Uništavanje widgeta trenutnog ekrana
        screen = SCREEN_RETURN_TO_FIRST;
    } else if (BUTTON_IsPressed(hBUTTON_Next)) {
        // ISTA ISPRAVKA I ZA NEXT DUGME
        if (thsta) {
            THSTAT_Save(pThst);
        }
        thsta = 0;
        DSP_KillSet1Scrn();
        DSP_InitSet2Scrn(); // Inicijalizacija sljedećeg ekrana
        screen = SCREEN_SETTINGS_2;
    }
}
/**
 * @brief Servisira drugi ekran podešavanja (vrijeme, datum, screensaver).
 * @note  Verzija 2.1: Ispravljena logika za ažuriranje prikaza boje.
 * Ova funkcija sinkronizuje vrijeme i datum s RTC modulom,
 * obrađuje promjene postavki screensavera i svjetline ekrana,
 * te ih pohranjuje u EEPROM kada se pritisne "SAVE".
 */
static void Service_SettingsScreen_2(void)
{
    /** @brief Dobijamo handle za termostat radi konzistentnosti sa ostalim funkcijama. */
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();

    /** @brief Detekcija promjena u postavkama. */
    if (rtctm.Hours != Dec2Bcd(SPINBOX_GetValue(hSPNBX_Hour))) {
        rtctm.Hours = Dec2Bcd(SPINBOX_GetValue(hSPNBX_Hour));
        HAL_RTC_SetTime(&hrtc, &rtctm, RTC_FORMAT_BCD);
        RtcTimeValidSet();
    }
    if (rtctm.Minutes != Dec2Bcd(SPINBOX_GetValue(hSPNBX_Minute))) {
        rtctm.Minutes = Dec2Bcd(SPINBOX_GetValue(hSPNBX_Minute));
        HAL_RTC_SetTime(&hrtc, &rtctm, RTC_FORMAT_BCD);
        RtcTimeValidSet();
    }
    if (rtcdt.Date != Dec2Bcd(SPINBOX_GetValue(hSPNBX_Day))) {
        rtcdt.Date = Dec2Bcd(SPINBOX_GetValue(hSPNBX_Day));
        HAL_RTC_SetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);
        RtcTimeValidSet();
    }
    if (rtcdt.Month != Dec2Bcd(SPINBOX_GetValue(hSPNBX_Month))) {
        rtcdt.Month = Dec2Bcd(SPINBOX_GetValue(hSPNBX_Month));
        HAL_RTC_SetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);
        RtcTimeValidSet();
    }
    if (rtcdt.Year != Dec2Bcd(SPINBOX_GetValue(hSPNBX_Year) - 2000)) {
        rtcdt.Year = Dec2Bcd(SPINBOX_GetValue(hSPNBX_Year) - 2000);
        HAL_RTC_SetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);
        RtcTimeValidSet();
    }
    if (rtcdt.WeekDay != (DROPDOWN_GetSel(hDRPDN_WeekDay) + 1)) { // Poređenje sa Dec vrijednošću
        rtcdt.WeekDay = (DROPDOWN_GetSel(hDRPDN_WeekDay) + 1);
        HAL_RTC_SetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);
        RtcTimeValidSet();
    }

    /**
     * @brief << ISPRAVKA: Ažuriranje prikaza boje screensaver sata. >>
     * @note  Sada, kada se detektuje promjena vrijednosti u spinbox-u,
     * prvo se postavlja nova boja sa `GUI_SetColor`, a tek onda se
     * ponovo iscrtava pravougaonik za pregled.
     */
    if (g_display_settings.scrnsvr_clk_clr != SPINBOX_GetValue(hSPNBX_ScrnsvrClockColour)) {
        g_display_settings.scrnsvr_clk_clr = SPINBOX_GetValue(hSPNBX_ScrnsvrClockColour);

        // Prvo postavimo novu boju
        GUI_SetColor(clk_clrs[g_display_settings.scrnsvr_clk_clr]);
        // Zatim iscrtamo pravougaonik tom bojom
        GUI_FillRect(settings_screen_2_layout.scrnsvr_color_preview_rect.x0, settings_screen_2_layout.scrnsvr_color_preview_rect.y0,
                     settings_screen_2_layout.scrnsvr_color_preview_rect.x1, settings_screen_2_layout.scrnsvr_color_preview_rect.y1);
    }

    // << ISPRAVKA 3: Ispravljena logika ažuriranja screensaver postavki >>
    // Prvo ažuriramo konfiguracionu varijablu na osnovu stanja checkbox-a.
    if (g_display_settings.scrnsvr_on_off != (bool)CHECKBOX_GetState(hCHKBX_ScrnsvrClock)) {
        g_display_settings.scrnsvr_on_off = (bool)CHECKBOX_GetState(hCHKBX_ScrnsvrClock);
        settingsChanged = 1; // Signaliziramo da je došlo do promjene za snimanje
    }

    // Zatim, na osnovu te konfiguracione varijable, ažuriramo runtime fleg.
    if (g_display_settings.scrnsvr_on_off) {
        ScrnsvrClkSet();
    } else {
        ScrnsvrClkReset();
    }

    /** @brief Ažuriranje ostalih konfiguracionih varijabli. */
    g_display_settings.high_bcklght = SPINBOX_GetValue(hSPNBX_DisplayHighBrightness);
    g_display_settings.low_bcklght = SPINBOX_GetValue(hSPNBX_DisplayLowBrightness);
    g_display_settings.scrnsvr_tout = SPINBOX_GetValue(hSPNBX_ScrnsvrTimeout);
    g_display_settings.scrnsvr_ena_hour = SPINBOX_GetValue(hSPNBX_ScrnsvrEnableHour);
    g_display_settings.scrnsvr_dis_hour = SPINBOX_GetValue(hSPNBX_ScrnsvrDisableHour);

    /** @brief Obrada pritiska na dugmad "SAVE" i "NEXT". */
    if (BUTTON_IsPressed(hBUTTON_Ok)) {
        if (thsta) {
            thsta = 0;
            THSTAT_Save(pThst);
        }
        if (lcsta) {
            lcsta = 0;
            LIGHTS_Save(); // Pretpostavka da `lcsta` signalizira promjenu u svjetlima
        }

        Display_Save();
        EE_WriteBuffer(&tfifa, EE_TFIFA, 1);
        DSP_KillSet2Scrn();
        screen = SCREEN_RETURN_TO_FIRST;
    } else if (BUTTON_IsPressed(hBUTTON_Next)) {
        // Snimi sve promjene prije prelaska na sljedeći ekran
        Display_Save();
        EE_WriteBuffer(&tfifa, EE_TFIFA, 1);
        if (thsta) {
            THSTAT_Save(pThst);
            thsta = 0;
        }
        if (lcsta) {
            LIGHTS_Save();
            lcsta = 0;
        }

        DSP_KillSet2Scrn();
        DSP_InitSet3Scrn();
        screen = SCREEN_SETTINGS_3;
    }
}
/**
 * @brief Servisira treći ekran podešavanja (Ventilator).
 * @note Ova funkcija obrađuje promjene u postavkama ventilatora (relej, odgode),
 * i pohranjuje ih u EEPROM kada se pritisne "SAVE".
 */
static void Service_SettingsScreen_3(void)
{
    static uint8_t old_selection = 0;
    Defroster_Handle* defHandle = Defroster_GetInstance();
    Ventilator_Handle* ventHandle = Ventilator_GetInstance();
    uint8_t current_selection = DROPDOWN_GetSel(hSelectControl_4);

    // Provjera da li se odabir u dropdown meniju promijenio.
    if (current_selection != old_selection) {
        old_selection = current_selection;
        g_display_settings.selected_control_mode = current_selection;
        DSP_KillSet3Scrn();
        DSP_InitSet3Scrn();
    }

    // Provjera promjena na widgetima DEFROSTER
    if(Defroster_getCycleTime(defHandle) != SPINBOX_GetValue(defroster_settingWidgets.cycleTime)) {
        Defroster_setCycleTime(defHandle, SPINBOX_GetValue(defroster_settingWidgets.cycleTime));
        settingsChanged = 1;
    }
    if(Defroster_getActiveTime(defHandle) != SPINBOX_GetValue(defroster_settingWidgets.activeTime)) {
        Defroster_setActiveTime(defHandle, SPINBOX_GetValue(defroster_settingWidgets.activeTime));
        settingsChanged = 1;
    }
    if(Defroster_getPin(defHandle) != SPINBOX_GetValue(defroster_settingWidgets.pin)) {
        Defroster_setPin(defHandle, SPINBOX_GetValue(defroster_settingWidgets.pin));
        settingsChanged = 1;
    }

    // Provjera promjena na widgetima VENTILATOR
    if(Ventilator_getRelay(ventHandle) != SPINBOX_GetValue(hVentilatorRelay)) {
        Ventilator_setRelay(ventHandle, SPINBOX_GetValue(hVentilatorRelay));
        settingsChanged = 1;
    }
    if(Ventilator_getDelayOnTime(ventHandle) != SPINBOX_GetValue(hVentilatorDelayOn)) {
        Ventilator_setDelayOnTime(ventHandle, SPINBOX_GetValue(hVentilatorDelayOn));
        settingsChanged = 1;
    }
    if(Ventilator_getDelayOffTime(ventHandle) != SPINBOX_GetValue(hVentilatorDelayOff)) {
        Ventilator_setDelayOffTime(ventHandle, SPINBOX_GetValue(hVentilatorDelayOff));
        settingsChanged = 1;
    }
    if(Ventilator_getTriggerSource1(ventHandle) != SPINBOX_GetValue(hVentilatorTriggerSource1)) {
        Ventilator_setTriggerSource1(ventHandle, SPINBOX_GetValue(hVentilatorTriggerSource1));
        settingsChanged = 1;
    }
    if(Ventilator_getTriggerSource2(ventHandle) != SPINBOX_GetValue(hVentilatorTriggerSource2)) {
        Ventilator_setTriggerSource2(ventHandle, SPINBOX_GetValue(hVentilatorTriggerSource2));
        settingsChanged = 1;
    }
    if(Ventilator_getLocalPin(ventHandle) != SPINBOX_GetValue(hVentilatorLocalPin)) {
        Ventilator_setLocalPin(ventHandle, SPINBOX_GetValue(hVentilatorLocalPin));
        settingsChanged = 1;
    }

    if (BUTTON_IsPressed(hBUTTON_Ok)) {
        if(settingsChanged) {
            Display_Save();
            Defroster_Save(defHandle);
            Ventilator_Save(ventHandle);
            settingsChanged = 0;
        }
        DSP_KillSet3Scrn();
        screen = SCREEN_RETURN_TO_FIRST;
    } else if (BUTTON_IsPressed(hBUTTON_Next)) {
        if(settingsChanged) {
            Display_Save();
            Defroster_Save(defHandle);
            Ventilator_Save(ventHandle);
            settingsChanged = 0;
        }
        DSP_KillSet3Scrn();
        DSP_InitSet4Scrn();
        screen = SCREEN_SETTINGS_4;
    }
}

/**
 * @brief Servisira četvrti ekran podešavanja (Zavjese).
 * @note Ovaj ekran omogućava podešavanje Modbus adresa releja za zavjese.
 * Koristi petlju za dinamičko upravljanje widgetima i provjeru promjena.
 */
static void Service_SettingsScreen_4(void)
{
    // Ažuriranje postavki za zavjese unutar petlje.
    for(uint8_t idx = curtainSettingMenu * 4; idx < (((CURTAINS_SIZE - (curtainSettingMenu * 4)) >= 4) ? ((curtainSettingMenu * 4) + 4) : CURTAINS_SIZE); idx++) {

        // << ISPRAVKA: Dobijamo handle za roletnu po fizičkom indeksu. >>
        Curtain_Handle* handle = Curtain_GetInstanceByIndex(idx);
        if (!handle) continue; // Ako handle nije validan, preskoči

        // << ISPRAVKA: Koristimo gettere za poređenje i settere za upis. >>
        if((Curtain_getRelayUp(handle) != SPINBOX_GetValue(hCurtainsRelay[idx * 2])) || (Curtain_getRelayDown(handle) != SPINBOX_GetValue(hCurtainsRelay[(idx * 2) + 1]))) {
            settingsChanged = 1;
            Curtain_setRelayUp(handle, SPINBOX_GetValue(hCurtainsRelay[idx * 2]));
            Curtain_setRelayDown(handle, SPINBOX_GetValue(hCurtainsRelay[(idx * 2) + 1]));
        }
    }

    if (BUTTON_IsPressed(hBUTTON_Ok)) {
        if(settingsChanged) {
            Curtains_Save(); // Spremi sve postavke zavjesa u EEPROM.
            settingsChanged = 0;
        }
        DSP_KillSet4Scrn();
        screen = SCREEN_RETURN_TO_FIRST;
    } else if (BUTTON_IsPressed(hBUTTON_Next)) {
        // Logika za prelazak na sljedeću stranicu podešavanja zavjesa ili na sljedeći ekran.
        if((CURTAINS_SIZE - ((curtainSettingMenu + 1) * 4)) > 0) {
            DSP_KillSet4Scrn();
            ++curtainSettingMenu;
            DSP_InitSet4Scrn();
        } else {
            if(settingsChanged) {
                Curtains_Save();
                settingsChanged = 0;
            }
            DSP_KillSet4Scrn();
            curtainSettingMenu = 0;
            DSP_InitSet5Scrn();
            screen = SCREEN_SETTINGS_5;
        }
    }
}
/**
 ******************************************************************************
 * @brief       Servisira peti ekran podešavanja (detaljne postavke za svjetla).
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija u petlji provjerava da li je korisnik promijenio vrijednost
 * nekog widgeta. Ako jeste, poziva odgovarajuću `setter` funkciju iz `lights` API-ja
 * da ažurira stanje u RAM-u. Stvarno snimanje u EEPROM se dešava tek pritiskom
 * na 'SAVE' ili 'NEXT' dugme. Također iscrtava preview ikonice sa
 * odgovarajućim opisima, osiguravajući da je područje iscrtavanja
 * prethodno obrisano radi izbjegavanja vizuelnih grešaka.
 ******************************************************************************
 */
static void Service_SettingsScreen_5(void)
{
    GUI_MULTIBUF_BeginEx(1);

    // Dohvatamo indeks svjetla koje trenutno konfigurišemo.
    uint8_t light_index = lightsModbusSettingsMenu;
    
    // KORAK 1: Dobijamo siguran "handle" na svjetlo.
    LIGHT_Handle* handle = LIGHTS_GetInstance(light_index);
    if (!handle) {
        GUI_MULTIBUF_EndEx(1);
        return; // Ako handle nije validan, prekidamo izvršavanje
    }

    // Logika za omogućavanje/onemogućavanje spinbox-a za minute ostaje ista.
    int current_hour_value = SPINBOX_GetValue(lightsWidgets[light_index].on_hour);
    if (current_hour_value == -1) {
        if (WM_IsEnabled(lightsWidgets[light_index].on_minute)) {
            WM_DisableWindow(lightsWidgets[light_index].on_minute);
        }
    } else {
        if (!WM_IsEnabled(lightsWidgets[light_index].on_minute)) {
            WM_EnableWindow(lightsWidgets[light_index].on_minute);
        }
    }

    // KORAK 2: Provjeravamo promjene na SVAKOM widgetu i pozivamo odgovarajući SETTER.
    if(LIGHT_GetRelay(handle) != SPINBOX_GetValue(lightsWidgets[light_index].relay)) {
        settingsChanged = 1;
        LIGHT_SetRelay(handle, SPINBOX_GetValue(lightsWidgets[light_index].relay));
    }
    if(LIGHT_GetIconID(handle) != SPINBOX_GetValue(lightsWidgets[light_index].iconID)) {
        settingsChanged = 1;
        LIGHT_SetIconID(handle, SPINBOX_GetValue(lightsWidgets[light_index].iconID));
    }
    if(LIGHT_GetControllerID(handle) != SPINBOX_GetValue(lightsWidgets[light_index].controllerID_on)) {
        settingsChanged = 1;
        LIGHT_SetControllerID(handle, SPINBOX_GetValue(lightsWidgets[light_index].controllerID_on));
    }
    if(LIGHT_GetOnDelayTime(handle) != SPINBOX_GetValue(lightsWidgets[light_index].controllerID_on_delay)) {
        settingsChanged = 1;
        LIGHT_SetOnDelayTime(handle, SPINBOX_GetValue(lightsWidgets[light_index].controllerID_on_delay));
    }
    if(LIGHT_GetOffTime(handle) != SPINBOX_GetValue(lightsWidgets[light_index].offTime)) {
        settingsChanged = 1;
        LIGHT_SetOffTime(handle, SPINBOX_GetValue(lightsWidgets[light_index].offTime));
    }
    if(LIGHT_GetOnHour(handle) != SPINBOX_GetValue(lightsWidgets[light_index].on_hour)) {
        settingsChanged = 1;
        LIGHT_SetOnHour(handle, SPINBOX_GetValue(lightsWidgets[light_index].on_hour));
    }
    if(LIGHT_GetOnMinute(handle) != SPINBOX_GetValue(lightsWidgets[light_index].on_minute)) {
        settingsChanged = 1;
        LIGHT_SetOnMinute(handle, SPINBOX_GetValue(lightsWidgets[light_index].on_minute));
    }
    if(LIGHT_GetCommunicationType(handle) != SPINBOX_GetValue(lightsWidgets[light_index].communication_type)) {
        settingsChanged = 1;
        LIGHT_SetCommunicationType(handle, SPINBOX_GetValue(lightsWidgets[light_index].communication_type));
    }
    if(LIGHT_GetLocalPin(handle) != SPINBOX_GetValue(lightsWidgets[light_index].local_pin)) {
        settingsChanged = 1;
        LIGHT_SetLocalPin(handle, SPINBOX_GetValue(lightsWidgets[light_index].local_pin));
    }
    if(LIGHT_GetSleepTime(handle) != SPINBOX_GetValue(lightsWidgets[light_index].sleep_time)) {
        settingsChanged = 1;
        LIGHT_SetSleepTime(handle, SPINBOX_GetValue(lightsWidgets[light_index].sleep_time));
    }
    if(LIGHT_GetButtonExternal(handle) != SPINBOX_GetValue(lightsWidgets[light_index].button_external)) {
        settingsChanged = 1;
        LIGHT_SetButtonExternal(handle, SPINBOX_GetValue(lightsWidgets[light_index].button_external));
    }
    if(LIGHT_isTiedToMainLight(handle) != CHECKBOX_GetState(lightsWidgets[light_index].tiedToMainLight)) {
        settingsChanged = 1;
        LIGHT_SetTiedToMainLight(handle, CHECKBOX_GetState(lightsWidgets[light_index].tiedToMainLight));
    }
    if(LIGHT_isBrightnessRemembered(handle) != CHECKBOX_GetState(lightsWidgets[light_index].rememberBrightness)) {
        settingsChanged = 1;
        LIGHT_SetRememberBrightness(handle, CHECKBOX_GetState(lightsWidgets[light_index].rememberBrightness));
    }

    // =======================================================================
    // === POČETAK LOGIKE ZA ISCRTAVANJE IKONICE I TEKSTA ===
    // =======================================================================
    uint16_t selection_index = SPINBOX_GetValue(lightsWidgets[light_index].iconID);

    if (selection_index < (sizeof(icon_mapping_table) / sizeof(IconMapping_t)))
    {
        const IconMapping_t* mapping = &icon_mapping_table[selection_index];
        IconID visual_icon_to_draw = mapping->visual_icon_id;
        TextID primary_text_id = mapping->primary_text_id;
        TextID secondary_text_id = mapping->secondary_text_id;
        bool is_active = LIGHT_isActive(handle);
        GUI_CONST_STORAGE GUI_BITMAP* icon_bitmap = light_modbus_images[(visual_icon_to_draw * 2) + is_active];

        const int16_t x_icon_pos = 480 - icon_bitmap->XSize;
        const int16_t y_icon_pos = 20;
        const int16_t y_primary_text_pos = 5;
        const int16_t y_secondary_text_pos = y_icon_pos + icon_bitmap->YSize + 5;

        // =======================================================================
        // === ISPRAVKA: Brisanje fiksne regije prije iscrtavanja ===
        // =======================================================================
        GUI_ClearRect(350, 0, 480, 130);
        GUI_SetTextMode(GUI_TM_TRANS);

        // Korištenje ispravnog fonta
        GUI_SetFont(&GUI_FontVerdana16_LAT);
        GUI_SetColor(GUI_WHITE);
        GUI_SetTextAlign(GUI_TA_HCENTER);
        GUI_DispStringAt(lng(primary_text_id), x_icon_pos + (icon_bitmap->XSize / 2), y_primary_text_pos);

        GUI_DrawBitmap(icon_bitmap, x_icon_pos, y_icon_pos);
        
        // Vraćanje poziva za poravnanje
        GUI_SetTextAlign(GUI_TA_HCENTER);
        GUI_SetColor(GUI_ORANGE);
        GUI_DispStringAt(lng(secondary_text_id), x_icon_pos + (icon_bitmap->XSize / 2), y_secondary_text_pos);
    }

    if (BUTTON_IsPressed(hBUTTON_Ok) || BUTTON_IsPressed(hBUTTON_Next))
    {
        if(settingsChanged)
        {
            LIGHTS_Save();
            settingsChanged = 0;
        }

        if (BUTTON_IsPressed(hBUTTON_Ok))
        {
            DSP_KillSet5Scrn();
            screen = SCREEN_RETURN_TO_FIRST;
            shouldDrawScreen = 1;
        }
        else if (BUTTON_IsPressed(hBUTTON_Next))
        {
            uint8_t current_count = LIGHTS_getCount();
            if (lightsModbusSettingsMenu < current_count) {
                DSP_KillSet5Scrn();
                ++lightsModbusSettingsMenu;
                DSP_InitSet5Scrn();
            } else {
                DSP_KillSet5Scrn();
                lightsModbusSettingsMenu = 0;
                DSP_InitSet6Scrn();
                screen = SCREEN_SETTINGS_6;
            }
        }
    }

    GUI_MULTIBUF_EndEx(1);
}
/**
 * @brief Servisira šesti ekran podešavanja (Device ID, Curtain Move Time, Screensaver touch behavior, Night Timer).
 * @note  Verzija 2.0: Dodata logika za očitavanje promjene jezika.
 * Ova funkcija obrađuje postavke koje nisu grupisane u prethodnim ekranima.
 * Uključuje opcije za postavljanje podrazumevanih vrednosti i restart sistema.
 */
static void Service_SettingsScreen_6(void)
{
    /** * @brief << ISPRAVKA: Varijabla je sada lokalna za funkciju. >>
     * @note  Inicijalizovaće se na 0 samo pri prvom pozivu funkcije,
     * a nakon toga će čuvati svoju vrijednost između poziva.
     */
    static uint8_t old_language_selection = 0;
    uint8_t current_language_selection = DROPDOWN_GetSel(hDRPDN_Language);
    if (current_language_selection != old_language_selection) {
        old_language_selection = current_language_selection;
        g_display_settings.language = current_language_selection;
        settingsChanged = 1;

        DSP_KillSet6Scrn();
        DSP_InitSet6Scrn();
        return;
    }

    if(BUTTON_IsPressed(hBUTTON_SET_DEFAULTS)) {
        SetDefault();
    } else if(BUTTON_IsPressed(hBUTTON_SYSRESTART)) {
        SYSRestart();
    } else {
        // Provjera promjena na widgetima.
        if (tfifa != SPINBOX_GetValue(hDEV_ID)) {
            tfifa = SPINBOX_GetValue(hDEV_ID);
            settingsChanged = 1;
        }
        if(Curtain_GetMoveTime() != SPINBOX_GetValue(hCurtainsMoveTime)) {
            Curtain_SetMoveTime(SPINBOX_GetValue(hCurtainsMoveTime));
            settingsChanged = 1;
        }
        if(g_display_settings.leave_scrnsvr_on_release != CHECKBOX_GetState(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH)) {
            g_display_settings.leave_scrnsvr_on_release = CHECKBOX_GetState(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH);
            settingsChanged = 1;
        }
        if(g_display_settings.light_night_timer_enabled != CHECKBOX_GetState(hCHKBX_LIGHT_NIGHT_TIMER)) {
            g_display_settings.light_night_timer_enabled = CHECKBOX_GetState(hCHKBX_LIGHT_NIGHT_TIMER);
            settingsChanged = 1;
        }
    }

    if(BUTTON_IsPressed(hBUTTON_Ok)) {
        if(settingsChanged) {
            Curtains_Save();
            EE_WriteBuffer(&tfifa, EE_TFIFA, 1);
            Display_Save();
            settingsChanged = 0;
        }
        DSP_KillSet6Scrn();
        screen = SCREEN_RETURN_TO_FIRST;
    } else if (BUTTON_IsPressed(hBUTTON_Next)) {
        if(settingsChanged) {
            Curtains_Save();
            EE_WriteBuffer(&tfifa, EE_TFIFA, 1);
            Display_Save();
            settingsChanged = 0;
        }
        DSP_KillSet6Scrn();
        DSP_InitSet7Scrn();
        screen = SCREEN_SETTINGS_7;
    }
}
/**
 ******************************************************************************
 * @brief       Servisira sedmi ekran podešavanja (Scene Backend).
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva u petlji iz `DISP_Service`. Odgovorna je za:
 * 1. Detekciju promjena na widgetima (checkbox za omogućavanje scena
 * i spinbox-ovi za adrese okidača).
 * 2. Ažuriranje `g_display_settings` strukture u RAM-u kada dođe do promjene.
 * 3. Obradu pritisaka na dugmad "SAVE" i "NEXT" za snimanje
 * promjena i navigaciju.
 ******************************************************************************
 */
static void Service_SettingsScreen_7(void)
{
    // --- Detekcija promjena na widgetima ---

    // 1. Provjera Checkbox-a za omogućavanje sistema scena
    if(g_display_settings.scenes_enabled != (bool)CHECKBOX_GetState(hCHKBX_EnableScenes))
    {
        g_display_settings.scenes_enabled = (bool)CHECKBOX_GetState(hCHKBX_EnableScenes);
        settingsChanged = 1; // Signaliziraj da postoji nesnimljena promjena
    }
    
    // 2. Provjera 8 Spinbox-ova za adrese okidača
    for (uint8_t i = 0; i < SCENE_MAX_TRIGGERS; i++)
    {
        if (g_display_settings.scene_homecoming_triggers[i] != (uint16_t)SPINBOX_GetValue(hSPNBX_SceneTriggers[i]))
        {
            g_display_settings.scene_homecoming_triggers[i] = (uint16_t)SPINBOX_GetValue(hSPNBX_SceneTriggers[i]);
            settingsChanged = 1; // Signaliziraj da postoji nesnimljena promjena
        }
    }

    // --- Obrada Navigacije (Dugmad SAVE i NEXT) ---
    
    if (BUTTON_IsPressed(hBUTTON_Ok)) // Dugme "SAVE"
    {
        if (settingsChanged)
        {
            Display_Save(); // Snimi sve promjene iz g_display_settings u EEPROM
            settingsChanged = 0;
        }
        DSP_KillSet7Scrn();
        screen = SCREEN_RETURN_TO_FIRST; // Vrati se na početni ekran
    }
    else if (BUTTON_IsPressed(hBUTTON_Next)) // Dugme "NEXT"
    {
        if (settingsChanged)
        {
            Display_Save();
            settingsChanged = 0;
        }
        DSP_KillSet7Scrn();
        DSP_InitSet1Scrn(); // Vrati se na prvi ekran podešavanja (kružna navigacija)
        screen = SCREEN_SETTINGS_1;
    }
}
/**
 * @brief       Servisna funkcija za ekran podešavanja kapija.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ova funkcija upravlja kompletnom interakcijom na ekranu.
 * Prati promjenu odabrane kapije, detektuje pritiske na EDIT
 * dugmad, poziva Numpad sa specifičnim kontekstom, i obrađuje
 * vraćeni rezultat kako bi ažurirala konfiguraciju kapije u RAM-u.
 * Stvarno snimanje u EEPROM se dešava tek pritiskom na "SAVE".
 * @param       None
 * @retval      None
 */
static void Service_SettingsGateScreen(void)
{
    Gate_Handle* handle = Gate_GetInstance(settings_gate_selected_index);
    if (!handle) return; // Sigurnosna provjera

    // === 1. Obrada Rezultata sa Numpada ===
    if (g_numpad_result.is_confirmed)
    {
        int value = atoi(g_numpad_result.value);
        switch (active_gate_edit_button_id)
        {
        case ID_GATE_RELAY_OPEN:
            Gate_SetRelayOpenAddr(handle, value);
            break;
        case ID_GATE_RELAY_CLOSE:
            Gate_SetRelayCloseAddr(handle, value);
            break;
        case ID_GATE_RELAY_PED:
            Gate_SetRelayPedAddr(handle, value);
            break;
        case ID_GATE_RELAY_STOP:
            Gate_SetRelayStopAddr(handle, value);
            break;
        case ID_GATE_FEEDBACK_OPEN:
            Gate_SetFeedbackOpenAddr(handle, value);
            break;
        case ID_GATE_FEEDBACK_CLOSE:
            Gate_SetFeedbackCloseAddr(handle, value);
            break;
        case ID_GATE_CYCLE_TIMER:
            Gate_SetCycleTimer(handle, value);
            break;
        case ID_GATE_PED_TIMER:
            Gate_SetPedestrianTimer(handle, value);
            break;
        case ID_GATE_PULSE_TIMER:
            Gate_SetPulseTimer(handle, value);
            break;
        }
        settingsChanged = 1;
        g_numpad_result.is_confirmed = false; // Resetuj fleg
        shouldDrawScreen = 1; // Zatraži ponovno iscrtavanje da se vidi nova vrijednost
    }
    if (g_numpad_result.is_cancelled) {
        g_numpad_result.is_cancelled = false; // Samo resetuj fleg
    }

    // Ako je zatraženo ponovno iscrtavanje, odradi to i izađi
    if (shouldDrawScreen) {
        DSP_KillSettingsGateScreen();
        DSP_InitSettingsGateScreen();
        shouldDrawScreen = 0;
        return;
    }

    // === 2. Promjena Odabrane Kapije ===
    if (SPINBOX_GetValue(hGateSelect) != (settings_gate_selected_index + 1))
    {
        settings_gate_selected_index = SPINBOX_GetValue(hGateSelect) - 1;
        DSP_KillSettingsGateScreen();
        DSP_InitSettingsGateScreen();
        return; // Prekini, jer je ekran ponovo iscrtan
    }

    // === 3. Pritisak na "EDIT" Dugmad i Pozivanje Numpada ===
    for (int i = 0; i < 9; i++)
    {
        if (WM_IsWindow(hGateEditButtons[i]) && BUTTON_IsPressed(hGateEditButtons[i]))
        {
            BuzzerOn();
            HAL_Delay(1);
            BuzzerOff();
            int button_id = WM_GetId(hGateEditButtons[i]);
            active_gate_edit_button_id = button_id; // Sačuvaj ID pritisnutog dugmeta

            NumpadContext_t context = { .allow_decimal = false, .allow_minus_one = false };

            // Konfiguriši kontekst za Numpad na osnovu pritisnutog dugmeta
            switch (button_id)
            {
            case ID_GATE_RELAY_OPEN:
                sprintf(context.initial_value, "%d", Gate_GetRelayOpenAddr(handle));
                context.title = "Adresa Releja OTVORI";
                context.min_val = 0;
                context.max_val = 65535;
                context.max_len = 5;
                break;

            case ID_GATE_RELAY_CLOSE:
                sprintf(context.initial_value, "%d", Gate_GetRelayCloseAddr(handle));
                context.title = "Adresa Releja ZATVORI";
                context.min_val = 0;
                context.max_val = 65535;
                context.max_len = 5;
                break;

            case ID_GATE_RELAY_PED:
                sprintf(context.initial_value, "%d", Gate_GetRelayPedAddr(handle));
                context.title = "Adresa Releja PJESAK";
                context.min_val = 0;
                context.max_val = 65535;
                context.max_len = 5;
                break;

            case ID_GATE_RELAY_STOP:
                sprintf(context.initial_value, "%d", Gate_GetRelayStopAddr(handle));
                context.title = "Adresa Releja STOP";
                context.min_val = 0;
                context.max_val = 65535;
                context.max_len = 5;
                break;

            case ID_GATE_FEEDBACK_OPEN:
                sprintf(context.initial_value, "%d", Gate_GetFeedbackOpenAddr(handle));
                context.title = "Adresa Senzora OTVORENO";
                context.min_val = 0;
                context.max_val = 65535;
                context.max_len = 5;
                break;

            case ID_GATE_FEEDBACK_CLOSE:
                sprintf(context.initial_value, "%d", Gate_GetFeedbackCloseAddr(handle));
                context.title = "Adresa Senzora ZATVORENO";
                context.min_val = 0;
                context.max_val = 65535;
                context.max_len = 5;
                break;

            case ID_GATE_CYCLE_TIMER:
                sprintf(context.initial_value, "%d", Gate_GetCycleTimer(handle));
                context.title = "Vrijeme Ciklusa (s)";
                context.min_val = 0;
                context.max_val = 255;
                context.max_len = 3;
                break;

            case ID_GATE_PED_TIMER:
                sprintf(context.initial_value, "%d", Gate_GetPedestrianTimer(handle));
                context.title = "Vrijeme Pjesak (s)";
                context.min_val = 0;
                context.max_val = 255;
                context.max_len = 3;
                break;

            case ID_GATE_PULSE_TIMER:
                sprintf(context.initial_value, "%d", Gate_GetPulseTimer(handle));
                context.title = "Trajanje Impulsa (ms)";
                context.min_val = 0;
                context.max_val = 65535;
                context.max_len = 5;
                break;
            }
            Display_ShowNumpad(&context);
            return; // Izadji iz funkcije jer se ekran mijenja
        }
    }

    // === 4. Snimanje i Navigacija ===
    if (BUTTON_IsPressed(hBUTTON_Ok)) {
        if (settingsChanged) {
            Gate_Save(); // Ovdje treba implementirati Gate_Save() da snimi sve kapije
            settingsChanged = 0;
        }
        DSP_KillSettingsGateScreen();
        screen = SCREEN_RETURN_TO_FIRST;
    } else if (BUTTON_IsPressed(hBUTTON_Next)) {
        if (settingsChanged) {
            Gate_Save();
            settingsChanged = 0;
        }
        DSP_KillSettingsGateScreen();
        // Ovdje ide prelaz na SCREEN_SETTINGS_HELP
        screen = SCREEN_SETTINGS_HELP;
        shouldDrawScreen = 1;
    }
}
/**
 ******************************************************************************
 * @brief       Servisira ekran za napredno podešavanje svjetla (dimmer i RGB).
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija je centralna tačka za interakciju na ekranu za detaljna
 * podešavanja svjetla. Odgovorna je za:
 * 1.  Iscrtavanje korisničkog interfejsa, koje se dinamički prilagođava
 * u zavisnosti od toga da li je odabrano jedno ili više svjetala,
 * i da li podržavaju dimovanje ili RGB kontrolu.
 * 2.  Obradu rezultata vraćenog sa alfanumeričke tastature nakon što
 * korisnik unese novi naziv za svjetlo.
 * 3.  Kreiranje i pozicioniranje dugmeta "[ Promijeni Naziv ]" koje
 * pokreće tastaturu.
 * 4.  Obradu dodira na slajdere i paletu boja u `HandlePress_LightSettingsScreen`.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void Service_LightSettingsScreen(void)
{
    // === 1. Obrada rezultata sa tastature (nepromijenjeno) ===
    if (g_keyboard_result.is_confirmed)
    {
        if (light_selectedIndex < LIGHTS_MODBUS_SIZE)
        {
            LIGHT_Handle* handle = LIGHTS_GetInstance(light_selectedIndex);
            if (handle) {
                LIGHT_SetCustomLabel(handle, g_keyboard_result.value);
                LIGHTS_Save();
            }
        }
        g_keyboard_result.is_confirmed = false;
        shouldDrawScreen = 1;
    }
    if (g_keyboard_result.is_cancelled) {
        g_keyboard_result.is_cancelled = false;
        shouldDrawScreen = 1;
    }

    // === 2. ISCRTAVANJE EKRANA (ako je zatraženo) ===
    if(shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        DrawHamburgerMenu(1);

        const int centerX = LCD_GetXSize() / 2;
        const int centerY = LCD_GetYSize() / 2;
        const int sliderWidth = bmblackWhiteGradient.XSize;
        const int sliderHeight = bmblackWhiteGradient.YSize;
        const int sliderX0 = centerX - (sliderWidth / 2);
        const int sliderY0 = centerY - (sliderHeight / 2);
        const int WHITE_SQUARE_SIZE = 60;
        const int WHITE_SQUARE_X0 = centerX - (WHITE_SQUARE_SIZE / 2);
        const int WHITE_SQUARE_Y0 = sliderY0 - WHITE_SQUARE_SIZE - 10;
        const int paletteWidth = bmcolorSpectrum.XSize;

        bool show_dimmer_slider = false;
        bool show_rgb_palette = false;

        if (light_selectedIndex == LIGHTS_MODBUS_SIZE) {
            if (lights_allSelected_hasRGB) { show_rgb_palette = true; } 
            else { show_dimmer_slider = true; }
        } else {
            LIGHT_Handle* handle = LIGHTS_GetInstance(light_selectedIndex);
            if (handle) {
                if (LIGHT_isRGB(handle)) { show_rgb_palette = true; } 
                else if (LIGHT_isDimmer(handle)) { show_dimmer_slider = true; }
            }
        }

        if (show_rgb_palette) {
            GUI_SetColor(GUI_WHITE);
            GUI_FillRect(WHITE_SQUARE_X0, WHITE_SQUARE_Y0, WHITE_SQUARE_X0 + WHITE_SQUARE_SIZE - 1, WHITE_SQUARE_Y0 + WHITE_SQUARE_SIZE - 1);
            GUI_DrawBitmap(&bmblackWhiteGradient, sliderX0, sliderY0);
            GUI_DrawBitmap(&bmcolorSpectrum, centerX - (paletteWidth / 2), sliderY0 + sliderHeight + 20);
        } else if (show_dimmer_slider) {
            GUI_DrawBitmap(&bmblackWhiteGradient, sliderX0, sliderY0);
        }

        // === POČETAK NOVE LOGIKE ZA ISPIS NASLOVA/NAZIVA ===
        // Nema više dugmeta, samo ispis teksta
        GUI_SetFont(&GUI_FontVerdana20_LAT);
        GUI_SetColor(GUI_WHITE);
        GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_TOP);
        
        if (light_selectedIndex < LIGHTS_MODBUS_SIZE) {
            // Ako podešavamo pojedinačno svjetlo -> prikaži njegov naziv
            LIGHT_Handle* handle = LIGHTS_GetInstance(light_selectedIndex);
            if (handle) {
                const char* custom_label = LIGHT_GetCustomLabel(handle);
                
                if (custom_label[0] != '\0') {
                    // Ako postoji custom label, ispiši ga
                    GUI_DispStringAt(custom_label, 10, 10);
                } else {
                    // Ako ne, ispiši default naziv sačinjen od dva dijela
                    uint16_t selection_index = LIGHT_GetIconID(handle);
                    if (selection_index < (sizeof(icon_mapping_table) / sizeof(IconMapping_t))) {
                        const IconMapping_t* mapping = &icon_mapping_table[selection_index];
                        char default_name[40];
                        sprintf(default_name, "%s - %s", lng(mapping->primary_text_id), lng(mapping->secondary_text_id));
                        GUI_DispStringAt(default_name, 10, 10);
                    }
                }
            }
        } else {
            // Ako smo u globalnom modu -> prikaži prevedeni naslov
            GUI_DispStringAt(lng(TXT_GLOBAL_SETTINGS), 10, 10);
        }
        // === KRAJ NOVE LOGIKE ZA ISPIS NASLOVA/NAZIVA ===
        
        GUI_MULTIBUF_EndEx(1);
        return;
    }

    // === 3. OBRADA KORISNIČKOG UNOSA (Ovaj dio ćemo izmijeniti u sljedećem koraku) ===
    GUI_PID_STATE ts_state;
    GUI_PID_GetState(&ts_state);

    if (ts_state.Pressed) {
        HandlePress_LightSettingsScreen(&ts_state);
    }
}
/**
 ******************************************************************************
 * @brief       Servisira ekran za odabir izgleda scene.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija poziva Init funkciju za iscrtavanje i čeka na
 * unos korisnika, koji se obrađuje u `HandlePress_SceneAppearanceScreen`.
 ******************************************************************************
 */
static void Service_SceneAppearanceScreen(void)
{
    if (shouldDrawScreen) {
        shouldDrawScreen = 0;
        DSP_InitSceneAppearanceScreen();
    }
}
/**
 ******************************************************************************
 * @brief       Servisira glavni ekran "Čarobnjaka" za scene (SCREEN_SCENE_EDIT).
 * @author      Gemini & [Vaše Ime]
 * @note        FINALNA ISPRAVLJENA VERZIJA. Ova funkcija sada ispravno
 * upravlja navigacijom pozivajući odgovarajuće Kill i Init funkcije
 * prilikom promjene ekrana, čime se osigurava da čarobnjak radi.
 ******************************************************************************
 */
static void Service_SceneEditScreen(void)
{
    Scene_t* scene_handle = Scene_GetInstance(scene_edit_index);
    if (!scene_handle) return;

    // --- Obrada pritiska na dugme "SNIMI" / "MEMORIŠI STANJE" ---
    if (BUTTON_IsPressed(hBUTTON_Ok))
    {
        if (!(scene_handle->is_configured == false && scene_handle->appearance_id == 0))
        {
            scene_handle->is_configured = true;
            Scene_Memorize(scene_edit_index);
            Scene_Save();
            
            DSP_KillSceneEditScreen();
            screen = SCREEN_SCENE;
            shouldDrawScreen = 1;
            return;
        }
    }
    
    // --- Obrada pritiska na dugme "OTKAŽI" ---
    else if (BUTTON_IsPressed(hBUTTON_Next))
    {
        DSP_KillSceneEditScreen();
        screen = SCREEN_SCENE;
        shouldDrawScreen = 1;
        return;
    }

    // --- Obrada pritiska na dugme "PROMIJENI" (samo kod nove scene) ---
    if (WM_IsWindow(hButtonChangeAppearance) && BUTTON_IsPressed(hButtonChangeAppearance))
    {
        DSP_KillSceneEditScreen();
        DSP_InitSceneAppearanceScreen(); // Inicijalizuj sljedeći ekran
        screen = SCREEN_SCENE_APPEARANCE;
        shouldDrawScreen = 0; // Init je već iscrtao, ne treba ponovo
        return;
    }
    
    // --- Obrada pritiska na dugme "OBRIŠI" (samo kod postojeće scene) ---
    if (WM_IsWindow(hButtonDeleteScene) && BUTTON_IsPressed(hButtonDeleteScene))
    {
        memset(scene_handle, 0, sizeof(Scene_t));
        Scene_Save();

        DSP_KillSceneEditScreen();
        screen = SCREEN_SCENE;
        shouldDrawScreen = 1;
        return;
    }
    
    // --- Obrada pritiska na dugme "DETALJNA PODEŠAVANJA" ---
    if (WM_IsWindow(hButtonDetailedSetup) && BUTTON_IsPressed(hButtonDetailedSetup))
    {
        is_in_scene_wizard_mode = true; 
        DSP_KillSceneEditScreen();
        
        // Na osnovu tipa scene, pokreni prvi korak "anketnog" čarobnjaka
        switch (scene_handle->scene_type)
        {
            // TODO: Ovdje dodati case-ove za LEAVING, HOMECOMING, SLEEP
            case SCENE_TYPE_STANDARD:
            default:
                DSP_InitSceneWizDevicesScreen();
                screen = SCREEN_SCENE_WIZ_DEVICES;
                break;
        }
        
        shouldDrawScreen = 0; // Init je već iscrtao
        return;
    }
}

/**
 ******************************************************************************
 * @brief       Servisira ekran za odabir grupa uređaja u čarobnjaku.
 * @author      Gemini & [Vaše Ime]
 * @note        FINALNA VERZIJA: Ova funkcija sada sadrži kompletnu logiku
 * za ažuriranje bitmaski scene i "pametnu" navigaciju. Pritiskom na
 * dugme "[Dalje]", sistem provjerava koje su grupe uređaja odabrane
 * i vodi korisnika na sljedeći relevantan ekran.
 ******************************************************************************
 */
static void Service_SceneWizDevicesScreen(void)
{
    Scene_t* scene_handle = Scene_GetInstance(scene_edit_index);
    if (!scene_handle) return;

    // --- Ažuriranje Stanja Checkbox-ova ---
    bool lights_checked = (bool)CHECKBOX_GetState(hCheckboxSceneLights);
    bool lights_in_scene = (scene_handle->lights_mask != 0);
    if (lights_checked != lights_in_scene)
    {
        if (lights_checked) {
            uint8_t temp_mask = 0;
            for(int i=0; i < LIGHTS_MODBUS_SIZE; i++) {
                LIGHT_Handle* l_handle = LIGHTS_GetInstance(i);
                if (l_handle && LIGHT_GetRelay(l_handle) != 0) {
                    temp_mask |= (1 << i);
                }
            }
            scene_handle->lights_mask = temp_mask;
        } else {
            scene_handle->lights_mask = 0;
        }
    }

    bool curtains_checked = (bool)CHECKBOX_GetState(hCheckboxSceneCurtains);
    bool curtains_in_scene = (scene_handle->curtains_mask != 0);
    if (curtains_checked != curtains_in_scene)
    {
        if (curtains_checked) {
            uint16_t temp_mask = 0;
            for(int i=0; i < CURTAINS_SIZE; i++) {
                Curtain_Handle* c_handle = Curtain_GetInstanceByIndex(i);
                if (c_handle && Curtain_hasRelays(c_handle)) {
                    temp_mask |= (1 << i);
                }
            }
            scene_handle->curtains_mask = temp_mask;
        } else {
            scene_handle->curtains_mask = 0;
        }
    }

    bool thermostat_checked = (bool)CHECKBOX_GetState(hCheckboxSceneThermostat);
    bool thermostat_in_scene = (scene_handle->thermostat_mask != 0);
    if (thermostat_checked != thermostat_in_scene)
    {
        scene_handle->thermostat_mask = thermostat_checked ? 1 : 0;
    }

    // --- Obrada Navigacionih Dugmića ---
    if (BUTTON_IsPressed(hButtonWizCancel))
    {
        is_in_scene_wizard_mode = false; // Resetuj fleg
        DSP_KillSceneWizDevicesScreen();
        screen = SCREEN_SCENE;
        shouldDrawScreen = 1;
    }
    else if (BUTTON_IsPressed(hButtonWizBack))
    {
        DSP_KillSceneWizDevicesScreen();
        DSP_InitSceneEditScreen();
        screen = SCREEN_SCENE_EDIT;
        shouldDrawScreen = 0;
    }
    else if (BUTTON_IsPressed(hButtonWizNext))
    {
        DSP_KillSceneWizDevicesScreen();
        
        // "Pametna" navigacija na osnovu tipa scene i odabira
        switch (scene_handle->scene_type)
        {
            // Sistemske scene imaju svoj, fiksni tok
            case SCENE_TYPE_LEAVING:
                screen = SCREEN_SCENE_WIZ_LEAVING;
                // TODO: DSP_InitSceneWizLeavingScreen();
                break;
            case SCENE_TYPE_HOMECOMING:
                screen = SCREEN_SCENE_WIZ_HOMECOMING;
                // TODO: DSP_InitSceneWizHomecomingScreen();
                break;
            case SCENE_TYPE_SLEEP:
                 screen = SCREEN_SCENE_WIZ_SLEEP;
                 // TODO: DSP_InitSceneWizSleepScreen();
                 break;
            
            // "Komfor" scene idu na prvi odabrani uređaj
            case SCENE_TYPE_STANDARD:
            default:
                if (scene_handle->lights_mask) {
                    screen = SCREEN_LIGHTS;
                } else if (scene_handle->curtains_mask) {
                    screen = SCREEN_CURTAINS;
                } else if (scene_handle->thermostat_mask) {
                    screen = SCREEN_THERMOSTAT;
                } else {
                    // Ako ništa nije odabrano, idi na finalni ekran
                    DSP_InitSceneWizFinalizeScreen();
                    screen = SCREEN_SCENE_WIZ_FINALIZE;
                    shouldDrawScreen = 0;
                    return;
                }
                break;
        }
        
        shouldDrawScreen = 1;
    }
}
/**
 * @brief Servisira ekran za resetovanje glavnih prekidača/menija.
 * @note Ova funkcija prvenstveno prikazuje odbrojavanje ako je aktivan
 * Light Night Timer. Koristi novi API za provjeru statusa i preostalog vremena.
 */
static void Service_MainScreenSwitch(void)
{
    // =======================================================================
    // === POČETAK REFAKTORISANJA ===

    // KORAK 1: Umjesto direktne provjere varijable, pozivamo API funkciju.
    if(LIGHTS_IsNightTimerActive()) {
        GUI_MULTIBUF_BeginEx(1);

        // KORAK 2: Umjesto ručnog računanja, pozivamo API funkciju koja vraća preostalo vrijeme.
        const uint8_t dispTime = LIGHTS_GetNightTimerCountdown();

        // Logika iscrtavanja ostaje ista.
        GUI_SetColor(GUI_WHITE);
        GUI_SetFont(GUI_FONT_D32);
        GUI_SetTextMode(GUI_TM_TRANS);
        GUI_SetTextAlign(GUI_TA_HCENTER|GUI_TA_VCENTER);
        GUI_ClearRect(220, 116, 265, 156);
        GUI_DispDecAt(dispTime + 1, 240, 136, 2);

        GUI_MULTIBUF_EndEx(1);
    }

    // === KRAJ REFAKTORISANJA ===
    // =======================================================================
}

/**
 ******************************************************************************
 * @brief       Servisira finalni ekran čarobnjaka.
 ******************************************************************************
 */
static void Service_SceneWizFinalizeScreen(void)
{
    if (BUTTON_IsPressed(hBUTTON_Ok)) // "Snimi Scenu"
    {
        Scene_Save();
        is_in_scene_wizard_mode = false;
        DSP_KillSceneWizFinalizeScreen();
        screen = SCREEN_SCENE;
        shouldDrawScreen = 1;
    }
    else if (BUTTON_IsPressed(hButtonWizCancel))
    {
        // Ne snimaj promjene u RAM-u, samo izađi
        is_in_scene_wizard_mode = false;
        DSP_KillSceneWizFinalizeScreen();
        screen = SCREEN_SCENE;
        shouldDrawScreen = 1;
    }
}
/**
 * @brief Prikazuje datum i vrijeme na ekranu, i upravlja logikom screensavera.
 * @note Ažurira se svake sekunde i odgovorna je za aktivaciju/deaktivaciju
 * screensavera na osnovu postavljenih sati. Ova refaktorirana verzija koristi
 * GUI funkcije LCD_GetXSize() i LCD_GetYSize() za dinamičko određivanje
 * dimenzija ekrana, čime se uklanjaju fiksne numeričke vrijednosti ("magični brojevi")
 * i poboljšava prenosivost koda.
 */
static void DISPDateTime(void)
{
    /**
     * @brief Konstante za dimenzioniranje područja za brisanje
     * @note Ove konstante su sada definirane u odnosu na GUI.
     */
    const int16_t TIME_CLEAR_RECT_WIDTH = 100;
    const int16_t TIME_CLEAR_RECT_HEIGHT = 50;

    // Konstante za brisanje screensaver ekrana
    const int16_t SCREENSAVER_TIME_Y_START = 80;
    const int16_t SCREENSAVER_TIME_Y_END = 192;
    const int16_t SCREENSAVER_DATE_Y_START = 220;
    const int16_t SCREENSAVER_DATE_Y_END = 270;


    char dbuf[64];
    static uint8_t old_day = 0;

    if (!IsRtcTimeValid()) return;

    HAL_RTC_GetTime(&hrtc, &rtctm, RTC_FORMAT_BCD);
    HAL_RTC_GetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);

    // Logika za automatsko paljenje/gašenje screensaver-a
    if (g_display_settings.scrnsvr_ena_hour >= g_display_settings.scrnsvr_dis_hour) {
        if (Bcd2Dec(rtctm.Hours) >= g_display_settings.scrnsvr_ena_hour || Bcd2Dec(rtctm.Hours) < g_display_settings.scrnsvr_dis_hour) {
            ScrnsvrEnable();
        } else if (IsScrnsvrEnabled()) {
            ScrnsvrDisable();
            screen = SCREEN_RETURN_TO_FIRST;
        }
    } else if (g_display_settings.scrnsvr_ena_hour < g_display_settings.scrnsvr_dis_hour) {
        if (Bcd2Dec(rtctm.Hours) >= g_display_settings.scrnsvr_ena_hour && Bcd2Dec(rtctm.Hours) < g_display_settings.scrnsvr_dis_hour) {
            ScrnsvrEnable();
        } else if (IsScrnsvrEnabled()) {
            ScrnsvrDisable();
            screen = SCREEN_RETURN_TO_FIRST;
        }
    }

    if (IsScrnsvrActiv() && IsScrnsvrEnabled() && IsScrnsvrClkActiv()) {
        if (!IsScrnsvrInitActiv() || (old_day != rtcdt.WeekDay)) {
            ScrnsvrInitSet();
            GUI_MULTIBUF_BeginEx(0);
            GUI_SelectLayer(0);
            GUI_Clear();
            GUI_MULTIBUF_EndEx(0);
            GUI_MULTIBUF_BeginEx(1);
            GUI_SelectLayer(1);
            GUI_SetBkColor(GUI_TRANSPARENT);
            GUI_Clear();
            old_min = 60U;
            old_day = rtcdt.WeekDay;
            GUI_MULTIBUF_EndEx(1);
        }

        GUI_MULTIBUF_BeginEx(1);

        // POPRAVAK: Brisanje ekrana za screensaver bez magičnih brojeva
        GUI_ClearRect(0, SCREENSAVER_TIME_Y_START, LCD_GetXSize(), SCREENSAVER_TIME_Y_END);
        GUI_ClearRect(0, SCREENSAVER_DATE_Y_START, TIME_CLEAR_RECT_WIDTH, SCREENSAVER_DATE_Y_END);

        HEX2STR(dbuf, &rtctm.Hours);
        if (rtctm.Seconds & 1) dbuf[2] = ':';
        else dbuf[2] = ' ';
        HEX2STR(&dbuf[3], &rtctm.Minutes);

        GUI_SetColor(clk_clrs[g_display_settings.scrnsvr_clk_clr]);
        GUI_SetFont(GUI_FONT_D80);
        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt(dbuf, main_screen_layout.time_pos_scrnsvr.x, main_screen_layout.time_pos_scrnsvr.y);

        /**
         * @brief Niz sa TextID-jevima za dane u sedmici.
         * @note Koristi se za dinamičko prevođenje imena dana na screensaver-u.
         */
        const TextID days[] = {TXT_MONDAY, TXT_TUESDAY, TXT_WEDNESDAY, TXT_THURSDAY, TXT_FRIDAY, TXT_SATURDAY, TXT_SUNDAY};

        /**
         * @brief Niz sa TextID-jevima za mjesece u godini.
         * @note Koristi se za dinamičko prevođenje imena mjeseci na screensaver-u.
         */
        const TextID months[] = {TXT_MONTH_JAN, TXT_MONTH_FEB, TXT_MONTH_MAR, TXT_MONTH_APR, TXT_MONTH_MAY, TXT_MONTH_JUN,
                                 TXT_MONTH_JUL, TXT_MONTH_AUG, TXT_MONTH_SEP, TXT_MONTH_OCT, TXT_MONTH_NOV, TXT_MONTH_DEC
                                };

        sprintf(dbuf, "%s, %02d. %s %d",
                lng(days[Bcd2Dec(rtcdt.WeekDay) - 1]),
                Bcd2Dec(rtcdt.Date),
                lng(months[Bcd2Dec(rtcdt.Month) - 1]),
                Bcd2Dec(rtcdt.Year) + 2000);

        GUI_SetFont(&GUI_FontVerdana32_LAT);
        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt(dbuf, main_screen_layout.date_pos_scrnsvr.x, main_screen_layout.date_pos_scrnsvr.y);

        GUI_MULTIBUF_EndEx(1);

    }

    if (old_day != rtcdt.WeekDay) {
        old_day = rtcdt.WeekDay;
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, rtcdt.Date);
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR3, rtcdt.Month);
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR4, rtcdt.WeekDay);
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR5, rtcdt.Year);
    }
}

/**
 * @brief Otkriva i obrađuje dugi pritisak za ulazak u meni za podešavanja.
 * @param btn Fleg koji ukazuje na početak pritiska (postavljen u PID_Hook).
 * @return uint8_t 1 ako je dugi pritisak detektovan, inače 0.
 */
static uint8_t DISPMenuSettings(uint8_t btn)
{
    static uint8_t last_state = 0U;
    static uint32_t menu_tmr = 0U;

    if ((btn == 1U) && (last_state == 0U)) {
        // Početak pritiska, postavi fleg i pokreni tajmer.
        last_state = 1U;
        menu_tmr = HAL_GetTick();
    } else if ((btn == 1U) && (last_state == 1U)) {
        // Pritisak traje, provjeri da li je dovoljno dugo.
        if((HAL_GetTick() - menu_tmr) >= SETTINGS_MENU_ENABLE_TIME) {
            last_state = 0U; // Resetuj stanje nakon detekcije.
            return (1U); // Uspješan dugi pritisak.
        }
    } else if ((btn == 0U) && (last_state == 1U)) {
        // Pritisak je prekinut prije isteka vremena.
        last_state = 0U;
    }

    return (0U); // Dugi pritisak nije detektovan.
}
/**
 * @brief Inicijalizuje prvi ekran podešavanja (kontrola termostata i ventilatora).
 * @note  Ova funkcija kreira sve potrebne GUI widgete, kao što su RADIO
 * i SPINBOX kontrole, te dugmad "NEXT" i "SAVE" za navigaciju i čuvanje.
 * Vrijednosti se inicijalizuju na osnovu trenutnih postavki.
 * Sav raspored elemenata je definisan u `settings_screen_1_layout` strukturi.
 */
static void DSP_InitSet1Scrn(void)
{
    /** @brief Dobijamo handle za termostat kako bismo pročitali trenutne postavke. */
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();

    /**
     * @brief Inicijalizacija iscrtavanja.
     * @note  Pokreće se višestruko baferovanje i čiste se oba grafička sloja.
     */
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    /**
     * @brief Kreiranje widgeta za kontrolu termostata.
     * @note  Pozicije, dimenzije i početne vrijednosti se uzimaju iz layout strukture
     * i API funkcija termostat modula.
     */
    hThstControl = RADIO_CreateEx(settings_screen_1_layout.thst_control_pos.x, settings_screen_1_layout.thst_control_pos.y, settings_screen_1_layout.thst_control_pos.w, settings_screen_1_layout.thst_control_pos.h, 0, WM_CF_SHOW, 0, ID_ThstControl, 3, 20);
    RADIO_SetTextColor(hThstControl, GUI_GREEN);
    RADIO_SetText(hThstControl, "OFF", 0);
    RADIO_SetText(hThstControl, "COOLING", 1);
    RADIO_SetText(hThstControl, "HEATING", 2);
    RADIO_SetValue(hThstControl, Thermostat_GetControlMode(pThst));

    hThstMaxSetPoint = SPINBOX_CreateEx(settings_screen_1_layout.thst_max_sp_pos.x, settings_screen_1_layout.thst_max_sp_pos.y, settings_screen_1_layout.thst_max_sp_pos.w, settings_screen_1_layout.thst_max_sp_pos.h, 0, WM_CF_SHOW, ID_MaxSetpoint, THST_SP_MIN, THST_SP_MAX);
    SPINBOX_SetEdge(hThstMaxSetPoint, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstMaxSetPoint, Thermostat_Get_SP_Max(pThst));

    hThstMinSetPoint = SPINBOX_CreateEx(settings_screen_1_layout.thst_min_sp_pos.x, settings_screen_1_layout.thst_min_sp_pos.y, settings_screen_1_layout.thst_min_sp_pos.w, settings_screen_1_layout.thst_min_sp_pos.h, 0, WM_CF_SHOW, ID_MinSetpoint, THST_SP_MIN, THST_SP_MAX);
    SPINBOX_SetEdge(hThstMinSetPoint, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstMinSetPoint, Thermostat_Get_SP_Min(pThst));

    /**
     * @brief Kreiranje widgeta za kontrolu ventilatora.
     */
    hFanControl = RADIO_CreateEx(settings_screen_1_layout.fan_control_pos.x, settings_screen_1_layout.fan_control_pos.y, settings_screen_1_layout.fan_control_pos.w, settings_screen_1_layout.fan_control_pos.h, 0, WM_CF_SHOW, 0, ID_FanControl, 2, 20);
    RADIO_SetTextColor(hFanControl, GUI_GREEN);
    RADIO_SetText(hFanControl, "ON / OFF", 0);
    RADIO_SetText(hFanControl, "3 SPEED", 1);
    RADIO_SetValue(hFanControl, Thermostat_GetFanControlMode(pThst));

    hFanDiff = SPINBOX_CreateEx(settings_screen_1_layout.fan_diff_pos.x, settings_screen_1_layout.fan_diff_pos.y, settings_screen_1_layout.fan_diff_pos.w, settings_screen_1_layout.fan_diff_pos.h, 0, WM_CF_SHOW, ID_FanDiff, 0, 10);
    SPINBOX_SetEdge(hFanDiff, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hFanDiff, Thermostat_GetFanDifference(pThst));

    hFanLowBand = SPINBOX_CreateEx(settings_screen_1_layout.fan_low_band_pos.x, settings_screen_1_layout.fan_low_band_pos.y, settings_screen_1_layout.fan_low_band_pos.w, settings_screen_1_layout.fan_low_band_pos.h, 0, WM_CF_SHOW, ID_FanLowBand, 0, 50);
    SPINBOX_SetEdge(hFanLowBand, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hFanLowBand, Thermostat_GetFanLowBand(pThst));

    hFanHiBand = SPINBOX_CreateEx(settings_screen_1_layout.fan_hi_band_pos.x, settings_screen_1_layout.fan_hi_band_pos.y, settings_screen_1_layout.fan_hi_band_pos.w, settings_screen_1_layout.fan_hi_band_pos.h, 0, WM_CF_SHOW, ID_FanHiBand, 0, 100);
    SPINBOX_SetEdge(hFanHiBand, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hFanHiBand, Thermostat_GetFanHighBand(pThst));

    /**
     * @brief Kreiranje widgeta za grupni rad termostata.
     */
    hThstGroup = SPINBOX_CreateEx(settings_screen_1_layout.thst_group_pos.x, settings_screen_1_layout.thst_group_pos.y, settings_screen_1_layout.thst_group_pos.w, settings_screen_1_layout.thst_group_pos.h, 0, WM_CF_SHOW, ID_THST_GROUP, 0, 254);
    SPINBOX_SetEdge(hThstGroup, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstGroup, Thermostat_GetGroup(pThst));

    // << ISPRAVKA: Dodat je 7. argument (ExFlags) sa vrijednošću 0. >>
    hThstMaster = CHECKBOX_CreateEx(settings_screen_1_layout.thst_master_pos.x, settings_screen_1_layout.thst_master_pos.y, settings_screen_1_layout.thst_master_pos.w, settings_screen_1_layout.thst_master_pos.h, 0, WM_CF_SHOW, 0, ID_THST_MASTER);
    CHECKBOX_SetTextColor(hThstMaster, GUI_GREEN);
    CHECKBOX_SetText(hThstMaster, "Master");
    CHECKBOX_SetState(hThstMaster, Thermostat_IsMaster(pThst));

    /**
     * @brief Kreiranje navigacionih dugmadi.
     */
    hBUTTON_Next = BUTTON_CreateEx(settings_screen_1_layout.next_button_pos.x, settings_screen_1_layout.next_button_pos.y, settings_screen_1_layout.next_button_pos.w, settings_screen_1_layout.next_button_pos.h, 0, WM_CF_SHOW, 0, ID_Next);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_CreateEx(settings_screen_1_layout.save_button_pos.x, settings_screen_1_layout.save_button_pos.y, settings_screen_1_layout.save_button_pos.w, settings_screen_1_layout.save_button_pos.h, 0, WM_CF_SHOW, 0, ID_Ok);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    /**
     * @brief Iscrtavanje tekstualnih labela i linija.
     * @note  Sve pozicije se dobijaju iz `settings_screen_1_layout` strukture.
     */
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);

    GUI_GotoXY(settings_screen_1_layout.label_thst_max_sp[0].x, settings_screen_1_layout.label_thst_max_sp[0].y);
    GUI_DispString("MAX. USER SETPOINT");
    GUI_GotoXY(settings_screen_1_layout.label_thst_max_sp[1].x, settings_screen_1_layout.label_thst_max_sp[1].y);
    GUI_DispString("TEMP. x1*C");

    GUI_GotoXY(settings_screen_1_layout.label_thst_min_sp[0].x, settings_screen_1_layout.label_thst_min_sp[0].y);
    GUI_DispString("MIN. USER SETPOINT");
    GUI_GotoXY(settings_screen_1_layout.label_thst_min_sp[1].x, settings_screen_1_layout.label_thst_min_sp[1].y);
    GUI_DispString("TEMP. x1*C");

    GUI_GotoXY(settings_screen_1_layout.label_fan_diff[0].x, settings_screen_1_layout.label_fan_diff[0].y);
    GUI_DispString("FAN SPEED DIFFERENCE");
    GUI_GotoXY(settings_screen_1_layout.label_fan_diff[1].x, settings_screen_1_layout.label_fan_diff[1].y);
    GUI_DispString("TEMP. x0.1*C");

    GUI_GotoXY(settings_screen_1_layout.label_fan_low[0].x, settings_screen_1_layout.label_fan_low[0].y);
    GUI_DispString("FAN LOW SPEED BAND");
    GUI_GotoXY(settings_screen_1_layout.label_fan_low[1].x, settings_screen_1_layout.label_fan_low[1].y);
    GUI_DispString("SETPOINT +/- x0.1*C");

    GUI_GotoXY(settings_screen_1_layout.label_fan_hi[0].x, settings_screen_1_layout.label_fan_hi[0].y);
    GUI_DispString("FAN HI SPEED BAND");
    GUI_GotoXY(settings_screen_1_layout.label_fan_hi[1].x, settings_screen_1_layout.label_fan_hi[1].y);
    GUI_DispString("SETPOINT +/- x0.1*C");

    GUI_GotoXY(settings_screen_1_layout.label_thst_ctrl_title.x, settings_screen_1_layout.label_thst_ctrl_title.y);
    GUI_DispString("THERMOSTAT CONTROL MODE");

    GUI_GotoXY(settings_screen_1_layout.label_fan_ctrl_title.x, settings_screen_1_layout.label_fan_ctrl_title.y);
    GUI_DispString("FAN SPEED CONTROL MODE");

    GUI_GotoXY(settings_screen_1_layout.label_thst_group.x, settings_screen_1_layout.label_thst_group.y);
    GUI_DispString("GROUP");

    GUI_DrawHLine(12, 5, 320);
    GUI_DrawHLine(130, 5, 320);

    GUI_MULTIBUF_EndEx(1);
}

/**
 * @brief Briše GUI widgete sa prvog ekrana podešavanja.
 * @note Ova funkcija se poziva pre prelaska na novi ekran kako bi se oslobodila
 * memorija i spriječili konflikti sa novim widgetima.
 */
static void DSP_KillSet1Scrn(void)
{
    WM_DeleteWindow(hThstControl);
    WM_DeleteWindow(hFanControl);
    WM_DeleteWindow(hThstMaxSetPoint);
    WM_DeleteWindow(hThstMinSetPoint);
    WM_DeleteWindow(hFanDiff);
    WM_DeleteWindow(hFanLowBand);
    WM_DeleteWindow(hFanHiBand);
    WM_DeleteWindow(hThstGroup);
    WM_DeleteWindow(hThstMaster);
    WM_DeleteWindow(hBUTTON_Ok);
    WM_DeleteWindow(hBUTTON_Next);
}

/**
 * @brief Inicijalizuje drugi ekran podešavanja (vrijeme, datum, screensaver).
 * @note  Verzija 2.2: Potpuno refaktorisana, bez magičnih brojeva, sa kompletnim
 * kodom za iscrtavanje i ispravnim pozivima CreateEx funkcija.
 * Kreira sve GUI widgete i iscrtava statičke elemente (labele, linije)
 * koristeći isključivo konstante iz `settings_screen_2_layout` strukture.
 */
static void DSP_InitSet2Scrn(void)
{
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    /** @brief Učitavanje trenutnog vremena i datuma sa RTC-a za inicijalizaciju widgeta. */
    HAL_RTC_GetTime(&hrtc, &rtctm, RTC_FORMAT_BCD);
    HAL_RTC_GetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);

    /** @brief Kreiranje svih widgeta koristeći pozicije iz `settings_screen_2_layout`. */
    hSPNBX_DisplayHighBrightness = SPINBOX_CreateEx(settings_screen_2_layout.high_brightness_pos.x, settings_screen_2_layout.high_brightness_pos.y, settings_screen_2_layout.high_brightness_pos.w, settings_screen_2_layout.high_brightness_pos.h, 0, WM_CF_SHOW, ID_DisplayHighBrightness, 1, 90);
    SPINBOX_SetEdge(hSPNBX_DisplayHighBrightness, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_DisplayHighBrightness, g_display_settings.high_bcklght);

    hSPNBX_DisplayLowBrightness = SPINBOX_CreateEx(settings_screen_2_layout.low_brightness_pos.x, settings_screen_2_layout.low_brightness_pos.y, settings_screen_2_layout.low_brightness_pos.w, settings_screen_2_layout.low_brightness_pos.h, 0, WM_CF_SHOW, ID_DisplayLowBrightness, 1, 90);
    SPINBOX_SetEdge(hSPNBX_DisplayLowBrightness, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_DisplayLowBrightness, g_display_settings.low_bcklght);

    hSPNBX_ScrnsvrTimeout = SPINBOX_CreateEx(settings_screen_2_layout.scrnsvr_timeout_pos.x, settings_screen_2_layout.scrnsvr_timeout_pos.y, settings_screen_2_layout.scrnsvr_timeout_pos.w, settings_screen_2_layout.scrnsvr_timeout_pos.h, 0, WM_CF_SHOW, ID_ScrnsvrTimeout, 1, 240);
    SPINBOX_SetEdge(hSPNBX_ScrnsvrTimeout, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_ScrnsvrTimeout, g_display_settings.scrnsvr_tout);

    hSPNBX_ScrnsvrEnableHour = SPINBOX_CreateEx(settings_screen_2_layout.scrnsvr_enable_hour_pos.x, settings_screen_2_layout.scrnsvr_enable_hour_pos.y, settings_screen_2_layout.scrnsvr_enable_hour_pos.w, settings_screen_2_layout.scrnsvr_enable_hour_pos.h, 0, WM_CF_SHOW, ID_ScrnsvrEnableHour, 0, 23);
    SPINBOX_SetEdge(hSPNBX_ScrnsvrEnableHour, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_ScrnsvrEnableHour, g_display_settings.scrnsvr_ena_hour);

    hSPNBX_ScrnsvrDisableHour = SPINBOX_CreateEx(settings_screen_2_layout.scrnsvr_disable_hour_pos.x, settings_screen_2_layout.scrnsvr_disable_hour_pos.y, settings_screen_2_layout.scrnsvr_disable_hour_pos.w, settings_screen_2_layout.scrnsvr_disable_hour_pos.h, 0, WM_CF_SHOW, ID_ScrnsvrDisableHour, 0, 23);
    SPINBOX_SetEdge(hSPNBX_ScrnsvrDisableHour, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_ScrnsvrDisableHour, g_display_settings.scrnsvr_dis_hour);

    hSPNBX_Hour = SPINBOX_CreateEx(settings_screen_2_layout.hour_pos.x, settings_screen_2_layout.hour_pos.y, settings_screen_2_layout.hour_pos.w, settings_screen_2_layout.hour_pos.h, 0, WM_CF_SHOW, ID_Hour, 0, 23);
    SPINBOX_SetEdge(hSPNBX_Hour, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_Hour, Bcd2Dec(rtctm.Hours));

    hSPNBX_Minute = SPINBOX_CreateEx(settings_screen_2_layout.minute_pos.x, settings_screen_2_layout.minute_pos.y, settings_screen_2_layout.minute_pos.w, settings_screen_2_layout.minute_pos.h, 0, WM_CF_SHOW, ID_Minute, 0, 59);
    SPINBOX_SetEdge(hSPNBX_Minute, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_Minute, Bcd2Dec(rtctm.Minutes));

    hSPNBX_Day = SPINBOX_CreateEx(settings_screen_2_layout.day_pos.x, settings_screen_2_layout.day_pos.y, settings_screen_2_layout.day_pos.w, settings_screen_2_layout.day_pos.h, 0, WM_CF_SHOW, ID_Day, 1, 31);
    SPINBOX_SetEdge(hSPNBX_Day, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_Day, Bcd2Dec(rtcdt.Date));

    hSPNBX_Month = SPINBOX_CreateEx(settings_screen_2_layout.month_pos.x, settings_screen_2_layout.month_pos.y, settings_screen_2_layout.month_pos.w, settings_screen_2_layout.month_pos.h, 0, WM_CF_SHOW, ID_Month, 1, 12);
    SPINBOX_SetEdge(hSPNBX_Month, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_Month, Bcd2Dec(rtcdt.Month));

    hSPNBX_Year = SPINBOX_CreateEx(settings_screen_2_layout.year_pos.x, settings_screen_2_layout.year_pos.y, settings_screen_2_layout.year_pos.w, settings_screen_2_layout.year_pos.h, 0, WM_CF_SHOW, ID_Year, 2000, 2099);
    SPINBOX_SetEdge(hSPNBX_Year, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_Year, (Bcd2Dec(rtcdt.Year) + 2000));

    hSPNBX_ScrnsvrClockColour = SPINBOX_CreateEx(settings_screen_2_layout.scrnsvr_color_pos.x, settings_screen_2_layout.scrnsvr_color_pos.y, settings_screen_2_layout.scrnsvr_color_pos.w, settings_screen_2_layout.scrnsvr_color_pos.h, 0, WM_CF_SHOW, ID_ScrnsvrClkColour, 1, COLOR_BSIZE);
    SPINBOX_SetEdge(hSPNBX_ScrnsvrClockColour, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_ScrnsvrClockColour, g_display_settings.scrnsvr_clk_clr);

    hCHKBX_ScrnsvrClock = CHECKBOX_CreateEx(settings_screen_2_layout.scrnsvr_checkbox_pos.x, settings_screen_2_layout.scrnsvr_checkbox_pos.y, settings_screen_2_layout.scrnsvr_checkbox_pos.w, settings_screen_2_layout.scrnsvr_checkbox_pos.h, 0, WM_CF_SHOW, 0, ID_ScrnsvrClock);
    CHECKBOX_SetTextColor(hCHKBX_ScrnsvrClock, GUI_GREEN);
    CHECKBOX_SetText(hCHKBX_ScrnsvrClock, "SCREENSAVER");
    // << ISPRAVKA 3: Inicijalizacija se sada vrši iz EEPROM strukture `g_display_settings` >>
    CHECKBOX_SetState(hCHKBX_ScrnsvrClock, g_display_settings.scrnsvr_on_off);

    hDRPDN_WeekDay = DROPDOWN_CreateEx(settings_screen_2_layout.weekday_dropdown_pos.x, settings_screen_2_layout.weekday_dropdown_pos.y, settings_screen_2_layout.weekday_dropdown_pos.w, settings_screen_2_layout.weekday_dropdown_pos.h, 0, WM_CF_SHOW, DROPDOWN_CF_AUTOSCROLLBAR, ID_WeekDay);
    /** * @brief << ISPRAVKA: Logika za popunjavanje dropdown menija za dane u sedmici. >>
     * @note  Petlja sada ispravno iterira 7 puta (za 7 dana) i dodaje stringove
     * za trenutno odabrani jezik (`g_display_settings.language`).
     */
    for (int i = 0; i < 7; i++) {
        DROPDOWN_AddString(hDRPDN_WeekDay, _acContent[g_display_settings.language][i]);
    }
    DROPDOWN_SetSel(hDRPDN_WeekDay, rtcdt.WeekDay - 1);

    hBUTTON_Next = BUTTON_CreateEx(settings_screen_2_layout.next_button_pos.x, settings_screen_2_layout.next_button_pos.y, settings_screen_2_layout.next_button_pos.w, settings_screen_2_layout.next_button_pos.h, 0, WM_CF_SHOW, 0, ID_Next);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_CreateEx(settings_screen_2_layout.save_button_pos.x, settings_screen_2_layout.save_button_pos.y, settings_screen_2_layout.save_button_pos.w, settings_screen_2_layout.save_button_pos.h, 0, WM_CF_SHOW, 0, ID_Ok);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    /** @brief Iscrtavanje labela, linija i pregleda boje, koristeći pozicije iz layout strukture. */
    GUI_SetColor(clk_clrs[g_display_settings.scrnsvr_clk_clr]);
    GUI_FillRect(settings_screen_2_layout.scrnsvr_color_preview_rect.x0, settings_screen_2_layout.scrnsvr_color_preview_rect.y0,
                 settings_screen_2_layout.scrnsvr_color_preview_rect.x1, settings_screen_2_layout.scrnsvr_color_preview_rect.y1);

    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);

    GUI_DrawHLine(settings_screen_2_layout.line1.y, settings_screen_2_layout.line1.x0, settings_screen_2_layout.line1.x1);
    GUI_GotoXY(settings_screen_2_layout.label_backlight_title.x, settings_screen_2_layout.label_backlight_title.y);
    GUI_DispString("DISPLAY BACKLIGHT");
    GUI_GotoXY(settings_screen_2_layout.label_high_brightness.x, settings_screen_2_layout.label_high_brightness.y);
    GUI_DispString("HIGH");
    GUI_GotoXY(settings_screen_2_layout.label_low_brightness.x, settings_screen_2_layout.label_low_brightness.y);
    GUI_DispString("LOW");

    GUI_DrawHLine(settings_screen_2_layout.line2.y, settings_screen_2_layout.line2.x0, settings_screen_2_layout.line2.x1);
    GUI_GotoXY(settings_screen_2_layout.label_time_title.x, settings_screen_2_layout.label_time_title.y);
    GUI_DispString("SET TIME");
    GUI_GotoXY(settings_screen_2_layout.label_hour.x, settings_screen_2_layout.label_hour.y);
    GUI_DispString("HOUR");
    GUI_GotoXY(settings_screen_2_layout.label_minute.x, settings_screen_2_layout.label_minute.y);
    GUI_DispString("MINUTE");

    GUI_DrawHLine(settings_screen_2_layout.line3.y, settings_screen_2_layout.line3.x0, settings_screen_2_layout.line3.x1);
    GUI_GotoXY(settings_screen_2_layout.label_color_title.x, settings_screen_2_layout.label_color_title.y);
    GUI_DispString("SET COLOR");
    GUI_GotoXY(settings_screen_2_layout.label_full_color.x, settings_screen_2_layout.label_full_color.y);
    GUI_DispString("FULL");
    GUI_GotoXY(settings_screen_2_layout.label_clock_color.x, settings_screen_2_layout.label_clock_color.y);
    GUI_DispString("CLOCK");

    GUI_DrawHLine(settings_screen_2_layout.line4.y, settings_screen_2_layout.line4.x0, settings_screen_2_layout.line4.x1);
    GUI_GotoXY(settings_screen_2_layout.label_scrnsvr_title.x, settings_screen_2_layout.label_scrnsvr_title.y);
    GUI_DispString("SCREENSAVER OPTION");
    GUI_GotoXY(settings_screen_2_layout.label_timeout.x, settings_screen_2_layout.label_timeout.y);
    GUI_DispString("TIMEOUT");
    GUI_GotoXY(settings_screen_2_layout.label_enable_hour[0].x, settings_screen_2_layout.label_enable_hour[0].y);
    GUI_DispString("ENABLE");
    GUI_GotoXY(settings_screen_2_layout.label_enable_hour[1].x, settings_screen_2_layout.label_enable_hour[1].y);
    GUI_DispString("HOUR");
    GUI_GotoXY(settings_screen_2_layout.label_disable_hour[0].x, settings_screen_2_layout.label_disable_hour[0].y);
    GUI_DispString("DISABLE");
    GUI_GotoXY(settings_screen_2_layout.label_disable_hour[1].x, settings_screen_2_layout.label_disable_hour[1].y);
    GUI_DispString("HOUR");

    GUI_DrawHLine(settings_screen_2_layout.line5.y, settings_screen_2_layout.line5.x0, settings_screen_2_layout.line5.x1);
    GUI_GotoXY(settings_screen_2_layout.label_date_title.x, settings_screen_2_layout.label_date_title.y);
    GUI_DispString("SET DATE");
    GUI_GotoXY(settings_screen_2_layout.label_day.x, settings_screen_2_layout.label_day.y);
    GUI_DispString("DAY");
    GUI_GotoXY(settings_screen_2_layout.label_month.x, settings_screen_2_layout.label_month.y);
    GUI_DispString("MONTH");
    GUI_GotoXY(settings_screen_2_layout.label_year.x, settings_screen_2_layout.label_year.y);
    GUI_DispString("YEAR");

    GUI_MULTIBUF_EndEx(1);
}
/**
 * @brief Briše GUI widgete sa drugog ekrana podešavanja.
 * @note Briše sve widgete vezane za vrijeme, datum, screensaver i svjetlinu
 * ekrana.
 */
static void DSP_KillSet2Scrn(void)
{
    WM_DeleteWindow(hSPNBX_DisplayHighBrightness);
    WM_DeleteWindow(hSPNBX_DisplayLowBrightness);
    WM_DeleteWindow(hSPNBX_ScrnsvrDisableHour);
    WM_DeleteWindow(hSPNBX_ScrnsvrClockColour);
    WM_DeleteWindow(hSPNBX_ScrnsvrEnableHour);
    WM_DeleteWindow(hSPNBX_ScrnsvrTimeout);
    WM_DeleteWindow(hCHKBX_ScrnsvrClock);
    WM_DeleteWindow(hSPNBX_Minute);
    WM_DeleteWindow(hSPNBX_Month);
    WM_DeleteWindow(hBUTTON_Next);
    WM_DeleteWindow(hSPNBX_Hour);
    WM_DeleteWindow(hSPNBX_Year);
    WM_DeleteWindow(hDRPDN_WeekDay);
    WM_DeleteWindow(hSPNBX_Day);
    WM_DeleteWindow(hBUTTON_Ok);
}
/**
 * @brief Inicijalizuje treći ekran podešavanja (ventilator i odmrzivač).
 * @note Kreira widgete za postavke releja, odgode paljenja i gašenja
 * ventilatora i odmrzivača. Sav raspored elemenata je definisan u
 * `settings_screen_3_layout` strukturi.
 */
static void DSP_InitSet3Scrn(void)
{
    /** @brief Dobijamo handle-ove za module čije postavke se mijenjaju. */
    Defroster_Handle* defHandle = Defroster_GetInstance();
    Ventilator_Handle* ventHandle = Ventilator_GetInstance();

    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    /** @brief Kreiranje DROPDOWN liste za odabir funkcije četvrte ikonice. */
    hSelectControl_4 = DROPDOWN_CreateEx(settings_screen_3_layout.select_control_pos.x, settings_screen_3_layout.select_control_pos.y, settings_screen_3_layout.select_control_pos.w, settings_screen_3_layout.select_control_pos.h, 0, WM_CF_SHOW, DROPDOWN_CF_AUTOSCROLLBAR, ID_SelectControl_4);
    DROPDOWN_AddString(hSelectControl_4, "OFF");
    DROPDOWN_AddString(hSelectControl_4, "DEFROSTER");
    DROPDOWN_AddString(hSelectControl_4, "VENTILATOR");
    DROPDOWN_SetSel(hSelectControl_4, g_display_settings.selected_control_mode);
    DROPDOWN_SetFont(hSelectControl_4, GUI_FONT_16_1);

    /** @brief Kreiranje navigacionih dugmadi. */
    hBUTTON_Next = BUTTON_CreateEx(settings_screen_3_layout.next_button_pos.x, settings_screen_3_layout.next_button_pos.y, settings_screen_3_layout.next_button_pos.w, settings_screen_3_layout.next_button_pos.h, 0, WM_CF_SHOW, 0, ID_Next);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_CreateEx(settings_screen_3_layout.save_button_pos.x, settings_screen_3_layout.save_button_pos.y, settings_screen_3_layout.save_button_pos.w, settings_screen_3_layout.save_button_pos.h, 0, WM_CF_SHOW, 0, ID_Ok);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    /** @brief Kreiranje widgeta za postavke odmrzivača (Defroster). */
    defroster_settingWidgets.cycleTime = SPINBOX_CreateEx(settings_screen_3_layout.defroster_cycle_time_pos.x, settings_screen_3_layout.defroster_cycle_time_pos.y, settings_screen_3_layout.defroster_cycle_time_pos.w, settings_screen_3_layout.defroster_cycle_time_pos.h, 0, WM_CF_SHOW, ID_DEFROSTER_CYCLE_TIME, 0, 254);
    SPINBOX_SetEdge(defroster_settingWidgets.cycleTime, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(defroster_settingWidgets.cycleTime, Defroster_getCycleTime(defHandle));

    defroster_settingWidgets.activeTime = SPINBOX_CreateEx(settings_screen_3_layout.defroster_active_time_pos.x, settings_screen_3_layout.defroster_active_time_pos.y, settings_screen_3_layout.defroster_active_time_pos.w, settings_screen_3_layout.defroster_active_time_pos.h, 0, WM_CF_SHOW, ID_DEFROSTER_ACTIVE_TIME, 0, 254);
    SPINBOX_SetEdge(defroster_settingWidgets.activeTime, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(defroster_settingWidgets.activeTime, Defroster_getActiveTime(defHandle));

    defroster_settingWidgets.pin = SPINBOX_CreateEx(settings_screen_3_layout.defroster_pin_pos.x, settings_screen_3_layout.defroster_pin_pos.y, settings_screen_3_layout.defroster_pin_pos.w, settings_screen_3_layout.defroster_pin_pos.h, 0, WM_CF_SHOW, ID_DEFROSTER_PIN, 0, 6);
    SPINBOX_SetEdge(defroster_settingWidgets.pin, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(defroster_settingWidgets.pin, Defroster_getPin(defHandle));

    /** @brief Kreiranje widgeta za postavke ventilatora. */
    hVentilatorRelay = SPINBOX_CreateEx(settings_screen_3_layout.ventilator_relay_pos.x, settings_screen_3_layout.ventilator_relay_pos.y, settings_screen_3_layout.ventilator_relay_pos.w, settings_screen_3_layout.ventilator_relay_pos.h, 0, WM_CF_SHOW, ID_VentilatorRelay, 0, 512);
    SPINBOX_SetEdge(hVentilatorRelay, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hVentilatorRelay, Ventilator_getRelay(ventHandle));

    hVentilatorDelayOn = SPINBOX_CreateEx(settings_screen_3_layout.ventilator_delay_on_pos.x, settings_screen_3_layout.ventilator_delay_on_pos.y, settings_screen_3_layout.ventilator_delay_on_pos.w, settings_screen_3_layout.ventilator_delay_on_pos.h, 0, WM_CF_SHOW, ID_VentilatorDelayOn, 0, 255);
    SPINBOX_SetEdge(hVentilatorDelayOn, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hVentilatorDelayOn, Ventilator_getDelayOnTime(ventHandle));

    hVentilatorDelayOff = SPINBOX_CreateEx(settings_screen_3_layout.ventilator_delay_off_pos.x, settings_screen_3_layout.ventilator_delay_off_pos.y, settings_screen_3_layout.ventilator_delay_off_pos.w, settings_screen_3_layout.ventilator_delay_off_pos.h, 0, WM_CF_SHOW, ID_VentilatorDelayOff, 0, 255);
    SPINBOX_SetEdge(hVentilatorDelayOff, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hVentilatorDelayOff, Ventilator_getDelayOffTime(ventHandle));

    hVentilatorTriggerSource1 = SPINBOX_CreateEx(settings_screen_3_layout.ventilator_trigger1_pos.x, settings_screen_3_layout.ventilator_trigger1_pos.y, settings_screen_3_layout.ventilator_trigger1_pos.w, settings_screen_3_layout.ventilator_trigger1_pos.h, 0, WM_CF_SHOW, ID_VentilatorTriggerSource1, 0, 6);
    SPINBOX_SetEdge(hVentilatorTriggerSource1, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hVentilatorTriggerSource1, Ventilator_getTriggerSource1(ventHandle));

    hVentilatorTriggerSource2 = SPINBOX_CreateEx(settings_screen_3_layout.ventilator_trigger2_pos.x, settings_screen_3_layout.ventilator_trigger2_pos.y, settings_screen_3_layout.ventilator_trigger2_pos.w, settings_screen_3_layout.ventilator_trigger2_pos.h, 0, WM_CF_SHOW, ID_VentilatorTriggerSource2, 0, 6);
    SPINBOX_SetEdge(hVentilatorTriggerSource2, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hVentilatorTriggerSource2, Ventilator_getTriggerSource2(ventHandle));

    hVentilatorLocalPin = SPINBOX_CreateEx(settings_screen_3_layout.ventilator_local_pin_pos.x, settings_screen_3_layout.ventilator_local_pin_pos.y, settings_screen_3_layout.ventilator_local_pin_pos.w, settings_screen_3_layout.ventilator_local_pin_pos.h, 0, WM_CF_SHOW, ID_VentilatorLocalPin, 0, 32);
    SPINBOX_SetEdge(hVentilatorLocalPin, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hVentilatorLocalPin, Ventilator_getLocalPin(ventHandle));

    /** @brief Iscrtavanje labela i linija. */
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);

    // Labele za VENTILATOR
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_relay[0].x, settings_screen_3_layout.label_ventilator_relay[0].y);
    GUI_DispString("VENTILATOR");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_relay[1].x, settings_screen_3_layout.label_ventilator_relay[1].y);
    GUI_DispString("BUS RELAY");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_delay_on[0].x, settings_screen_3_layout.label_ventilator_delay_on[0].y);
    GUI_DispString("VENTILATOR");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_delay_on[1].x, settings_screen_3_layout.label_ventilator_delay_on[1].y);
    GUI_DispString("DELAY ON");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_delay_off[0].x, settings_screen_3_layout.label_ventilator_delay_off[0].y);
    GUI_DispString("VENTILATOR");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_delay_off[1].x, settings_screen_3_layout.label_ventilator_delay_off[1].y);
    GUI_DispString("DELAY OFF");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_trigger1[0].x, settings_screen_3_layout.label_ventilator_trigger1[0].y);
    GUI_DispString("VENTILATOR");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_trigger1[1].x, settings_screen_3_layout.label_ventilator_trigger1[1].y);
    GUI_DispString("TRIGGER 1");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_trigger2[0].x, settings_screen_3_layout.label_ventilator_trigger2[0].y);
    GUI_DispString("VENTILATOR");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_trigger2[1].x, settings_screen_3_layout.label_ventilator_trigger2[1].y);
    GUI_DispString("TRIGGER 2");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_local_pin[0].x, settings_screen_3_layout.label_ventilator_local_pin[0].y);
    GUI_DispString("VENTILATOR");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_local_pin[1].x, settings_screen_3_layout.label_ventilator_local_pin[1].y);
    GUI_DispString("LOCAL PIN");

    // Labele za DEFROSTER
    GUI_GotoXY(settings_screen_3_layout.label_defroster_cycle_time[0].x, settings_screen_3_layout.label_defroster_cycle_time[0].y);
    GUI_DispString("DEFROSTER");
    GUI_GotoXY(settings_screen_3_layout.label_defroster_cycle_time[1].x, settings_screen_3_layout.label_defroster_cycle_time[1].y);
    GUI_DispString("CYCLE TIME");
    GUI_GotoXY(settings_screen_3_layout.label_defroster_active_time[0].x, settings_screen_3_layout.label_defroster_active_time[0].y);
    GUI_DispString("DEFROSTER");
    GUI_GotoXY(settings_screen_3_layout.label_defroster_active_time[1].x, settings_screen_3_layout.label_defroster_active_time[1].y);
    GUI_DispString("ACTIVE TIME");
    GUI_GotoXY(settings_screen_3_layout.label_defroster_pin[0].x, settings_screen_3_layout.label_defroster_pin[0].y);
    GUI_DispString("DEFROSTER");
    GUI_GotoXY(settings_screen_3_layout.label_defroster_pin[1].x, settings_screen_3_layout.label_defroster_pin[1].y);
    GUI_DispString("PIN");

    // Labele za naslove i odabir kontrole
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_title.x, settings_screen_3_layout.label_ventilator_title.y);
    GUI_DispString("VENTILATOR CONTROL");
    GUI_GotoXY(settings_screen_3_layout.label_defroster_title.x, settings_screen_3_layout.label_defroster_title.y);
    GUI_DispString("DEFROSTER CONTROL");
    GUI_GotoXY(settings_screen_3_layout.label_select_control_title.x, settings_screen_3_layout.label_select_control_title.y);
    GUI_DispString("SELECT CONTROL 4");

    // Linije
    GUI_DrawHLine(settings_screen_3_layout.line_ventilator_title.y, settings_screen_3_layout.line_ventilator_title.x0, settings_screen_3_layout.line_ventilator_title.x1);
    GUI_DrawHLine(settings_screen_3_layout.line_defroster_title.y, settings_screen_3_layout.line_defroster_title.x0, settings_screen_3_layout.line_defroster_title.x1);
    GUI_DrawHLine(settings_screen_3_layout.line_select_control.y, settings_screen_3_layout.line_select_control.x0, settings_screen_3_layout.line_select_control.x1);

    GUI_MULTIBUF_EndEx(1);
}
/**
 * @brief Briše GUI widgete sa trećeg ekrana podešavanja.
 * @note Briše sve widgete vezane za postavke ventilatora.
 */
static void DSP_KillSet3Scrn(void)
{
    WM_DeleteWindow(defroster_settingWidgets.cycleTime);
    WM_DeleteWindow(defroster_settingWidgets.activeTime);
    WM_DeleteWindow(defroster_settingWidgets.pin);
    WM_DeleteWindow(hVentilatorRelay);
    WM_DeleteWindow(hVentilatorDelayOn);
    WM_DeleteWindow(hVentilatorDelayOff);
    WM_DeleteWindow(hVentilatorTriggerSource1);
    WM_DeleteWindow(hVentilatorTriggerSource2);
    WM_DeleteWindow(hVentilatorLocalPin);
    WM_DeleteWindow(hSelectControl_4);
    WM_DeleteWindow(hBUTTON_Next);
    WM_DeleteWindow(hBUTTON_Ok);
}

/**
 * @brief Inicijalizuje četvrti ekran podešavanja (zavjese).
 * @note  Dinamički kreira SPINBOX-ove za do 4 zavjese po ekranu
 * za podešavanje releja "GORE" i "DOLJE". Koristi `settings_screen_4_layout`
 * strukturu da dinamički izračuna pozicije svih elemenata u 2x2 mreži.
 */
static void DSP_InitSet4Scrn(void)
{
    /**
     * @brief Inicijalizacija iscrtavanja.
     */
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    /**
     * @brief Petlja za kreiranje widgeta.
     * @note  Iterira kroz 4 roletne relevantne za trenutnu stranicu menija
     * (`curtainSettingMenu`).
     */
    for(uint8_t i = curtainSettingMenu * 4; i < (((CURTAINS_SIZE - (curtainSettingMenu * 4)) >= 4) ? ((curtainSettingMenu * 4) + 4) : CURTAINS_SIZE); i++) {

        /** @brief Dobijamo handle za roletnu po njenom fizičkom indeksu u nizu. */
        Curtain_Handle* handle = Curtain_GetInstanceByIndex(i);

        /**
         * @brief Dinamičko izračunavanje pozicija za trenutnu roletnu u mreži.
         * @note  Ova logika postavlja widgete u dvije kolone i dva reda.
         */
        int col = ((i % 4) < 2) ? 0 : 1; // 0 za prvu kolonu, 1 za drugu
        int row = (i % 4) % 2;          // 0 za prvi red, 1 za drugi
        int x = settings_screen_4_layout.grid_start_pos.x + (col * settings_screen_4_layout.x_col_spacing);
        int y = settings_screen_4_layout.grid_start_pos.y + (row * settings_screen_4_layout.y_group_spacing);

        /**
         * @brief Kreiranje SPINBOX-a za relej "GORE".
         * @note  Poziv `SPINBOX_CreateEx` ima 9 argumenata.
         */
        hCurtainsRelay[i * 2] = SPINBOX_CreateEx(x, y, settings_screen_4_layout.widget_width, settings_screen_4_layout.widget_height, 0, WM_CF_SHOW, ID_CurtainsRelay + (i * 2), 0, 512);
        SPINBOX_SetEdge(hCurtainsRelay[i * 2], SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(hCurtainsRelay[i * 2], Curtain_getRelayUp(handle));

        /**
         * @brief Kreiranje SPINBOX-a za relej "DOLJE".
         * @note  Pozicija se računa na osnovu pozicije "GORE" widgeta i `y_row_spacing` konstante.
         */
        hCurtainsRelay[(i * 2) + 1] = SPINBOX_CreateEx(x, y + settings_screen_4_layout.y_row_spacing, settings_screen_4_layout.widget_width, settings_screen_4_layout.widget_height, 0, WM_CF_SHOW, ID_CurtainsRelay + (i * 2) + 1, 0, 512);
        SPINBOX_SetEdge(hCurtainsRelay[(i * 2) + 1], SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(hCurtainsRelay[(i * 2) + 1], Curtain_getRelayDown(handle));

        /**
         * @brief Iscrtavanje labela pored kreiranih widgeta.
         * @note  Pozicije labela se računaju relativno u odnosu na poziciju widgeta.
         */
        GUI_SetColor(GUI_WHITE);
        GUI_SetFont(GUI_FONT_13_1);
        GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);

        // Labela za "GORE"
        GUI_GotoXY(x + settings_screen_4_layout.label_line1_offset.x, y + settings_screen_4_layout.label_line1_offset.y);
        GUI_DispString("CURTAIN ");
        GUI_DispDec(i + 1, 2);
        GUI_GotoXY(x + settings_screen_4_layout.label_line1_offset.x, y + settings_screen_4_layout.label_line1_offset.y + settings_screen_4_layout.label_line2_offset_y);
        GUI_DispString("RELAY UP");

        // Labela za "DOLE"
        GUI_GotoXY(x + settings_screen_4_layout.label_line1_offset.x, y + settings_screen_4_layout.y_row_spacing + settings_screen_4_layout.label_line1_offset.y);
        GUI_DispString("CURTAIN ");
        GUI_DispDec(i + 1, 2);
        GUI_GotoXY(x + settings_screen_4_layout.label_line1_offset.x, y + settings_screen_4_layout.y_row_spacing + settings_screen_4_layout.label_line1_offset.y + settings_screen_4_layout.label_line2_offset_y);
        GUI_DispString("RELAY DOWN");
    }

    /**
     * @brief Kreiranje navigacionih dugmadi.
     * @note  Pozivi `BUTTON_CreateEx` imaju 8 argumenata.
     */
    hBUTTON_Next = BUTTON_CreateEx(settings_screen_4_layout.next_button_pos.x, settings_screen_4_layout.next_button_pos.y, settings_screen_4_layout.next_button_pos.w, settings_screen_4_layout.next_button_pos.h, 0, WM_CF_SHOW, 0, ID_Next);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_CreateEx(settings_screen_4_layout.save_button_pos.x, settings_screen_4_layout.save_button_pos.y, settings_screen_4_layout.save_button_pos.w, settings_screen_4_layout.save_button_pos.h, 0, WM_CF_SHOW, 0, ID_Ok);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    GUI_MULTIBUF_EndEx(1);
}
/**
 * @brief Briše GUI widgete sa četvrtog ekrana podešavanja.
 * @note Briše dinamički kreirane widgete za zavjese na trenutnoj stranici.
 */
static void DSP_KillSet4Scrn(void)
{
    for(uint8_t i = curtainSettingMenu * 4; i < (((CURTAINS_SIZE - (curtainSettingMenu * 4)) >= 4) ? ((curtainSettingMenu * 4) + 4) : CURTAINS_SIZE); i++) {
        if(hCurtainsRelay[i * 2]) { // Provjeri da li widget postoji
            WM_DeleteWindow(hCurtainsRelay[i * 2]);
            hCurtainsRelay[i * 2] = 0; // Resetuj handle
        }
        if(hCurtainsRelay[(i * 2) + 1]) { // Provjeri da li widget postoji
            WM_DeleteWindow(hCurtainsRelay[(i * 2) + 1]);
            hCurtainsRelay[(i * 2) + 1] = 0; // Resetuj handle
        }
    }
    WM_DeleteWindow(hBUTTON_Next);
    WM_DeleteWindow(hBUTTON_Ok);
}

/**
 * @brief Inicijalizuje peti ekran podešavanja (detaljne postavke za svjetla).
 * @note  Ova funkcija dinamički kreira sve GUI widgete za podešavanje jednog
 * svjetla po ekranu. Koristi `settings_screen_5_layout` strukturu za
 * pozicioniranje svih elemenata u dvije kolone.
 */
static void DSP_InitSet5Scrn(void)
{
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    /** @brief Dohvatamo indeks i handle svjetla čije postavke trenutno prikazujemo. */
    uint8_t light_index = lightsModbusSettingsMenu;
    LIGHT_Handle* handle = LIGHTS_GetInstance(light_index);

    if (!handle) {
        GUI_MULTIBUF_EndEx(1);
        return; // Sigurnosna provjera
    }

    /**
     * @brief Kreiranje widgeta u prvoj koloni.
     * @note  Pozicije se računaju na osnovu početnih koordinata i visine reda (y_step)
     * definisanih u `settings_screen_5_layout`.
     */
    const WidgetRect_t* sb_size = &settings_screen_5_layout.spinbox_size;
    int16_t x = settings_screen_5_layout.col1_x;
    int16_t y = settings_screen_5_layout.start_y;
    int16_t y_step = settings_screen_5_layout.y_step;

    // << ISPRAVKA 2: Promijenjen množilac za ID-jeve sa 12 na 16 radi izbjegavanja preklapanja >>
    const int id_step = 16;

    // << ISPRAVKA 1: Vraćena linija za kreiranje RELAY spinbox-a >>
    lightsWidgets[light_index].relay = SPINBOX_CreateEx(x, y, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 0, 0, 512);

    // << ISPRAVKA 1: Opseg za IconID je sada ispravan i nema duplirane linije >>
    uint16_t max_icon_id = (sizeof(icon_mapping_table) / sizeof(IconMapping_t)) - 1;
    lightsWidgets[light_index].iconID = SPINBOX_CreateEx(x, y + 1 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 1, 0, max_icon_id);

    lightsWidgets[light_index].controllerID_on = SPINBOX_CreateEx(x, y + 2 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 2, 0, 512);
    lightsWidgets[light_index].controllerID_on_delay  = SPINBOX_CreateEx(x, y + 3 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 3, 0, 255);
    lightsWidgets[light_index].on_hour = SPINBOX_CreateEx(x, y + 4 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 4, -1, 23);
    lightsWidgets[light_index].on_minute = SPINBOX_CreateEx(x, y + 5 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 5, 0, 59);

    x = settings_screen_5_layout.col2_x;

    lightsWidgets[light_index].offTime = SPINBOX_CreateEx(x, y, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 6, 0, 255);
    lightsWidgets[light_index].communication_type = SPINBOX_CreateEx(x, y + 1 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 7, 1, 3);
    lightsWidgets[light_index].local_pin = SPINBOX_CreateEx(x, y + 2 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 8, 0, 32);
    lightsWidgets[light_index].sleep_time = SPINBOX_CreateEx(x, y + 3 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 9, 0, 255);
    lightsWidgets[light_index].button_external = SPINBOX_CreateEx(x, y + 4 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * id_step) + 10, 0, 3);

    const WidgetRect_t* cb1_size = &settings_screen_5_layout.checkbox1_size;
    lightsWidgets[light_index].tiedToMainLight = CHECKBOX_CreateEx(x, y + 5 * y_step, cb1_size->w, cb1_size->h, 0, WM_CF_SHOW, 0, ID_LightsModbusRelay + (light_index * id_step) + 11);

    const WidgetRect_t* cb2_size = &settings_screen_5_layout.checkbox2_size;
    lightsWidgets[light_index].rememberBrightness = CHECKBOX_CreateEx(x, y + 5 * y_step + 23, cb2_size->w, cb2_size->h, 0, WM_CF_SHOW, 0, ID_LightsModbusRelay + (light_index * id_step) + 12);

    /** @brief Postavljanje početnih vrijednosti za sve kreirane widgete. */
    SPINBOX_SetEdge(lightsWidgets[light_index].relay, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].relay, LIGHT_GetRelay(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].iconID, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].iconID, LIGHT_GetIconID(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].controllerID_on, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].controllerID_on, LIGHT_GetControllerID(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].controllerID_on_delay, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].controllerID_on_delay, LIGHT_GetOnDelayTime(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].on_hour, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].on_hour, LIGHT_GetOnHour(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].on_minute, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].on_minute, LIGHT_GetOnMinute(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].offTime, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].offTime, LIGHT_GetOffTime(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].communication_type, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].communication_type, LIGHT_GetCommunicationType(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].local_pin, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].local_pin, LIGHT_GetLocalPin(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].sleep_time, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].sleep_time, LIGHT_GetSleepTime(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].button_external, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].button_external, LIGHT_GetButtonExternal(handle));

    CHECKBOX_SetTextColor(lightsWidgets[light_index].tiedToMainLight, GUI_GREEN);
    CHECKBOX_SetText(lightsWidgets[light_index].tiedToMainLight, "TIED TO MAIN LIGHT");
    CHECKBOX_SetState(lightsWidgets[light_index].tiedToMainLight, LIGHT_isTiedToMainLight(handle));

    CHECKBOX_SetTextColor(lightsWidgets[light_index].rememberBrightness, GUI_GREEN);
    CHECKBOX_SetText(lightsWidgets[light_index].rememberBrightness, "REMEMBER BRIGHTNESS");
    CHECKBOX_SetState(lightsWidgets[light_index].rememberBrightness, LIGHT_isBrightnessRemembered(handle));

    /** @brief Kreiranje navigacionih dugmadi. */
    hBUTTON_Next = BUTTON_CreateEx(settings_screen_5_layout.next_button_pos.x, settings_screen_5_layout.next_button_pos.y, settings_screen_5_layout.next_button_pos.w, settings_screen_5_layout.next_button_pos.h, 0, WM_CF_SHOW, 0, ID_Next);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_CreateEx(settings_screen_5_layout.save_button_pos.x, settings_screen_5_layout.save_button_pos.y, settings_screen_5_layout.save_button_pos.w, settings_screen_5_layout.save_button_pos.h, 0, WM_CF_SHOW, 0, ID_Ok);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    /** @brief Iscrtavanje labela. */
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);

    const GUI_POINT* label_offset = &settings_screen_5_layout.label_line1_offset;
    const int16_t label_y2_offset = settings_screen_5_layout.label_line2_offset_y;
    x = settings_screen_5_layout.col1_x;
    y = settings_screen_5_layout.start_y;

    // Prva kolona labela
    GUI_GotoXY(x + label_offset->x, y + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + label_offset->y + label_y2_offset);
    GUI_DispString("RELAY");

    GUI_GotoXY(x + label_offset->x, y + 1 * y_step + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 1 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("ICON");

    GUI_GotoXY(x + label_offset->x, y + 2 * y_step + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 2 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("ON ID");

    GUI_GotoXY(x + label_offset->x, y + 3 * y_step + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 3 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("ON ID DELAY");

    GUI_GotoXY(x + label_offset->x, y + 4 * y_step + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 4 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("HOUR ON");

    GUI_GotoXY(x + label_offset->x, y + 5 * y_step + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 5 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("MINUTE ON");

    // Druga kolona labela
    x = settings_screen_5_layout.col2_x;

    GUI_GotoXY(x + label_offset->x, y + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + label_offset->y + label_y2_offset);
    GUI_DispString("DELAY OFF");

    GUI_GotoXY(x + label_offset->x, y + 1 * y_step + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 1 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("COMM. TYPE");

    GUI_GotoXY(x + label_offset->x, y + 2 * y_step + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 2 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("LOCAL PIN");

    GUI_GotoXY(x + label_offset->x, y + 3 * y_step + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 3 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("SLEEP TIME");

    GUI_GotoXY(x + label_offset->x, y + 4 * y_step + label_offset->y);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 4 * y_step + label_offset->y + label_y2_offset);
    GUI_DispString("BUTTON EXT.");

    GUI_MULTIBUF_EndEx(1);
}
/**
 * @brief Briše GUI widgete sa petog ekrana podešavanja.
 * @note Briše dinamički kreirane widgete za svjetla na trenutnoj stranici.
 */
static void DSP_KillSet5Scrn(void)
{
    // Računamo indeks svjetla čiji su widgeti trenutno na ekranu.
    uint8_t i = lightsModbusSettingsMenu; // Ovdje ne treba množenje

    // Brišemo svaki widget pojedinačno za taj specifični indeks.
    WM_DeleteWindow(lightsWidgets[i].relay);
    WM_DeleteWindow(lightsWidgets[i].iconID);
    WM_DeleteWindow(lightsWidgets[i].controllerID_on);
    WM_DeleteWindow(lightsWidgets[i].controllerID_on_delay);
    WM_DeleteWindow(lightsWidgets[i].offTime);
    WM_DeleteWindow(lightsWidgets[i].on_hour);
    WM_DeleteWindow(lightsWidgets[i].on_minute);
    WM_DeleteWindow(lightsWidgets[i].communication_type);
    WM_DeleteWindow(lightsWidgets[i].local_pin);
    WM_DeleteWindow(lightsWidgets[i].sleep_time);
    WM_DeleteWindow(lightsWidgets[i].button_external);
    WM_DeleteWindow(lightsWidgets[i].tiedToMainLight);
    WM_DeleteWindow(lightsWidgets[i].rememberBrightness);

    // Brišemo i zajedničke dugmiće.
    WM_DeleteWindow(hBUTTON_Next);
    WM_DeleteWindow(hBUTTON_Ok);
}
/**
 * @brief Inicijalizuje šesti ekran podešavanja (opšte postavke).
 * @note  Verzija 2.0: Dodat DROPDOWN meni za odabir jezika.
 * Kreira widgete za podešavanje ID-a uređaja, vremena za kretanje roletni,
 * ponašanja screensaver-a, te opcije za noćni tajmer i dugmad za
 * restart i vraćanje na podrazumijevane vrijednosti.
 * Sav raspored elemenata je definisan u `settings_screen_6_layout` strukturi.
 */
static void DSP_InitSet6Scrn(void)
{
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    /** @brief Kreiranje widgeta koristeći pozicije i dimenzije iz layout strukture. */
    hDEV_ID = SPINBOX_CreateEx(settings_screen_6_layout.device_id_pos.x, settings_screen_6_layout.device_id_pos.y, settings_screen_6_layout.device_id_pos.w, settings_screen_6_layout.device_id_pos.h, 0, WM_CF_SHOW, ID_DEV_ID, 1, 254);
    SPINBOX_SetEdge(hDEV_ID, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hDEV_ID, tfifa);

    hCurtainsMoveTime = SPINBOX_CreateEx(settings_screen_6_layout.curtain_move_time_pos.x, settings_screen_6_layout.curtain_move_time_pos.y, settings_screen_6_layout.curtain_move_time_pos.w, settings_screen_6_layout.curtain_move_time_pos.h, 0, WM_CF_SHOW, ID_CurtainsMoveTime, 0, 60);
    SPINBOX_SetEdge(hCurtainsMoveTime, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hCurtainsMoveTime, Curtain_GetMoveTime());

    const WidgetRect_t* cb1_pos = &settings_screen_6_layout.leave_scrnsvr_checkbox_pos;
    hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH = CHECKBOX_CreateEx(cb1_pos->x, cb1_pos->y, cb1_pos->w, cb1_pos->h, 0, WM_CF_SHOW, 0, ID_LEAVE_SCRNSVR_AFTER_TOUCH);
    CHECKBOX_SetTextColor(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, GUI_GREEN);
    CHECKBOX_SetText(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, "ONLY LEAVE SCRNSVR AFTER TOUCH");
    CHECKBOX_SetState(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, g_display_settings.leave_scrnsvr_on_release);

    const WidgetRect_t* cb2_pos = &settings_screen_6_layout.night_timer_checkbox_pos;
    hCHKBX_LIGHT_NIGHT_TIMER = CHECKBOX_CreateEx(cb2_pos->x, cb2_pos->y, cb2_pos->w, cb2_pos->h, 0, WM_CF_SHOW, 0, ID_LIGHT_NIGHT_TIMER);
    CHECKBOX_SetTextColor(hCHKBX_LIGHT_NIGHT_TIMER, GUI_GREEN);
    CHECKBOX_SetText(hCHKBX_LIGHT_NIGHT_TIMER, "LIGHT OFF TIMER AFTER 20h");
    CHECKBOX_SetState(hCHKBX_LIGHT_NIGHT_TIMER, g_display_settings.light_night_timer_enabled);


    /** @brief << NOVO: Kreiranje DROPDOWN-a za odabir jezika >> */
    const WidgetRect_t* lang_pos = &settings_screen_6_layout.language_dropdown_pos;
    hDRPDN_Language = DROPDOWN_CreateEx(lang_pos->x, lang_pos->y, lang_pos->w, lang_pos->h, 0, WM_CF_SHOW, DROPDOWN_CF_AUTOSCROLLBAR, ID_LanguageSelect);
    for (int i = 0; i < LANGUAGE_COUNT; i++) {
        // Koristimo TXT_LANGUAGE_NAME da dobijemo ime jezika na tom jeziku
        DROPDOWN_AddString(hDRPDN_Language, language_strings[TXT_LANGUAGE_NAME][i]);
    }
    DROPDOWN_SetSel(hDRPDN_Language, g_display_settings.language);
    DROPDOWN_SetFont(hDRPDN_Language, GUI_FONT_16_1);

    /** @brief Kreiranje dugmadi za specijalne akcije. */
    const WidgetRect_t* defaults_pos = &settings_screen_6_layout.set_defaults_button_pos;
    hBUTTON_SET_DEFAULTS = BUTTON_CreateEx(defaults_pos->x, defaults_pos->y, defaults_pos->w, defaults_pos->h, 0, WM_CF_SHOW, 0, ID_SET_DEFAULTS);
    BUTTON_SetText(hBUTTON_SET_DEFAULTS, "SET DEFAULTS");

    const WidgetRect_t* restart_pos = &settings_screen_6_layout.restart_button_pos;
    hBUTTON_SYSRESTART = BUTTON_CreateEx(restart_pos->x, restart_pos->y, restart_pos->w, restart_pos->h, 0, WM_CF_SHOW, 0, ID_SYSRESTART);
    BUTTON_SetText(hBUTTON_SYSRESTART, "RESTART");

    /** @brief Kreiranje navigacionih dugmadi. */
    const WidgetRect_t* next_pos = &settings_screen_6_layout.next_button_pos;
    hBUTTON_Next = BUTTON_CreateEx(next_pos->x, next_pos->y, next_pos->w, next_pos->h, 0, WM_CF_SHOW, 0, ID_Next);
    BUTTON_SetText(hBUTTON_Next, "NEXT");

    const WidgetRect_t* save_pos = &settings_screen_6_layout.save_button_pos;
    hBUTTON_Ok = BUTTON_CreateEx(save_pos->x, save_pos->y, save_pos->w, save_pos->h, 0, WM_CF_SHOW, 0, ID_Ok);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    /** @brief Iscrtavanje labela. */
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);

    GUI_GotoXY(settings_screen_6_layout.device_id_label_pos[0].x, settings_screen_6_layout.device_id_label_pos[0].y);
    GUI_DispString("DEVICE");
    GUI_GotoXY(settings_screen_6_layout.device_id_label_pos[1].x, settings_screen_6_layout.device_id_label_pos[1].y);
    GUI_DispString("BUS ID");

    GUI_GotoXY(settings_screen_6_layout.curtain_move_time_label_pos[0].x, settings_screen_6_layout.curtain_move_time_label_pos[0].y);
    GUI_DispString("CURTAINS");
    GUI_GotoXY(settings_screen_6_layout.curtain_move_time_label_pos[1].x, settings_screen_6_layout.curtain_move_time_label_pos[1].y);
    GUI_DispString("MOVE TIME");

    /** @brief << NOVO: Iscrtavanje labele za jezik >> */
    GUI_GotoXY(settings_screen_6_layout.language_label_pos.x, settings_screen_6_layout.language_label_pos.y);
    GUI_DispString("LANGUAGE");

    GUI_MULTIBUF_EndEx(1);
}

/**
 * @brief Briše GUI widgete sa šestog ekrana podešavanja.
 * @note Briše sve widgete vezane za Device ID, screensaver i navigaciju.
 */
static void DSP_KillSet6Scrn(void)
{
    WM_DeleteWindow(hDEV_ID);
    WM_DeleteWindow(hCurtainsMoveTime);
    WM_DeleteWindow(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH);
    WM_DeleteWindow(hCHKBX_LIGHT_NIGHT_TIMER);
    WM_DeleteWindow(hBUTTON_SET_DEFAULTS);
    WM_DeleteWindow(hBUTTON_SYSRESTART);
    WM_DeleteWindow(hDRPDN_Language);
    WM_DeleteWindow(hBUTTON_Next);
    WM_DeleteWindow(hBUTTON_Ok);
}
/**
 ******************************************************************************
 * @brief       Inicijalizuje sedmi ekran podešavanja (Scene Backend).
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija kreira korisnički interfejs za napredna podešavanja
 * sistema scena. Uključuje opciju za globalno omogućavanje/onemogućavanje
 * scena i tabelu za mapiranje do 8 logičkih okidača za "Povratak"
 * scenu na njihove stvarne adrese na RS485 busu.
 ******************************************************************************
 */
static void DSP_InitSet7Scrn(void)
{
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);
    
    // --- Kreiranje Glavnog Checkbox-a ---
    const WidgetRect_t* scenes_cb_pos = &settings_screen_7_layout.enable_scenes_checkbox_pos;
    hCHKBX_EnableScenes = CHECKBOX_CreateEx(scenes_cb_pos->x, scenes_cb_pos->y, scenes_cb_pos->w, scenes_cb_pos->h, 0, WM_CF_SHOW, 0, ID_ENABLE_SCENES);
    CHECKBOX_SetTextColor(hCHKBX_EnableScenes, GUI_GREEN);
    CHECKBOX_SetText(hCHKBX_EnableScenes, "ENABLE SCENE"); // "Enable Scene"
    CHECKBOX_SetState(hCHKBX_EnableScenes, g_display_settings.scenes_enabled);
    
    // Naslov za sekciju okidača
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetColor(GUI_WHITE);
    GUI_DispStringAt("Mapiranje Okidaca za 'Povratak' Scenu:", 10, 30);
    
    // --- Kreiranje Mreže za Mapiranje Okidača (8 komada) ---
    for (uint8_t i = 0; i < SCENE_MAX_TRIGGERS; i++)
    {
        // Dinamičko izračunavanje pozicija u dvije kolone
        int col = i / 4; // 0 za prvu kolonu (okidači 1-4), 1 za drugu (okidači 5-8)
        int row = i % 4; // 0-3 red unutar kolone
        
        int x = settings_screen_7_layout.grid_start_pos.x + (col * settings_screen_7_layout.x_col_spacing);
        int y = settings_screen_7_layout.grid_start_pos.y + (row * settings_screen_7_layout.y_spacing);
        
        // Kreiranje Spinbox-a
        hSPNBX_SceneTriggers[i] = SPINBOX_CreateEx(x, y, settings_screen_7_layout.widget_width, settings_screen_7_layout.widget_height, 0, WM_CF_SHOW, ID_SCENE_TRIGGER_1 + i, 0, 65535);
        SPINBOX_SetEdge(hSPNBX_SceneTriggers[i], SPINBOX_EDGE_CENTER);
        // TODO: Učitati snimljenu vrijednost iz EEPROM-a kada se definiše struktura za to
        // SPINBOX_SetValue(hSPNBX_SceneTriggers[i], g_scene_settings.triggers[i]); 
        
        // Kreiranje Labele
        char label_buffer[20];
        sprintf(label_buffer, "Okidac %d", i + 1);
        GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);
        GUI_GotoXY(x + settings_screen_7_layout.label_offset.x, y + settings_screen_7_layout.label_offset.y);
        GUI_DispString(label_buffer);
    }

    // --- Kreiranje Navigacionih Dugmadi ---
    hBUTTON_Next = BUTTON_CreateEx(settings_screen_7_layout.next_button_pos.x, settings_screen_7_layout.next_button_pos.y, settings_screen_7_layout.next_button_pos.w, settings_screen_7_layout.next_button_pos.h, 0, WM_CF_SHOW, 0, ID_Next);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_CreateEx(settings_screen_7_layout.save_button_pos.x, settings_screen_7_layout.save_button_pos.y, settings_screen_7_layout.save_button_pos.w, settings_screen_7_layout.save_button_pos.h, 0, WM_CF_SHOW, 0, ID_Ok);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    GUI_MULTIBUF_EndEx(1);
}
/**
 ******************************************************************************
 * @brief       Briše GUI widgete sa sedmog ekrana podešavanja (Scene Backend).
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva prilikom napuštanja ekrana
 * `SCREEN_SETTINGS_7` kako bi se oslobodila memorija i spriječili
 * konflikti sa widgetima na drugim ekranima. Uklanja sve
 * dinamički kreirane elemente.
 ******************************************************************************
 */
static void DSP_KillSet7Scrn(void)
{
    // Brišemo checkbox za omogućavanje scena
    WM_DeleteWindow(hCHKBX_EnableScenes);

    // Brišemo svih 8 spinbox-ova za adrese okidača
    for (uint8_t i = 0; i < SCENE_MAX_TRIGGERS; i++)
    {
        if (WM_IsWindow(hSPNBX_SceneTriggers[i])) // Sigurnosna provjera
        {
            WM_DeleteWindow(hSPNBX_SceneTriggers[i]);
        }
    }
    
    // Brišemo navigacione dugmiće
    WM_DeleteWindow(hBUTTON_Next);
    WM_DeleteWindow(hBUTTON_Ok);
}
/**
 ******************************************************************************
 * @brief       Inicijalizuje i iscrtava ekran za podešavanje kapija.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija dinamički kreira sve GUI widgete za podešavanje
 * jedne kapije po ekranu, uključujući SPINBOX za odabir kapije,
 * DROPDOWN za tip kapije i dugmad za unos numeričkih vrijednosti.
 * Potpuno je refaktorisana da koristi isključivo javne API funkcije
 * iz `gate.h`, čime se poštuje princip enkapsulacije.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void DSP_InitSettingsGateScreen(void)
{
    char buffer[20]; // Pomoćni bafer za ispis vrijednosti na dugmad

    GUI_MULTIBUF_BeginEx(1);
    GUI_Clear();

    Gate_Handle* handle = Gate_GetInstance(settings_gate_selected_index);
    if (!handle) { // Sigurnosna provjera
        GUI_DispStringAt("GRESKA: Kapija nije dostupna!", 10, 60);
        GUI_MULTIBUF_EndEx(1);
        return;
    }

    // === 1. Odabir Kapije i Dohvatanje Podataka ===
    hGateSelect = SPINBOX_CreateEx(10, 5, 80, 40, 0, WM_CF_SHOW, ID_GATE_SELECT, 1, GATE_MAX_COUNT);
    SPINBOX_SetValue(hGateSelect, settings_gate_selected_index + 1);

    GUI_SetFont(GUI_FONT_20_1);
    GUI_SetColor(GUI_WHITE);
    GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
    GUI_DispStringAt("Podesavanje Kapije:", 100, 25);

    // === 2. Kreiranje Osnovnih Widgeta (Tip Kapije) ===
    hGateType = DROPDOWN_CreateEx(10, 60, 150, 100, 0, WM_CF_SHOW, DROPDOWN_CF_AUTOSCROLLBAR, ID_GATE_TYPE);
    DROPDOWN_AddString(hGateType, "Nije Konfigurisano");
    DROPDOWN_AddString(hGateType, "Krilna Kapija");
    DROPDOWN_AddString(hGateType, "Klizna Kapija");
    DROPDOWN_AddString(hGateType, "Garazna Vrata");
    
    // Ispravljen poziv: koristi se getter funkcija
    DROPDOWN_SetSel(hGateType, Gate_GetType(handle));

    // === 3. Kreiranje "EDIT" Dugmadi za numeričke vrijednosti ===
    const int x_col1 = 10, x_col2 = 170, x_col3 = 330;
    const int y_row1 = 100, y_row2 = 145, y_row3 = 190;
    const int btn_w = 140, btn_h = 35;

    // Pomoćna struktura za lakše kreiranje dugmadi u petlji
    struct {
        int id;
        int x, y;
        const char* label;
    } button_map[] = {
        { ID_GATE_RELAY_OPEN,     x_col1, y_row1, "Relej OTVORI:" },
        { ID_GATE_RELAY_CLOSE,    x_col1, y_row2, "Relej ZATVORI:" },
        { ID_GATE_RELAY_PED,      x_col1, y_row3, "Relej PJESAK:" },
        { ID_GATE_FEEDBACK_OPEN,  x_col2, y_row1, "Senzor OTVORENO:" },
        { ID_GATE_FEEDBACK_CLOSE, x_col2, y_row2, "Senzor ZATVORENO:" },
        { ID_GATE_RELAY_STOP,     x_col2, y_row3, "Relej STOP:" },
        { ID_GATE_CYCLE_TIMER,    x_col3, y_row1, "Vrijeme Ciklusa:" },
        { ID_GATE_PED_TIMER,      x_col3, y_row2, "Vrijeme Pjesak:" },
        { ID_GATE_PULSE_TIMER,    x_col3, y_row3, "Trajanje Impulsa:" }
    };
    
    GUI_SetFont(GUI_FONT_13_1);
    for (int i = 0; i < 9; i++) {
        GUI_DispStringAt(button_map[i].label, button_map[i].x, button_map[i].y - 13);
        hGateEditButtons[i] = BUTTON_CreateEx(button_map[i].x, button_map[i].y, btn_w, btn_h, 0, WM_CF_SHOW, 0, button_map[i].id);
        
        // NOVO: Pozivanje gettera unutar switch bloka
        // Ovo rješava grešku "incompatible pointer to integer conversion"
        switch (button_map[i].id) {
            case ID_GATE_RELAY_OPEN:     sprintf(buffer, "%d", Gate_GetRelayOpenAddr(handle)); break;
            case ID_GATE_RELAY_CLOSE:    sprintf(buffer, "%d", Gate_GetRelayCloseAddr(handle)); break;
            case ID_GATE_RELAY_PED:      sprintf(buffer, "%d", Gate_GetRelayPedAddr(handle)); break;
            case ID_GATE_RELAY_STOP:     sprintf(buffer, "%d", Gate_GetRelayStopAddr(handle)); break;
            case ID_GATE_FEEDBACK_OPEN:  sprintf(buffer, "%d", Gate_GetFeedbackOpenAddr(handle)); break;
            case ID_GATE_FEEDBACK_CLOSE: sprintf(buffer, "%d", Gate_GetFeedbackCloseAddr(handle)); break;
            case ID_GATE_CYCLE_TIMER:    sprintf(buffer, "%d", Gate_GetCycleTimer(handle)); break;
            case ID_GATE_PED_TIMER:      sprintf(buffer, "%d", Gate_GetPedestrianTimer(handle)); break;
            case ID_GATE_PULSE_TIMER:    sprintf(buffer, "%d", Gate_GetPulseTimer(handle)); break;
            default: sprintf(buffer, "ERR"); break;
        }

        BUTTON_SetText(hGateEditButtons[i], buffer);
        BUTTON_SetFont(hGateEditButtons[i], GUI_FONT_20_1);
    }
    
    // === 4. Kreiranje Navigacionih Dugmadi ===
    hBUTTON_Ok = BUTTON_CreateEx(10, 235, 100, 30, 0, WM_CF_SHOW, 0, ID_Ok);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    hBUTTON_Next = BUTTON_CreateEx(370, 235, 100, 30, 0, WM_CF_SHOW, 0, ID_Next);
    BUTTON_SetText(hBUTTON_Next, "NEXT");

    GUI_MULTIBUF_EndEx(1);
}
/**
 ******************************************************************************
 * @brief       Kreira i inicijalizuje sve GUI widgete za glavni ekran "Čarobnjaka".
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija je centralni dio iscrtavanja za `SCREEN_SCENE_EDIT`.
 * Prvo dohvati podatke za scenu koja se trenutno mijenja
 * (`scene_edit_index`). Zatim iscrtava njen trenutni izgled (ikonicu
 * i naziv) i kreira dugmad za navigaciju: "[ Promijeni ]" za ulazak
 * u mod izmjene izgleda, te "Snimi" i "Otkaži".
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void DSP_InitSceneEditScreen(void)
{
    DSP_KillSceneEditScreen();
    GUI_MULTIBUF_BeginEx(1);
    GUI_Clear();

    Scene_t* scene_handle = Scene_GetInstance(scene_edit_index);
    if (!scene_handle) {
        GUI_SetFont(&GUI_FontVerdana20_LAT);
        GUI_SetColor(GUI_RED);
        GUI_DispStringAt("GRESKA: Scena nije dostupna!", 10, 10);
        GUI_MULTIBUF_EndEx(1);
        return;
    }

    hBUTTON_Ok = BUTTON_CreateEx(370, 230, 100, 35, 0, WM_CF_SHOW, 0, ID_Ok);
    hBUTTON_Next = BUTTON_CreateEx(10, 230, 100, 35, 0, WM_CF_SHOW, 0, ID_Next);
    BUTTON_SetText(hBUTTON_Next, lng(TXT_CANCEL));

    if (scene_handle->is_configured == false)
    {
        BUTTON_SetText(hBUTTON_Ok, lng(TXT_SAVE));
        const SceneAppearance_t* appearance = &scene_appearance_table[scene_handle->appearance_id];
        
        GUI_SetFont(&GUI_FontVerdana20_LAT);
        GUI_SetColor(GUI_WHITE);
        GUI_DispStringAt("Izgled i Naziv Scene:", 10, 10);
        
        const GUI_BITMAP* icon_to_draw = scene_icon_images[appearance->icon_id - ICON_SCENE_WIZZARD];
        GUI_DrawBitmap(icon_to_draw, 15, 40);
        
        GUI_SetFont(&GUI_FontVerdana32_LAT);
        GUI_SetColor(GUI_ORANGE);
        GUI_DispStringAt(lng(appearance->text_id), 100, 70);

        hButtonChangeAppearance = BUTTON_CreateEx(300, 50, 150, 40, 0, WM_CF_SHOW, 0, ID_BUTTON_CHANGE_APPEARANCE);
        BUTTON_SetText(hButtonChangeAppearance, "[ Promijeni ]");
        
        if (scene_handle->appearance_id == 0) {
            WM_DisableWindow(hBUTTON_Ok);
        }
    }
    else
    {
        BUTTON_SetText(hBUTTON_Ok, "Memorisi Stanje");
        const SceneAppearance_t* appearance = &scene_appearance_table[scene_handle->appearance_id];
        
        GUI_SetFont(&GUI_FontVerdana20_LAT);
        GUI_SetColor(GUI_WHITE);
        GUI_DispStringAt("Izgled i Naziv Scene:", 10, 10);

        int scene_icon_index = appearance->icon_id - ICON_SCENE_WIZZARD;
        if (scene_icon_index >= 0 && scene_icon_index < (sizeof(scene_icon_images) / sizeof(scene_icon_images[0])))
        {
            const GUI_BITMAP* icon_to_draw = scene_icon_images[scene_icon_index];
            GUI_DrawBitmap(icon_to_draw, 15, 40);
        }
        
        GUI_SetFont(&GUI_FontVerdana32_LAT);
        GUI_SetColor(GUI_ORANGE);
        GUI_DispStringAt(lng(appearance->text_id), 100, 70);

        hButtonDeleteScene = BUTTON_CreateEx(190, 230, 100, 35, 0, WM_CF_SHOW, 0, ID_BUTTON_DELETE_SCENE);
        BUTTON_SetText(hButtonDeleteScene, lng(TXT_DELETE));
        
        hButtonDetailedSetup = BUTTON_CreateEx(10, 150, 200, 40, 0, WM_CF_SHOW, 0, ID_BUTTON_DETAILED_SETUP);
        BUTTON_SetText(hButtonDetailedSetup, "Detaljna Podesavanja");
    }

    GUI_MULTIBUF_EndEx(1);
}
/**
 ******************************************************************************
 * @brief       Uništava sve GUI widgete kreirane za glavni ekran "Čarobnjaka".
 * @author      Gemini & [Vaše Ime]
 * @note        FINALNA VERZIJA. Ova funkcija sada provjerava i briše sve
 * dinamički kreirane widgete ("Promijeni", "Obriši",
 * "Detaljna Podešavanja") i resetuje njihove handle-ove na nulu,
 * čime se sprečava pojava "duhova" na drugim ekranima.
 ******************************************************************************
 */
static void DSP_KillSceneEditScreen(void)
{
    if (WM_IsWindow(hButtonChangeAppearance)) {
        WM_DeleteWindow(hButtonChangeAppearance);
        hButtonChangeAppearance = 0;
    }
    if (WM_IsWindow(hButtonDeleteScene)) {
        WM_DeleteWindow(hButtonDeleteScene);
        hButtonDeleteScene = 0;
    }
    if (WM_IsWindow(hButtonDetailedSetup)) {
        WM_DeleteWindow(hButtonDetailedSetup);
        hButtonDetailedSetup = 0;
    }
    if (WM_IsWindow(hBUTTON_Ok)) {
        WM_DeleteWindow(hBUTTON_Ok);
        hBUTTON_Ok = 0;
    }
    if (WM_IsWindow(hBUTTON_Next)) {
        WM_DeleteWindow(hBUTTON_Next);
        hBUTTON_Next = 0;
    }
}
/**
 ******************************************************************************
 * @brief       Inicijalizuje i iscrtava ekran za odabir izgleda scene.
 * @author      Gemini & [Vaše Ime]
 * @note        FINALNA VERZIJA 2.1: Funkcija sada sadrži logiku za FILTRIRANJE.
 * Prije iscrtavanja, provjerava sve postojeće scene i prikazuje
 * samo one izglede koji još uvijek nisu iskorišteni, čime se
 * sprečava kreiranje duplih scena sa istim izgledom.
 ******************************************************************************
 */
static void DSP_InitSceneAppearanceScreen(void)
{
    GUI_MULTIBUF_BeginEx(1);
    GUI_Clear();
    
    GUI_SetFont(&GUI_FontVerdana20_LAT);
    GUI_SetColor(GUI_WHITE);
    GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_TOP);
    GUI_DispStringAt("Odaberite Izgled Scene", LCD_GetXSize() / 2, 5);

    // --- KORAK 1: Pronađi sve iskorištene ID-jeve izgleda ---
    uint8_t used_appearance_ids[SCENE_MAX_COUNT] = {0};
    uint8_t used_count = 0;
    for (int i = 0; i < SCENE_MAX_COUNT; i++)
    {
        Scene_t* temp_handle = Scene_GetInstance(i);
        if (temp_handle && temp_handle->is_configured)
        {
            used_appearance_ids[used_count++] = temp_handle->appearance_id;
        }
    }

    // --- KORAK 2: Iscrtaj samo dostupne ikonice ---
    const int ICONS_PER_PAGE = 6;
    int total_available_appearances = 0;
    const SceneAppearance_t* available_appearances[sizeof(scene_appearance_table)/sizeof(SceneAppearance_t)];

    for (int i = 1; i < (sizeof(scene_appearance_table) / sizeof(SceneAppearance_t)); i++) // Počni od 1 da preskočiš wizard
    {
        bool is_used = false;
        for (int j = 0; j < used_count; j++)
        {
            if (used_appearance_ids[j] == i)
            {
                is_used = true;
                break;
            }
        }
        if (!is_used)
        {
            available_appearances[total_available_appearances++] = &scene_appearance_table[i];
        }
    }

    // --- Logika Paginacije (sada radi sa filtriranom listom) ---
    int total_pages = (total_available_appearances + ICONS_PER_PAGE - 1) / ICONS_PER_PAGE;
    if (scene_appearance_page >= total_pages && total_pages > 0) {
        scene_appearance_page = total_pages - 1;
    }

    int start_index = scene_appearance_page * ICONS_PER_PAGE;
    int end_index = start_index + ICONS_PER_PAGE;
    if (end_index > total_available_appearances) {
        end_index = total_available_appearances;
    }

    // Iscrtaj ikonice za trenutnu stranicu
    for (int i = start_index; i < end_index; i++)
    {
        const SceneAppearance_t* appearance = available_appearances[i];
        
        int display_index = i % ICONS_PER_PAGE;
        int row = display_index / scene_screen_layout.items_per_row;
        int col = display_index % scene_screen_layout.items_per_row;
        int x_center = (scene_screen_layout.slot_width / 2) + (col * scene_screen_layout.slot_width);
        int y_center = (scene_screen_layout.slot_height / 2) + (row * scene_screen_layout.slot_height) + 10;

        int scene_icon_index = appearance->icon_id - ICON_SCENE_WIZZARD;
        if (scene_icon_index >= 0 && scene_icon_index < (sizeof(scene_icon_images) / sizeof(scene_icon_images[0])))
        {
            const GUI_BITMAP* icon_to_draw = scene_icon_images[scene_icon_index];
            GUI_DrawBitmap(icon_to_draw, x_center - (icon_to_draw->XSize / 2), y_center - (icon_to_draw->YSize / 2));
        }
        
        GUI_SetFont(&GUI_FontVerdana16_LAT);
        GUI_SetColor(GUI_ORANGE);
        GUI_SetTextAlign(GUI_TA_HCENTER);
        GUI_DispStringAt(lng(appearance->text_id), x_center, y_center + scene_screen_layout.text_y_offset);
    }

    // --- Iscrtavanje "Next" Dugmeta (samo ako ima više stranica) ---
    if (total_pages > 1)
    {
        const GUI_BITMAP* iconNext = &bmnext;
        int x_pos = select_screen2_drawing_layout.next_button_x_pos;
        int y_pos = select_screen2_drawing_layout.next_button_y_center - (iconNext->YSize / 2);
        GUI_DrawBitmap(iconNext, x_pos, y_pos);
    }
    
    GUI_MULTIBUF_EndEx(1);
}
/**
 ******************************************************************************
 * @brief       Kreira i inicijalizuje GUI za prvi korak čarobnjaka: Odabir Uređaja.
 * @author      Gemini & [Vaše Ime]
 * @note        FINALNA VERZIJA 2.0: Funkcija je sada "pametna". Prije iscrtavanja,
 * provjerava koji moduli (svjetla, roletne, termostat) imaju
 * konfigurisane uređaje i dinamički prikazuje checkbox-ove samo
 * za dostupne opcije, čime se korisnički interfejs čini čistijim i
 * relevantnijim.
 ******************************************************************************
 */
static void DSP_InitSceneWizDevicesScreen(void)
{
    DSP_KillSceneEditScreen();
    
    GUI_MULTIBUF_BeginEx(1);
    GUI_Clear();

    Scene_t* scene_handle = Scene_GetInstance(scene_edit_index);
    if (!scene_handle) {
        screen = SCREEN_SCENE_EDIT;
        shouldDrawScreen = 1;
        GUI_MULTIBUF_EndEx(1);
        return;
    }

    // --- Iscrtavanje Naslova i Uputstva ---
    GUI_SetFont(&GUI_FontVerdana20_LAT);
    GUI_SetColor(GUI_WHITE);
    GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_TOP);
    GUI_DispStringAt("Podesavanje Scene (Korak 1)", LCD_GetXSize() / 2, 10);
    
    GUI_SetFont(&GUI_FontVerdana16_LAT);
    GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_TOP); // Ponovni poziv radi pravila biblioteke
    GUI_DispStringAt("Odaberite koje uredjaje zelite ukljuciti:", LCD_GetXSize() / 2, 40);

    // --- KORAK 1: Provjera dostupnosti grupa uređaja ---
    bool lights_available = (LIGHTS_getCount() > 0);
    bool curtains_available = (Curtains_getCount() > 0);
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();
    bool thermostat_available = (Thermostat_GetGroup(pThst) > 0);

    // --- KORAK 2: Dinamičko kreiranje i pozicioniranje Checkbox-ova ---
    const int chkbx_x = 50;
    const int chkbx_w = 200;
    const int chkbx_h = 30;
    const int y_spacing = 40; // Razmak između checkbox-ova
    int16_t current_y = 80;   // Početna Y pozicija

    if (lights_available)
    {
        hCheckboxSceneLights = CHECKBOX_CreateEx(chkbx_x, current_y, chkbx_w, chkbx_h, 0, WM_CF_SHOW, 0, ID_WIZ_CHECKBOX_LIGHTS);
        CHECKBOX_SetText(hCheckboxSceneLights, lng(TXT_LIGHTS));
        CHECKBOX_SetFont(hCheckboxSceneLights, &GUI_FontVerdana20_LAT);
        CHECKBOX_SetTextColor(hCheckboxSceneLights, GUI_WHITE);
        if (scene_handle->lights_mask != 0) {
            CHECKBOX_SetState(hCheckboxSceneLights, 1);
        }
        current_y += y_spacing; // Povećaj Y poziciju za sljedeći element
    }

    if (curtains_available)
    {
        hCheckboxSceneCurtains = CHECKBOX_CreateEx(chkbx_x, current_y, chkbx_w, chkbx_h, 0, WM_CF_SHOW, 0, ID_WIZ_CHECKBOX_CURTAINS);
        CHECKBOX_SetText(hCheckboxSceneCurtains, lng(TXT_BLINDS));
        CHECKBOX_SetFont(hCheckboxSceneCurtains, &GUI_FontVerdana20_LAT);
        CHECKBOX_SetTextColor(hCheckboxSceneCurtains, GUI_WHITE);
        if (scene_handle->curtains_mask != 0) {
            CHECKBOX_SetState(hCheckboxSceneCurtains, 1);
        }
        current_y += y_spacing;
    }

    if (thermostat_available)
    {
        hCheckboxSceneThermostat = CHECKBOX_CreateEx(chkbx_x, current_y, chkbx_w, chkbx_h, 0, WM_CF_SHOW, 0, ID_WIZ_CHECKBOX_THERMOSTAT);
        CHECKBOX_SetText(hCheckboxSceneThermostat, lng(TXT_THERMOSTAT));
        CHECKBOX_SetFont(hCheckboxSceneThermostat, &GUI_FontVerdana20_LAT);
        CHECKBOX_SetTextColor(hCheckboxSceneThermostat, GUI_WHITE);
        if (scene_handle->thermostat_mask != 0) {
            CHECKBOX_SetState(hCheckboxSceneThermostat, 1);
        }
    }

    // --- Kreiranje Navigacionih Dugmadi ---
    hButtonWizCancel = BUTTON_CreateEx(10, 230, 100, 35, 0, WM_CF_SHOW, 0, ID_WIZ_CANCEL);
    BUTTON_SetText(hButtonWizCancel, lng(TXT_CANCEL));

    hButtonWizBack = BUTTON_CreateEx(190, 230, 100, 35, 0, WM_CF_SHOW, 0, ID_WIZ_BACK);
    BUTTON_SetText(hButtonWizBack, "Nazad");

    hButtonWizNext = BUTTON_CreateEx(370, 230, 100, 35, 0, WM_CF_SHOW, 0, ID_WIZ_NEXT);
    BUTTON_SetText(hButtonWizNext, "Dalje");
    
    GUI_MULTIBUF_EndEx(1);
}
/**
 ******************************************************************************
 * @brief       Uništava sve GUI widgete kreirane za ekran odabira uređaja u čarobnjaku.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva prilikom napuštanja ekrana
 * `SCREEN_SCENE_WIZ_DEVICES`. Odgovorna je za brisanje svih dinamički
 * kreiranih widgeta (checkbox-ova i dugmadi) kako bi se oslobodili
 * resursi i spriječilo curenje memorije.
 ******************************************************************************
 */
static void DSP_KillSceneWizDevicesScreen(void)
{
    // Provjeri da li widget postoji prije brisanja da se izbjegne greška
    if (WM_IsWindow(hCheckboxSceneLights)) {
        WM_DeleteWindow(hCheckboxSceneLights);
        hCheckboxSceneLights = 0; // Resetuj handle na nulu
    }
    if (WM_IsWindow(hCheckboxSceneCurtains)) {
        WM_DeleteWindow(hCheckboxSceneCurtains);
        hCheckboxSceneCurtains = 0;
    }
    if (WM_IsWindow(hCheckboxSceneThermostat)) {
        WM_DeleteWindow(hCheckboxSceneThermostat);
        hCheckboxSceneThermostat = 0;
    }
    if (WM_IsWindow(hButtonWizCancel)) {
        WM_DeleteWindow(hButtonWizCancel);
        hButtonWizCancel = 0;
    }
    if (WM_IsWindow(hButtonWizBack)) {
        WM_DeleteWindow(hButtonWizBack);
        hButtonWizBack = 0;
    }
    if (WM_IsWindow(hButtonWizNext)) {
        WM_DeleteWindow(hButtonWizNext);
        hButtonWizNext = 0;
    }
}
/**
 * @brief       Uništava sve GUI widgete kreirane za ekran podešavanja kapija.
 * @author      Gemini (po specifikaciji korisnika)
 * @param       None
 * @retval      None
 */
static void DSP_KillSettingsGateScreen(void)
{
    WM_DeleteWindow(hGateSelect);
    WM_DeleteWindow(hGateType);
    for (int i = 0; i < 9; i++) {
        if (WM_IsWindow(hGateEditButtons[i])) {
            WM_DeleteWindow(hGateEditButtons[i]);
            hGateEditButtons[i] = 0;
        }
    }
    WM_DeleteWindow(hBUTTON_Ok);
    WM_DeleteWindow(hBUTTON_Next);
}

/**
 * @brief       Uništava sve GUI widgete kreirane za ekran podešavanja svjetla.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ova nova funkcija je neophodna za brisanje dugmeta za promjenu
 * imena prilikom izlaska sa ekrana.
 * @param       None
 * @retval      None
 */
static void DSP_KillLightSettingsScreen(void)
{
    if (WM_IsWindow(hButtonRenameLight)) {
        WM_DeleteWindow(hButtonRenameLight);
        hButtonRenameLight = 0;
    }
}
/**
 ******************************************************************************
 * @brief       Uništava sve GUI widgete kreirane za glavni ekran sa scenama.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva prilikom napuštanja ekrana `SCREEN_SCENE`.
 * Pošto ovaj ekran nema dinamički kreirane widgete, njena uloga je
 * primarno da formalno postoji unutar "Init -> Service -> Kill" paterna
 * i da obriše ekran.
 ******************************************************************************
 */
static void DSP_KillSceneScreen(void)
{
    // Trenutno ovaj ekran nema dinamičke widgete, pa samo čistimo ekran.
    // Ako bismo u budućnosti dodali npr. dugmad, ovdje bi išao njihov WM_DeleteWindow poziv.
    GUI_Clear();
}
/**
 ******************************************************************************
 * @brief       Uništava sve GUI elemente sa ekrana za odabir izgleda scene.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva prilikom napuštanja ekrana
 * `SCREEN_SCENE_APPEARANCE`. S obzirom da ovaj ekran ne koristi
 * dinamički kreirane widgete sa handle-ovima, već samo direktne
 * funkcije za iscrtavanje, njen zadatak je da obriše ekran.
 ******************************************************************************
 */
static void DSP_KillSceneAppearanceScreen(void)
{
    GUI_Clear(); // Briše kompletan sadržaj aktivnog sloja (layer-a)
}
/**
 ******************************************************************************
 * @brief       Uništava widgete kreirane od strane `Service_SceneEditLightsScreen`.
 * @author      Gemini & [Vaše Ime]
 * @note        Specifično briše "[Next]" dugme kreirano za wizard mod.
 ******************************************************************************
 */
static void DSP_KillSceneEditLightsScreen(void)
{
    if (WM_IsWindow(hButtonWizNext)) {
        WM_DeleteWindow(hButtonWizNext);
        hButtonWizNext = 0;
    }
}
/**
 ******************************************************************************
 * @brief       Uništava widgete kreirane od strane `Service_SceneEditCurtainsScreen`.
 * @author      Gemini & [Vaše Ime]
 * @note        Specifično briše "[Next]" dugme kreirano za wizard mod na ekranu
 * za roletne, osiguravajući čistu navigaciju.
 ******************************************************************************
 */
static void DSP_KillSceneEditCurtainsScreen(void)
{
    if (WM_IsWindow(hButtonWizNext)) {
        WM_DeleteWindow(hButtonWizNext);
        hButtonWizNext = 0;
    }
}
/**
 ******************************************************************************
 * @brief       Uništava widgete kreirane od strane `Service_SceneEditThermostatScreen`.
 * @author      Gemini & [Vaše Ime]
 * @note        Specifično briše "[Next]" dugme kreirano za wizard mod na ekranu
 * za termostat.
 ******************************************************************************
 */
static void DSP_KillSceneEditThermostatScreen(void)
{
    if (WM_IsWindow(hButtonWizNext)) {
        WM_DeleteWindow(hButtonWizNext);
        hButtonWizNext = 0;
    }
}
/**
 ******************************************************************************
 * @brief       Kreira GUI za finalni ekran čarobnjaka.
 * @note        Prikazuje poruku za potvrdu i dugmad "Snimi Scenu" i "Otkaži".
 ******************************************************************************
 */
static void DSP_InitSceneWizFinalizeScreen(void)
{
    GUI_MULTIBUF_BeginEx(1);
    GUI_Clear();
    
    char buffer[100];
    Scene_t* scene_handle = Scene_GetInstance(scene_edit_index);
    const SceneAppearance_t* appearance = &scene_appearance_table[scene_handle->appearance_id];

    sprintf(buffer, "Scena '%s' je konfigurisana.", lng(appearance->text_id));

//    DSP_DrawWizardScreenHeader("Zavrsetak Podesavanja", buffer);

    // Kreiramo samo "Snimi" i "Otkaži" dugmad
    hButtonWizCancel = BUTTON_CreateEx(10, 230, 120, 35, 0, WM_CF_SHOW, 0, ID_WIZ_CANCEL);
    BUTTON_SetText(hButtonWizCancel, lng(TXT_CANCEL));

    hBUTTON_Ok = BUTTON_CreateEx(350, 230, 120, 35, 0, WM_CF_SHOW, 0, ID_Ok);
    BUTTON_SetText(hBUTTON_Ok, "Snimi Scenu");

    GUI_MULTIBUF_EndEx(1);
}

/**
 ******************************************************************************
 * @brief       Uništava widgete sa finalnog ekrana čarobnjaka.
 ******************************************************************************
 */
static void DSP_KillSceneWizFinalizeScreen(void)
{
    if (WM_IsWindow(hButtonWizCancel)) WM_DeleteWindow(hButtonWizCancel);
    if (WM_IsWindow(hBUTTON_Ok)) WM_DeleteWindow(hBUTTON_Ok);
}
/**
 ******************************************************************************
 * @brief       Upravlja svim periodičnim događajima i tajmerima.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija je srž logike za pozadinske procese koji se ne
 * odnose direktno na trenutni ekran. Uključuje logiku
 * screensaver-a, tajmera za automatsko paljenje svjetala,
 * detekciju dugog pritiska i ažuriranje vremena. Refaktorisana
 * je da koristi isključivo javne API-je drugih modula.
 ******************************************************************************
 */
static void Handle_PeriodicEvents(void)
{
    #define LONG_PRESS_DURATION 1000 // Prag za dugi pritisak u ms (1 sekunda)
    if (scene_press_timer_start != 0 && (HAL_GetTick() - scene_press_timer_start) > LONG_PRESS_DURATION)
    {
        uint8_t configured_scenes_count = Scene_GetCount();
        
        if (scene_pressed_index != -1 && scene_pressed_index < configured_scenes_count)
        {
            uint8_t scene_counter = 0;
            for (int i = 0; i < SCENE_MAX_COUNT; i++)
            {
                Scene_t* temp_handle = Scene_GetInstance(i);
                if (temp_handle && temp_handle->is_configured)
                {
                    if (scene_counter == scene_pressed_index)
                    {
                        scene_edit_index = i;
                        break;
                    }
                    scene_counter++;
                }
            }
            
            // --- ISPRAVNA NAVIGACIJA ---
            DSP_KillSceneScreen();          // 1. Ubij stari ekran
            DSP_InitSceneEditScreen();      // 2. Inicijalizuj novi ekran
            screen = SCREEN_SCENE_EDIT;     // 3. Postavi novo stanje
            shouldDrawScreen = 0;           // 4. Ne treba ponovo crtati
            
            scene_press_timer_start = 0;
            scene_pressed_index = -1;
        }
    }
    // === KRAJ NOVOG BLOKA ===
    if (rename_light_timer_start && (HAL_GetTick() - rename_light_timer_start) >= 2000)
    {
        rename_light_timer_start = 0; // Resetuj tajmer
        
        LIGHT_Handle* handle = (light_selectedIndex < LIGHTS_MODBUS_SIZE) ? LIGHTS_GetInstance(light_selectedIndex) : NULL;
        if(handle) {
            // Pripremi kontekst za tastaturu
            KeyboardContext_t kbd_context = {
                .title = lng(TXT_ENTER_NEW_NAME),
                .max_len = 20
            };
            strncpy(kbd_context.initial_value, LIGHT_GetCustomLabel(handle), sizeof(kbd_context.initial_value) - 1);
            
            // Postavi "povratnu adresu"
            keyboard_return_screen = screen;
            // Kopiraj kontekst u globalnu varijablu
            memcpy(&g_keyboard_context, &kbd_context, sizeof(KeyboardContext_t));
            // Resetuj rezultat
            memset(&g_keyboard_result, 0, sizeof(KeyboardResult_t));
            keyboard_shift_active = false;
            
            // Postavi novi ekran
            screen = SCREEN_KEYBOARD_ALPHA;
            
            // === FORSIRANO ISCRTAVANJE ODMAH ===
            DSP_KillLightSettingsScreen();
            DSP_InitKeyboardScreen();
            // Postavi fleg da se izbjegne duplo iscrtavanje u DISP_Service
            shouldDrawScreen = 0; 
        }
        return; // Prekini dalje izvršavanje u ovoj funkciji
    }
    // === "FAIL-SAFE" SKENER ZA DUHOVE (Ovaj dio ostaje nepromijenjen) ===
    static uint32_t ghost_widget_scan_timer = 0;
    if ((HAL_GetTick() - ghost_widget_scan_timer) >= GHOST_WIDGET_SCAN_INTERVAL) {
        ghost_widget_scan_timer = HAL_GetTick();
        // Ažurirano da koristi novi `SCREEN_SELECT_LAST`
        if (screen == SCREEN_MAIN || screen == SCREEN_SELECT_1 || screen == SCREEN_SELECT_LAST) {
            ForceKillAllSettingsWidgets();
        }
    }

    // === TAJMER ZA AUTOMATSKO PALJENJE SVJETALA (SVAKE MINUTE) ===
    if (IsRtcTimeValid() && (HAL_GetTick() - everyMinuteTimerStart) >= (60 * 1000)) {
        everyMinuteTimerStart = HAL_GetTick();

        // Refaktorisana petlja koja koristi novi API
        RTC_TimeTypeDef currentTime;
        HAL_RTC_GetTime(&hrtc, &currentTime, RTC_FORMAT_BCD);
        uint8_t currentHour = Bcd2Dec(currentTime.Hours);
        uint8_t currentMinute = Bcd2Dec(currentTime.Minutes);

        for (uint8_t i = 0; i < LIGHTS_getCount(); i++) {
            LIGHT_Handle* handle = LIGHTS_GetInstance(i);
            if (handle) {
                if (LIGHT_GetOnHour(handle) != -1) {
                    if ((LIGHT_GetOnHour(handle) == currentHour) && (LIGHT_GetOnMinute(handle) == currentMinute)) {
                        LIGHT_SetState(handle, true);
                        // Logika za ažuriranje ekrana ostaje ista
                        if (screen == SCREEN_LIGHTS) {
                            shouldDrawScreen = 1;
                        } else if((screen == SCREEN_RESET_MENU_SWITCHES) || (screen == SCREEN_MAIN)) {
                            screen = SCREEN_RETURN_TO_FIRST;
                        }
                    }
                }
            }
        }
    }

    // === TAJMER ZA ULAZAK U MOD PODEŠAVANJA (Ovaj dio ostaje nepromijenjen) ===
    if (light_settingsTimerStart && ((HAL_GetTick() - light_settingsTimerStart) >= (2 * 1000))) {
        light_settingsTimerStart = 0;
        light_settings_return_screen = screen;
        screen = SCREEN_LIGHT_SETTINGS;
        shouldDrawScreen = 1;
    }

    // === SCREENSAVER TAJMER ===
    if (!IsScrnsvrActiv()) {
        if ((HAL_GetTick() - scrnsvr_tmr) >= (uint32_t)(g_display_settings.scrnsvr_tout * 1000)) {
            // =======================================================================
            // ===          NOVI BLOK: Provjera i gašenje Čarobnjaka prvo         ===
            // =======================================================================
            if (is_in_scene_wizard_mode)
            {
                // Uništi widgete sa bilo kog ekrana koji je dio čarobnjaka
                switch (screen)
                {
                    case SCREEN_SCENE_EDIT:
                        DSP_KillSceneEditScreen();
                        break;
                    case SCREEN_SCENE_APPEARANCE:
                        DSP_KillSceneAppearanceScreen();
                        break;
                    case SCREEN_SCENE_WIZ_DEVICES:
                        DSP_KillSceneWizDevicesScreen();
                        break;
                    case SCREEN_LIGHTS:
                    case SCREEN_CURTAINS:
                    case SCREEN_THERMOSTAT:
                        // Za ove ekrane, dovoljno je obrisati [Next] dugme
                        if (WM_IsWindow(hButtonWizNext)) {
                            WM_DeleteWindow(hButtonWizNext);
                            hButtonWizNext = 0;
                        }
                        break;
                    // TODO: Dodati Kill funkcije za ostale WIZ ekrane kada budu kreirane
                    default:
                        break;
                }
            
                // Ključni korak: Resetuj fleg za Wizard Mode
                is_in_scene_wizard_mode = false;
            }
            // =======================================================================
            // ===        Originalna logika za ekrane podešavanja ostaje         ===
            // =======================================================================
            else if (screen == SCREEN_SETTINGS_1)      DSP_KillSet1Scrn();
            else if (screen == SCREEN_SETTINGS_2)      DSP_KillSet2Scrn();
            else if (screen == SCREEN_SETTINGS_3)      DSP_KillSet3Scrn();
            else if (screen == SCREEN_SETTINGS_4)      DSP_KillSet4Scrn();
            else if (screen == SCREEN_SETTINGS_5)      DSP_KillSet5Scrn();
            else if (screen == SCREEN_SETTINGS_6)      DSP_KillSet6Scrn();
            else if (screen == SCREEN_SETTINGS_7)      DSP_KillSet7Scrn();
            else if (screen == SCREEN_SETTINGS_GATE)   DSP_KillSettingsGateScreen();
            else if (screen == SCREEN_LIGHT_SETTINGS)  DSP_KillLightSettingsScreen();

            // Aktivacija screensaver-a (ostaje isto)
            DISPSetBrightnes(g_display_settings.low_bcklght);
            ScrnsvrInitReset();
            ScrnsvrSet();
            screen = SCREEN_RETURN_TO_FIRST;
        }
    }

    // === AŽURIRANJE VREMENA NA EKRANU ===
    if (HAL_GetTick() - rtctmr >= 1000) {
        rtctmr = HAL_GetTick();
        if(++refresh_tmr > 10) {
            refresh_tmr = 0;
            if (!IsScrnsvrActiv()) MVUpdateSet();
        }
        // Poziv za iscrtavanje sata (logika će biti dorađena kako smo diskutovali)
        if (screen < SCREEN_SELECT_1) DISPDateTime();
    }
}
/**
 ******************************************************************************
 * @brief       Centralni dispečer za obradu događaja PRITISKA na ekran.
 * @author      [Original Author] & Gemini
 * @note        Ova funkcija se poziva iz `PID_Hook`. Njena uloga je da, na osnovu
 * trenutnog ekrana, pozove odgovarajuću `HandlePress_...` funkciju.
 * Za `SCREEN_MAIN`, poziva logiku za menije i pamti koordinate pritiska.
 * @param       pTS Pokazivač na strukturu sa stanjem dodira.
 * @param       click_flag Pokazivač na fleg za zvučni signal.
 ******************************************************************************
 */
static void HandleTouchPressEvent(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    if     (screen == SCREEN_MAIN){
        // Provjera 1: Da li je dodir u DONJEM LIJEVOM meniju za scene?
        if (g_display_settings.scenes_enabled && (pTS->x < 80 && pTS->y > 192))
        {
            *click_flag = 1;
            // Očisti ekran prije prelaska
            GUI_SelectLayer(0);
            GUI_Clear();
            GUI_SelectLayer(1);
            GUI_Clear();
            
            // Prebaci direktno na ekran sa scenama
            screen = SCREEN_SCENE;
            shouldDrawScreen = 1;
        }
        // Provjera 2: Da li je dodir unutar CENTRALNOG GLAVNOG PREKIDAČA?
        else if (pTS->x >= reset_menu_switches_layout.main_switch_zone.x0 &&
                 pTS->x <  reset_menu_switches_layout.main_switch_zone.x1 &&
                 pTS->y >= reset_menu_switches_layout.main_switch_zone.y0 &&
                 pTS->y <  reset_menu_switches_layout.main_switch_zone.y1)
        {
            *click_flag = 1;
            // Tek sada, kada smo sigurni da je pritisak u pravoj zoni,
            // pozivamo funkciju koja pokreće tajmer za dugi pritisak.
            HandlePress_MainScreenSwitch(pTS);
        }
        
        // Ako dodir nije bio ni u jednoj od ove dvije zone, ne radi se ništa.
        // Onemogućen je `else` blok koji je uzrokovao grešku.
        
        // Uvijek sačuvaj koordinate pritiska za kasniju provjeru pri otpuštanju
        memcpy(&last_press_state, pTS, sizeof(GUI_PID_STATE));
    }
    else if(screen == SCREEN_SELECT_1) {
        HandlePress_SelectScreen1(pTS, click_flag);
    }
    else if(screen == SCREEN_SELECT_2) {
        HandlePress_SelectScreen2(pTS, click_flag);
    }
    else if(screen == SCREEN_SELECT_LAST) {
        HandlePress_SelectScreenLast(pTS, click_flag);
    }
    else if(screen == SCREEN_THERMOSTAT) {
        HandlePress_ThermostatScreen(pTS, click_flag);
    }
    else if(screen == SCREEN_LIGHTS) {
        HandlePress_LightsScreen(pTS, click_flag);
    }
    else if(screen == SCREEN_CURTAINS) {
        HandlePress_CurtainsScreen(pTS, click_flag);
    }
    else if(screen == SCREEN_SCENE) {
        HandlePress_SceneScreen(pTS, click_flag);
    }
    else if(screen == SCREEN_LIGHT_SETTINGS) {
        HandlePress_LightSettingsScreen(pTS);
    }
    else if(screen == SCREEN_SCENE_APPEARANCE) {
        HandlePress_SceneAppearanceScreen(pTS, click_flag);
    }
}

/**
 ******************************************************************************
 * @brief       Obrađuje događaj otpuštanja dodira sa ekrana, u zavisnosti od ekrana.
 * @author      [Original Author] & Gemini
 * @note        Ova funkcija je centralna tačka za logiku koja se dešava nakon
 * što korisnik podigne prst sa ekrana. Sadrži originalnu, kompletnu
 * logiku za sve ekrane, sa dodatkom provjere zone za glavni prekidač.
 * @param       pTS Pokazivač na strukturu sa stanjem dodira.
 ******************************************************************************
 */
static void HandleTouchReleaseEvent(GUI_PID_STATE * pTS)
{
     // Provjera 1: Da li je otpuštanje prsta rezultat uspješnog dugog pritiska?
    // Ako je tastatura već aktivirana, 'screen' varijabla će biti SCREEN_KEYBOARD_ALPHA.
    if (screen == SCREEN_KEYBOARD_ALPHA)
    {
        // U ovom slučaju, dugi pritisak je uspio i ekran se već mijenja.
        // Jedino što treba da uradimo je da resetujemo tajmer i izađemo.
        rename_light_timer_start = 0;
        return;
    }

    // Ako NIJE bio uspješan dugi pritisak, nastavljamo sa standardnom logikom.

    // Poništi tajmer za dugi pritisak (za slučaj kratkog pritiska)
    rename_light_timer_start = 0; 
    
    if (LIGHTS_IsNightTimerActive()) {
        LIGHTS_StopNightTimer();
    }
    // =======================================================================
    // === POČETAK IZMJENE: Logika za MAIN_SCREEN sa provjerom zone ===
    // =======================================================================
    if(screen == SCREEN_MAIN && !touch_in_menu_zone) {
        // Provjerava se da li je POČETNI PRITISAK bio unutar zone
        // definisane u strukturi `reset_menu_switches_layout`.
        if (last_press_state.x >= reset_menu_switches_layout.main_switch_zone.x0 &&
            last_press_state.x <  reset_menu_switches_layout.main_switch_zone.x1 &&
            last_press_state.y >= reset_menu_switches_layout.main_switch_zone.y0 &&
            last_press_state.y <  reset_menu_switches_layout.main_switch_zone.y1)
        {
            HandleRelease_MainScreenSwitch(pTS);
        }
    }
    // =======================================================================
    // === KRAJ IZMJENE (Ostatak funkcije je Vaš originalni, sačuvani kod) ===
    // =======================================================================
    else if(screen == SCREEN_LIGHTS) {
        if(light_selectedIndex < LIGHTS_MODBUS_SIZE) {
            LIGHT_Handle* handle = LIGHTS_GetInstance(light_selectedIndex);
            if (handle) {
                if (!LIGHT_isBinary(handle)) {
                    if ((HAL_GetTick() - light_settingsTimerStart) < 2000) {
                        LIGHT_Flip(handle);
                    }
                } else {
                    LIGHT_Flip(handle);
                }
            }
        }
        light_settingsTimerStart = 0;
        light_selectedIndex = LIGHTS_MODBUS_SIZE + 1;
    }
    else if(screen == SCREEN_RESET_MENU_SWITCHES) {
        HandleRelease_MainScreenSwitch(pTS);
    }
    else if(screen == SCREEN_SCENE) {
        HandleRelease_SceneScreen();
    }
    // Resetovanje svih opštih kontrolnih flegova.
    btnset = 0;
    btndec = 0U;
    btninc = 0U;
    thermostatOnOffTouch_timer = 0;
    
    // Resetovanje `last_press_state` da se izbjegne ponovna aktivacija
    memset(&last_press_state, 0, sizeof(GUI_PID_STATE));
}
/**
 * @brief Obrada događaja pritiska za ekran "Control Select".
 * @note  Verzija 2.3.3: Ispravljena greška gdje nisu bili inicijalizovani
 * handle-ovi za module, što je sprečavalo ispravan rad funkcije.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 * @param click_flag Pokazivač na fleg koji se postavlja ako treba generisati zvučni signal.
 */
static void HandlePress_SelectScreen1(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    // << ISPRAVKA: Dodata inicijalizacija handle-ova koja je nedostajala. >>
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();
    Defroster_Handle* defHandle = Defroster_GetInstance();
    Ventilator_Handle* ventHandle = Ventilator_GetInstance();

    // Ponovo radimo detekciju da bismo znali koji su moduli TRENUTNO prikazani
    // --- KORAK 1: Detekcija aktivnih modula (identično kao u Service_... funkciji) ---
    typedef struct {
        const GUI_BITMAP* icon;
        TextID text_id;
        eScreen target_screen;
        bool is_dynamic_toggle; // Fleg za dinamičku ikonu
    } DynamicMenuItem;

    DynamicMenuItem all_modules[] = {
        { &bmSijalicaOff, TXT_LIGHTS, SCREEN_LIGHTS, false },
        { &bmTermometar, TXT_THERMOSTAT, SCREEN_THERMOSTAT, false },
        { &bmblindMedium, TXT_BLINDS, SCREEN_CURTAINS, false },
        { NULL, TXT_DUMMY, SCREEN_SELECT_1, true } // Mjesto za dinamičku ikonu
    };

    DynamicMenuItem active_modules[4];
    uint8_t active_modules_count = 0;

    if (LIGHTS_getCount() > 0) active_modules[active_modules_count++] = all_modules[0];
    if (Thermostat_GetGroup(pThst) > 0) active_modules[active_modules_count++] = all_modules[1];
    if (Curtains_getCount() > 0) active_modules[active_modules_count++] = all_modules[2];

    if (g_display_settings.selected_control_mode == MODE_DEFROSTER && Defroster_getPin(defHandle) > 0) {
        active_modules[active_modules_count++] = all_modules[3];
    } else if (g_display_settings.selected_control_mode == MODE_VENTILATOR && (Ventilator_getRelay(ventHandle) > 0 || Ventilator_getLocalPin(ventHandle) > 0)) {
        active_modules[active_modules_count++] = all_modules[3];
    }

    // --- KORAK 2: Dinamičko računanje zona dodira i provjera ---
    TouchZone_t zone;
    bool touched = false;

    switch (active_modules_count) {
    case 1:
        zone.x0 = 0;
        zone.y0 = 0;
        zone.x1 = DRAWING_AREA_WIDTH;
        zone.y1 = LCD_GetYSize();
        if (pTS->x >= zone.x0 && pTS->x < zone.x1 && pTS->y >= zone.y0 && pTS->y < zone.y1) {
            screen = active_modules[0].target_screen;
            touched = true;
        }
        break;
    case 2:
        for (int i = 0; i < 2; i++) {
            zone.x0 = (DRAWING_AREA_WIDTH / 2) * i;
            zone.y0 = 0;
            zone.x1 = zone.x0 + (DRAWING_AREA_WIDTH / 2);
            zone.y1 = LCD_GetYSize();
            if (pTS->x >= zone.x0 && pTS->x < zone.x1 && pTS->y >= zone.y0 && pTS->y < zone.y1) {
                screen = active_modules[i].target_screen;
                touched = true;
                break;
            }
        }
        break;
    case 3:
        for (int i = 0; i < 3; i++) {
            zone.x0 = (DRAWING_AREA_WIDTH / 3) * i;
            zone.y0 = 0;
            zone.x1 = zone.x0 + (DRAWING_AREA_WIDTH / 3);
            zone.y1 = LCD_GetYSize();
            if (pTS->x >= zone.x0 && pTS->x < zone.x1 && pTS->y >= zone.y0 && pTS->y < zone.y1) {
                screen = active_modules[i].target_screen;
                touched = true;
                break;
            }
        }
        break;
    case 4:
    default:
        for (int i = 0; i < 4; i++) {
            zone.x0 = (i % 2 == 0) ? 0 : DRAWING_AREA_WIDTH / 2;
            zone.y0 = (i < 2) ? 0 : LCD_GetYSize() / 2;
            zone.x1 = zone.x0 + (DRAWING_AREA_WIDTH / 2);
            zone.y1 = zone.y0 + (LCD_GetYSize() / 2);
            if (pTS->x >= zone.x0 && pTS->x < zone.x1 && pTS->y >= zone.y0 && pTS->y < zone.y1) {
                screen = active_modules[i].target_screen;
                touched = true;
                break;
            }
        }
        break;
    }

    // Provjera dodira na "NEXT" dugme
    if (!touched && (pTS->x >= 400 && pTS->x < 480)) {
        screen = SCREEN_SELECT_2;
        touched = true;
    }

    // Ako je dodir registrovan, postavi flegove
    if (touched) {
        // Ako je pritisnuta dinamička ikona, uradi toggle
        if (screen == SCREEN_SELECT_1) {
            if (g_display_settings.selected_control_mode == MODE_DEFROSTER) {
                if(Defroster_isActive(defHandle)) Defroster_Off(defHandle);
                else Defroster_On(defHandle);
                dynamicIconUpdateFlag = 1;
            } else if (g_display_settings.selected_control_mode == MODE_VENTILATOR) {
                if(Ventilator_isActive(ventHandle)) Ventilator_Off(ventHandle);
                else Ventilator_On(ventHandle, false);
                dynamicIconUpdateFlag = 1;
            }
        }
        // Ako je pritisnuta roletna, resetuj selekciju na "SVE"
        else if (screen == SCREEN_CURTAINS) {
            Curtain_ResetSelection();
        }

        shouldDrawScreen = 1;
        *click_flag = 1;
    }
}
/**
 ******************************************************************************
 * @brief       Obrađuje događaj pritiska za drugi ekran za odabir (SCREEN_SELECT_2).
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija implementira "pametnu" logiku detekcije dodira. Prvo
 * provjerava koji su moduli (Kapija, Tajmer, itd.) aktivni, a zatim
 * dinamički izračunava zone dodira na osnovu broja prikazanih ikonica.
 * Ovo osigurava da logika ispravno radi bez obzira na konfiguraciju
 * sistema (sa 1, 2, 3 ili 4 ikonice).
 * @param       pTS Pokazivač na strukturu sa stanjem dodira.
 * @param       click_flag Pokazivač na fleg koji se postavlja ako treba
 * generisati zvučni signal.
 ******************************************************************************
 */
static void HandlePress_SelectScreen2(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    // === KORAK 1: Ponovna detekcija aktivnih modula (mora odgovarati logici iscrtavanja) ===
    typedef struct {
        eScreen target_screen;
        bool is_direct_action; // Fleg za multi-funkciju
    } DynamicMenuItem;
    
    DynamicMenuItem active_modules[4];
    uint8_t active_modules_count = 0;

    // TODO: Ovdje će ići provjere da li su moduli konfigurisani u postavkama.
    // Za sada, radi razvoja GUI-ja, pretpostavljamo da su svi aktivni.
    bool is_gate_active = true;     // (Gate_IsConfigured())
    bool is_timer_active = true;    // (Timer_IsConfigured())
    bool is_security_active = true; // (Security_IsConfigured())
    bool is_multi_active = true;    // (Multi_IsConfigured())

    if (is_gate_active)     active_modules[active_modules_count++] = (DynamicMenuItem){SCREEN_GATE, false};
    if (is_timer_active)    active_modules[active_modules_count++] = (DynamicMenuItem){SCREEN_TIMER, false};
    if (is_security_active) active_modules[active_modules_count++] = (DynamicMenuItem){SCREEN_SECURITY, false};
    if (is_multi_active)    active_modules[active_modules_count++] = (DynamicMenuItem){SCREEN_SELECT_2, true}; // Ostaje na istom ekranu
    
    // === KORAK 2: Dinamičko računanje zona dodira i provjera ===
    TouchZone_t zone;
    bool touched = false;

    switch (active_modules_count)
    {
        case 1:
            zone = (TouchZone_t){ .x0 = 0, .y0 = 0, .x1 = DRAWING_AREA_WIDTH, .y1 = LCD_GetYSize() };
            if (pTS->x >= zone.x0 && pTS->x < zone.x1 && pTS->y >= zone.y0 && pTS->y < zone.y1) {
                if (!active_modules[0].is_direct_action) screen = active_modules[0].target_screen;
                touched = true;
            }
            break;
        
        case 2:
            for (int i = 0; i < 2; i++) {
                zone = (TouchZone_t){ .x0 = (DRAWING_AREA_WIDTH / 2) * i, .y0 = 0, .x1 = (DRAWING_AREA_WIDTH / 2) * (i + 1), .y1 = LCD_GetYSize() };
                if (pTS->x >= zone.x0 && pTS->x < zone.x1 && pTS->y >= zone.y0 && pTS->y < zone.y1) {
                    if (!active_modules[i].is_direct_action) screen = active_modules[i].target_screen;
                    touched = true;
                    break;
                }
            }
            break;

        case 3:
            for (int i = 0; i < 3; i++) {
                zone = (TouchZone_t){ .x0 = (DRAWING_AREA_WIDTH / 3) * i, .y0 = 0, .x1 = (DRAWING_AREA_WIDTH / 3) * (i + 1), .y1 = LCD_GetYSize() };
                if (pTS->x >= zone.x0 && pTS->x < zone.x1 && pTS->y >= zone.y0 && pTS->y < zone.y1) {
                    if (!active_modules[i].is_direct_action) screen = active_modules[i].target_screen;
                    touched = true;
                    break;
                }
            }
            break;
        
        case 4:
        default: // Podrazumijevani slučaj je 2x2 mreža
            for (int i = 0; i < 4; i++) {
                zone = (TouchZone_t){ 
                    .x0 = (i % 2 == 0) ? 0 : DRAWING_AREA_WIDTH / 2, 
                    .y0 = (i < 2) ? 0 : LCD_GetYSize() / 2, 
                    .x1 = (i % 2 == 0) ? DRAWING_AREA_WIDTH / 2 : DRAWING_AREA_WIDTH, 
                    .y1 = (i < 2) ? LCD_GetYSize() / 2 : LCD_GetYSize()
                };
                if (pTS->x >= zone.x0 && pTS->x < zone.x1 && pTS->y >= zone.y0 && pTS->y < zone.y1) {
                    if (!active_modules[i].is_direct_action) screen = active_modules[i].target_screen;
                    touched = true;
                    break;
                }
            }
            break;
    }

    // Provjera dodira na "NEXT" dugme
    if (!touched && (pTS->x >= select_screen2_drawing_layout.next_button_zone.x0 && pTS->x < select_screen2_drawing_layout.next_button_zone.x1)) {
        screen = SCREEN_SELECT_LAST;
        touched = true;
    }

    if (touched) {
        shouldDrawScreen = 1;
        *click_flag = 1;
    }
}
/**
 ******************************************************************************
 * @brief       Obrađuje događaj pritiska za posljednji ekran za odabir.
 * @author      [Original Author] & Gemini
 * @note        FINALNA ISPRAVLJENA VERZIJA. Logika je restrukturirana da
 * ispravno rukuje specijalnim slučajem poziva PIN tastature. Nakon
 * poziva `Display_ShowNumpad`, funkcija se odmah završava (`return`)
 * kako bi se spriječilo pogrešno vraćanje na prethodni ekran.
 * @param       pTS Pokazivač na strukturu sa stanjem dodira.
 * @param       click_flag Pokazivač na fleg za zvučni signal.
 ******************************************************************************
 */
static void HandlePress_SelectScreenLast(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    // Provjera dodira na zonu za ČIŠĆENJE
    if (pTS->x >= select_screen2_drawing_layout.clean_zone.x0 && pTS->x < select_screen2_drawing_layout.clean_zone.x1 &&
            pTS->y >= select_screen2_drawing_layout.clean_zone.y0 && pTS->y < select_screen2_drawing_layout.clean_zone.y1) {
        screen = SCREEN_CLEAN;
        shouldDrawScreen = 1;
        *click_flag = 1;
    }
    // Provjera dodira na zonu za WIFI
    else if (pTS->x >= select_screen2_drawing_layout.wifi_zone.x0 && pTS->x < select_screen2_drawing_layout.wifi_zone.x1 &&
             pTS->y >= select_screen2_drawing_layout.wifi_zone.y0 && pTS->y < select_screen2_drawing_layout.wifi_zone.y1) {
        menu_lc  = 0;
        screen = SCREEN_QR_CODE;
        shouldDrawScreen = 1;
        *click_flag = 1;
    }
    // Provjera dodira na zonu za APP
    else if (pTS->x >= select_screen2_drawing_layout.app_zone.x0 && pTS->x < select_screen2_drawing_layout.app_zone.x1 &&
             pTS->y >= select_screen2_drawing_layout.app_zone.y0 && pTS->y < select_screen2_drawing_layout.app_zone.y1) {
        menu_lc  = 1;
        screen = SCREEN_QR_CODE;
        shouldDrawScreen = 1;
        *click_flag = 1;
    }
    // Provjera dodira na zonu za PODEŠAVANJA
    else if (pTS->x >= select_screen2_drawing_layout.settings_zone.x0 && pTS->x < select_screen2_drawing_layout.settings_zone.x1 &&
             pTS->y >= select_screen2_drawing_layout.settings_zone.y0 && pTS->y < select_screen2_drawing_layout.settings_zone.y1) {

        NumpadContext_t pin_context = {
            .title = "UNESITE PIN",
            .initial_value = "",
            .min_val = 0,
            .max_val = 9999,
            .max_len = 4,
            .allow_decimal = false,
            .allow_minus_one = false
        };

        Display_ShowNumpad(&pin_context);
        *click_flag = 1;
        
        // =======================================================================
        // ===       KLJUČNA ISPRAVKA: Odmah izađi iz funkcije               ===
        // =======================================================================
        // Nakon poziva `Display_ShowNumpad`, koji je već postavio novi ekran,
        // moramo odmah izaći da spriječimo dalju logiku u ovoj funkciji da
        // pogrešno vrati ekran na staru vrijednost.
        return;
    }
    // Provjera dodira na "NEXT" dugme
    else if (pTS->x >= select_screen2_drawing_layout.next_button_zone.x0 && pTS->x < select_screen2_drawing_layout.next_button_zone.x1 &&
             pTS->y >= select_screen2_drawing_layout.next_button_zone.y0 && pTS->y < select_screen2_drawing_layout.next_button_zone.y1) {
        screen = SCREEN_SELECT_1;
        shouldDrawScreen = 1;
        *click_flag = 1;
    }

    // =======================================================================
    // ===       SAČUVANA BLOKIRAJUĆA PETLJA ZA SPREČAVANJE           ===
    // =======================================================================
    // Ova petlja se izvršava samo ako je došlo do promjene ekrana
    if (*click_flag) {
        GUI_PID_STATE ts_release;
        do {
            TS_Service();
            GUI_PID_GetState(&ts_release);
            HAL_Delay(100);
        } while (ts_release.Pressed);
    }
}

/**
 * @brief Obrada događaja pritiska za ekran "Thermostat".
 * @note Rukuje dodirima na zone za povećanje (+) i smanjenje (-) zadane temperature,
 * kao i detekcijom početka dugog pritiska za paljenje/gašenje termostata.
 * Koordinate zona dodira su definisane kao lokalne konstante radi bolje
 * enkapsulacije i čitljivosti koda.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 * @param click_flag Pokazivač na fleg koji se postavlja ako treba generisati zvučni signal.
 */
static void HandlePress_ThermostatScreen(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    // Provjera da li je dodir unutar zone za POVEĆANJE temperature
    if ((pTS->x >= thermostat_layout.increase_zone.x0) && (pTS->x < thermostat_layout.increase_zone.x1) &&
            (pTS->y >= thermostat_layout.increase_zone.y0) && (pTS->y < thermostat_layout.increase_zone.y1))
    {
        *click_flag = 1; // Signaliziraj da treba generisati "klik" zvuk
        btninc = 1;      // Postavi fleg da je dugme "+" pritisnuto
    }
    // Provjera da li je dodir unutar zone za SMANJENJE temperature
    else if ((pTS->x >= thermostat_layout.decrease_zone.x0) && (pTS->x < thermostat_layout.decrease_zone.x1) &&
             (pTS->y >= thermostat_layout.decrease_zone.y0) && (pTS->y < thermostat_layout.decrease_zone.y1))
    {
        *click_flag = 1; // Signaliziraj "klik"
        btndec = 1;      // Postavi fleg da je dugme "-" pritisnuto
    }
    // Provjera da li je dodir unutar zone za ON/OFF
    else if ((pTS->x >= thermostat_layout.on_off_zone.x0) && (pTS->x < thermostat_layout.on_off_zone.x1) &&
             (pTS->y >= thermostat_layout.on_off_zone.y0) && (pTS->y < thermostat_layout.on_off_zone.y1))
    {
        *click_flag = 1; // Signaliziraj "klik"

        // Pokreni tajmer za detekciju dugog pritiska.
        // Sama logika gašenja/paljenja se nalazi u `Service_ThermostatScreen` funkciji
        // koja provjerava koliko dugo ovaj tajmer traje.
        thermostatOnOffTouch_timer = HAL_GetTick();
        if(!thermostatOnOffTouch_timer) {
            thermostatOnOffTouch_timer = 1; // Osiguraj da nije 0
        }
    }
}

/**
 * @brief Obrada događaja pritiska za ekran "Lights".
 * @note  Detektuje dodir na ikonu specifičnog svjetla. Ako svjetlo nije binarno (dimer/RGB),
 * pokreće tajmer za dugi pritisak kako bi se omogućio ulazak u meni za podešavanja.
 * Refaktorisana je da koristi isključivo `lights` API i centralizovanu layout strukturu.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 * @param click_flag Pokazivač na fleg koji se postavlja ako treba generisati zvučni signal.
 */
static void HandlePress_LightsScreen(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    // Resetujemo globalne varijable stanja na početku obrade dodira.
    light_selectedIndex = LIGHTS_MODBUS_SIZE + 1;
    light_settingsTimerStart = 0;

    // Logika za dinamički raspored i detekciju dodira.
    int y = (LIGHTS_Rows_getCount() > 1) ? 10 : 86;
    uint8_t lightsInRowSum = 0;

    // Prolazimo kroz redove ikonica...
    for(uint8_t row = 0; row < LIGHTS_Rows_getCount(); ++row) {
        uint8_t lightsInRow = LIGHTS_getCount();
        if(LIGHTS_getCount() > 3) {
            if(LIGHTS_getCount() == 4) lightsInRow = 2;
            else if(LIGHTS_getCount() == 5) lightsInRow = (row > 0) ? 2 : 3;
            else lightsInRow = 3;
        }
        uint8_t currentLightsMenuSpaceBetween = (400 - (80 * lightsInRow)) / (lightsInRow - 1 + 2);

        // ...i kroz ikonice u trenutnom redu.
        for(uint8_t i_light = 0; i_light < lightsInRow; ++i_light) {
            int x = (currentLightsMenuSpaceBetween * ((i_light % lightsInRow) + 1)) + (80 * (i_light % lightsInRow));

            // Provjeravamo da li koordinate dodira upadaju u "hitbox" trenutne ikonice,
            // koristeći dimenzije iz `lights_screen_layout` strukture.
            if((pTS->x > x) && (pTS->x < (x + lights_screen_layout.icon_width)) &&
                    (pTS->y > y) && (pTS->y < (y + lights_screen_layout.icon_height)))
            {
                *click_flag = 1; // Signaliziraj "klik"
                light_selectedIndex = lightsInRowSum + i_light; // Zabilježi koje je svjetlo pritisnuto

                LIGHT_Handle* handle = LIGHTS_GetInstance(light_selectedIndex);

                if (handle) {
                    // Sada se tajmer za dugi pritisak pokreće za SVE tipove svjetala.
                    light_settingsTimerStart = HAL_GetTick();
                }

                // Zaustavljamo noćni tajmer ako je bio aktivan.
                LIGHTS_StopNightTimer();

                // Prekidamo obje petlje jer smo pronašli pritisnutu ikonicu.
                goto exit_loops;
            }
        }
        lightsInRowSum += lightsInRow;
        y += 130;
    }

exit_loops:; // Labela za izlazak iz ugniježdenih petlji
}

/**
 * @brief Obrada događaja pritiska za ekran "Curtains".
 * @note  Rukuje pritiskom na trouglove za pomjeranje GORE/DOLE,
 * kao i strelicama za prebacivanje između pojedinačnih roletni i grupe "SVE".
 * Koristi `curtains_screen_layout` za preciznu detekciju dodira.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 * @param click_flag Pokazivač na fleg koji se postavlja ako treba generisati zvučni signal.
 */
static void HandlePress_CurtainsScreen(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    // Provjera da li je dodir u zoni za pomjeranje GORE
    if ((pTS->x >= curtains_screen_layout.up_zone.x0) && (pTS->x < curtains_screen_layout.up_zone.x1) &&
            (pTS->y >= curtains_screen_layout.up_zone.y0) && (pTS->y < curtains_screen_layout.up_zone.y1))
    {
        *click_flag = 1;
        shouldDrawScreen = 1;
        Curtain_HandleTouchLogic(CURTAIN_UP);
    }
    // Provjera da li je dodir u zoni za pomjeranje DOLE
    else if ((pTS->x >= curtains_screen_layout.down_zone.x0) && (pTS->x < curtains_screen_layout.down_zone.x1) &&
             (pTS->y >= curtains_screen_layout.down_zone.y0) && (pTS->y < curtains_screen_layout.down_zone.y1))
    {
        *click_flag = 1;
        shouldDrawScreen = 1;
        Curtain_HandleTouchLogic(CURTAIN_DOWN);
    }
    // Provjera da li je dodir u zoni za PRETHODNU roletnu (samo ako ima više od jedne)
    else if (Curtains_getCount() > 1 &&
             (pTS->x >= curtains_screen_layout.previous_arrow_zone.x0) && (pTS->x < curtains_screen_layout.previous_arrow_zone.x1) &&
             (pTS->y >= curtains_screen_layout.previous_arrow_zone.y0) && (pTS->y < curtains_screen_layout.previous_arrow_zone.y1))
    {
        if(curtain_selected > 0) {
            Curtain_Select(curtain_selected - 1);
        } else {
            Curtain_Select(Curtains_getCount()); // Vrati se na opciju "SVE"
        }
        shouldDrawScreen = 1;
        *click_flag = 1;
    }
    // Provjera da li je dodir u zoni za SLJEDEĆU roletnu (samo ako ima više od jedne)
    else if (Curtains_getCount() > 1 &&
             (pTS->x >= curtains_screen_layout.next_arrow_zone.x0) && (pTS->x < curtains_screen_layout.next_arrow_zone.x1) &&
             (pTS->y >= curtains_screen_layout.next_arrow_zone.y0) && (pTS->y < curtains_screen_layout.next_arrow_zone.y1))
    {
        if (curtain_selected < Curtains_getCount()) {
            Curtain_Select(curtain_selected + 1);
        } else {
            Curtain_Select(0); // Vrati se na prvu roletnu
        }
        shouldDrawScreen = 1;
        *click_flag = 1;
    }
}
/**
 ******************************************************************************
 * @brief       Obrada događaja pritiska za ekran "Light Settings".
 * @author      Gemini & [Vaše Ime]
 * @note        KONAČNA ISPRAVKA: Ova verzija sadrži kompletan kod. Ispravno
 * provjerava da li je dodir u zoni za izmjenu naziva i pokreće
 * tajmer. Ako nije, izvršava kompletnu, originalnu logiku za
 * obradu dodira na slajderu svjetline i paleti boja.
 ******************************************************************************
 */
static void HandlePress_LightSettingsScreen(GUI_PID_STATE * pTS)
{
    // Provjera da li je dodir unutar definisane zone za izmjenu naziva (samo za pojedinačna svjetla)
    if (!rename_light_timer_start && light_selectedIndex < LIGHTS_MODBUS_SIZE &&
        pTS->x >= light_settings_screen_layout.rename_text_zone.x0 && pTS->x < light_settings_screen_layout.rename_text_zone.x1 &&
        pTS->y >= light_settings_screen_layout.rename_text_zone.y0 && pTS->y < light_settings_screen_layout.rename_text_zone.y1)
    {
        // Ako je dodir u zoni, pokreni tajmer za detekciju dugog pritiska
        rename_light_timer_start = HAL_GetTick() ? HAL_GetTick() : 1;
    }
    else // Ako dodir NIJE bio u zoni za naziv, obradi slajdere i paletu
    {
        // =======================================================================
        // === POČETAK KOMPLETNOG, ORIGINALNOG KODA ZA SLAJDERE I PALETU ===
        // =======================================================================
        uint8_t new_brightness = 255;
        uint32_t new_color = 0;

        bool is_rgb_mode = false;
        if (light_selectedIndex == LIGHTS_MODBUS_SIZE) {
            is_rgb_mode = lights_allSelected_hasRGB;
        } else {
            LIGHT_Handle* handle = LIGHTS_GetInstance(light_selectedIndex);
            if (handle) {
                is_rgb_mode = LIGHT_isRGB(handle);
            }
        }

        // Provjera dodira na bijeli kvadrat (samo u RGB modu)
        if (is_rgb_mode &&
                (pTS->x >= light_settings_screen_layout.white_square_zone.x0) && (pTS->x < light_settings_screen_layout.white_square_zone.x1) &&
                (pTS->y >= light_settings_screen_layout.white_square_zone.y0) && (pTS->y < light_settings_screen_layout.white_square_zone.y1))
        {
            new_color = 0x00FFFFFF;
        }
        // Provjera dodira na slajder svjetline
        else if ((pTS->x >= light_settings_screen_layout.brightness_slider_zone.x0) && (pTS->x < light_settings_screen_layout.brightness_slider_zone.x1) &&
                 (pTS->y >= light_settings_screen_layout.brightness_slider_zone.y0) && (pTS->y < light_settings_screen_layout.brightness_slider_zone.y1))
        {
            g_high_precision_mode = true;

            const int slider_x0 = light_settings_screen_layout.brightness_slider_zone.x0;
            const int slider_x1 = light_settings_screen_layout.brightness_slider_zone.x1;
            const int slider_width = slider_x1 - slider_x0;

            const int zone_width = slider_width * 0.04f;
            const int left_zone_end = slider_x0 + zone_width;
            const int right_zone_start = slider_x1 - zone_width;

            if (pTS->x < left_zone_end) {
                new_brightness = 0;
            } else if (pTS->x > right_zone_start) {
                new_brightness = 100;
            } else {
                const int middle_zone_width = right_zone_start - left_zone_end;
                const int relative_touch_in_middle = pTS->x - left_zone_end;
                float percentage = (float)relative_touch_in_middle / (float)middle_zone_width;
                new_brightness = 1 + (uint8_t)(percentage * 98.0f);
            }

            if (new_brightness > 100) new_brightness = 100;
        }
        // Provjera dodira na paletu boja (samo u RGB modu)
        else if (is_rgb_mode &&
                 (pTS->x >= light_settings_screen_layout.color_palette_zone.x0) && (pTS->x < light_settings_screen_layout.color_palette_zone.x1) &&
                 (pTS->y >= light_settings_screen_layout.color_palette_zone.y0) && (pTS->y < light_settings_screen_layout.color_palette_zone.y1))
        {
            new_color = LCD_GetPixelColor(pTS->x, pTS->y) & 0x00FFFFFF;
        }

        // Primjena detektovanih promjena
        if (new_brightness != 255 || new_color != 0) {
            if (light_selectedIndex == LIGHTS_MODBUS_SIZE) { // Sva svjetla
                for(uint8_t i = 0; i < LIGHTS_getCount(); i++) {
                    LIGHT_Handle* handle = LIGHTS_GetInstance(i);
                    if (handle && LIGHT_isTiedToMainLight(handle) && !LIGHT_isBinary(handle)) {
                        if (new_brightness != 255) LIGHT_SetBrightness(handle, new_brightness);
                        else if (LIGHT_isRGB(handle) && new_color != 0) LIGHT_SetColor(handle, new_color);
                    }
                }
            } else { // Pojedinačno svjetlo
                LIGHT_Handle* handle = LIGHTS_GetInstance(light_selectedIndex);
                if (handle) {
                    if (new_brightness != 255) LIGHT_SetBrightness(handle, new_brightness);
                    else if (LIGHT_isRGB(handle) && new_color != 0) LIGHT_SetColor(handle, new_color);
                }
            }
        }
        // =======================================================================
        // === KRAJ KOMPLETNOG, ORIGINALNOG KODA ZA SLAJDERE I PALETU ===
        // =======================================================================
    }
}
/**
 * @brief Obrada događaja pritiska u zoni glavnog prekidača na ekranu.
 * @note Pokreće tajmer za dugi pritisak kako bi se ušlo u podešavanja
 * svjetla ako je bar jedno od odabranih svjetala dimabilno.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 */
static void HandlePress_MainScreenSwitch(GUI_PID_STATE * pTS)
{
    // Logika za screensaver ostaje ista.
    if((!g_display_settings.leave_scrnsvr_on_release) || (g_display_settings.leave_scrnsvr_on_release && (!IsScrnsvrActiv()))) {

        // Resetujemo flegove na početku
        light_selectedIndex = LIGHTS_MODBUS_SIZE + 1; // Koristi se kao fleg da li je dimer/RGB pronađen
        lights_allSelected_hasRGB = false;

        // =======================================================================
        // === POČETAK REFAKTORISANJA ===
        //
        // Prolazimo kroz sva konfigurisana svjetla koristeći novi API.
        for(uint8_t i = 0; i < LIGHTS_getCount(); i++) {
            LIGHT_Handle* handle = LIGHTS_GetInstance(i);

            // Provjeravamo da li handle postoji i da li je svjetlo vezano za glavni prekidač.
            if (handle && LIGHT_isTiedToMainLight(handle) && !LIGHT_isBinary(handle)) {
                // Ako smo pronašli bar jedno svjetlo koje nije binarno (dimer ili RGB),
                // postavljamo fleg i prekidamo dalju pretragu.
                light_selectedIndex = LIGHTS_MODBUS_SIZE; // Postavljamo "SVA SVJETLA"

                // Dodatno provjeravamo da li je to svjetlo RGB tipa.
                if (LIGHT_isRGB(handle)) {
                    lights_allSelected_hasRGB = true;
                }
            }
        }
        // === KRAJ REFAKTORISANJA ===
        // =======================================================================

        // Ako smo pronašli bar jedno dimer/RGB svjetlo, pokrećemo tajmer za dugi pritisak.
        if(light_selectedIndex == LIGHTS_MODBUS_SIZE) {
            light_settingsTimerStart = HAL_GetTick();
        }
    }
}


/**
 * @brief Obrada otpuštanja dodira na glavnom ekranu (glavni prekidač).
 * @note Ova funkcija implementira "toggle" logiku za sva svjetla koja su povezana
 * sa glavnim prekidačem. Takođe upravlja pokretanjem i zaustavljanjem
 * noćnog tajmera. Refaktorisana je da bude drastično jednostavnija
 * korištenjem novog `lights` API-ja.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 */
static void HandleRelease_MainScreenSwitch(GUI_PID_STATE * pTS)
{
    // =======================================================================
    // === POČETAK REFAKTORISANJA ===

    // << KLJUČNA ISPRAVKA: Resetovanje tajmera za dugi pritisak >>
    // Ovo osigurava da se, nakon kratkog pritiska, ne aktivira
    // meni za podešavanje svjetala.
    light_settingsTimerStart = 0;
    // KORAK 1: Provjera trenutnog stanja svih glavnih svjetala.
    // Umjesto kompleksne petlje, sada pozivamo JEDNU funkciju iz `lights` API-ja.
    // `LIGHTS_isAnyLightOn()` interno provjerava sva svjetla koja su `tiedToMainLight`
    // i vraća `true` ako je ijedno upaljeno.
    bool isAnyLightCurrentlyOn = LIGHTS_isAnyLightOn();

    // KORAK 2: Određujemo novo, željeno stanje.
    // Ako je bilo šta bilo upaljeno, novo stanje je "ugasi sve".
    // Ako je sve bilo ugašeno, novo stanje je "upali sve".
    bool newStateIsOn = !isAnyLightCurrentlyOn;

    // KORAK 3: Primjenjujemo novo stanje na sva glavna svjetla.
    // Prolazimo kroz sva konfigurisana svjetla...
    for(uint8_t i = 0; i < LIGHTS_getCount(); ++i) {
        // ...dobijamo njihov handle...
        LIGHT_Handle* handle = LIGHTS_GetInstance(i);

        // ...i ako su vezana za glavni prekidač, postavljamo im novo stanje.
        if (handle && LIGHT_isTiedToMainLight(handle)) {
            LIGHT_SetState(handle, newStateIsOn);
        }
    }

    // KORAK 4: Upravljanje noćnim tajmerom preko novog, čistog API-ja.
    // Provjeravamo da li je noćni tajmer omogućen u postavkama i da li je noć.
    if (g_display_settings.light_night_timer_enabled && !((Bcd2Dec(rtctm.Hours) > 6) && (Bcd2Dec(rtctm.Hours) < 20))) {

        // Ako je korisnik upalio svjetla, pozivamo API da pokrene tajmer.
        if (newStateIsOn) {
            LIGHTS_StartNightTimer();
        } else { // Ako je korisnik ugasio svjetla, pozivamo API da zaustavi tajmer.
            LIGHTS_StopNightTimer();
        }
    } else {
        // Ako je dan ili je tajmer onemogućen, osiguravamo da je zaustavljen.
        LIGHTS_StopNightTimer();
    }

    // === KRAJ REFAKTORISANJA ===
    // =======================================================================

    // Ostatak funkcije (zahtjev za iscrtavanje i povratak na glavni ekran) ostaje isti.
    shouldDrawScreen = 1;
    screen = SCREEN_MAIN;
}
/**
 ******************************************************************************
 * @brief       Obrađuje događaj pritiska za glavni ekran "Čarobnjaka" scena.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija detektuje pritisak na tri moguća dugmeta: "Snimi",
 * "Otkaži" i "[ Promijeni ]". Na osnovu pritisnutog dugmeta,
 * izvršava odgovarajuću akciju: snima scenu, otkazuje izmjene
 * ili pokreće novi ekran za odabir izgleda scene.
 * @param       pTS Pokazivač na strukturu sa stanjem dodira (nije korišten).
 * @param       click_flag Pokazivač na fleg za generisanje zvučnog signala.
 ******************************************************************************
 */
static void HandlePress_SceneEditScreen(GUI_PID_STATE* pTS, uint8_t* click_flag)
{
    // Provjera pritiska na dugme "Snimi" (ID_Ok)
    if (BUTTON_IsPressed(hBUTTON_Ok))
    {
        *click_flag = 1; // Aktiviraj zvučni signal
        // Pozovi backend da "uslika" trenutno stanje svih relevantnih uređaja
        Scene_Memorize(scene_edit_index);
        // Pozovi backend da snimi sve scene (uključujući novu izmjenu) u EEPROM
        Scene_Save();
        
        // Vrati se na ekran sa pregledom svih scena
        DSP_KillSceneEditScreen(); // Obriši widgete prije prelaska
        screen = SCREEN_SCENE;
        shouldDrawScreen = 1;
    }
    // Provjera pritiska na dugme "Otkaži" (ID_Next)
    else if (BUTTON_IsPressed(hBUTTON_Next))
    {
        *click_flag = 1;
        // Samo se vrati na prethodni ekran bez snimanja
        DSP_KillSceneEditScreen();
        screen = SCREEN_SCENE;
        shouldDrawScreen = 1;
    }
    // Provjera pritiska na dugme "[ Promijeni ]"
    else if (BUTTON_IsPressed(hButtonChangeAppearance))
    {
        *click_flag = 1;
        // Prebaci se na novi ekran za odabir izgleda scene
        DSP_KillSceneEditScreen();
        screen = SCREEN_SCENE_APPEARANCE;
        shouldDrawScreen = 1;
    }
}
/**
 ******************************************************************************
 * @brief       Obrađuje događaj pritiska za ekran odabira izgleda scene.
 * @author      Gemini & [Vaše Ime]
 * @note        FINALNA ISPRAVLJENA VERZIJA. Funkcija sada sadrži ispravnu
 * "Kill -> Init" logiku za povratak na `SCREEN_SCENE_EDIT`, čime
 * se osigurava da se odabrana ikonica odmah prikaže bez crnog ekrana.
 ******************************************************************************
 */
static void HandlePress_SceneAppearanceScreen(GUI_PID_STATE* pTS, uint8_t* click_flag)
{
    // --- 1. Provjera Pritiska na "Next" Dugme ---
    const GUI_BITMAP* iconNext = &bmnext;
    TouchZone_t next_button_zone = {
        .x0 = select_screen2_drawing_layout.next_button_x_pos,
        .y0 = select_screen2_drawing_layout.next_button_y_center - (iconNext->YSize / 2),
        .x1 = 480,
        .y1 = 272
    };

    if (pTS->x >= next_button_zone.x0 && pTS->x < next_button_zone.x1 &&
        pTS->y >= next_button_zone.y0 && pTS->y < next_button_zone.y1)
    {
        *click_flag = 1;
        const int ICONS_PER_PAGE = 6;
        int total_available_appearances = 0;
        const SceneAppearance_t* available_appearances[sizeof(scene_appearance_table)/sizeof(SceneAppearance_t)];

        // Ponovo filtriraj da dobiješ tačan broj stranica
        uint8_t used_appearance_ids[SCENE_MAX_COUNT] = {0};
        uint8_t used_count = 0;
        for (int i = 0; i < SCENE_MAX_COUNT; i++)
        {
            Scene_t* temp_handle = Scene_GetInstance(i);
            if (temp_handle && temp_handle->is_configured)
            {
                used_appearance_ids[used_count++] = temp_handle->appearance_id;
            }
        }
        for (int i = 1; i < (sizeof(scene_appearance_table) / sizeof(SceneAppearance_t)); i++)
        {
            bool is_used = false;
            for (int j = 0; j < used_count; j++) { if (used_appearance_ids[j] == i) { is_used = true; break; } }
            if (!is_used) { available_appearances[total_available_appearances++] = &scene_appearance_table[i]; }
        }
        int total_pages = (total_available_appearances + ICONS_PER_PAGE - 1) / ICONS_PER_PAGE;
        
        scene_appearance_page++;
        if (scene_appearance_page >= total_pages)
        {
            scene_appearance_page = 0;
        }
        
        // Ponovo iscrtaj ovaj isti ekran sa novom stranicom
        DSP_InitSceneAppearanceScreen();
        shouldDrawScreen = 0;
        return;
    }

    // --- 2. Provjera Pritiska na Ikonice ---
    const int ICONS_PER_PAGE = 6;
    int row = (pTS->y - 10) / (scene_screen_layout.slot_height);
    int col = pTS->x / scene_screen_layout.slot_width;
    int display_index = row * scene_screen_layout.items_per_row + col;

    // Ponovo filtriraj listu da pronađeš tačno koja je ikonica pritisnuta
    int total_available_appearances = 0;
    const SceneAppearance_t* available_appearances[sizeof(scene_appearance_table)/sizeof(SceneAppearance_t)];
    uint8_t used_appearance_ids[SCENE_MAX_COUNT] = {0};
    uint8_t used_count = 0;
    for (int i = 0; i < SCENE_MAX_COUNT; i++)
    {
        Scene_t* temp_handle = Scene_GetInstance(i);
        if (temp_handle && temp_handle->is_configured)
        {
            used_appearance_ids[used_count++] = temp_handle->appearance_id;
        }
    }
    for (int i = 1; i < (sizeof(scene_appearance_table) / sizeof(SceneAppearance_t)); i++)
    {
        bool is_used = false;
        for (int j = 0; j < used_count; j++) { if (used_appearance_ids[j] == i) { is_used = true; break; } }
        if (!is_used) { available_appearances[total_available_appearances++] = &scene_appearance_table[i]; }
    }

    int actual_index_in_available_list = (scene_appearance_page * ICONS_PER_PAGE) + display_index;

    if (actual_index_in_available_list < total_available_appearances)
    {
        *click_flag = 1;
        const SceneAppearance_t* chosen_appearance = available_appearances[actual_index_in_available_list];
        
        // Pronađi originalni ID u `scene_appearance_table`
        int selected_appearance_id = 0;
        for(int i=0; i < (sizeof(scene_appearance_table) / sizeof(SceneAppearance_t)); i++) {
            if (&scene_appearance_table[i] == chosen_appearance) {
                selected_appearance_id = i;
                break;
            }
        }
        
        Scene_t* scene_handle = Scene_GetInstance(scene_edit_index);
        if (scene_handle)
        {
            scene_handle->appearance_id = selected_appearance_id;
            
            if (chosen_appearance->text_id == TXT_SCENE_LEAVING) {
                scene_handle->scene_type = SCENE_TYPE_LEAVING;
            } else if (chosen_appearance->text_id == TXT_SCENE_HOMECOMING) {
                scene_handle->scene_type = SCENE_TYPE_HOMECOMING;
            } else if (chosen_appearance->text_id == TXT_SCENE_SLEEP) {
                scene_handle->scene_type = SCENE_TYPE_SLEEP;
            } else {
                scene_handle->scene_type = SCENE_TYPE_STANDARD;
            }
        }
        
        // === ISPRAVNA NAVIGACIJA ZA POVRATAK ===
        DSP_KillSceneAppearanceScreen();    // 1. Ubij trenutni ekran
        DSP_InitSceneEditScreen();          // 2. Inicijalizuj prethodni ekran (čarobnjaka)
        screen = SCREEN_SCENE_EDIT;         // 3. Postavi novo stanje
        shouldDrawScreen = 0;               // 4. Ne treba ponovo crtati
    }
}
/**
 ******************************************************************************
 * @brief       Obrađuje događaj PRITISKA za ekran sa scenama (SCREEN_SCENE).
 * @author      Gemini & [Vaše Ime]
 * @note        Verzija 2.0: Ova funkcija sada samo bilježi koji je slot
 * dodirnut i pokreće tajmer za mjerenje trajanja pritiska.
 ******************************************************************************
 */
static void HandlePress_SceneScreen(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    uint8_t configured_scenes_count = Scene_GetCount();

    // --- Provjera dodira na ikonicu Čarobnjaka ---
    const GUI_BITMAP* wizard_icon = &bmicons_scene_wizzard;
    TouchZone_t wizard_zone = {
        .x0 = select_screen2_drawing_layout.next_button_x_pos,
        .y0 = select_screen2_drawing_layout.next_button_y_center - (wizard_icon->YSize / 2),
        .x1 = 480,
        .y1 = 272
    };

    if ((configured_scenes_count < SCENE_MAX_COUNT) && (pTS->x >= wizard_zone.x0 && pTS->x < wizard_zone.x1 && pTS->y >= wizard_zone.y0 && pTS->y < wizard_zone.y1))
    {
        *click_flag = 1;
        scene_pressed_index = configured_scenes_count; // Postavi indeks na poziciju čarobnjaka
        scene_press_timer_start = HAL_GetTick() ? HAL_GetTick() : 1;
    }
    // --- Provjera dodira na Mrežu sa Scenama ---
    else if (pTS->x < DRAWING_AREA_WIDTH) // Osiguraj da dodir nije u zoni menija
    {
        int row = pTS->y / scene_screen_layout.slot_height;
        int col = pTS->x / scene_screen_layout.slot_width;
        int touched_slot_index = row * scene_screen_layout.items_per_row + col;

        if (touched_slot_index < configured_scenes_count)
        {
            *click_flag = 1;
            scene_pressed_index = touched_slot_index;
            scene_press_timer_start = HAL_GetTick() ? HAL_GetTick() : 1;
        }
    }
}
/**
 ******************************************************************************
 * @brief       Obrađuje događaj OTPUŠTANJA za ekran sa scenama (SCREEN_SCENE).
 * @author      Gemini & [Vaše Ime]
 * @note        FINALNA VERZIJA: Ova funkcija obrađuje isključivo KRATAK KLIK.
 * Ispravno aktivira postojeću scenu ili pokreće čarobnjaka za
 * kreiranje nove, poštujući "Kill -> Init" navigacioni patern.
 ******************************************************************************
 */
static void HandleRelease_SceneScreen(void)
{
    // Logika ostaje ista, jer je `HandlePress_SceneScreen` već ispravno
    // postavio `scene_pressed_index`.
    
    if (scene_press_timer_start == 0) return; // Dugi pritisak je već obrađen

    if ((HAL_GetTick() - scene_press_timer_start) < LONG_PRESS_DURATION)
    {
        uint8_t configured_scenes_count = Scene_GetCount();
        if (scene_pressed_index < configured_scenes_count)
        {
            // Kratak klik na postojeću scenu -> Aktiviraj je
            uint8_t scene_counter = 0;
            for (int i = 0; i < SCENE_MAX_COUNT; i++)
            {
                Scene_t* temp_handle = Scene_GetInstance(i);
                if (temp_handle && temp_handle->is_configured)
                {
                    if (scene_counter == scene_pressed_index)
                    {
                        Scene_Activate(i);
                        break;
                    }
                    scene_counter++;
                }
            }
        }
        else
        {
            // Kratak klik na "Dodaj Scenu" ikonicu
            uint8_t free_slot = 0;
            for (int i = 0; i < SCENE_MAX_COUNT; i++)
            {
                Scene_t* temp_handle = Scene_GetInstance(i);
                if (!temp_handle || !temp_handle->is_configured)
                {
                    free_slot = i;
                    break;
                }
            }
            scene_edit_index = free_slot;
            
            DSP_KillSceneScreen();
            DSP_InitSceneEditScreen();
            screen = SCREEN_SCENE_EDIT;
            shouldDrawScreen = 0;
        }
    }

    scene_press_timer_start = 0;
    scene_pressed_index = -1;
}

/******************************************************************************
 * @brief       Kreira i inicijalizuje sve GUI widgete za univerzalni numpad.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ova funkcija je adaptirana iz stare `DSP_InitPinpadScreen` funkcije.
 * Sada je univerzalna i konfiguriše se čitanjem parametara iz
 * statičke `g_numpad_context` strukture. Dinamički iscrtava
 * tastaturu i naslov prema proslijeđenom kontekstu.
 * @param       None
 * @retval      None
 *****************************************************************************/
static void DSP_InitNumpadScreen(void)
{
    // Sigurnosno brisanje eventualno zaostalih widgeta sa drugih ekrana.
    ForceKillAllSettingsWidgets();

    GUI_MULTIBUF_BeginEx(1);
    GUI_Clear();
    DrawHamburgerMenu(1);

    // Definicije za dimenzije i pozicioniranje elemenata ostaju iste
    const int text_h = 50;
    const int btn_w = 80, btn_h = 40;
    const int x_gap = 10, y_gap = 10;
    const int x_start = (DRAWING_AREA_WIDTH - (3 * btn_w + 2 * x_gap)) / 2;
    const int keypad_h = (4 * btn_h + 3 * y_gap);
    const int total_h = text_h + y_gap + keypad_h;
    const int y_block_start = (LCD_GetYSize() - total_h) / 2;
    const int y_keypad_start = y_block_start + text_h + y_gap;

    // Tekstovi i ID-jevi za dugmad se definišu dinamički
    const char* key_texts[12];
    const int key_ids[] = {
        ID_PINPAD_1, ID_PINPAD_2, ID_PINPAD_3,
        ID_PINPAD_4, ID_PINPAD_5, ID_PINPAD_6,
        ID_PINPAD_7, ID_PINPAD_8, ID_PINPAD_9,
        ID_PINPAD_DEL, ID_PINPAD_0, ID_PINPAD_OK
    };

    // Popunjavanje tekstova za dugmad
    key_texts[0] = "1";
    key_texts[1] = "2";
    key_texts[2] = "3";
    key_texts[3] = "4";
    key_texts[4] = "5";
    key_texts[5] = "6";
    key_texts[6] = "7";
    key_texts[7] = "8";
    key_texts[8] = "9";

    // Dinamička konfiguracija zadnje tri tipke na osnovu konteksta
    key_texts[9] = g_numpad_context.allow_decimal ? "." : "DEL"; // Tipka 10: DEL ili tačka
    key_texts[10] = "0";                                        // Tipka 11: Uvijek nula
    key_texts[11] = g_numpad_context.allow_minus_one ? "ISKLJ." : "OK"; // Tipka 12: OK ili ISKLJ.

    // Kreiranje dugmadi u petlji
    for (int i = 0; i < 12; i++) {
        int row = i / 3;
        int col = i % 3;
        int x_pos = x_start + col * (btn_w + x_gap);
        int y_pos = y_keypad_start + row * (btn_h + y_gap);

        hKeypadButtons[i] = BUTTON_CreateEx(x_pos, y_pos, btn_w, btn_h, 0, WM_CF_SHOW, 0, key_ids[i]);
        BUTTON_SetText(hKeypadButtons[i], key_texts[i]);
        BUTTON_SetFont(hKeypadButtons[i], &GUI_Font24_1);
    }

    // Inicijalizacija i iscrtavanje početnog stanja
    pin_buffer_idx = 0;
    memset(pin_buffer, 0, sizeof(pin_buffer));
    strncpy(pin_buffer, g_numpad_context.initial_value, MAX_PIN_LENGTH);
    pin_buffer_idx = strlen(pin_buffer);

    pin_mask_timer = 0;
    pin_error_active = false;
    DSP_DrawNumpadText(); // Poziv nove, preimenovane funkcije

    GUI_MULTIBUF_EndEx(1);
}
/******************************************************************************
 * @brief       Glavna servisna funkcija za univerzalni numerički keypad.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        FINALNA VERZIJA. Ova funkcija sada sadrži kompletnu logiku.
 * Na osnovu naslova (`g_numpad_context.title`), odlučuje da li
 * treba izvršiti provjeru PIN-a ili standardnu numeričku
 * validaciju, čime je postignuta puna univerzalnost.
 * @param       None
 * @retval      None
 *****************************************************************************/
static void Service_NumpadScreen(void)
{
    static int button_pressed_id = -1; // -1 = nijedno dugme nije pritisnuto
    static bool should_redraw_text = false; // Lokalni fleg za kontrolu ponovnog iscrtavanja teksta

    // Provjeri je li zatraženo ponovno iscrtavanje (kreiranje widgeta)
    if (shouldDrawScreen) {
        shouldDrawScreen = 0; // Resetuj fleg kako se ne bi ponavljalo
        DSP_InitNumpadScreen(); // Kreiraj sve NumPad widgete
        // Nakon inicijalizacije, odmah nacrtaj tekst.
        DSP_DrawNumpadText();
    }
    
    // Provjera pritiska na svako dugme (polling)
    int currently_pressed_id = -1;
    for (int i = 0; i < 12; i++) {
        if (WM_IsWindow(hKeypadButtons[i]) && BUTTON_IsPressed(hKeypadButtons[i])) {
            currently_pressed_id = i;
            break;
        }
    }

    // Događaj se aktivira kada se dugme OTPUSTI
    if (currently_pressed_id == -1 && button_pressed_id != -1) {
        BuzzerOn();
        HAL_Delay(1);
        BuzzerOff();
        
        // Korigovano: Koristimo WM_GetId da dobijemo ID dugmeta
        int Id = WM_GetId(hKeypadButtons[button_pressed_id]);
        should_redraw_text = false;

        // Logika unosa karaktera
        if (Id >= ID_PINPAD_0 && Id <= ID_PINPAD_9) {
            if (pin_buffer_idx < g_numpad_context.max_len) {
                // Sacuvaj posljednju cifru prije nego je upises u buffer
                pin_last_char = ((Id - ID_PINPAD_0) + '0');
                pin_buffer[pin_buffer_idx++] = pin_last_char;
                pin_buffer[pin_buffer_idx] = '\0';
                pin_mask_timer = HAL_GetTick(); // Pokreni tajmer pri unosu
                should_redraw_text = true;
            }
        }
        // Logika za DEL ili decimalnu tačku
        else if (Id == ID_PINPAD_DEL) {
            if (g_numpad_context.allow_decimal) {
                if (pin_buffer_idx < g_numpad_context.max_len && strchr(pin_buffer, '.') == NULL) {
                    pin_last_char = '.';
                    pin_buffer[pin_buffer_idx++] = pin_last_char;
                    pin_buffer[pin_buffer_idx] = '\0';
                    pin_mask_timer = HAL_GetTick();
                    should_redraw_text = true;
                }
            } else {
                if (pin_buffer_idx > 0) {
                    pin_buffer[--pin_buffer_idx] = '\0';
                    should_redraw_text = true;
                }
            }
        }
        // Logika za OK / ISKLJ. / Potvrdu unosa
        else if (Id == ID_PINPAD_OK) {
            bool is_valid = false;
            
            // Logika za provjeru konteksta
            if (strcmp(g_numpad_context.title, "UNESITE PIN") == 0) {
                if (strcmp(pin_buffer, system_pin) == 0) {
                    is_valid = true;
                    DSP_KillNumpadScreen();
                    DSP_InitSet1Scrn();
                    screen = SCREEN_SETTINGS_1;
                }
            } else if (g_numpad_context.allow_minus_one) {
                strcpy(g_numpad_result.value, "-1");
                g_numpad_result.is_confirmed = true;
                screen = numpad_return_screen;
                is_valid = true;
            } else {
                long entered_value = atol(pin_buffer);
                if (entered_value >= g_numpad_context.min_val && entered_value <= g_numpad_context.max_val) {
                    is_valid = true;
                    strcpy(g_numpad_result.value, pin_buffer);
                    g_numpad_result.is_confirmed = true;
                    DSP_KillNumpadScreen();
                    screen = numpad_return_screen; // Vraća se na ekran koji je pozvao NumPad
                    shouldDrawScreen = 1; // Signalizira se ponovno crtanje
                }
            }
            
            if (!is_valid) {
                pin_error_active = true;
                pin_mask_timer = HAL_GetTick();
            }
            
            // Uvijek crtaj nakon pritiska na OK, bez obzira na ishod
            should_redraw_text = true;
        }

        if (should_redraw_text) {
            DSP_DrawNumpadText();
        }
    }
    
    button_pressed_id = currently_pressed_id;

    // Upravljanje tajmerom za maskiranje i grešku
    if (pin_mask_timer != 0 && (HAL_GetTick() - pin_mask_timer) >= PIN_MASK_DELAY) {
        pin_mask_timer = 0; // Zaustavi tajmer
        pin_error_active = false; // Ukloni poruku o grešci ako je bila aktivna
        DSP_DrawNumpadText(); // Pokreni iscrtavanje kako bi se zvjezdica pojavila
    }
}

/******************************************************************************
 * @brief       Uništava sve GUI widgete kreirane za Numpad.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Adaptirana `DSP_KillPinpadScreen` funkcija. Poziva se prilikom
 * napuštanja `SCREEN_NUMPAD` ekrana kako bi se oslobodili resursi.
 * @param       None
 * @retval      None
 *****************************************************************************/
static void DSP_KillNumpadScreen(void)
{
    for (int i = 0; i < 12; i++) {
        if (WM_IsWindow(hKeypadButtons[i])) {
            WM_DeleteWindow(hKeypadButtons[i]);
            hKeypadButtons[i] = 0;
        }
    }
}

/******************************************************************************
 * @brief       Pomoćna funkcija koja iscrtava tekstualni unos za Numpad.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Adaptirana `DSP_DrawPinpadText` funkcija. Centralizuje logiku
 * za prikaz naslova, unesene vrijednosti ili poruke o grešci.
 * @param       None
 * @retval      None
 *****************************************************************************/
static void DSP_DrawNumpadText(void)
{
    const int text_h = 50;
    const int keypad_h = (4 * 40 + 3 * 10);
    const int total_h = text_h + 10 + keypad_h;
    const int y_block_start = (LCD_GetYSize() - total_h) / 2;
    const int y_text_pos = y_block_start;
    const int y_text_center = y_text_pos + (text_h / 2);

    // Privremeni bafer za prikaz na ekranu
    char display_buffer[MAX_PIN_LENGTH + 1] = {0};

    GUI_MULTIBUF_BeginEx(1);
    GUI_ClearRect(10, y_text_pos, 370, y_text_pos + text_h);

    // Iscrtavanje naslova
    GUI_SetFont(GUI_FONT_24_1);
    GUI_SetColor(GUI_WHITE);
    GUI_SetTextMode(GUI_TM_TRANS);
    GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
    GUI_DispStringAt(g_numpad_context.title, DRAWING_AREA_WIDTH / 2, y_text_pos - 20);

    // Logika za iscrtavanje maskiranog unosa
    GUI_SetFont(GUI_FONT_32B_1);
    GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);

    if (pin_error_active) {
        GUI_SetColor(GUI_RED);
        GUI_DispStringAt("GRESKA", DRAWING_AREA_WIDTH / 2, y_text_center);
    } else {
        GUI_SetColor(GUI_ORANGE);
        for(int i = 0; i < pin_buffer_idx; i++) {
            // Provjeri je li tajmer aktivan i je li ovo posljednja unesena cifra
            if (pin_mask_timer != 0 && i == (pin_buffer_idx - 1)) {
                // Prikaz posljednje cifre
                display_buffer[i] = pin_buffer[i];
            } else {
                // Prikaz zvjezdice
                display_buffer[i] = '*';
            }
        }
        // Dodaj null terminator
        display_buffer[pin_buffer_idx] = '\0';
        GUI_DispStringAt(display_buffer, DRAWING_AREA_WIDTH / 2, y_text_center);
    }

    GUI_MULTIBUF_EndEx(1);
}
/******************************************************************************
 * @brief       Priprema i prebacuje sistem na ekran univerzalnog numpada.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ovo je interna, ne-blokirajuća funkcija. Kopira proslijeđeni
 * kontekst u internu statičku varijablu, resetuje rezultat,
 * i postavlja fleg za promjenu ekrana.
 * @param       context Pokazivač na NumpadContext_t strukturu sa parametrima.
 * @retval      None
 *****************************************************************************/
static void Display_ShowNumpad(const NumpadContext_t* context)
{
    // 1. Sačuvaj ekran sa kojeg je funkcija pozvana
    numpad_return_screen = screen;

    // 2. Kopiraj proslijeđeni kontekst u internu, statičku varijablu
    if (context != NULL) {
        memcpy(&g_numpad_context, context, sizeof(NumpadContext_t));
    } else {
        memset(&g_numpad_context, 0, sizeof(NumpadContext_t));
        g_numpad_context.title = "Greska";
    }

    // 3. Resetuj strukturu za rezultat prije svakog prikazivanja
    memset(&g_numpad_result, 0, sizeof(NumpadResult_t));

    // 4. Postavi flegove za prebacivanje na ekran numpada
    screen = SCREEN_NUMPAD;
    shouldDrawScreen = 1;
}
/******************************************************************************
 * @brief       Kreira i inicijalizuje sve GUI widgete za alfanumeričku tastaturu.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ova funkcija je srce UI dijela `SCREEN_KEYBOARD_ALPHA`. Ona čita
 * trenutno odabrani jezik iz `g_display_settings` i, na osnovu
 * njega i `shift` stanja, bira odgovarajući raspored iz
 * `key_layouts` matrice. Zatim dinamički kreira sve tastere,
 * postavlja tekst na njima i iscrtava polje za unos.
 * @param       None
 * @retval      None
 *****************************************************************************/
static void DSP_InitKeyboardScreen(void)
{
    // Sigurnosno brisanje widgeta sa prethodnih ekrana
    ForceKillAllSettingsWidgets();

    GUI_MULTIBUF_BeginEx(1);
    GUI_Clear();

    // === 1. Definicija rasporeda (Layout) ===
    const int16_t key_w = 42, key_h = 38;
    const int16_t x_gap = 5, y_gap = 5;
    const int16_t x_start = (LCD_GetXSize() - (KEYS_PER_ROW * key_w + (KEYS_PER_ROW - 1) * x_gap)) / 2;
    const int16_t y_start_keys = 40;

    // === 2. Odabir jezičkog rasporeda ===
    // Ovaj pokazivač će pokazivati na odgovarajući 2D niz karaktera (npr. BHS, mala slova)
    const char* (*layout)[KEYS_PER_ROW] =
        key_layouts[g_display_settings.language][keyboard_shift_active];

    // Provjera da li je layout za odabrani jezik definisan, ako nije, koristi ENG
    if (layout[0][0] == NULL) {
        layout = key_layouts[ENG][keyboard_shift_active];
    }

    // === 3. Dinamičko kreiranje tastera sa karakterima ===
    for (int row = 0; row < KEY_ROWS; row++) {
        for (int col = 0; col < KEYS_PER_ROW; col++) {
            // Preskoči ako za neku poziciju nije definisan taster (za kraće redove)
            if (layout[row][col] == NULL || strlen(layout[row][col]) == 0) continue;

            int x_pos = x_start + col * (key_w + x_gap);
            int y_pos = y_start_keys + row * (key_h + y_gap);
            int index = row * KEYS_PER_ROW + col;

            hKeyboardButtons[index] = BUTTON_CreateEx(x_pos, y_pos, key_w, key_h, 0, WM_CF_SHOW, 0, GUI_ID_USER + index);
            BUTTON_SetText(hKeyboardButtons[index], layout[row][col]);
            BUTTON_SetFont(hKeyboardButtons[index], &GUI_Font20_1);
        }
    }

    // === 4. Kreiranje specijalnih tastera (Shift, Space, OK, itd.) ===
    const int16_t y_special_row = y_start_keys + KEY_ROWS * (key_h + y_gap);

    // SHIFT taster
    hKeyboardSpecialButtons[0] = BUTTON_CreateEx(x_start, y_special_row, 60, key_h, 0, WM_CF_SHOW, 0, GUI_ID_SHIFT);
    BUTTON_SetText(hKeyboardSpecialButtons[0], "Shift");

    // SPACE taster
    hKeyboardSpecialButtons[1] = BUTTON_CreateEx(x_start + 60 + x_gap, y_special_row, 240, key_h, 0, WM_CF_SHOW, 0, GUI_ID_SPACE);
    BUTTON_SetText(hKeyboardSpecialButtons[1], "Space");

    // BACKSPACE taster
    hKeyboardSpecialButtons[2] = BUTTON_CreateEx(x_start + 300 + 2*x_gap, y_special_row, 60, key_h, 0, WM_CF_SHOW, 0, GUI_ID_BACKSPACE);
    BUTTON_SetText(hKeyboardSpecialButtons[2], "Del");

    // OK taster
    hKeyboardSpecialButtons[3] = BUTTON_CreateEx(x_start + 360 + 3*x_gap, y_special_row, 60, key_h, 0, WM_CF_SHOW, 0, GUI_ID_OKAY);
    BUTTON_SetText(hKeyboardSpecialButtons[3], "OK");

    // === 5. Inicijalizacija bafera i iscrtavanje polja za unos ===
    memset(keyboard_buffer, 0, sizeof(keyboard_buffer));
    strncpy(keyboard_buffer, g_keyboard_context.initial_value, sizeof(keyboard_buffer) - 1);
    keyboard_buffer_idx = strlen(keyboard_buffer);

    // Iscrtavanje naslova i polja za unos
    GUI_SetFont(GUI_FONT_20_1);
    GUI_SetColor(GUI_WHITE);
    GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
    GUI_DispStringAt(g_keyboard_context.title, LCD_GetXSize() / 2, 15);

    GUI_SetColor(GUI_ORANGE);
    GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
    GUI_DispStringAt(keyboard_buffer, x_start, 40);

    GUI_MULTIBUF_EndEx(1);
}
/**
 ******************************************************************************
 * @brief       Servisna funkcija za alfanumeričku tastaturu.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija obrađuje pritiske na sve tastere. Upravlja unosom
 * karaktera u bafer, specijalnim funkcijama kao što su Shift i
 * Backspace, te potvrdom unosa. Nakon potvrde, rezultat se
 * sprema u `g_keyboard_result` i kontrola se vraća na ekran
 * koji je pozvao tastaturu.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
static void Service_KeyboardScreen(void)
{
    static int button_pressed_idx = -1; // -1 = nijedan taster nije pritisnut
    int current_pressed_idx = -1;
    WM_HWIN hPressedButton = 0;

    // Provjera pritiska na tastere sa karakterima
    for (int i = 0; i < (KEY_ROWS * KEYS_PER_ROW); i++) {
        if (WM_IsWindow(hKeyboardButtons[i]) && BUTTON_IsPressed(hKeyboardButtons[i])) {
            current_pressed_idx = i;
            hPressedButton = hKeyboardButtons[i];
            break;
        }
    }
    // Provjera pritiska na specijalne tastere
    if (!hPressedButton) {
        for (int i = 0; i < 5; i++) {
            if (WM_IsWindow(hKeyboardSpecialButtons[i]) && BUTTON_IsPressed(hKeyboardSpecialButtons[i])) {
                // Koristimo negativan indeks da razlikujemo specijalne tastere
                current_pressed_idx = -(i + 1); 
                hPressedButton = hKeyboardSpecialButtons[i];
                break;
            }
        }
    }

    // Događaj se aktivira kada se taster OTPUSTI
    if (current_pressed_idx == -1 && button_pressed_idx != -1) {
        BuzzerOn(); HAL_Delay(1); BuzzerOff();

        int Id = WM_GetId(hPressedButton);

        if (Id >= GUI_ID_USER && Id < (GUI_ID_USER + (KEY_ROWS * KEYS_PER_ROW))) {
            // Pritisnut je taster sa karakterom
            if (keyboard_buffer_idx < g_keyboard_context.max_len) {
                
                // ISPRAVLJENI POZIV: Sva tri argumenta su prisutna
                char key_text[10]; // Privremeni bafer dovoljno velik za jedan karakter
                BUTTON_GetText(hPressedButton, key_text, sizeof(key_text));
                
                strcat(keyboard_buffer, key_text);
                keyboard_buffer_idx = strlen(keyboard_buffer);
            }
        } else {
            // Pritisnut je specijalni taster
            switch(Id) {
                case GUI_ID_SHIFT:
                    keyboard_shift_active = !keyboard_shift_active;
                    DSP_KillKeyboardScreen(); // Obriši stare tastere
                    DSP_InitKeyboardScreen(); // Iscrtaj nove (mala/velika slova)
                    return; // Prekini dalje izvršavanje jer je ekran ponovo iscrtan
                
                case GUI_ID_BACKSPACE:
                    if (keyboard_buffer_idx > 0) {
                        keyboard_buffer[--keyboard_buffer_idx] = '\0';
                    }
                    break;
                
                case GUI_ID_SPACE:
                    if (keyboard_buffer_idx < g_keyboard_context.max_len) {
                        keyboard_buffer[keyboard_buffer_idx++] = ' ';
                        keyboard_buffer[keyboard_buffer_idx] = '\0';
                    }
                    break;

                case GUI_ID_OKAY:
                    strncpy(g_keyboard_result.value, keyboard_buffer, sizeof(g_keyboard_result.value));
                    g_keyboard_result.value[sizeof(g_keyboard_result.value) - 1] = '\0'; // Osiguraj NULL terminator
                    g_keyboard_result.is_confirmed = true;
                    DSP_KillKeyboardScreen(); // Obriši tastaturu prije povratka
                    screen = keyboard_return_screen;
                    return; // Prekini dalje izvršavanje
            }
        }
        
        // Ažuriraj prikaz teksta, osim ako se ekran već mijenja
        if (screen == SCREEN_KEYBOARD_ALPHA) {
            GUI_MULTIBUF_BeginEx(1);
            const int x_start = (LCD_GetXSize() - (10 * 42 + 9 * 5)) / 2; // Ponovo izracunaj x_start
            GUI_ClearRect(x_start, 35, x_start + 42 * 10 + 5 * 9, 55); // Očisti samo područje teksta
            GUI_SetColor(GUI_ORANGE);
            GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
            GUI_DispStringAt(keyboard_buffer, x_start, 40);
            GUI_MULTIBUF_EndEx(1);
        }
    }

    button_pressed_idx = current_pressed_idx;
}

/******************************************************************************
 * @brief       Uništava sve GUI widgete kreirane za alfanumeričku tastaturu.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Poziva se prilikom napuštanja `SCREEN_KEYBOARD_ALPHA` ekrana.
 * @param       None
 * @retval      None
 *****************************************************************************/
static void DSP_KillKeyboardScreen(void)
{
    // Brisanje tastera sa karakterima
    for (int i = 0; i < (KEY_ROWS * KEYS_PER_ROW); i++) {
        if (WM_IsWindow(hKeyboardButtons[i])) {
            WM_DeleteWindow(hKeyboardButtons[i]);
            hKeyboardButtons[i] = 0;
        }
    }
    // Brisanje specijalnih tastera
    for (int i = 0; i < 5; i++) {
        if (WM_IsWindow(hKeyboardSpecialButtons[i])) {
            WM_DeleteWindow(hKeyboardSpecialButtons[i]);
            hKeyboardSpecialButtons[i] = 0;
        }
    }
}
/******************************************************************************
 * @brief       Priprema i prebacuje sistem na ekran alfanumeričke tastature.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Pamti ekran sa kojeg je pozvana radi povratka.
 * @param       context Pokazivač na KeyboardContext_t strukturu.
 * @retval      None
 *****************************************************************************/
static void Display_ShowKeyboard(const KeyboardContext_t* context)
{
    keyboard_return_screen = screen;

    if (context != NULL) {
        memcpy(&g_keyboard_context, context, sizeof(KeyboardContext_t));
    } else {
        memset(&g_keyboard_context, 0, sizeof(KeyboardContext_t));
        g_keyboard_context.title = "Greska";
    }

    memset(&g_keyboard_result, 0, sizeof(KeyboardResult_t));
    keyboard_shift_active = false; // Uvijek počni sa malim slovima

    screen = SCREEN_KEYBOARD_ALPHA;
    shouldDrawScreen = 1;
}

/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
