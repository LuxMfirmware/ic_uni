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
#define LIGHT_ICON_COUNT                2       ///< Svrha: Ukupan broj različitih tipova ikonica za svjetla. Vrijednost: 2 (sijalica i ventilator).
#define LIGHT_ICON_ID_BULB              0       ///< Svrha: Jedinstveni ID za ikonicu sijalice. Vrijednost: 0.
#define LIGHT_ICON_ID_VENTILATOR        1       ///< Svrha: Jedinstveni ID za ikonicu ventilatora. Vrijednost: 1.
/** @} */

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
/**
 * @brief Definicija opšteg tipa za zonu dodira (pravougaonik).
 * @note  Olakšava kreiranje i rad sa layout strukturama.
 */
typedef struct {
    int16_t x0, y0, x1, y1;
} TouchZone_t;

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
    TouchZone_t white_square_zone;      /**< Zona za odabir bijele boje. */
    TouchZone_t brightness_slider_zone; /**< Zona za slajder svjetline. */
    TouchZone_t color_palette_zone;     /**< Zona za paletu boja. */
}
light_settings_screen_layout =
{
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
    .main_switch_zone = { .x0 = 100, .y0 = 100, .x1 = 400, .y1 = 272 }
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
}
main_screen_layout =
{
    .circle_center_x = 240,
    .circle_center_y = 136,
    .circle_radius_x = 50,
    .circle_radius_y = 50
};
/**
 * @brief Struktura koja sadrži konstante za ISCRTAVANJE elemenata
 * na prvom ekranu za odabir.
 */
static const struct
{
    // Geometrija linija
    int16_t x_separator_pos;        /**< X pozicija vertikalne linije desno. */
    int16_t x_mid_line_pos;         /**< X pozicija centralne vertikalne linije. */
    int16_t y_mid_line_pos;         /**< Y pozicija centralne horizontalne linije. */
    int16_t vert_line_y_padding;    /**< Vertikalni razmak (padding) za linije. */
    int16_t horiz_line_x_padding;   /**< Horizontalni razmak (padding) za linije. */
    
    // Pozicije centara kvadranta
    int16_t x_center_left;          /**< X koordinata centra lijeve polovine ekrana. */
    int16_t x_center_right;         /**< X koordinata centra desne polovine ekrana. */
    int16_t y_center_top;           /**< Y koordinata centra gornje polovine ekrana. */
    int16_t y_center_bottom;        /**< Y koordinata centra donje polovine ekrana. */
    
    // Ofseti za elemente
    int16_t icon_vertical_offset;   /**< Vertikalni pomak za ikonice. */
    int16_t text_vertical_offset;   /**< Vertikalni pomak za tekst ispod ikonica. */

    // Pozicija "NEXT" dugmeta
    int16_t next_button_y_center;   /**< Y koordinata centra "NEXT" dugmeta. */
}
select_screen1_drawing_layout =
{
    .x_separator_pos        = DRAWING_AREA_WIDTH,
    .x_mid_line_pos         = DRAWING_AREA_WIDTH / 2,
    .y_mid_line_pos         = 136,
    .vert_line_y_padding    = 10,
    .horiz_line_x_padding   = 30,
    
    .x_center_left          = (DRAWING_AREA_WIDTH / 2) / 2,
    .x_center_right         = (DRAWING_AREA_WIDTH / 2) + (DRAWING_AREA_WIDTH / 2) / 2,
    .y_center_top           = 136 / 2,
    .y_center_bottom        = 136 + (272 - 136) / 2,
    
    .icon_vertical_offset   = -10,
    .text_vertical_offset   = 10,

    .next_button_y_center   = 192
};
/**
 * @brief Struktura koja sadrži konstante za ISCRTAVANJE elemenata
 * na drugom ekranu za odabir (Clean, WiFi, App).
 */
static const struct
{
    /** @brief X pozicija prve vertikalne linije koja dijeli ekran na trećine. */
    int16_t x_line1_pos;
    /** @brief X pozicija druge vertikalne linije koja dijeli ekran na trećine. */
    int16_t x_line2_pos;
    /** @brief X pozicija krajnje desne vertikalne linije. */
    int16_t x_separator_pos;

    /** @brief X koordinata centra prve kolone. */
    int16_t x_center_col1;
    /** @brief X koordinata centra druge kolone. */
    int16_t x_center_col2;
    /** @brief X koordinata centra treće kolone. */
    int16_t x_center_col3;

    /** @brief Y koordinata centra za sve ikonice. */
    int16_t y_icon_center;
    /** @brief Y koordinata za ispis teksta ispod ikonica. */
    int16_t y_text_pos;

    /** @brief Y koordinata centra za "NEXT" dugme. */
    int16_t y_next_button_center;

    /** @brief Početna Y koordinata za linije između kolona. */
    int16_t col_line_y_start;
    /** @brief Krajnja Y koordinata za linije između kolona. */
    int16_t col_line_y_end;

}
select_screen2_drawing_layout =
{
    .x_line1_pos            = DRAWING_AREA_WIDTH / 3,
    .x_line2_pos            = (DRAWING_AREA_WIDTH / 3) * 2,
    .x_separator_pos        = DRAWING_AREA_WIDTH,

    .x_center_col1          = (DRAWING_AREA_WIDTH / 3) / 2,
    .x_center_col2          = (DRAWING_AREA_WIDTH / 3) + ((DRAWING_AREA_WIDTH / 3) / 2),
    .x_center_col3          = ((DRAWING_AREA_WIDTH / 3) * 2) + ((DRAWING_AREA_WIDTH / 3) / 2),
    
    .y_icon_center          = 116,
    .y_text_pos             = 176,
    .y_next_button_center   = 192,

    .col_line_y_start       = 60,
    .col_line_y_end         = 212,
};
/**
 * @brief Pomoćna struktura za definisanje pozicije i dimenzija widget-a.
 */
typedef struct {
    int16_t x, y, w, h;
} WidgetRect_t;

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

/** @brief Pomoćna struktura za definisanje horizontalne linije. */
typedef struct { int16_t y, x0, x1; } HLine_t;

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
    .set_defaults_button_pos    = { 10, 190, 80, 30 },
    .restart_button_pos         = { 10, 230, 80, 30 },
    .next_button_pos            = { 410, 180, 60, 30 },
    .save_button_pos            = { 410, 230, 60, 30 },

    .device_id_label_pos        = { {130, 20}, {130, 32} },
    .curtain_move_time_label_pos= { {130, 70}, {130, 82} },
    
    .language_dropdown_pos      = { 220, 10, 150, 120 },
    .language_label_pos         = { 380, 25 } // 220 (x) + 150 (w) + 10 (padding)
};

/** @brief Niz sa pokazivačima na bitmape za ikonice svjetala. Premješteno iz `lights.c`. */
static GUI_CONST_STORAGE GUI_BITMAP* light_modbus_images[] = {
    &bmSijalicaOff, &bmSijalicaOn,
    &bmVENTILATOR_OFF, &bmVENTILATOR_ON
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
// ===        Automatsko generisanje `const` niza za "Skener"      ===
//
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
static LIGHT_settingsWidgets lightsWidgets[LIGHTS_MODBUS_SIZE]; // Niz struktura za postavke svjetala
static Defroster_settingsWidgets defroster_settingWidgets;
static DROPDOWN_Handle hDRPDN_Language;

/*============================================================================*/
/* GLOBALNE VARIJABLE NA NIVOU PROJEKTA                                        */
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
static uint8_t clrtmr = 0;                                  // Fleg za odbrojavanje na ekranu za ciscenje.

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
static void DSP_KillSet1Scrn(void);
static void DSP_KillSet2Scrn(void);
static void DSP_KillSet3Scrn(void);
static void DSP_KillSet4Scrn(void);
static void DSP_KillSet5Scrn(void);
static void DSP_KillSet6Scrn(void);
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
static void Service_ThermostatScreen(void);
static void Service_ReturnToFirst(void);
static void Service_CleanScreen(void);
static void Service_SettingsScreen_1(void);
static void Service_SettingsScreen_2(void);
static void Service_SettingsScreen_3(void);
static void Service_SettingsScreen_4(void);
static void Service_SettingsScreen_5(void);
static void Service_SettingsScreen_6(void);
static void Service_LightsScreen(void);
static void Service_CurtainsScreen(void);
static void Service_QrCodeScreen(void);
static void Service_LightSettingsScreen(void);
static void Service_ResetMenuSwitches(void);
/** @} */

/**
 * @name Funkcije za Obradu Događaja (Events)
 * @brief Centralne funkcije za obradu korisničkog unosa i pozadinskih događaja.
 * @{
 */

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
static void HandlePress_ThermostatScreen(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_LightsScreen(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_CurtainsScreen(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_SelectScreen2(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_LightSettingsScreen(GUI_PID_STATE * pTS);
static void HandlePress_ResetMenuSwitchesScreenArea(GUI_PID_STATE * pTS);
static void HandleRelease_MainScreenLogic(GUI_PID_STATE * pTS);
static void HandleRelease_ResetMenuSwitchesScreenArea(GUI_PID_STATE * pTS);
/** @} */

/**
 * @name Pomoćne (Utility) Funkcije
 * @brief Grupa internih funkcija za razne zadatke kao što su snimanje,
 * iscrtavanje zajedničkih elemenata, upravljanje menijima, itd.
 * @{
 */

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
static void DrawHamburgerMenu(void);

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

    // Forsirano iscrtavanje glavnog ekrana odmah nakon inicijalizacije
    GUI_MULTIBUF_BeginEx(1);
    GUI_Clear();
    DrawHamburgerMenu();

    // Provjeravamo da li je ijedno svjetlo upaljeno koristeći novu funkciju.
    if (LIGHTS_isAnyLightOn()) {
        GUI_SetColor(GUI_GREEN);
    } else {
        GUI_SetColor(GUI_RED);
    }
    GUI_DrawEllipse(240, 136, 50, 50);
    GUI_MULTIBUF_EndEx(1);

    // Ažurira se shouldDrawScreen za prvu petlju
    shouldDrawScreen = 1;

    // NOVO: Postavlja se ispravno početno stanje
    screen = SCREEN_MAIN;
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
    case SCREEN_CLEAN:
        Service_CleanScreen();
        break;
    case SCREEN_LIGHTS:
        Service_LightsScreen();
        break;
    case SCREEN_CURTAINS:
        Service_CurtainsScreen();
        break;
    case SCREEN_QR_CODE:
        Service_QrCodeScreen();
        break;
    case SCREEN_LIGHT_SETTINGS:
        Service_LightSettingsScreen();
        break;
    case SCREEN_RESET_MENU_SWITCHES:
        Service_ResetMenuSwitches();
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
 * @brief Postavlja svjetlinu pozadinskog osvjetljenja.
 * @param val Vrijednost svjetline (od 1 do 90).
 */
void DISPSetBrightnes(uint8_t val)
{
    if (val < DISP_BRGHT_MIN) val = DISP_BRGHT_MIN;
    else if (val > DISP_BRGHT_MAX) val = DISP_BRGHT_MAX;
    __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, (uint16_t)(val * 10U));
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
            (screen < SCREEN_SETTINGS_1))
        {
            touch_in_menu_zone = true; // Postavi fleg da je dodir počeo u zoni menija
            click = 1;                 // Svaki dodir u ovoj zoni generiše zvučni signal

            switch(screen) { // Logika povratka na prethodni ekran
            case SCREEN_THERMOSTAT:
            case SCREEN_LIGHTS:
            case SCREEN_CURTAINS:
            case SCREEN_SELECT_2:
                screen = SCREEN_SELECT_1;
                menu_lc = 0;
                break;
            case SCREEN_SELECT_1:
                screen = SCREEN_RETURN_TO_FIRST;
                break;
            case SCREEN_QR_CODE:
                screen = SCREEN_SELECT_2;
                shouldDrawScreen = 1;
                break;
            case SCREEN_LIGHT_SETTINGS:
                screen = SCREEN_LIGHTS;
                shouldDrawScreen = 1;
                break;
            case SCREEN_MAIN:
                screen = SCREEN_SELECT_1;
                break;
            }
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

/*============================================================================*/
/* IMPLEMENTACIJA STATIČKIH (PRIVATNIH) FUNKCIJA                              */
/*============================================================================*/
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
}
/**
 * @brief Iscrtava ikonu hamburger menija u gornjem desnom uglu.
 * @note Ova funkcija centralizuje logiku iscrtavanja kako bi se izbjeglo
 * ponavljanje koda. Koristi fiksne koordinate za poziciju.
 */
static void DrawHamburgerMenu(void)
{
    const int xStart = 400;
    const int xEnd = 450;
    const int yStart = 20;
    const int yGap = 20;

    GUI_SetPenSize(9);
    GUI_SetColor(clk_clrs[g_display_settings.scrnsvr_clk_clr]);
    GUI_DrawLine(xStart, yStart, xEnd, yStart);
    GUI_DrawLine(xStart, yStart + yGap, xEnd, yStart + yGap);
    GUI_DrawLine(xStart, yStart + (yGap * 2), xEnd, yStart + (yGap * 2));
}
/**
 * @brief Provjerava status ažuriranja firmvera i prikazuje odgovarajuću poruku.
 * @note  Ova funkcija blokira ostatak GUI logike dok je ažuriranje u toku.
 * @return uint8_t 1 ako je ažuriranje aktivno, inače 0.
 */
static uint8_t Service_HandleFirmwareUpdate(void)
{
    static uint8_t fwmsg = 2; // Statički fleg za praćenje stanja iscrtavanja poruke

    // Provjeri globalni fleg koji postavlja RS485 modul
    if (IsFwUpdateActiv()) {
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
            GUI_DispStringAt(lng(TXT_FIRMWARE_UPDATE), 240, 135);
            GUI_MULTIBUF_EndEx(1);
            DISPResetScrnsvr();
        }
        return 1; // Vrati 1 da signalizira da je ažuriranje u toku
    }
    // Ako je ažuriranje upravo završeno (fleg je bio 1, a sada je IsFwUpdateActiv() false)
    else if (fwmsg == 1) {
        fwmsg = 0; // Resetuj fleg
        scrnsvr_tmr = 0; // Resetuj tajmer za screensaver
    }
    // Jednokratno iscrtavanje inicijalne grafike na samom početku
    else if (fwmsg == 2) {
        fwmsg = 0;
        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        // Iscrtaj hamburger meni ikonicu
        DrawHamburgerMenu();
        GUI_MULTIBUF_EndEx(1);
    }
    return 0; // Ažuriranje nije aktivno
}
/**
 * @brief Servisira glavni ekran.
 * @note  Ova funkcija se poziva u petlji kada je `screen == SCREEN_MAIN`.
 * Odgovorna je za resetovanje internih flegova menija i za iscrtavanje
 * osnovnih elemenata glavnog ekrana (hamburger meni i crveni/zeleni krug
 * koji indicira stanje svjetala).
 * Optimizovana je tako da se ponovno iscrtavanje dešava samo kada je
 * to neophodno (kada se promijeni stanje svjetala ili kada se forsira).
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
        DrawHamburgerMenu(); // Crtanje hamburger menija

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
 * @brief Servisira ekran za odabir glavnih kontrola (Svjetla, Termostat, Roletne, Dinamička ikona).
 * @note  Finalna, ispravna verzija koja koristi sinhronizovani TextID enum, tabelu prevoda,
 * uklonjene magične brojeve i prati originalnu logiku iscrtavanja sa ispravnim
 * pozicioniranjem teksta i kompletnim Doxygen komentarima.
 */
static void Service_SelectScreen1(void)
{
    /**
     * @brief Pokazivači (handle-ovi) na module čije stanje se prikazuje na ovom ekranu.
     * @note  Dobavljaju se na početku funkcije kako bi bili dostupni za provjeru stanja.
     */
    Defroster_Handle* defHandle = Defroster_GetInstance();
    Ventilator_Handle* ventHandle = Ventilator_GetInstance();
    
    /**
     * @brief Pokazivači na bitmape koje se koriste za iscrtavanje ikonica.
     * @note  Definisani su kao `const` jer se njihove vrijednosti ne mijenjaju tokom izvršavanja funkcije.
     */
    const GUI_BITMAP* iconLights = &bmSijalicaOff;
    const GUI_BITMAP* iconThermostat = &bmTermometar;
    const GUI_BITMAP* iconCurtains = &bmblindMedium;
    const GUI_BITMAP* iconDefroster = &bmdefrosterico;
    const GUI_BITMAP* iconVentilator = &bmVENTILATOR_OFF;
    const GUI_BITMAP* iconNext = &bmnext;
    const GUI_BITMAP* iconDefrosterOn = &bmdefrostericoOn;
    const GUI_BITMAP* iconVentilatorOn = &bmVENTILATOR_ON;
    
    /**
     * @brief Varijable za dinamičko određivanje ikonice i teksta u četvrtom kvadrantu.
     * @note  Ove varijable se popunjavaju u `switch` bloku na osnovu globalnih podešavanja (`g_display_settings`).
     */
    const GUI_BITMAP* dynamicIcon = NULL;
    TextID dynamicTextID = TXT_DUMMY;
    bool is_active = false;

    /**
     * @brief Određuje koja dinamička ikonica i tekst će biti prikazani.
     * @note  Ova `switch` petlja provjerava `selected_control_mode` iz globalnih
     * podešavanja i postavlja odgovarajuće vrijednosti za `dynamicIcon` i `dynamicTextID`.
     */
    switch(g_display_settings.selected_control_mode) {
    case MODE_DEFROSTER:
        is_active = Defroster_isActive(defHandle);
        dynamicIcon = is_active ? iconDefrosterOn : iconDefroster;
        dynamicTextID = TXT_DEFROSTER;
        break;
    case MODE_VENTILATOR:
        is_active = Ventilator_isActive(ventHandle);
        dynamicIcon = is_active ? iconVentilatorOn : iconVentilator;
        dynamicTextID = TXT_VENTILATOR;
        break;
    case MODE_OFF:
        // Ako je mod isključen, ne prikazuje se ni ikonica ni tekst.
        break;
    }
    
    /**
     * @brief Glavna logika za iscrtavanje ekrana.
     * @note  Koristi statičku varijablu `menu_lc` kao "state machine" fleg da bi se
     * kompletno iscrtavanje izvršilo samo jednom (kada je `menu_lc == 0`).
     */
    if (menu_lc == 0) {
        menu_lc = 1; // Postavlja fleg da je inicijalno iscrtavanje završeno.

        /**
         * @brief Inicijalizacija iscrtavanja.
         * @note  Pokreće se višestruko baferovanje i čiste se oba grafička sloja
         * kako bi se pripremilo "platno" za nove elemente.
         */
        GUI_MULTIBUF_BeginEx(1);
        GUI_SelectLayer(0);
        GUI_Clear();
        GUI_SelectLayer(1);
        GUI_SetBkColor(GUI_TRANSPARENT);
        GUI_Clear();

        /**
         * @brief Iscrtavanje statičkih elemenata interfejsa.
         * @note  Ovdje se iscrtavaju elementi koji se ne mijenjaju: hamburger meni i linije
         * koje dijele ekran na četiri kvadranta. Pozicije se uzimaju iz
         * `select_screen1_drawing_layout` strukture.
         */
        DrawHamburgerMenu();
        
        GUI_DrawLine(select_screen1_drawing_layout.x_separator_pos, select_screen1_drawing_layout.vert_line_y_padding, 
                     select_screen1_drawing_layout.x_separator_pos, 272 - select_screen1_drawing_layout.vert_line_y_padding);

        GUI_DrawLine(select_screen1_drawing_layout.horiz_line_x_padding, select_screen1_drawing_layout.y_mid_line_pos, 
                     select_screen1_drawing_layout.x_separator_pos - select_screen1_drawing_layout.horiz_line_x_padding, select_screen1_drawing_layout.y_mid_line_pos);

        GUI_DrawLine(select_screen1_drawing_layout.x_mid_line_pos, select_screen1_drawing_layout.vert_line_y_padding + 10, 
                     select_screen1_drawing_layout.x_mid_line_pos, 272 - (select_screen1_drawing_layout.vert_line_y_padding + 10));

        /**
         * @brief Izračunavanje pozicija za ikonice.
         * @note  Koordinate za svaku ikonicu se računaju na osnovu centra odgovarajućeg
         * kvadranta (definisanog u layout strukturi) i dimenzija same ikonice,
         * kako bi ikonice uvijek bile savršeno centrirane.
         */
        const uint16_t xLights = select_screen1_drawing_layout.x_center_left - (iconLights->XSize / 2);
        const uint16_t yLights = select_screen1_drawing_layout.y_center_top - (iconLights->YSize / 2) + select_screen1_drawing_layout.icon_vertical_offset;
        const uint16_t xThermostat = select_screen1_drawing_layout.x_center_right - (iconThermostat->XSize / 2);
        const uint16_t yThermostat = select_screen1_drawing_layout.y_center_top - (iconThermostat->YSize / 2) + select_screen1_drawing_layout.icon_vertical_offset;
        const uint16_t xCurtains = select_screen1_drawing_layout.x_center_left - (iconCurtains->XSize / 2);
        const uint16_t yCurtains = select_screen1_drawing_layout.y_center_bottom - (iconCurtains->YSize / 2) + select_screen1_drawing_layout.icon_vertical_offset;
        const uint16_t xDynamic = select_screen1_drawing_layout.x_center_right - (dynamicIcon ? dynamicIcon->XSize / 2 : iconDefroster->XSize / 2);
        const uint16_t yDynamic = select_screen1_drawing_layout.y_center_bottom - (dynamicIcon ? dynamicIcon->YSize / 2 : iconDefroster->YSize / 2) + select_screen1_drawing_layout.icon_vertical_offset;

        /**
         * @brief Iscrtavanje bit mapa (ikonica) na izračunatim pozicijama.
         */
        GUI_DrawBitmap(iconLights, xLights, yLights);
        GUI_DrawBitmap(iconThermostat, xThermostat, yThermostat);
        GUI_DrawBitmap(iconCurtains, xCurtains, yCurtains);
        if(dynamicIcon) {
            GUI_DrawBitmap(dynamicIcon, xDynamic, yDynamic);
        }
        GUI_DrawBitmap(iconNext, select_screen1_drawing_layout.x_separator_pos + 5, select_screen1_drawing_layout.next_button_y_center - (iconNext->YSize / 2));

        /**
         * @brief Podešavanje fonta i ispisivanje tekstualnih labela ispod ikonica.
         * @note  Y pozicija se računa kao centar kvadranta + pola visine ikonice + mali ofset,
         * čime se tekst ispravno i konzistentno pozicionira ISPOD svake ikonice.
         */
        GUI_SetFont(&GUI_FontVerdana20); 
        GUI_SetColor(GUI_ORANGE);
        GUI_SetTextMode(GUI_TM_TRANS);
        
        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt(lng(TXT_LIGHTS),     select_screen1_drawing_layout.x_center_left,  select_screen1_drawing_layout.y_center_top    + (iconLights->YSize / 2) + select_screen1_drawing_layout.text_vertical_offset);
        
        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt(lng(TXT_THERMOSTAT), select_screen1_drawing_layout.x_center_right, select_screen1_drawing_layout.y_center_top    + (iconThermostat->YSize / 2) + select_screen1_drawing_layout.text_vertical_offset);
        
        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt(lng(TXT_BLINDS),     select_screen1_drawing_layout.x_center_left,  select_screen1_drawing_layout.y_center_bottom + (iconCurtains->YSize / 2) + select_screen1_drawing_layout.text_vertical_offset);
        
        if(dynamicTextID != TXT_DUMMY) {
            GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
            GUI_DispStringAt(lng(dynamicTextID), select_screen1_drawing_layout.x_center_right, select_screen1_drawing_layout.y_center_bottom + (dynamicIcon->YSize / 2) + select_screen1_drawing_layout.text_vertical_offset);
        }
        
        /**
         * @brief Završetak operacije iscrtavanja.
         * @note  Prikazuje promjene na ekranu i resetuje `thermostatMenuState`.
         */
        GUI_MULTIBUF_EndEx(1);
        thermostatMenuState = 0;
        
    } else if (menu_lc == 1) {
        /**
         * @brief Ažuriranje dinamičke ikone.
         * @note  Ako je `dynamicIconUpdateFlag` postavljen (npr. nakon pritiska),
         * ovaj blok koda će samo obrisati staru i iscrtati novu dinamičku ikonicu,
         * bez potrebe za ponovnim iscrtavanjem cijelog ekrana.
         */
        if (dynamicIconUpdateFlag) {
            dynamicIconUpdateFlag = 0;
            const uint16_t xDynamic = select_screen1_drawing_layout.x_center_right - (dynamicIcon ? dynamicIcon->XSize / 2 : iconDefroster->XSize / 2);
            const uint16_t yDynamic = select_screen1_drawing_layout.y_center_bottom - (dynamicIcon ? dynamicIcon->YSize / 2 : iconDefroster->YSize / 2) + select_screen1_drawing_layout.icon_vertical_offset;
            
            GUI_MULTIBUF_BeginEx(1);
            GUI_ClearRect(xDynamic, yDynamic, xDynamic + (dynamicIcon ? dynamicIcon->XSize : iconDefroster->XSize), yDynamic + (dynamicIcon ? dynamicIcon->YSize : iconDefroster->YSize));
            if(dynamicIcon) {
                GUI_DrawBitmap(dynamicIcon, xDynamic, yDynamic);
            }
            GUI_MULTIBUF_EndEx(1);
        }
    }
}
/**
 * @brief Servisira drugi ekran za odabir (čišćenje, Wi-Fi QR, App QR).
 * @note  Ova funkcija iscrtava tri opcije sa ikonama i tekstom, te omogućava prelazak
 * na odgovarajuće ekrane. Cjelokupan raspored elemenata je definisan u
 * `select_screen2_drawing_layout` strukturi. Iscrtavanje se vrši samo
 * pri prvom ulasku na ekran (`shouldDrawScreen` fleg).
 */
static void Service_SelectScreen2(void)
{
    if(shouldDrawScreen) {
        shouldDrawScreen = 0;

        /**
         * @brief Inicijalizacija iscrtavanja.
         * @note  Pokreće se višestruko baferovanje i čiste se oba grafička sloja.
         */
        GUI_MULTIBUF_BeginEx(1);
        GUI_SelectLayer(0);
        GUI_Clear();
        GUI_SelectLayer(1);
        GUI_SetBkColor(GUI_TRANSPARENT);
        GUI_Clear();

        /**
         * @brief Iscrtavanje statičkih elemenata interfejsa (linije i meni).
         * @note  Sve pozicije se dobijaju iz `select_screen2_drawing_layout` strukture.
         */
        DrawHamburgerMenu();
        GUI_DrawLine(select_screen2_drawing_layout.x_separator_pos, 10, select_screen2_drawing_layout.x_separator_pos, 262);
        GUI_DrawLine(select_screen2_drawing_layout.x_line1_pos, select_screen2_drawing_layout.col_line_y_start, select_screen2_drawing_layout.x_line1_pos, select_screen2_drawing_layout.col_line_y_end);
        GUI_DrawLine(select_screen2_drawing_layout.x_line2_pos, select_screen2_drawing_layout.col_line_y_start, select_screen2_drawing_layout.x_line2_pos, select_screen2_drawing_layout.col_line_y_end);

        /**
         * @brief Pointeri na bitmape koje se koriste za iscrtavanje.
         */
        const GUI_BITMAP* iconNext = &bmnext;
        const GUI_BITMAP* iconClean = &bmCLEAN;
        const GUI_BITMAP* iconWifi = &bmwifi;
        const GUI_BITMAP* iconApp = &bmmobilePhone;
        
        /**
         * @brief Iscrtavanje "NEXT" dugmeta.
         */
        GUI_DrawBitmap(iconNext, select_screen2_drawing_layout.x_separator_pos + 5, select_screen2_drawing_layout.y_next_button_center - (iconNext->YSize / 2));

        /**
         * @brief Iscrtavanje ikona na izračunatim, centriranim pozicijama.
         * @note  Pozicije se računaju na osnovu centra kolone i dimenzija same ikonice.
         */
        GUI_DrawBitmap(iconClean, select_screen2_drawing_layout.x_center_col1 - (iconClean->XSize / 2), select_screen2_drawing_layout.y_icon_center - (iconClean->YSize / 2));
        GUI_DrawBitmap(iconWifi,  select_screen2_drawing_layout.x_center_col2 - (iconWifi->XSize / 2),  select_screen2_drawing_layout.y_icon_center - (iconWifi->YSize / 2));
        GUI_DrawBitmap(iconApp,   select_screen2_drawing_layout.x_center_col3 - (iconApp->XSize / 2),   select_screen2_drawing_layout.y_icon_center - (iconApp->YSize / 2));

        /**
         * @brief Podešavanje fonta i ispisivanje tekstualnih labela ispod ikonica.
         */
        GUI_SetFont(&GUI_FontVerdana20); 
        GUI_SetColor(GUI_ORANGE);
        GUI_SetTextMode(GUI_TM_TRANS);

        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt(lng(TXT_CLEAN), select_screen2_drawing_layout.x_center_col1, select_screen2_drawing_layout.y_text_pos);

        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt(lng(TXT_WIFI), select_screen2_drawing_layout.x_center_col2, select_screen2_drawing_layout.y_text_pos);

        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt(lng(TXT_APP), select_screen2_drawing_layout.x_center_col3, select_screen2_drawing_layout.y_text_pos);

        /**
         * @brief Završetak operacije iscrtavanja i prikaz na ekranu.
         */
        GUI_MULTIBUF_EndEx(1);
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
        DrawHamburgerMenu();

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
 * @brief Servisira ekran za čišćenje ekrana (privremeno onemogućava dodir).
 * @note Prikazuje tajmer sa odbrojavanjem. Nakon što tajmer istekne, vraća
 * se na početni ekran.
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
            GUI_SetFont(GUI_FONT_32_1);
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
    
    /** @brief Ažuriranje statusa screensaver-a na osnovu checkbox-a. */
    if (CHECKBOX_GetState(hCHKBX_ScrnsvrClock) == 1) {
        ScrnsvrClkSet();
    } else {
        ScrnsvrClkReset();
    }
    g_display_settings.scrnsvr_on_off = IsScrnsvrClkActiv();

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
        if (thsta) { THSTAT_Save(pThst); thsta = 0; }
        if (lcsta) { LIGHTS_Save(); lcsta = 0; }

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
 * @brief Servisira peti ekran podešavanja (Modbus svjetla).
 * @note Ova funkcija u petlji provjerava da li je korisnik promijenio vrijednost
 * nekog widgeta. Ako jeste, poziva odgovarajuću `setter` funkciju iz `lights` API-ja
 * da ažurira stanje u RAM-u. Stvarno snimanje u EEPROM se dešava tek pritiskom
 * na 'SAVE' ili 'NEXT' dugme, pozivom jedne `LIGHTS_Save()` funkcije.
 */
static void Service_SettingsScreen_5(void)
{
    GUI_MULTIBUF_BeginEx(1);

    // Dohvatamo indeks svjetla koje trenutno konfigurišemo.
    uint8_t light_index = lightsModbusSettingsMenu;

    // =======================================================================
    // === POČETAK REFAKTORISANJA ===

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
    // Display modul više ne piše direktno u '.config' strukturu, već samo
    // "govori" lights modulu šta da uradi.
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

    // KORAK 3: Ažuriranje prikaza ikonice sa strane, sada koristeći lokalnu logiku.
    GUI_ClearRect(380, 0, 480, 100);
    uint8_t icon_id = LIGHT_GetIconID(handle);
    bool is_active = LIGHT_isActive(handle);
    GUI_CONST_STORAGE GUI_BITMAP* icon_to_draw = light_modbus_images[(icon_id * 2) + is_active];
    GUI_DrawBitmap(icon_to_draw, 480 - icon_to_draw->XSize, 0);

    // KORAK 4: Refaktorisanje logike za "OK" i "NEXT" dugmad.
    if (BUTTON_IsPressed(hBUTTON_Ok) || BUTTON_IsPressed(hBUTTON_Next))
    {
        if(settingsChanged)
        {
            // Pozivamo JEDNU funkciju iz API-ja. `lights` modul je sada interno
            // odgovoran za eventualnu defragmentaciju i snimanje u EEPROM.
            LIGHTS_Save();
            settingsChanged = 0;
        }

        // Logika prelaska na sljedeći ekran ostaje uglavnom ista.
        if (BUTTON_IsPressed(hBUTTON_Ok))
        {
            DSP_KillSet5Scrn();
            screen = SCREEN_RETURN_TO_FIRST;
            shouldDrawScreen = 1;
        }
        else if (BUTTON_IsPressed(hBUTTON_Next))
        {
            // Provjera da li je posljednji slot
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

    // === KRAJ REFAKTORISANJA ===
    // =======================================================================

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
        /** @brief << NOVO: Provjera promjene u DROPDOWN-u za jezik >> */
        if (g_display_settings.language != DROPDOWN_GetSel(hDRPDN_Language)) {
            g_display_settings.language = DROPDOWN_GetSel(hDRPDN_Language);
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
        DSP_InitSet1Scrn();
        screen = SCREEN_SETTINGS_1;
    }
}
/**
 * @brief Servisira ekran sa svjetlima.
 * @note Ova funkcija dinamički iscrtava ikone svjetala na osnovu broja
 * i rasporeda definisanog u konfiguraciji. Iscrtavanje se obavlja samo
 * kada je to neophodno, koristeći 'shouldDrawScreen' flag.
 * Refaktorisana je da koristi novi, enkapsulirani API `lights` modula.
 */
static void Service_LightsScreen(void)
{
    // Iscrtavanje se vrši samo ako je zatraženo, radi optimizacije.
    if(shouldDrawScreen) {
        shouldDrawScreen = 0; // Odmah resetujemo fleg

        // Korištenje višestrukog baferovanja za glatko iscrtavanje bez treperenja.
        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        DrawHamburgerMenu();

        // --- Logika za dinamički raspored ikonica na ekranu ---
        // Ova kompleksna logika računa pozicije ikonica tako da budu
        // estetski raspoređene, u zavisnosti od toga koliko ih ima.
        int y = (LIGHTS_Rows_getCount() > 1) ? 10 : 86;
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

                // Prvo izračunamo apsolutni indeks svjetla u nizu.
                uint8_t absolute_light_index = lightsInRowSum + idx_in_row;

                // =======================================================================
                // === POČETAK REFAKTORISANJA ===
                //
                // KORAK 1: Umjesto direktnog pristupa nizu, pozivamo API funkciju
                // `LIGHTS_GetInstance()` da dobijemo siguran "handle" na svjetlo.
                LIGHT_Handle* handle = LIGHTS_GetInstance(absolute_light_index);

                // Uvijek je dobra praksa provjeriti da li je handle validan.
                if (handle) {

                    // KORAK 2: Dohvatamo sve potrebne podatke ISKLJUČIVO preko API funkcija.
                    // `display` modul više ne zna ništa o internoj strukturi svjetla.
                    uint8_t icon_id = LIGHT_GetIconID(handle); // Koji tip ikonice? (0=sijalica, 1=ventilator)
                    bool is_active = LIGHT_isActive(handle);  // Da li je svjetlo upaljeno? (true/false)

                    // KORAK 3: Logika za odabir sličice je SADA unutar display.c.
                    // Koristimo `icon_id` i `is_active` da izračunamo tačan indeks
                    // u našem lokalnom `light_modbus_images` nizu.
                    // Formula: (ID * 2) + stanje (0 za OFF, 1 za ON)
                    GUI_CONST_STORAGE GUI_BITMAP* icon_to_draw = light_modbus_images[(icon_id * 2) + is_active];

                    // Logika za X poziciju ostaje ista.
                    int x = (currentLightsMenuSpaceBetween * ((idx_in_row % lightsInRow) + 1)) + (80 * (idx_in_row % lightsInRow));

                    // KORAK 4: Iscrtavamo sličicu koju smo upravo odabrali.
                    GUI_DrawBitmap(icon_to_draw, x, y);
                }
                // === KRAJ REFAKTORISANJA ===
                // =======================================================================
            }
            lightsInRowSum += lightsInRow;
            y += 130;
        }
        GUI_MULTIBUF_EndEx(1);
    }
}

/**
 * @brief Servisira ekran sa zavjesama.
 * @note Ova funkcija dinamički iscrtava korisnički interfejs za kontrolu zavjesa.
 * Odgovorna je za crtanje trouglova za smjer kretanja, bijele linije kao vizualnog
 * separatora i navigacijskih dugmadi. Stanje trouglova (obojeno/neobojeno)
 * precizno reflektuje status odabrane zavjese ili grupe.
 */
static void Service_CurtainsScreen(void)
{
    // Glavna provjera: ponovo iscrtavamo ekran samo ako je došlo do promjene stanja.
    if(shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);

        GUI_Clear();

        // 1. CRTANJE GLOBALNIH UI ELEMENATA
        //--------------------------------------------------------------------------------
        // Iscrtavanje hamburger menija
        DrawHamburgerMenu();

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
            GUI_SetFont(GUI_FONT_32B_1); // Koristi font za tekst "SVE"
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
            GUI_DispStringAt(lng(TXT_ALL), 50, 50);
        }

        //--------------------------------------------------------------------------------

        // 2. CRTANJE GLAVNOG KONTROLNOG ELEMENTA (TROUGLOVA I LINIJE)
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
        DrawHamburgerMenu();

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
 * @brief Servisira ekran za podešavanje svjetla (dimmer i RGB).
 * @note Na ovom ekranu se prikazuju kontrole za svjetlinu i boju,
 * omogućavajući korisniku da ih direktno mijenja dodirom.
 * Refaktorisana je da koristi novi `lights` API i lokalne flegove za
 * jasniju logiku iscrtavanja.
 */
static void Service_LightSettingsScreen(void)
{
    if(shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);
        GUI_SelectLayer(0);
        GUI_Clear();
        GUI_SelectLayer(1);
        GUI_SetBkColor(GUI_TRANSPARENT);
        GUI_Clear();
        DrawHamburgerMenu();

        // --- Definicije konstanti za pozicioniranje (Layout) ---
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

        // =======================================================================
        // === POČETAK REFAKTORISANJA ===

        // KORAK 1: Definišemo lokalne flegove da bismo pojednostavili logiku.
        // Inicijalno su oba `false`.
        bool show_dimmer_slider = false;
        bool show_rgb_palette = false;

        // KORAK 2: Provjeravamo da li se radi o grupi ("SVA SVJETLA") ili o jednom svjetlu.
        if (light_selectedIndex == LIGHTS_MODBUS_SIZE) { // Sva svjetla su odabrana
            // Koristimo interni fleg 'lights_allSelected_hasRGB' koji je postavljen
            // prilikom ulaska na ovaj ekran da odredimo koje kontrole prikazati.
            if (lights_allSelected_hasRGB) {
                show_rgb_palette = true; // Ako u grupi ima RGB, prikaži sve
            } else {
                show_dimmer_slider = true; // Inače, prikaži samo dimer
            }
        } else { // Jedno, specifično svjetlo je odabrano

            // KORAK 3: Dobijamo siguran "handle" na odabrano svjetlo putem API-ja.
            LIGHT_Handle* handle = LIGHTS_GetInstance(light_selectedIndex);

            if (handle) {
                // KORAK 4: Koristimo API funkcije da pitamo `lights` modul za tip svjetla.
                if (LIGHT_isRGB(handle)) {
                    show_rgb_palette = true;
                } else if (LIGHT_isDimmer(handle)) {
                    show_dimmer_slider = true;
                }
            }
        }

        // KORAK 5: Na osnovu gore postavljenih flegova, iscrtavamo odgovarajuće kontrole.
        // Ova logika je sada odvojena i mnogo čistija.
        if (show_rgb_palette) {
            // Crtanje elemenata za RGB kontrolu (bijeli kvadrat, slajder, paleta boja)
            GUI_SetColor(GUI_WHITE);
            GUI_FillRect(WHITE_SQUARE_X0, WHITE_SQUARE_Y0, WHITE_SQUARE_X0 + WHITE_SQUARE_SIZE - 1, WHITE_SQUARE_Y0 + WHITE_SQUARE_SIZE - 1);
            GUI_DrawBitmap(&bmblackWhiteGradient, sliderX0, sliderY0);
            GUI_DrawBitmap(&bmcolorSpectrum, centerX - (paletteWidth / 2), sliderY0 + sliderHeight + 20);

        } else if (show_dimmer_slider) {
            // Crtanje samo slajdera za dimovanje
            GUI_DrawBitmap(&bmblackWhiteGradient, sliderX0, sliderY0);
        }

        // === KRAJ REFAKTORISANJA ===
        // =======================================================================

        GUI_MULTIBUF_EndEx(1);
    }
}

/**
 * @brief Servisira ekran za resetovanje glavnih prekidača/menija.
 * @note Ova funkcija prvenstveno prikazuje odbrojavanje ako je aktivan
 * Light Night Timer. Koristi novi API za provjeru statusa i preostalog vremena.
 */
static void Service_ResetMenuSwitches(void)
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
 * @brief Upravlja svim periodičnim događajima i tajmerima.
 * @note Ova funkcija je srž logike za pozadinske procese koji se ne odnose
 * direktno na trenutni ekran. Uključuje logiku screensaver-a, tajmera za
 * automatsko paljenje svjetala i ažuriranje vremena.
 */
static void Handle_PeriodicEvents(void)
{
    // === "FAIL-SAFE" SKENER ZA DUHOVE (Ovaj dio ostaje nepromijenjen) ===
    static uint32_t ghost_widget_scan_timer = 0;
    if ((HAL_GetTick() - ghost_widget_scan_timer) >= GHOST_WIDGET_SCAN_INTERVAL) {
        ghost_widget_scan_timer = HAL_GetTick();
        if (screen == SCREEN_MAIN || screen == SCREEN_SELECT_1 || screen == SCREEN_SELECT_2) {
            ForceKillAllSettingsWidgets();
        }
    }

    // === TAJMER ZA AUTOMATSKO PALJENJE SVJETALA (SVAKE MINUTE) ===
    if (IsRtcTimeValid() && (HAL_GetTick() - everyMinuteTimerStart) >= (60 * 1000)) {
        everyMinuteTimerStart = HAL_GetTick();

        // ** POČETAK IZMJENE **
        // Kompletna petlja je prepravljena da koristi novi API.

        // Dobijamo trenutno vrijeme sa RTC-a samo jednom, prije petlje, radi efikasnosti.
        RTC_TimeTypeDef currentTime;
        HAL_RTC_GetTime(&hrtc, &currentTime, RTC_FORMAT_BCD);
        uint8_t currentHour = Bcd2Dec(currentTime.Hours);
        uint8_t currentMinute = Bcd2Dec(currentTime.Minutes);

        // Prolazimo kroz sva STVARNO konfigurisana svjetla.
        for (uint8_t i = 0; i < LIGHTS_getCount(); i++) {

            // KORAK 1: Umjesto direktnog pristupa nizu, dobijamo siguran "handle".
            LIGHT_Handle* handle = LIGHTS_GetInstance(i);

            if (handle) {
                // KORAK 2: Logiku starih funkcija (`isTimeOnEnabled` i `isTimeToTurnOn`)
                // sada implementiramo ovdje, koristeći isključivo javne gettere.

                // Provjeravamo da li je tajmer za paljenje uopšte omogućen (ako sat nije -1).
                if (LIGHT_GetOnHour(handle) != -1) {
                    // Provjeravamo da li se vrijeme za paljenje poklapa sa trenutnim vremenom.
                    if ((LIGHT_GetOnHour(handle) == currentHour) && (LIGHT_GetOnMinute(handle) == currentMinute)) {

                        // KORAK 3: Palimo svjetlo isključivo preko API funkcije.
                        LIGHT_SetState(handle, true);

                        // Ostatak logike za ažuriranje ekrana je isti.
                        if (screen == SCREEN_LIGHTS) {
                            shouldDrawScreen = 1;
                        } else if((screen == SCREEN_RESET_MENU_SWITCHES) || (screen == SCREEN_MAIN)) {
                            screen = SCREEN_RETURN_TO_FIRST;
                        }
                    }
                }
            }
        }
        // ** KRAJ IZMJENE **
    }

    // === TAJMER ZA ULAZAK U MOD PODEŠAVANJA (Ovaj dio ostaje nepromijenjen) ===
    if (light_settingsTimerStart && ((HAL_GetTick() - light_settingsTimerStart) >= (2 * 1000))) {
        light_settingsTimerStart = 0;
        screen = SCREEN_LIGHT_SETTINGS;
        shouldDrawScreen = 1;
    }

    // === SCREENSAVER TAJMER ===
    if (!IsScrnsvrActiv()) {
        if ((HAL_GetTick() - scrnsvr_tmr) >= (uint32_t)(g_display_settings.scrnsvr_tout * 1000)) {
            // Gašenje ekrana podešavanja (ostaje isto)
            if (screen == SCREEN_SETTINGS_1)      DSP_KillSet1Scrn();
            else if (screen == SCREEN_SETTINGS_2) DSP_KillSet2Scrn();
            // ... itd. za sve ekrane ...

            // ** IZMJENA: Uklonjena logika za "pametno snimanje" **
            // Petlja koja je provjeravala "is_dirty_for_saving" flegove je obrisana.
            // Ta odgovornost je sada u potpunosti enkapsulirana unutar `lights.c` modula
            // i njegove `HandleDelayedSave` funkcije, koja se poziva iz `LIGHT_Service()`.
            // `display.c` više ne mora da brine o tome.

            // Aktivacija screensaver-a (ostaje isto)
            DISPSetBrightnes(g_display_settings.low_bcklght);
            ScrnsvrInitReset();
            ScrnsvrSet();
            screen = SCREEN_RETURN_TO_FIRST;
        }
    }

    // === AŽURIRANJE VREMENA NA EKRANU (Ovaj dio ostaje nepromijenjen) ===
    if (HAL_GetTick() - rtctmr >= 1000) {
        rtctmr = HAL_GetTick();
        if(++refresh_tmr > 10) {
            refresh_tmr = 0;
            if (!IsScrnsvrActiv()) MVUpdateSet();
        }
        if (screen < SCREEN_SELECT_1) DISPDateTime();
    }

    // ** IZMJENA: Uklonjena logika za provjeru promjene stanja **
    // Petlja koja je provjeravala `LIGHT_hasStatusChanged` je u potpunosti obrisana.
    // `lights.c` u svojoj `HandleLightStatusChanges` funkciji sada sam postavlja
    // `shouldDrawScreen` fleg ako detektuje promjenu statusa (npr. sa RS485).
    // Ova provjera u `display.c` je postala nepotrebna i duplirana.
}

/**
 * @brief Prikazuje datum i vrijeme na ekranu, i upravlja logikom screensavera.
 * @note Ažurira se svake sekunde i odgovorna je za aktivaciju/deaktivaciju
 * screensavera na osnovu postavljenih sati.
 */
static void DISPDateTime(void)
{
    char dbuf[32];
    static uint8_t old_day = 0;

    // pozicije sata
    const int clockHpos = 240;  // Horizontalna pozicija za sat
    const int closckVpos = 136; // Vertikalna pozicija za sat

    if(!IsRtcTimeValid()) return;

    HAL_RTC_GetTime(&hrtc, &rtctm, RTC_FORMAT_BCD);
    HAL_RTC_GetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);

    // Provjera da li treba aktivirati screensaver na osnovu sata.
    if(g_display_settings.scrnsvr_ena_hour >= g_display_settings.scrnsvr_dis_hour) {
        if (Bcd2Dec(rtctm.Hours) >= g_display_settings.scrnsvr_ena_hour || Bcd2Dec(rtctm.Hours) < g_display_settings.scrnsvr_dis_hour) {
            ScrnsvrEnable();
        } else if (IsScrnsvrEnabled()) {
            ScrnsvrDisable();
            screen = SCREEN_RETURN_TO_FIRST;
        }
    } else if(g_display_settings.scrnsvr_ena_hour < g_display_settings.scrnsvr_dis_hour) {
        if (Bcd2Dec(rtctm.Hours) >= g_display_settings.scrnsvr_ena_hour && Bcd2Dec(rtctm.Hours) < g_display_settings.scrnsvr_dis_hour) {
            ScrnsvrEnable();
        } else if (IsScrnsvrEnabled()) {
            ScrnsvrDisable();
            screen = SCREEN_RETURN_TO_FIRST;
        }
    }

    // Prikaz vremena i datuma u zavisnosti od stanja screensavera.
    if (IsScrnsvrActiv() && IsScrnsvrEnabled() && IsScrnsvrClkActiv()) {
        // Logika za iscrtavanje sata na screensaveru.
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
            GUI_SetPenSize(9);
            GUI_SetColor(GUI_GREEN);
            GUI_MULTIBUF_EndEx(1);
        }
        // Formatiranje i prikaz vremena.
        HEX2STR(dbuf, &rtctm.Hours);
        if(rtctm.Seconds & 1) dbuf[2] = ':';
        else dbuf[2] = ' ';
        HEX2STR(&dbuf[3], &rtctm.Minutes);
        GUI_GotoXY(clockHpos, closckVpos);
        GUI_SetColor(clk_clrs[g_display_settings.scrnsvr_clk_clr]);
        GUI_SetFont(GUI_FONT_D80);
        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_MULTIBUF_BeginEx(1);
        GUI_ClearRect(0, 80, 480, 192);
        GUI_ClearRect(0, 220, 100, 270);
        GUI_DispString(dbuf);
        // Prikaz datuma.
        // ... (logika za prikaz dana, mjeseca i godine)
        GUI_MULTIBUF_EndEx(1);
    } else if (old_min != rtctm.Minutes) {
        // Ažuriranje vremena na standardnom ekranu samo kada se promijeni minuta.
        old_min = rtctm.Minutes;
        HEX2STR(dbuf, &rtctm.Hours);
        dbuf[2] = ':';
        HEX2STR(&dbuf[3], &rtctm.Minutes);
        GUI_SetFont(GUI_FONT_32_1);
        GUI_SetColor(GUI_WHITE);
        GUI_SetTextMode(GUI_TM_TRANS);
        GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
        GUI_MULTIBUF_BeginEx(1);
        GUI_GotoXY(5, 245);
        GUI_ClearRect(0, 220, 100, 270);
        GUI_DispString(dbuf);
        GUI_MULTIBUF_EndEx(1);
    }

    // Čuvanje datuma u backup registre.
    if (old_day != rtcdt.WeekDay) {
        old_day  = rtcdt.WeekDay;
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
    CHECKBOX_SetState(hCHKBX_ScrnsvrClock, IsScrnsvrClkActiv() ? 1 : 0);
    
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
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_relay[0].x, settings_screen_3_layout.label_ventilator_relay[0].y); GUI_DispString("VENTILATOR");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_relay[1].x, settings_screen_3_layout.label_ventilator_relay[1].y); GUI_DispString("BUS RELAY");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_delay_on[0].x, settings_screen_3_layout.label_ventilator_delay_on[0].y); GUI_DispString("VENTILATOR");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_delay_on[1].x, settings_screen_3_layout.label_ventilator_delay_on[1].y); GUI_DispString("DELAY ON");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_delay_off[0].x, settings_screen_3_layout.label_ventilator_delay_off[0].y); GUI_DispString("VENTILATOR");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_delay_off[1].x, settings_screen_3_layout.label_ventilator_delay_off[1].y); GUI_DispString("DELAY OFF");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_trigger1[0].x, settings_screen_3_layout.label_ventilator_trigger1[0].y); GUI_DispString("VENTILATOR");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_trigger1[1].x, settings_screen_3_layout.label_ventilator_trigger1[1].y); GUI_DispString("TRIGGER 1");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_trigger2[0].x, settings_screen_3_layout.label_ventilator_trigger2[0].y); GUI_DispString("VENTILATOR");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_trigger2[1].x, settings_screen_3_layout.label_ventilator_trigger2[1].y); GUI_DispString("TRIGGER 2");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_local_pin[0].x, settings_screen_3_layout.label_ventilator_local_pin[0].y); GUI_DispString("VENTILATOR");
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_local_pin[1].x, settings_screen_3_layout.label_ventilator_local_pin[1].y); GUI_DispString("LOCAL PIN");

    // Labele za DEFROSTER
    GUI_GotoXY(settings_screen_3_layout.label_defroster_cycle_time[0].x, settings_screen_3_layout.label_defroster_cycle_time[0].y); GUI_DispString("DEFROSTER");
    GUI_GotoXY(settings_screen_3_layout.label_defroster_cycle_time[1].x, settings_screen_3_layout.label_defroster_cycle_time[1].y); GUI_DispString("CYCLE TIME");
    GUI_GotoXY(settings_screen_3_layout.label_defroster_active_time[0].x, settings_screen_3_layout.label_defroster_active_time[0].y); GUI_DispString("DEFROSTER");
    GUI_GotoXY(settings_screen_3_layout.label_defroster_active_time[1].x, settings_screen_3_layout.label_defroster_active_time[1].y); GUI_DispString("ACTIVE TIME");
    GUI_GotoXY(settings_screen_3_layout.label_defroster_pin[0].x, settings_screen_3_layout.label_defroster_pin[0].y); GUI_DispString("DEFROSTER");
    GUI_GotoXY(settings_screen_3_layout.label_defroster_pin[1].x, settings_screen_3_layout.label_defroster_pin[1].y); GUI_DispString("PIN");

    // Labele za naslove i odabir kontrole
    GUI_GotoXY(settings_screen_3_layout.label_ventilator_title.x, settings_screen_3_layout.label_ventilator_title.y); GUI_DispString("VENTILATOR CONTROL");
    GUI_GotoXY(settings_screen_3_layout.label_defroster_title.x, settings_screen_3_layout.label_defroster_title.y); GUI_DispString("DEFROSTER CONTROL");
    GUI_GotoXY(settings_screen_3_layout.label_select_control_title.x, settings_screen_3_layout.label_select_control_title.y); GUI_DispString("SELECT CONTROL 4");

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

    // << ISPRAVKA: Uklonjen višak argumenta iz svih SPINBOX_CreateEx poziva. Sada imaju 9 argumenata. >>
    lightsWidgets[light_index].relay                  = SPINBOX_CreateEx(x, y, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12) + 0, 0, 512);
    lightsWidgets[light_index].iconID                 = SPINBOX_CreateEx(x, y + 1 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12) + 1, 0, LIGHT_ICON_COUNT - 1);
    lightsWidgets[light_index].controllerID_on        = SPINBOX_CreateEx(x, y + 2 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12) + 2, 0, 512);
    lightsWidgets[light_index].controllerID_on_delay  = SPINBOX_CreateEx(x, y + 3 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12) + 3, 0, 255);
    lightsWidgets[light_index].on_hour                = SPINBOX_CreateEx(x, y + 4 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12) + 4, -1, 23);
    lightsWidgets[light_index].on_minute              = SPINBOX_CreateEx(x, y + 5 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12) + 5, 0, 59);

    /** @brief Kreiranje widgeta u drugoj koloni. */
    x = settings_screen_5_layout.col2_x;
    
    lightsWidgets[light_index].offTime                = SPINBOX_CreateEx(x, y, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12) + 6, 0, 255);
    lightsWidgets[light_index].communication_type     = SPINBOX_CreateEx(x, y + 1 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12) + 7, 1, 3);
    lightsWidgets[light_index].local_pin              = SPINBOX_CreateEx(x, y + 2 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12) + 8, 0, 32);
    lightsWidgets[light_index].sleep_time             = SPINBOX_CreateEx(x, y + 3 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12) + 9, 0, 255);
    lightsWidgets[light_index].button_external        = SPINBOX_CreateEx(x, y + 4 * y_step, sb_size->w, sb_size->h, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12) + 10, 0, 3);
    
    const WidgetRect_t* cb1_size = &settings_screen_5_layout.checkbox1_size;
    lightsWidgets[light_index].tiedToMainLight = CHECKBOX_CreateEx(x, y + 5 * y_step, cb1_size->w, cb1_size->h, 0, WM_CF_SHOW, 0, ID_LightsModbusRelay + (light_index * 12) + 11);

    const WidgetRect_t* cb2_size = &settings_screen_5_layout.checkbox2_size;
    lightsWidgets[light_index].rememberBrightness = CHECKBOX_CreateEx(x, y + 5 * y_step + 23, cb2_size->w, cb2_size->h, 0, WM_CF_SHOW, 0, ID_LightsModbusRelay + (light_index * 12) + 12);


    /** @brief Postavljanje početnih vrijednosti za sve kreirane widgete. */
    SPINBOX_SetEdge(lightsWidgets[light_index].relay, SPINBOX_EDGE_CENTER);                 SPINBOX_SetValue(lightsWidgets[light_index].relay, LIGHT_GetRelay(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].iconID, SPINBOX_EDGE_CENTER);                SPINBOX_SetValue(lightsWidgets[light_index].iconID, LIGHT_GetIconID(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].controllerID_on, SPINBOX_EDGE_CENTER);      SPINBOX_SetValue(lightsWidgets[light_index].controllerID_on, LIGHT_GetControllerID(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].controllerID_on_delay, SPINBOX_EDGE_CENTER);SPINBOX_SetValue(lightsWidgets[light_index].controllerID_on_delay, LIGHT_GetOnDelayTime(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].on_hour, SPINBOX_EDGE_CENTER);              SPINBOX_SetValue(lightsWidgets[light_index].on_hour, LIGHT_GetOnHour(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].on_minute, SPINBOX_EDGE_CENTER);            SPINBOX_SetValue(lightsWidgets[light_index].on_minute, LIGHT_GetOnMinute(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].offTime, SPINBOX_EDGE_CENTER);              SPINBOX_SetValue(lightsWidgets[light_index].offTime, LIGHT_GetOffTime(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].communication_type, SPINBOX_EDGE_CENTER);  SPINBOX_SetValue(lightsWidgets[light_index].communication_type, LIGHT_GetCommunicationType(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].local_pin, SPINBOX_EDGE_CENTER);            SPINBOX_SetValue(lightsWidgets[light_index].local_pin, LIGHT_GetLocalPin(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].sleep_time, SPINBOX_EDGE_CENTER);            SPINBOX_SetValue(lightsWidgets[light_index].sleep_time, LIGHT_GetSleepTime(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].button_external, SPINBOX_EDGE_CENTER);      SPINBOX_SetValue(lightsWidgets[light_index].button_external, LIGHT_GetButtonExternal(handle));
    
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
    GUI_GotoXY(x + label_offset->x, y + label_offset->y); GUI_DispString("LIGHT "); GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + label_offset->y + label_y2_offset); GUI_DispString("RELAY");

    GUI_GotoXY(x + label_offset->x, y + 1 * y_step + label_offset->y); GUI_DispString("LIGHT "); GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 1 * y_step + label_offset->y + label_y2_offset); GUI_DispString("ICON");
    
    GUI_GotoXY(x + label_offset->x, y + 2 * y_step + label_offset->y); GUI_DispString("LIGHT "); GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 2 * y_step + label_offset->y + label_y2_offset); GUI_DispString("ON ID");

    GUI_GotoXY(x + label_offset->x, y + 3 * y_step + label_offset->y); GUI_DispString("LIGHT "); GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 3 * y_step + label_offset->y + label_y2_offset); GUI_DispString("ON ID DELAY");

    GUI_GotoXY(x + label_offset->x, y + 4 * y_step + label_offset->y); GUI_DispString("LIGHT "); GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 4 * y_step + label_offset->y + label_y2_offset); GUI_DispString("HOUR ON");

    GUI_GotoXY(x + label_offset->x, y + 5 * y_step + label_offset->y); GUI_DispString("LIGHT "); GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 5 * y_step + label_offset->y + label_y2_offset); GUI_DispString("MINUTE ON");
    
    // Druga kolona labela
    x = settings_screen_5_layout.col2_x;
    
    GUI_GotoXY(x + label_offset->x, y + label_offset->y); GUI_DispString("LIGHT "); GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + label_offset->y + label_y2_offset); GUI_DispString("DELAY OFF");
    
    GUI_GotoXY(x + label_offset->x, y + 1 * y_step + label_offset->y); GUI_DispString("LIGHT "); GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 1 * y_step + label_offset->y + label_y2_offset); GUI_DispString("COMM. TYPE");

    GUI_GotoXY(x + label_offset->x, y + 2 * y_step + label_offset->y); GUI_DispString("LIGHT "); GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 2 * y_step + label_offset->y + label_y2_offset); GUI_DispString("LOCAL PIN");

    GUI_GotoXY(x + label_offset->x, y + 3 * y_step + label_offset->y); GUI_DispString("LIGHT "); GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 3 * y_step + label_offset->y + label_y2_offset); GUI_DispString("SLEEP TIME");

    GUI_GotoXY(x + label_offset->x, y + 4 * y_step + label_offset->y); GUI_DispString("LIGHT "); GUI_DispDec(light_index + 1, 2);
    GUI_GotoXY(x + label_offset->x, y + 4 * y_step + label_offset->y + label_y2_offset); GUI_DispString("BUTTON EXT.");
    
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
    CHECKBOX_SetText(hCHKBX_LIGHT_NIGHT_TIMER, "LiGHT OFF TIMER AFTER 20h");
    CHECKBOX_SetState(hCHKBX_LIGHT_NIGHT_TIMER, g_display_settings.light_night_timer_enabled);

    /** @brief << NOVO: Kreiranje DROPDOWN-a za odabir jezika >> */
    const WidgetRect_t* lang_pos = &settings_screen_6_layout.language_dropdown_pos;
    hDRPDN_Language = DROPDOWN_CreateEx(lang_pos->x, lang_pos->y, lang_pos->w, lang_pos->h, 0, WM_CF_SHOW, DROPDOWN_CF_AUTOSCROLLBAR, ID_LanguageSelect);
    for (int i = 0; i < LANGUAGE_COUNT; i++) {
        // Koristimo TXT_LANGUAGE_NAME da dobijemo ime jezika na tom jeziku
        DROPDOWN_AddString(hDRPDN_Language, language_strings[TXT_LANGUAGE_NAME][i]);
    }
    DROPDOWN_SetSel(hDRPDN_Language, g_display_settings.language);


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
 * @brief Dispečer za događaje pritiska na ekran.
 * @note  Poziva se iz `PID_Hook` kada je ekran pritisnut. Na osnovu trenutnog
 * ekrana (`screen`), prosljeđuje događaj odgovarajućoj `HandlePress_...` funkciji.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 * @param click_flag Pokazivač na fleg koji se postavlja ako treba generisati zvučni signal.
 */
static void HandleTouchPressEvent(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    // Prosljeđivanje događaja specifičnim handlerima za svaki ekran.
    if(screen == SCREEN_MAIN) {
        *click_flag = 1; // Generisanje klika na Main ekranu
    }
    else if(screen == SCREEN_SELECT_1) {
        HandlePress_SelectScreen1(pTS, click_flag);
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
    else if(screen == SCREEN_SELECT_2) {
        HandlePress_SelectScreen2(pTS, click_flag);
    }
    else if(screen == SCREEN_LIGHT_SETTINGS) {
        HandlePress_LightSettingsScreen(pTS);
    }
    // << IZMJENA: Provjera zone sada koristi `reset_menu_switches_layout` strukturu >>
    else if ((screen == SCREEN_RESET_MENU_SWITCHES) &&
             (pTS->x >= reset_menu_switches_layout.main_switch_zone.x0) && (pTS->x < reset_menu_switches_layout.main_switch_zone.x1) &&
             (pTS->y >= reset_menu_switches_layout.main_switch_zone.y0) && (pTS->y < reset_menu_switches_layout.main_switch_zone.y1))
    {
        HandlePress_ResetMenuSwitchesScreenArea(pTS);
    }
}
/**
 * @brief Obrada događaja otpuštanja dodira sa ekrana, u zavisnosti od ekrana.
 * @note Ova funkcija prosljeđuje događaj otpuštanja logici za specifičan ekran,
 * a zatim resetuje sve opšte flegove. Refaktorisana je da koristi novi `lights` API.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 */
static void HandleTouchReleaseEvent(GUI_PID_STATE * pTS)
{
    uint8_t release = 0; // Lokalni fleg, originalna logika ga je koristila

    // Logika za obradu otpuštanja na glavnom ekranu (nepromijenjena)
    if(screen == SCREEN_MAIN && !touch_in_menu_zone) {
        HandleRelease_MainScreenLogic(pTS);
    }
    else if(screen == SCREEN_LIGHTS) {

        // =======================================================================
        // === POČETAK REFAKTORISANJA ===
        //
        // KORAK 1: Provjeravamo da li je neko svjetlo bilo odabrano (pritisnuto).
        if(light_selectedIndex < LIGHTS_MODBUS_SIZE) { // Provjera bez '+1' je sigurnija

            // KORAK 2: Dobijamo "handle" na to svjetlo.
            LIGHT_Handle* handle = LIGHTS_GetInstance(light_selectedIndex);

            if (handle) {
                // KORAK 3: Logiku dijelimo na osnovu tipa svjetla, koristeći API funkcije.
                if (!LIGHT_isBinary(handle)) {
                    // Ako je dimer/RGB, mijenjamo stanje samo ako pritisak NIJE bio dug
                    // (ako jeste bio dug, korisnik je ušao u meni za podešavanje).
                    if ((HAL_GetTick() - light_settingsTimerStart) < 2000) {
                        LIGHT_Flip(handle);
                    }
                } else {
                    // Ako je binarno svjetlo, uvijek mijenjamo stanje na otpuštanje.
                    LIGHT_Flip(handle);
                }
            }
        }

        // KORAK 4: Resetujemo tajmer i indeks bez obzira na sve.
        light_settingsTimerStart = 0;
        light_selectedIndex = LIGHTS_MODBUS_SIZE + 1; // Vraćamo na nevalidnu vrijednost

        // === KRAJ REFAKTORISANJA ===
        // =======================================================================
    }
    else if(screen == SCREEN_RESET_MENU_SWITCHES) {
        HandleRelease_ResetMenuSwitchesScreenArea(pTS);
    }

    // Resetovanje svih opštih kontrolnih flegova.
    btnset = 0;
    btndec = 0U;
    btninc = 0U;
    dynamicIconUpdateFlag  = 0;
    thermostatOnOffTouch_timer = 0;

    // Korištenje API funkcije za zaustavljanje noćnog tajmera.
    LIGHTS_StopNightTimer();
}

/**
 * @brief Obrada događaja pritiska za ekran "Control Select".
 * @note Ova funkcija detektuje koji je od 4 glavna ekrana (Svjetla, Termostat, Zavjese, Odmrzivač)
 * ili dugme "NEXT" pritisnuto, te postavlja odgovarajuće stanje.
 */
static void HandlePress_SelectScreen1(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    Defroster_Handle* defHandle = Defroster_GetInstance();
    Ventilator_Handle* ventHandle = Ventilator_GetInstance();

    // << IZMJENA: Logika sada koristi `select_screen1_layout` strukturu >>
    if ((pTS->x >= select_screen1_layout.lights_zone.x0) && (pTS->x < select_screen1_layout.lights_zone.x1) &&
        (pTS->y >= select_screen1_layout.lights_zone.y0) && (pTS->y < select_screen1_layout.lights_zone.y1))
    {
        // Pritisak u zoni za svjetla
        screen = SCREEN_LIGHTS;
    }
    else if ((pTS->x >= select_screen1_layout.thermostat_zone.x0) && (pTS->x < select_screen1_layout.thermostat_zone.x1) &&
             (pTS->y >= select_screen1_layout.thermostat_zone.y0) && (pTS->y < select_screen1_layout.thermostat_zone.y1))
    {
        // Pritisak u zoni za termostat
        screen = SCREEN_THERMOSTAT;
    }
    else if ((pTS->x >= select_screen1_layout.curtains_zone.x0) && (pTS->x < select_screen1_layout.curtains_zone.x1) &&
             (pTS->y >= select_screen1_layout.curtains_zone.y0) && (pTS->y < select_screen1_layout.curtains_zone.y1))
    {
        // Pritisak u zoni za zavjese
        screen = SCREEN_CURTAINS;
        Curtain_ResetSelection();
    }
    else if ((pTS->x >= select_screen1_layout.dynamic_zone.x0) && (pTS->x < select_screen1_layout.dynamic_zone.x1) &&
             (pTS->y >= select_screen1_layout.dynamic_zone.y0) && (pTS->y < select_screen1_layout.dynamic_zone.y1))
    {
        // Pritisak u dinamičkoj zoni (Defroster/Ventilator)
        switch(g_display_settings.selected_control_mode) {
        case MODE_DEFROSTER:
            if(Defroster_isActive(defHandle)) Defroster_Off(defHandle);
            else Defroster_On(defHandle);
            dynamicIconUpdateFlag = 1;
            break;
        case MODE_VENTILATOR:
            if(Ventilator_isActive(ventHandle)) Ventilator_Off(ventHandle);
            else Ventilator_On(ventHandle, false);
            dynamicIconUpdateFlag = 1;
            break;
        case MODE_OFF:
            break;
        }
        *click_flag = 1;
    }
    else if ((pTS->x >= select_screen1_layout.next_button_zone.x0) && (pTS->x < select_screen1_layout.next_button_zone.x1) &&
             (pTS->y >= select_screen1_layout.next_button_zone.y0) && (pTS->y < select_screen1_layout.next_button_zone.y1))
    {
        // Pritisak na dugme "NEXT"
        screen = SCREEN_SELECT_2;
    }

    if(screen != SCREEN_SELECT_1) {
        shouldDrawScreen = 1;
        *click_flag = 1;
    }
}
/**
 * @brief Obrada događaja pritiska za ekran "Select Screen 2".
 * @note Odgovoran je za prelazak na ekrane za čišćenje, Wi-Fi QR kod i App QR kod.
 */
static void HandlePress_SelectScreen2(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    // << IZMJENA: Logika sada koristi `select_screen2_layout` strukturu >>

    // Provjera da li je dodir unutar zone za čišćenje
    if ((pTS->x >= select_screen2_layout.clean_zone.x0) && (pTS->x < select_screen2_layout.clean_zone.x1) &&
        (pTS->y >= select_screen2_layout.clean_zone.y0) && (pTS->y < select_screen2_layout.clean_zone.y1))
    {
        screen = SCREEN_CLEAN;
        menu_clean = 0;
    }
    // Provjera da li je dodir unutar zone za Wi-Fi
    else if ((pTS->x >= select_screen2_layout.wifi_zone.x0) && (pTS->x < select_screen2_layout.wifi_zone.x1) &&
             (pTS->y >= select_screen2_layout.wifi_zone.y0) && (pTS->y < select_screen2_layout.wifi_zone.y1))
    {
        screen = SCREEN_QR_CODE;
        qr_code_draw_id = QR_CODE_WIFI_ID;
        shouldDrawScreen = 1;
    }
    // Provjera da li je dodir unutar zone za App
    else if ((pTS->x >= select_screen2_layout.app_zone.x0) && (pTS->x < select_screen2_layout.app_zone.x1) &&
             (pTS->y >= select_screen2_layout.app_zone.y0) && (pTS->y < select_screen2_layout.app_zone.y1))
    {
        screen = SCREEN_QR_CODE;
        qr_code_draw_id = QR_CODE_APP_ID;
        shouldDrawScreen = 1;
    }
    // Provjera da li je dodir unutar zone za dugme "NEXT"
    else if ((pTS->x >= select_screen2_layout.next_button_zone.x0) && (pTS->x < select_screen2_layout.next_button_zone.x1) &&
             (pTS->y >= select_screen2_layout.next_button_zone.y0) && (pTS->y < select_screen2_layout.next_button_zone.y1))
    {
        screen = SCREEN_SELECT_1;
        menu_lc = 0;
        shouldDrawScreen = 1;
    }

    if(screen != SCREEN_SELECT_2) {
        *click_flag = 1;
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
                    // Ako svjetlo NIJE binarno (tj. dimer ili RGB), pokrećemo tajmer
                    // za detekciju dugog pritiska, koji vodi na ekran za podešavanje.
                    if(!LIGHT_isBinary(handle)) {
                        light_settingsTimerStart = HAL_GetTick();
                    }
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
 * @brief Obrada događaja pritiska za ekran "Light Settings".
 * @note  Omogućava direktno podešavanje svjetline i boje svjetla
 * dodirom na gradijent i spektar boja. Koristi `light_settings_screen_layout`
 * za detekciju zona i `lights` API za primjenu promjena.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 */
static void HandlePress_LightSettingsScreen(GUI_PID_STATE * pTS)
{
    // Varijable za detekciju promjene
    uint8_t new_brightness = 255; // Default vrijednost koja označava da nema promjene
    uint32_t new_color = 0;       // Default vrijednost koja označava da nema promjene

    // Provjeravamo da li je aktivan RGB mod koristeći `lights` API
    bool is_rgb_mode = false;
    if (light_selectedIndex == LIGHTS_MODBUS_SIZE) { // Sva svjetla
        is_rgb_mode = lights_allSelected_hasRGB;
    } else { // Jedno svjetlo
        LIGHT_Handle* handle = LIGHTS_GetInstance(light_selectedIndex);
        if (handle) {
            is_rgb_mode = LIGHT_isRGB(handle);
        }
    }

    // << IZMJENA: Logika za detekciju zona sada koristi `light_settings_screen_layout` >>

    // Provjera dodira na bijeli kvadrat (samo u RGB modu)
    if (is_rgb_mode &&
        (pTS->x >= light_settings_screen_layout.white_square_zone.x0) && (pTS->x < light_settings_screen_layout.white_square_zone.x1) &&
        (pTS->y >= light_settings_screen_layout.white_square_zone.y0) && (pTS->y < light_settings_screen_layout.white_square_zone.y1))
    {
        new_color = 0x00FFFFFF; // Bijela boja
    }
    // Provjera dodira na slajder svjetline
    else if ((pTS->x >= light_settings_screen_layout.brightness_slider_zone.x0) && (pTS->x < light_settings_screen_layout.brightness_slider_zone.x1) &&
             (pTS->y >= light_settings_screen_layout.brightness_slider_zone.y0) && (pTS->y < light_settings_screen_layout.brightness_slider_zone.y1))
    {
        g_high_precision_mode = true; // Aktiviraj visoku preciznost za glatko pomjeranje
        int relative_touch_x = pTS->x - light_settings_screen_layout.brightness_slider_zone.x0;
        new_brightness = (uint8_t)((float)relative_touch_x / light_settings_screen_layout.brightness_slider_zone.x1 * 100.0f);
    }
    // Provjera dodira na paletu boja (samo u RGB modu)
    else if (is_rgb_mode &&
             (pTS->x >= light_settings_screen_layout.color_palette_zone.x0) && (pTS->x < light_settings_screen_layout.color_palette_zone.x1) &&
             (pTS->y >= light_settings_screen_layout.color_palette_zone.y0) && (pTS->y < light_settings_screen_layout.color_palette_zone.y1))
    {
        // Uzimamo boju piksela sa ekrana na mjestu dodira
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
}

/**
 * @brief Obrada događaja pritiska u zoni glavnog prekidača na ekranu.
 * @note Pokreće tajmer za dugi pritisak kako bi se ušlo u podešavanja
 * svjetla ako je bar jedno od odabranih svjetala dimabilno.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 */
static void HandlePress_ResetMenuSwitchesScreenArea(GUI_PID_STATE * pTS)
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
static void HandleRelease_MainScreenLogic(GUI_PID_STATE * pTS)
{
    // =======================================================================
    // === POČETAK REFAKTORISANJA ===

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
 * @brief Obrada otpuštanja dodira na ekranu za resetovanje (ponaša se kao glavni prekidač).
 * @note Logika je identična kao `HandleRelease_MainScreenLogic`. Refaktorisana
 * je da koristi isključivo `lights` API.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 */
static void HandleRelease_ResetMenuSwitchesScreenArea(GUI_PID_STATE * pTS)
{
    // Logika je u potpunosti identična onoj u `HandleRelease_MainScreenLogic`

    bool isAnyLightCurrentlyOn = LIGHTS_isAnyLightOn();
    bool newStateIsOn = !isAnyLightCurrentlyOn;

    for(uint8_t i = 0; i < LIGHTS_getCount(); ++i) {
        LIGHT_Handle* handle = LIGHTS_GetInstance(i);
        if (handle && LIGHT_isTiedToMainLight(handle)) {
            LIGHT_SetState(handle, newStateIsOn);
        }
    }

    if (g_display_settings.light_night_timer_enabled && !((Bcd2Dec(rtctm.Hours) > 6) && (Bcd2Dec(rtctm.Hours) < 20))) {
        if (newStateIsOn) {
            LIGHTS_StartNightTimer();
        } else {
            LIGHTS_StopNightTimer();
        }
    } else {
        LIGHTS_StopNightTimer();
    }

    shouldDrawScreen = 1;
    screen = SCREEN_MAIN;
}
