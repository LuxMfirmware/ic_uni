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
/** @} */

/*============================================================================*/
/* PRIVATNE STRUKTURE I DEKLARACIJE VARIJABLI                                 */
/*============================================================================*/

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

/*============================================================================*/
/* TABELA SA PREVODIMA (INTERNACIONALIZACIJA)                                 */
/*============================================================================*/
/**
 * @brief Tabela koja sadrži sve tekstualne stringove za sve podržane jezike.
 * @note Redovi odgovaraju TextID enum-u, a kolone Languages enum-u.
 */
static const char* language_strings[TEXT_COUNT][LANGUAGE_COUNT] = {
    { "", "" },                                     // TXT_DUMMY (index 0)
    { "ALARM", "ALARM" },                           // TXT_ALARM
    { "TERMOSTAT", "THERMOSTAT" },                  // TXT_THERMOSTAT
    { "ZAVJESE", "CURTAINS" },                      // TXT_CURTAINS
    { "SLJEDECE", "NEXT" },                         // TXT_NEXT
    { "TV", "TV" },                                 // TXT_TV
    { "CISCENJE", "CLEAN" },                        // TXT_CLEAN
    { "POSTAVKE", "SETTINGS" },                     // TXT_SETTINGS
    { "Sati", "Hours" },                            // TXT_HOURS
    { "Minute", "Minutes" },                        // TXT_MINUTES
    { "PONISTI", "RESET" },                         // TXT_RESET
    { "AKTIVIRAJ", "ACTIVATE" },                    // TXT_ACTIVATE
    { "VRIJEME ALARMA", "ALARM TIME" },             // TXT_ALARM_TIME
    { "VRIJEME BRISANJA EKRANA:", "DISPLAY CLEAN TIME:" }, // TXT_DISPLAY_CLEAN_TIME
    { "UNESI SIFRU", "ENTER PASSWORD" },            // TXT_ENTER_PASSWORD
    { "SIFRA TACNA", "PASSWORD CORRECT" },          // TXT_PASSWORD_CORRECT
    { "POGRESNA SIFRA", "WRONG PASSWORD" },         // TXT_WRONG_PASSWORD
    { "BOS", "ENG" },                               // TXT_LANGUAGE_NAME
    { "MUZIKA", "MUSIC" },                          // TXT_MUSIC
    { "SVJETLO", "LIGHT" },                         // TXT_LIGHT
    { "SVJETLA", "LIGHTS" },                        // TXT_LIGHTS
    { "ROLETNE", "BLINDS" },                        // TXT_BLINDS
    { "SPAVACA", "BED" },                           // TXT_BED
    { "HODNIK", "HALLWAY" },                        // TXT_HALLWAY
    { "WC", "WC" },                                 // TXT_WC
    { "TERASA", "TERRACE" },                        // TXT_TERRACE
    { "KUHINJA", "KITCHEN" },                       // TXT_KITCHEN
    { "STEP.", "STAIRS" },                          // TXT_STAIRS
    { "DNEVNI B. 1", "LIVING R. 1" },               // TXT_LIVING_R_1
    { "DNEVNI B. 2", "LIVING R. 2" },               // TXT_LIVING_R_2
    { "DNEVNI B. 3", "LIVING R. 3" },               // TXT_LIVING_R_3
    { "TER. L.", "TERR. L." },                      // TXT_TERR_L
    { "TER. R.", "TERR. R." },                      // TXT_TERR_R
    { "BOČ. PRO.", "SIDE WIN." },                   // TXT_SIDE_WIN
    { "PROZORI", "WINDOWS" },                       // TXT_WINDOWS
    { "FASADA", "FACADE" },                         // TXT_FACADE
    { "BEDROOM", "BEDROOM" },                       // TXT_BEDROOM
    { "BEDROOM 1", "BEDROOM 1" },                   // TXT_BEDROOM_1
    { "BEDROOM 2", "BEDROOM 2" },                   // TXT_BEDROOM_2
    { "TERRACE 1", "TERRACE 1" },                   // TXT_TERRACE_1
    { "TERRACE 2", "TERRACE 2" },                   // TXT_TERRACE_2
    { "LIVING\nROOM 1", "LIVING\nROOM 1" },         // TXT_LIVING_ROOM_1
    { "LIVING\nROOM 2", "LIVING\nROOM 2" },         // TXT_LIVING_ROOM_2
    { "BAZEN 1", "POOL 1" },                        // TXT_POOL_1
    { "BAZEN 2", "POOL 2" },                        // TXT_POOL_2
    { "BAZEN 3", "POOL 3" },                        // TXT_POOL_3
    { "LIJEVE", "LEFT" },                           // TXT_LEFT
    { "SREDNJE", "MIDDLE" },                        // TXT_MIDDLE
    { "DESNE", "RIGHT" },                           // TXT_RIGHT
    { "DNEVNI ", "LIVING " },                       // TXT_LIVING
    { "SVE", "ALL" },                               // TXT_ALL
    { "Wi-Fi", "Wi-Fi" },                           // TXT_WIFI
    { "APP", "APP" },                               // TXT_APP
    { "ODMRZIVAC", "DEFROSTER" },                   // TXT_DEFROSTER
    { "SPASI", "SAVE" },                            // TXT_SAVE
    { "FIRMWARE_UPDATE", "FIRMWARE_UPDATE" },       // TXT_FIRMWARE_UPDATE
    { "FAN", "VENTILATOR" }                         // TXT_VENTILATOR
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


/** @brief Niz sa stringovima za dane u sedmici (za dropdown meni). */
static const char * _acContent[] = { "PON", "UTO", "SRI", "CET", "PET", "SUB", "NED" };


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

        if ((pTS->x > 400) && (pTS->y < 80) && (screen < SCREEN_SETTINGS_1)) {// Provjera zone hamburger menija
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
void DISP_SetThermostatMenuState(uint8_t state) {
    thermostatMenuState = state;
}

/**
 * @brief Dobiva trenutno stanje menija termostata.
 * @note Ova funkcija vraća vrijednost internog fleg-a koji
 * prati status ekrana menija termostata.
 * @param None
 * @retval uint8_t Trenutno stanje menija.
 */
uint8_t DISP_GetThermostatMenuState(void) {
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
void DISP_SignalDynamicIconUpdate(void) {
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
    g_display_settings.language = BOS; // Početni jezik je bosanski
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
        GUI_DrawEllipse(240,136,50,50); // Crtanje kruga

        GUI_MULTIBUF_EndEx(1); // Završava dvostruko baferovanje
    }
}

/**
 * @brief Iscrtava ekran za odabir kontrole (Svjetla, Termostat, Zavjese, Odmrzivač).
 */
static void Service_SelectScreen1(void)
{
    // --- 1. DEFINICIJA SVIH KONSTANTI NA POČETKU FUNKCIJE ---
    // Ključne koordinate za poravnanje
    const uint16_t xSeparator = DRAWING_AREA_WIDTH;
    const uint16_t xMidLine = xSeparator / 2;
    const uint16_t yMidLine = 136;

    // Pozicije centara kvadranta
    const uint16_t xCenterLeft = xMidLine / 2;
    const uint16_t xCenterRight = xMidLine + (xSeparator - xMidLine) / 2;
    const uint16_t yCenterTop = yMidLine / 2;
    const uint16_t yCenterBottom = yMidLine + (272 - yMidLine) / 2;

    // Pointeri na bitmape
    const GUI_BITMAP* iconLights = &bmSijalicaOff;
    const GUI_BITMAP* iconThermostat = &bmTermometar;
    const GUI_BITMAP* iconCurtains = &bmblindMedium;
    const GUI_BITMAP* iconDefroster = &bmdefrosterico;
    const GUI_BITMAP* iconVentilator = &bmVENTILATOR_OFF;
    const GUI_BITMAP* iconNext = &bmnext;
    // NOVO: Dodajemo ON verzije ikona
    const GUI_BITMAP* iconDefrosterOn = &bmdefrostericoOn;
    const GUI_BITMAP* iconVentilatorOn = &bmVENTILATOR_ON;

    // Offset za vertikalno pomjeranje ikona
    const int16_t iconVerticalOffset = -10; // Negativna vrijednost pomjera ikone prema gore

    // Izračunate pozicije za ikone
    const uint16_t xLights = xCenterLeft - (iconLights->XSize / 2);
    const uint16_t yLights = yCenterTop - (iconLights->YSize / 2) + iconVerticalOffset;
    const uint16_t xThermostat = xCenterRight - (iconThermostat->XSize / 2);
    const uint16_t yThermostat = yCenterTop - (iconThermostat->YSize / 2) + iconVerticalOffset;
    const uint16_t xCurtains = xCenterLeft - (iconCurtains->XSize / 2);
    const uint16_t yCurtains = yCenterBottom - (iconCurtains->YSize / 2) + iconVerticalOffset;
    const uint16_t xDefroster = xCenterRight - (iconDefroster->XSize / 2);
    const uint16_t yDefroster = yCenterBottom - (iconDefroster->YSize / 2) + iconVerticalOffset;

    // Ostale konstante
    const uint16_t yNextButtonCenter = 192;
    const uint8_t textVerticalOffset = 10;
    // --- KRAJ DEFINICIJE KONSTANTI ---

    // NOVO: Dodajemo pointere za dinamičku ikonu i tekst
    const GUI_BITMAP* dynamicIcon = NULL;
    TextID dynamicTextID = TXT_DUMMY;
    bool is_active = false;

    // NOVO: Logika za odabir dinamičke ikone i teksta na osnovu `selected_control_mode`
    switch(g_display_settings.selected_control_mode) {
    case MODE_DEFROSTER:
        is_active = Defroster_isActive();
        dynamicIcon = is_active ? iconDefrosterOn : iconDefroster;
        dynamicTextID = TXT_DEFROSTER;
        break;
    case MODE_VENTILATOR:
        is_active = VENTILATOR_IS_ACTIVE();
        dynamicIcon = is_active ? iconVentilatorOn : iconVentilator;
        dynamicTextID = TXT_VENTILATOR;
        break;
    case MODE_OFF:
        // Ne crtaj ništa
        break;
    }


    // Iscrtaj ekran samo jednom, kada se prvi put uđe u njega
    if (menu_lc == 0) {
        menu_lc = 1;

        GUI_MULTIBUF_BeginEx(1);
        GUI_SelectLayer(0);
        GUI_Clear();
        GUI_SelectLayer(1);
        GUI_SetBkColor(GUI_TRANSPARENT);
        GUI_Clear();

        // --- Iscrtavanje linija i ikona ---
        // Poziva se funkcija koja centralizovano iscrtava hamburger meni
        DrawHamburgerMenu();

        GUI_DrawLine(xSeparator, 10, xSeparator, 262);
        GUI_DrawLine(30, yMidLine, xSeparator - 30, yMidLine);
        GUI_DrawLine(xMidLine, 20, xMidLine, 252);

        // Iscrtavanje statičkih ikona
        GUI_DrawBitmap(iconLights, xLights, yLights);
        GUI_DrawBitmap(iconThermostat, xThermostat, yThermostat);
        GUI_DrawBitmap(iconCurtains, xCurtains, yCurtains);

        // --- NOVO: Iscrtavanje dinamičke ikone ---
        if(dynamicIcon) {
            GUI_DrawBitmap(dynamicIcon, xDefroster, yDefroster);
        }

        // Crtanje NEXT dugmeta
        GUI_DrawBitmap(iconNext, xSeparator + 5, yNextButtonCenter - (iconNext->YSize / 2));

        // --- Ispisivanje teksta ---
        GUI_SetFont(GUI_FONT_24B_1);
        GUI_SetColor(GUI_ORANGE);
        GUI_SetTextMode(GUI_TM_TRANS);

        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt(lng(TXT_LIGHTS), xCenterLeft, yCenterTop + (iconLights->YSize / 2) + textVerticalOffset);

        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt(lng(TXT_THERMOSTAT), xCenterRight, yCenterTop + (iconThermostat->YSize / 2) + textVerticalOffset);

        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt(lng(TXT_BLINDS), xCenterLeft, yCenterBottom + (iconCurtains->YSize / 2) + textVerticalOffset);

        // --- NOVO: Dinamički ispis teksta za četvrtu ikonu ---
        if(dynamicTextID != TXT_DUMMY) {
            GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
            GUI_DispStringAt(lng(dynamicTextID), xCenterRight, yCenterBottom + (iconDefroster->YSize / 2) + textVerticalOffset);
        }

        GUI_MULTIBUF_EndEx(1);

        thermostatMenuState = 0;
    }
    else if (menu_lc == 1) {
        // Ažuriranje se dešava samo za dinamičku ikonu
        if (dynamicIconUpdateFlag ) {
            dynamicIconUpdateFlag  = 0;
            GUI_MULTIBUF_BeginEx(1);

            // NOVO: Brisanje stare ikone i crtanje nove
            GUI_ClearRect(xDefroster, yDefroster, xDefroster + iconDefroster->XSize, yDefroster + iconDefroster->YSize);
            if(dynamicIcon) {
                GUI_DrawBitmap(dynamicIcon, xDefroster, yDefroster);
            }

            GUI_MULTIBUF_EndEx(1);
        }
    }
}
/**
 * @brief Servisira drugi ekran za odabir (čišćenje, Wi-Fi QR, App QR).
 * @note Ova funkcija prikazuje tri opcije sa ikonama i tekstom,
 * te omogućava prelazak na te ekrane.
 */
static void Service_SelectScreen2(void)
{
    if(shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);

        GUI_SelectLayer(0);
        GUI_Clear();
        GUI_SelectLayer(1);
        GUI_SetBkColor(GUI_TRANSPARENT);
        GUI_Clear();

        // --- Definisanje i izračun ključnih pozicija na osnovu globalnih konstanti ---
        const uint16_t xSeparator = DRAWING_AREA_WIDTH;
        const uint16_t yLine = 136; // Koristi se GUI funkcija

        // Izračun vertikalnih linija za podjelu na tri kolone
        const uint16_t xLine1 = DRAWING_AREA_WIDTH / 3;
        const uint16_t xLine2 = (DRAWING_AREA_WIDTH / 3) * 2;

        // Izračun centara kolona
        const uint16_t xCenterCol1 = xLine1 / 2;
        const uint16_t xCenterCol2 = xLine1 + (xLine2 - xLine1) / 2;
        const uint16_t xCenterCol3 = xLine2 + (xSeparator - xLine2) / 2;

        // Izračun vertikalnih centara
        const uint16_t yIconCenter = 76 + 40;
        const uint16_t yTextPosition = 176;

        // Iscrtavanje linija.
        DrawHamburgerMenu();

        GUI_DrawLine(xSeparator,10,xSeparator,262);

        GUI_DrawLine(xLine1,60,xLine1,212);
        GUI_DrawLine(xLine2,60,xLine2,212);

        // Vertikalni centar za dugme "NEXT"
        const uint16_t yNextButtonCenter = 192;
        const GUI_BITMAP* iconNext = &bmnext;
        GUI_DrawBitmap(iconNext, xSeparator + 5, yNextButtonCenter - (iconNext->YSize / 2));

        // --- Definiranje ikona ---
        const GUI_BITMAP* iconClean = &bmCLEAN;
        const GUI_BITMAP* iconWifi = &bmwifi;
        const GUI_BITMAP* iconApp = &bmmobilePhone;

        // Iscrtavanje ikona na izračunate, centrirane pozicije
        GUI_DrawBitmap(iconClean, xCenterCol1 - (iconClean->XSize / 2), yIconCenter - (iconClean->YSize / 2));
        GUI_DrawBitmap(iconWifi, xCenterCol2 - (iconWifi->XSize / 2), yIconCenter - (iconWifi->YSize / 2));
        GUI_DrawBitmap(iconApp, xCenterCol3 - (iconApp->XSize / 2), yIconCenter - (iconApp->YSize / 2));

        // --- Ispisivanje teksta ---
        GUI_SetFont(GUI_FONT_24B_1);
        GUI_SetColor(GUI_ORANGE);
        GUI_SetTextMode(GUI_TM_TRANS);

        // Postavljanje poravnanja prije svakog ispisivanja
        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt(lng(TXT_CLEAN), xCenterCol1, yTextPosition);

        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt(lng(TXT_WIFI), xCenterCol2, yTextPosition);

        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt(lng(TXT_APP), xCenterCol3, yTextPosition);

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
 * @note Ova funkcija sinkronizuje vrijeme i datum s RTC modulom,
 * obrađuje promjene postavki screensavera i svjetline ekrana,
 * te ih pohranjuje u EEPROM kada se pritisne "SAVE".
 */
static void Service_SettingsScreen_2(void)
{
    // Dobijamo handle za termostat na početku funkcije.
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();
    uint8_t ebuf[8]; // Lokalni bafer za pisanje u EEPROM

    // Sinhronizacija vremena sa RTC modulom
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
    // Sinhronizacija datuma sa RTC modulom
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
    if (rtcdt.WeekDay != Dec2Bcd(DROPDOWN_GetSel(hDRPDN_WeekDay) + 1)) {
        rtcdt.WeekDay = Dec2Bcd(DROPDOWN_GetSel(hDRPDN_WeekDay) + 1);
        HAL_RTC_SetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);
        RtcTimeValidSet();
    }

    // Ažuriranje boje sata na screensaveru.
    if (g_display_settings.scrnsvr_clk_clr != SPINBOX_GetValue(hSPNBX_ScrnsvrClockColour)) {
        g_display_settings.scrnsvr_clk_clr = SPINBOX_GetValue(hSPNBX_ScrnsvrClockColour);
        GUI_FillRect(340, 51, 430, 59);
    }
    // Ažuriranje statusa screensavera.
    if (CHECKBOX_GetState(hCHKBX_ScrnsvrClock) == 1) {
        ScrnsvrClkSet();
    } else {
        ScrnsvrClkReset();
    }

    // Ažuriranje ostalih konfiguracionih varijabli.
    g_display_settings.high_bcklght = SPINBOX_GetValue(hSPNBX_DisplayHighBrightness);
    g_display_settings.low_bcklght = SPINBOX_GetValue(hSPNBX_DisplayLowBrightness);
    g_display_settings.scrnsvr_tout = SPINBOX_GetValue(hSPNBX_ScrnsvrTimeout);
    g_display_settings.scrnsvr_ena_hour = SPINBOX_GetValue(hSPNBX_ScrnsvrEnableHour);
    g_display_settings.scrnsvr_dis_hour = SPINBOX_GetValue(hSPNBX_ScrnsvrDisableHour);
    g_display_settings.scrnsvr_clk_clr = SPINBOX_GetValue(hSPNBX_ScrnsvrClockColour);

    // Obrada pritiska na dugmad "SAVE" i "NEXT".
    if (BUTTON_IsPressed(hBUTTON_Ok)) {
        if (thsta) {
            thsta = 0;
            THSTAT_Save(pThst);
        }
        if (lcsta) {
            lcsta = 0;
        }

        // Upisivanje svih promijenjenih postavki u EEPROM.
        Display_Save();
        EE_WriteBuffer(&tfifa, EE_TFIFA, 1);
        DSP_KillSet2Scrn();
        screen = SCREEN_RETURN_TO_FIRST;
    } else if (BUTTON_IsPressed(hBUTTON_Next)) {
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
    uint8_t current_selection = DROPDOWN_GetSel(hSelectControl_4);

    // Provjera da li se odabir u dropdown meniju promijenio.
    if (current_selection != old_selection) {
        old_selection = current_selection;
        g_display_settings.selected_control_mode = current_selection;
        DSP_KillSet3Scrn();
        DSP_InitSet3Scrn();
    }

    // Provjera promjena na widgetima DEFROSTER
    if(defroster.config.cycleTime != SPINBOX_GetValue(defroster_settingWidgets.cycleTime)) {
        Defroster_SetCycleTime(SPINBOX_GetValue(defroster_settingWidgets.cycleTime));
        settingsChanged = 1;
    }
    if(defroster.config.activeTime != SPINBOX_GetValue(defroster_settingWidgets.activeTime)) {
        Defroster_SetActiveTime(SPINBOX_GetValue(defroster_settingWidgets.activeTime));
        settingsChanged = 1;
    }
    if(defroster.config.pin != SPINBOX_GetValue(defroster_settingWidgets.pin)) {
        defroster.config.pin = SPINBOX_GetValue(defroster_settingWidgets.pin);
        settingsChanged = 1;
    }

    // Provjera promjena na widgetima VENTILATOR
    if(Ventilator_getRelay(&ventilator) != SPINBOX_GetValue(hVentilatorRelay))
    {
        Ventilator_setRelay(&ventilator, SPINBOX_GetValue(hVentilatorRelay));
        settingsChanged = 1;
    }
    if(Ventilator_getDelayOnTime(&ventilator) != SPINBOX_GetValue(hVentilatorDelayOn)) {
        Ventilator_setDelayOnTime(&ventilator, SPINBOX_GetValue(hVentilatorDelayOn));
        settingsChanged = 1;
    }
    if(Ventilator_getDelayOffTime(&ventilator) != SPINBOX_GetValue(hVentilatorDelayOff))
    {
        Ventilator_setDelayOffTime(&ventilator, SPINBOX_GetValue(hVentilatorDelayOff));
        settingsChanged = 1;
    }
    if(Ventilator_getTriggerSource1(&ventilator) != SPINBOX_GetValue(hVentilatorTriggerSource1)) {
        Ventilator_setTriggerSource1(&ventilator, SPINBOX_GetValue(hVentilatorTriggerSource1));
        settingsChanged = 1;
    }
    if(Ventilator_getTriggerSource2(&ventilator) != SPINBOX_GetValue(hVentilatorTriggerSource2)) {
        Ventilator_setTriggerSource2(&ventilator, SPINBOX_GetValue(hVentilatorTriggerSource2));
        settingsChanged = 1;
    }
    if(Ventilator_getLocalPin(&ventilator) != SPINBOX_GetValue(hVentilatorLocalPin)) {
        Ventilator_setLocalPin(&ventilator, SPINBOX_GetValue(hVentilatorLocalPin));
        settingsChanged = 1;
    }

    if (BUTTON_IsPressed(hBUTTON_Ok)) {
        if(settingsChanged) {
            Display_Save();
            Defroster_Save();
            Ventilator_Save();
            settingsChanged = 0;
        }
        DSP_KillSet3Scrn();
        screen = SCREEN_RETURN_TO_FIRST;
    } else if (BUTTON_IsPressed(hBUTTON_Next)) {
        if(settingsChanged) {
            Display_Save();
            Defroster_Save();
            Ventilator_Save();
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
        if((Curtain_GetRelayUp(curtains + idx) != SPINBOX_GetValue(hCurtainsRelay[idx * 2])) || (Curtain_GetRelayDown(curtains + idx) != SPINBOX_GetValue(hCurtainsRelay[(idx * 2) + 1]))) {
            settingsChanged = 1;
            Curtain_SetRelayUp(curtains + idx, SPINBOX_GetValue(hCurtainsRelay[idx * 2]));
            Curtain_SetRelayDown(curtains + idx, SPINBOX_GetValue(hCurtainsRelay[(idx * 2) + 1]));
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
 * @note Ova funkcija obrađuje postavke koje nisu grupisane u prethodnim ekranima.
 * Uključuje opcije za postavljanje podrazumevanih vrednosti i restart sistema.
 */
static void Service_SettingsScreen_6(void)
{
    if(BUTTON_IsPressed(hBUTTON_SET_DEFAULTS)) {
        SetDefault(); // Poziva funkciju za postavljanje podrazumevanih vrednosti.
    } else if(BUTTON_IsPressed(hBUTTON_SYSRESTART)) {
        SYSRestart(); // Poziva funkciju za restart sistema.
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
        if(!Curtain_areAllSelected()) {
            GUI_SetFont(GUI_FONT_D48); // Koristi font za brojeve kada je odabrana jedna roletna
            uint8_t physical_index = 0;
            uint8_t count = 0;
            for (uint8_t i = 0; i < CURTAINS_SIZE; i++) {
                if (Curtain_hasRelays(curtains + i)) {
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
            Curtain* cur = Curtain_GetByLogicalIndex(curtain_selected);
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
 * @brief Inicijalizuje prvi ekran podešavanja (termostat i ventilator).
 * @note Ova funkcija kreira sve potrebne GUI widgete, kao što su RADIO
 * i SPINBOX kontrole, te dugmad "NEXT" i "SAVE" za navigaciju i čuvanje.
 * Vrednosti se inicijalizuju na osnovu trenutnih postavki.
 */
static void DSP_InitSet1Scrn(void)
{
    // Dobijamo handle za termostat.
    THERMOSTAT_TypeDef* pThst = Thermostat_GetInstance();

    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    // Kreiranje RADIO dugmadi za kontrolu termostata.
    hThstControl = RADIO_CreateEx(10, 20, 150, 80, 0, WM_CF_SHOW, 0, ID_ThstControl, 3, 20);
    RADIO_SetTextColor(hThstControl, GUI_GREEN);
    RADIO_SetText(hThstControl, "OFF", 0);
    RADIO_SetText(hThstControl, "COOLING", 1);
    RADIO_SetText(hThstControl, "HEATING", 2);
    RADIO_SetValue(hThstControl, Thermostat_GetControlMode(pThst));

    // Kreiranje RADIO dugmadi za kontrolu brzine ventilatora.
    hFanControl = RADIO_CreateEx(10, 150, 150, 80, 0, WM_CF_SHOW, 0, ID_FanControl, 2, 20);
    RADIO_SetTextColor(hFanControl, GUI_GREEN);
    RADIO_SetText(hFanControl, "ON / OFF", 0);
    RADIO_SetText(hFanControl, "3 SPEED", 1);
    RADIO_SetValue(hFanControl, Thermostat_GetFanControlMode(pThst));

    // Kreiranje SPINBOX-ova za maksimalnu i minimalnu zadanu temperaturu.
    hThstMaxSetPoint = SPINBOX_CreateEx(110, 20, 90, 30, 0, WM_CF_SHOW, ID_MaxSetpoint, THST_SP_MIN, THST_SP_MAX);
    SPINBOX_SetEdge(hThstMaxSetPoint, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstMaxSetPoint, Thermostat_Get_SP_Max(pThst));
    hThstMinSetPoint = SPINBOX_CreateEx(110, 70, 90, 30, 0, WM_CF_SHOW, ID_MinSetpoint, THST_SP_MIN, THST_SP_MAX);
    SPINBOX_SetEdge(hThstMinSetPoint, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstMinSetPoint, Thermostat_Get_SP_Min(pThst));

    // Kreiranje SPINBOX-ova za postavke ventilatora.
    hFanDiff = SPINBOX_CreateEx(110, 150, 90, 30, 0, WM_CF_SHOW, ID_FanDiff, 0, 10);
    SPINBOX_SetEdge(hFanDiff, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hFanDiff, Thermostat_GetFanDifference(pThst));
    hFanLowBand = SPINBOX_CreateEx(110, 190, 90, 30, 0, WM_CF_SHOW, ID_FanLowBand, 0, 50);
    SPINBOX_SetEdge(hFanLowBand, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hFanLowBand, Thermostat_GetFanLowBand(pThst));
    hFanHiBand = SPINBOX_CreateEx(110, 230, 90, 30, 0, WM_CF_SHOW, ID_FanHiBand, 0, 100);
    SPINBOX_SetEdge(hFanHiBand, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hFanHiBand, Thermostat_GetFanHighBand(pThst));

    // Kreiranje SPINBOX-a i CHECKBOX-a za grupni rad termostata.
    hThstGroup = SPINBOX_CreateEx(320, 20, 100, 40, 0, WM_CF_SHOW, ID_THST_GROUP, 0, 254);
    SPINBOX_SetEdge(hThstGroup, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstGroup, Thermostat_GetGroup(pThst));
    hThstMaster = CHECKBOX_Create(320, 70, 170, 20, 0, ID_THST_MASTER, WM_CF_SHOW);
    CHECKBOX_SetTextColor(hThstMaster, GUI_GREEN);
    CHECKBOX_SetText(hThstMaster, "Master");
    CHECKBOX_SetState(hThstMaster, Thermostat_IsMaster(pThst));

    // Kreiranje navigacionih dugmadi.
    hBUTTON_Next = BUTTON_Create(340, 180, 130, 30, ID_Next, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_Create(340, 230, 130, 30, ID_Ok, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    // Iscrtavanje labela i linija.
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);
    GUI_GotoXY(210, 24);
    GUI_DispString("MAX. USER SETPOINT");
    GUI_GotoXY(210, 36);
    GUI_DispString("TEMP. x1*C");
    GUI_GotoXY(210, 74);
    GUI_DispString("MIN. USER SETPOINT");
    GUI_GotoXY(210, 86);
    GUI_DispString("TEMP. x1*C");
    GUI_GotoXY(210, 154);
    GUI_DispString("FAN SPEED DIFFERENCE");
    GUI_GotoXY(210, 166);
    GUI_DispString("TEMP. x0.1*C");
    GUI_GotoXY(210, 194);
    GUI_DispString("FAN LOW SPEED BAND");
    GUI_GotoXY(210, 206);
    GUI_DispString("SETPOINT +/- x0.1*C");
    GUI_GotoXY(210, 234);
    GUI_DispString("FAN HI SPEED BAND");
    GUI_GotoXY(210, 246);
    GUI_DispString("SETPOINT +/- x0.1*C");
    GUI_GotoXY(10, 4);
    GUI_DispString("THERMOSTAT CONTROL MODE");
    GUI_GotoXY(10, 120);
    GUI_DispString("FAN SPEED CONTROL MODE");
    GUI_GotoXY(320 + 100 + 10, 20 + 17);
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
 * @note Kreira SPINBOX-ove za podešavanje vremena i datuma, dropdown listu
 * za dane u sedmici, te kontrole za svjetlinu ekrana i screensaver.
 */
static void DSP_InitSet2Scrn(void)
{
    int i;
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    // Učitavanje vremena i datuma sa RTC-a.
    HAL_RTC_GetTime(&hrtc, &rtctm, RTC_FORMAT_BCD);
    HAL_RTC_GetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);

    // Kreiranje widgeta za postavke svjetline ekrana.
    hSPNBX_DisplayHighBrightness = SPINBOX_CreateEx(10, 20, 90, 30, 0, WM_CF_SHOW, ID_DisplayHighBrightness, 1, 90);
    SPINBOX_SetEdge(hSPNBX_DisplayHighBrightness, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_DisplayHighBrightness, g_display_settings.high_bcklght);
    hSPNBX_DisplayLowBrightness = SPINBOX_CreateEx(10, 60, 90, 30, 0, WM_CF_SHOW, ID_DisplayLowBrightness, 1, 90);
    SPINBOX_SetEdge(hSPNBX_DisplayLowBrightness, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_DisplayLowBrightness, g_display_settings.low_bcklght);

    // Kreiranje widgeta za postavke screensavera.
    hSPNBX_ScrnsvrTimeout = SPINBOX_CreateEx(10, 130, 90, 30, 0, WM_CF_SHOW, ID_ScrnsvrTimeout, 1, 240);
    SPINBOX_SetEdge(hSPNBX_ScrnsvrTimeout, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_ScrnsvrTimeout, g_display_settings.scrnsvr_tout);
    hSPNBX_ScrnsvrEnableHour = SPINBOX_CreateEx(10, 170, 90, 30, 0, WM_CF_SHOW, ID_ScrnsvrEnableHour, 0, 23);
    SPINBOX_SetEdge(hSPNBX_ScrnsvrEnableHour, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_ScrnsvrEnableHour, g_display_settings.scrnsvr_ena_hour);
    hSPNBX_ScrnsvrDisableHour = SPINBOX_CreateEx(10, 210, 90, 30, 0, WM_CF_SHOW, ID_ScrnsvrDisableHour, 0, 23);
    SPINBOX_SetEdge(hSPNBX_ScrnsvrDisableHour, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_ScrnsvrDisableHour, g_display_settings.scrnsvr_dis_hour);

    // Kreiranje widgeta za podešavanje vremena i datuma.
    hSPNBX_Hour = SPINBOX_CreateEx(190, 20, 90, 30, 0, WM_CF_SHOW, ID_Hour, 0, 23);
    SPINBOX_SetEdge(hSPNBX_Hour, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_Hour, Bcd2Dec(rtctm.Hours));
    hSPNBX_Minute = SPINBOX_CreateEx(190, 60, 90, 30, 0, WM_CF_SHOW, ID_Minute, 0, 59);
    SPINBOX_SetEdge(hSPNBX_Minute, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_Minute, Bcd2Dec(rtctm.Minutes));
    hSPNBX_Day = SPINBOX_CreateEx(190, 130, 90, 30, 0, WM_CF_SHOW, ID_Day, 1, 31);
    SPINBOX_SetEdge(hSPNBX_Day, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_Day, Bcd2Dec(rtcdt.Date));
    hSPNBX_Month = SPINBOX_CreateEx(190, 170, 90, 30, 0, WM_CF_SHOW, ID_Month, 1, 12);
    SPINBOX_SetEdge(hSPNBX_Month, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_Month, Bcd2Dec(rtcdt.Month));
    hSPNBX_Year = SPINBOX_CreateEx(190, 210, 90, 30, 0, WM_CF_SHOW, ID_Year, 2000, 2099);
    SPINBOX_SetEdge(hSPNBX_Year, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_Year, (Bcd2Dec(rtcdt.Year) + 2000));

    // Kreiranje widgeta za boju screensaver sata i opciju prikaza sata.
    hSPNBX_ScrnsvrClockColour = SPINBOX_CreateEx(340, 20, 90, 30, 0, WM_CF_SHOW, ID_ScrnsvrClkColour, 1, COLOR_BSIZE);
    SPINBOX_SetEdge(hSPNBX_ScrnsvrClockColour, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_ScrnsvrClockColour, g_display_settings.scrnsvr_clk_clr);
    hCHKBX_ScrnsvrClock = CHECKBOX_Create(340, 70, 110, 20, 0, ID_ScrnsvrClock, WM_CF_SHOW);
    CHECKBOX_SetTextColor(hCHKBX_ScrnsvrClock, GUI_GREEN);
    CHECKBOX_SetText(hCHKBX_ScrnsvrClock, "SCREENSAVER");
    if(IsScrnsvrClkActiv()) {
        CHECKBOX_SetState(hCHKBX_ScrnsvrClock, 1);
    } else {
        CHECKBOX_SetState(hCHKBX_ScrnsvrClock, 0);
    }

    // Kreiranje dropdown liste za odabir dana u sedmici.
    hDRPDN_WeekDay = DROPDOWN_CreateEx(340, 100, 130, 100, 0, WM_CF_SHOW, DROPDOWN_CF_AUTOSCROLLBAR, ID_WeekDay);
    for (i = 0; i < GUI_COUNTOF(_acContent); i++) {
        DROPDOWN_AddString(hDRPDN_WeekDay, *(_acContent + i));
    }
    DROPDOWN_SetSel(hDRPDN_WeekDay, rtcdt.WeekDay - 1);

    // Kreiranje navigacionih dugmadi.
    hBUTTON_Next = BUTTON_Create(340, 180, 130, 30, ID_Next, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_Create(340, 230, 130, 30, ID_Ok, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    // Iscrtavanje labela i linija.
    GUI_SetColor(clk_clrs[g_display_settings.scrnsvr_clk_clr]);
    GUI_FillRect(340, 51, 430, 59);
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);
    GUI_DrawHLine(15, 5, 160);
    GUI_GotoXY(10, 5);
    GUI_DispString("DISPLAY BACKLIGHT");
    GUI_GotoXY(110, 35);
    GUI_DispString("HIGH");
    GUI_GotoXY(110, 75);
    GUI_DispString("LOW");
    GUI_DrawHLine(15, 185, 320);
    GUI_GotoXY(190, 5);
    GUI_DispString("SET TIME");
    GUI_GotoXY(290, 35);
    GUI_DispString("HOUR");
    GUI_GotoXY(290, 75);
    GUI_DispString("MINUTE");
    GUI_DrawHLine(15, 335, 475);
    GUI_GotoXY(340, 5);
    GUI_DispString("SET COLOR");
    GUI_GotoXY(440, 26);
    GUI_DispString("FULL");
    GUI_GotoXY(440, 38);
    GUI_DispString("CLOCK");
    GUI_DrawHLine(125, 5, 160);
    GUI_GotoXY(10, 115);
    GUI_DispString("SCREENSAVER OPTION");
    GUI_GotoXY(110, 145);
    GUI_DispString("TIMEOUT");
    GUI_GotoXY(110, 176);
    GUI_DispString("ENABLE");
    GUI_GotoXY(110, 188);
    GUI_DispString("HOUR");
    GUI_GotoXY(110, 216);
    GUI_DispString("DISABLE");
    GUI_GotoXY(110, 228);
    GUI_DispString("HOUR");
    GUI_DrawHLine(125, 185, 320);
    GUI_GotoXY(190, 115);
    GUI_DispString("SET DATE");
    GUI_GotoXY(290, 145);
    GUI_DispString("DAY");
    GUI_GotoXY(290, 185);
    GUI_DispString("MONTH");
    GUI_GotoXY(290, 225);
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
 * @brief Inicijalizuje treći ekran podešavanja (ventilator).
 * @note Kreira widgete za postavke releja, odgode paljenja i gašenja
 * ventilatora, kao i opcije za korištenje tih odgoda.
 */
static void DSP_InitSet3Scrn(void)
{
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    // Kreiranje DROPDOWN liste za odabir funkcije četvrte ikonice
    hSelectControl_4 = DROPDOWN_CreateEx(200, 170, 110, 80, 0, WM_CF_SHOW, DROPDOWN_CF_AUTOSCROLLBAR, ID_SelectControl_4);
    DROPDOWN_AddString(hSelectControl_4, "OFF");
    DROPDOWN_AddString(hSelectControl_4, "DEFROSTER");
    DROPDOWN_AddString(hSelectControl_4, "VENTILATOR");
    DROPDOWN_SetSel(hSelectControl_4, g_display_settings.selected_control_mode);
    DROPDOWN_SetFont(hSelectControl_4, GUI_FONT_16_1);

    // Kreiranje navigacionih dugmadi.
    hBUTTON_Next = BUTTON_Create(410, 180, 60, 30, ID_Next, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_Create(410, 230, 60, 30, ID_Ok, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    // --- Kreiranje svih widgeta koji su sada uvijek vidljivi ---
    // Defroster widgeti
    defroster_settingWidgets.cycleTime = SPINBOX_CreateEx(200, 20, 110, 35, 0, WM_CF_SHOW, ID_DEFROSTER_CYCLE_TIME, 0, 254);
    SPINBOX_SetEdge(defroster_settingWidgets.cycleTime, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(defroster_settingWidgets.cycleTime, defroster.config.cycleTime);

    defroster_settingWidgets.activeTime = SPINBOX_CreateEx(200, 60, 110, 35, 0, WM_CF_SHOW, ID_DEFROSTER_ACTIVE_TIME, 0, 254);
    SPINBOX_SetEdge(defroster_settingWidgets.activeTime, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(defroster_settingWidgets.activeTime, defroster.config.activeTime);

    defroster_settingWidgets.pin = SPINBOX_CreateEx(200, 100, 110, 35, 0, WM_CF_SHOW, ID_DEFROSTER_PIN, 0, 6);
    SPINBOX_SetEdge(defroster_settingWidgets.pin, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(defroster_settingWidgets.pin, defroster.config.pin);

    // Ventilator widgeti
    hVentilatorRelay = SPINBOX_CreateEx(10, 20, 110, 35, 0, WM_CF_SHOW, ID_VentilatorRelay, 0, 512);
    SPINBOX_SetEdge(hVentilatorRelay, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hVentilatorRelay, ventilator.config.relay);

    hVentilatorDelayOn = SPINBOX_CreateEx(10, 60, 110, 35, 0, WM_CF_SHOW, ID_VentilatorDelayOn, 0, 255);
    SPINBOX_SetEdge(hVentilatorDelayOn, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hVentilatorDelayOn, ventilator.config.delayOnTime);

    hVentilatorDelayOff = SPINBOX_CreateEx(10, 100, 110, 35, 0, WM_CF_SHOW, ID_VentilatorDelayOff, 0, 255);
    SPINBOX_SetEdge(hVentilatorDelayOff, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hVentilatorDelayOff, ventilator.config.delayOffTime);

    hVentilatorTriggerSource1 = SPINBOX_CreateEx(10, 140, 110, 35, 0, WM_CF_SHOW, ID_VentilatorTriggerSource1, 0, 6);
    SPINBOX_SetEdge(hVentilatorTriggerSource1, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hVentilatorTriggerSource1, ventilator.config.trigger_source1);

    hVentilatorTriggerSource2 = SPINBOX_CreateEx(10, 180, 110, 35, 0, WM_CF_SHOW, ID_VentilatorTriggerSource2, 0, 6);
    SPINBOX_SetEdge(hVentilatorTriggerSource2, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hVentilatorTriggerSource2, ventilator.config.trigger_source2);

    hVentilatorLocalPin = SPINBOX_CreateEx(10, 220, 110, 35, 0, WM_CF_SHOW, ID_VentilatorLocalPin, 0, 32);
    SPINBOX_SetEdge(hVentilatorLocalPin, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hVentilatorLocalPin, ventilator.config.local_pin);

    // --- ISPRAVLJENO: Dodavanje svih LABELA ---
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);

    // Labele za VENTILATOR
    GUI_GotoXY(130, 30);
    GUI_DispString("VENTILATOR");
    GUI_GotoXY(130, 42);
    GUI_DispString("BUS RELAY");

    GUI_GotoXY(130, 70);
    GUI_DispString("VENTILATOR");
    GUI_GotoXY(130, 82);
    GUI_DispString("DELAY ON");

    GUI_GotoXY(130, 110);
    GUI_DispString("VENTILATOR");
    GUI_GotoXY(130, 122);
    GUI_DispString("DELAY OFF");

    GUI_GotoXY(130, 150);
    GUI_DispString("VENTILATOR");
    GUI_GotoXY(130, 162);
    GUI_DispString("TRIGGER 1");

    GUI_GotoXY(130, 190);
    GUI_DispString("VENTILATOR");
    GUI_GotoXY(130, 202);
    GUI_DispString("TRIGGER 2");

    GUI_GotoXY(130, 230);
    GUI_DispString("VENTILATOR");
    GUI_GotoXY(130, 242);
    GUI_DispString("LOCAL PIN");

    // Labele za DEFROSTER
    GUI_GotoXY(320, 30);
    GUI_DispString("DEFROSTER");
    GUI_GotoXY(320, 42);
    GUI_DispString("CYCLE TIME");

    GUI_GotoXY(320, 70);
    GUI_DispString("DEFROSTER");
    GUI_GotoXY(320, 82);
    GUI_DispString("ACTIVE TIME");

    GUI_GotoXY(320, 110);
    GUI_DispString("DEFROSTER");
    GUI_GotoXY(320, 122);
    GUI_DispString("PIN");

    // Labele za naslove i odabir kontrole
    GUI_GotoXY(10, 4);
    GUI_DispString("VENTILATOR CONTROL");
    GUI_GotoXY(210, 4);
    GUI_DispString("DEFROSTER CONTROL");
    GUI_GotoXY(200, 154);
    GUI_DispString("SELECT CONTROL 4");

    // Linije
    GUI_DrawHLine(12, 5, 180);
    GUI_DrawHLine(12, 200, 375);
    GUI_DrawHLine(162, 200, 375);

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
 * @note Dinamički kreira SPINBOX-ove za do 4 zavjese po ekranu
 * za podešavanje releja "GORE" i "DOLJE".
 */
static void DSP_InitSet4Scrn(void)
{
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    int x = 10, y = 20;

    // Petlja za kreiranje widgeta za 4 zavjese po stranici.
    for(uint8_t i = curtainSettingMenu * 4; i < (((CURTAINS_SIZE - (curtainSettingMenu * 4)) >= 4) ? ((curtainSettingMenu * 4) + 4) : CURTAINS_SIZE); i++) {
        // Kreiranje SPINBOX-a za relej "GORE".
        hCurtainsRelay[i * 2] = SPINBOX_CreateEx(x, y, 110, 40, 0, WM_CF_SHOW, ID_CurtainsRelay + (i * 2), 0, 512);
        SPINBOX_SetEdge(hCurtainsRelay[i * 2], SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(hCurtainsRelay[i * 2], Curtain_GetRelayUp(curtains + i));

        // Kreiranje SPINBOX-a za relej "DOLJE".
        hCurtainsRelay[(i * 2) + 1] = SPINBOX_CreateEx(x, y + 50, 110, 40, 0, WM_CF_SHOW, ID_CurtainsRelay + (i * 2) + 1, 0, 512);
        SPINBOX_SetEdge(hCurtainsRelay[(i * 2) + 1], SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(hCurtainsRelay[(i * 2) + 1], Curtain_GetRelayDown(curtains + i));

        // Iscrtavanje labela.
        GUI_SetColor(GUI_WHITE);
        GUI_SetFont(GUI_FONT_13_1);
        GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);
        GUI_GotoXY(x + 110 + 10, y + 8);
        GUI_DispString("CURTAIN ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 110 + 10, y + 8 +12);
        GUI_DispString("RELAY UP");
        GUI_GotoXY(x + 110 + 10, y + 50 + 8);
        GUI_DispString("CURTAIN ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 110 + 10, y + 50 + 8 + 12);
        GUI_DispString("RELAY DOWN");

        // Pozicioniranje za sljedeću zavjesu.
        if((i % 4) == 1) {
            x = 200;
            y = 20;
        } else {
            y += 50 * 2;
        }
    }

    // Kreiranje navigacionih dugmadi.
    hBUTTON_Next = BUTTON_Create(410, 180, 60, 30, ID_Next, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_Create(410, 230, 60, 30, ID_Ok, WM_CF_SHOW);
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
 * @brief Inicijalizuje peti ekran podešavanja (Modbus svjetla).
 * @note Ova funkcija dinamički kreira sve GUI widgete za podešavanje jednog svjetla.
 * Refaktorisana je da koristi isključivo javni API `lights` modula (GetInstance i gettere)
 * za popunjavanje početnih vrijednosti widgeta, bez direktnog pristupa podacima.
 */
static void DSP_InitSet5Scrn(void)
{
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    // Definicije za pozicioniranje widgeta
    int x1 = 10, x2 = 200, y_start = 5, y_step = 43;

    // Dohvatamo indeks svjetla čije postavke trenutno prikazujemo.
    uint8_t light_index = lightsModbusSettingsMenu;

    // =======================================================================
    // === POČETAK REFAKTORISANJA ===

    // KORAK 1: Dobijamo siguran "handle" na svjetlo koje konfigurišemo.
    // Ne koristimo više direktan pristup `lights_modbus` nizu.
    LIGHT_Handle* handle = LIGHTS_GetInstance(light_index);

    // Ako handle nije validan (npr. indeks van opsega), prekidamo funkciju
    // da bismo spriječili greške.
    if (!handle) {
        GUI_MULTIBUF_EndEx(1);
        return;
    }

    // KORAK 2: Kreiramo sve widgete (SPINBOX-ove i CHECKBOX-ove).
    // Njihovo kreiranje je isto kao i prije, ali ih odmah povezujemo sa
    // `lightsWidgets` nizom radi lakšeg pristupa.
    lightsWidgets[light_index].relay = SPINBOX_CreateEx(x1, y_start, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12), 0, 512);
    lightsWidgets[light_index].iconID = SPINBOX_CreateEx(x1, y_start + 1 * y_step, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12) + 1, 0, LIGHT_ICON_COUNT - 1);
    lightsWidgets[light_index].controllerID_on = SPINBOX_CreateEx(x1, y_start + 2 * y_step, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12) + 2, 0, 512);
    lightsWidgets[light_index].controllerID_on_delay = SPINBOX_CreateEx(x1, y_start + 3 * y_step, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12) + 3, 0, 255);
    lightsWidgets[light_index].on_hour = SPINBOX_CreateEx(x1, y_start + 4 * y_step, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12) + 4, -1, 23);
    lightsWidgets[light_index].on_minute = SPINBOX_CreateEx(x1, y_start + 5 * y_step, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12) + 5, 0, 59);
    lightsWidgets[light_index].offTime = SPINBOX_CreateEx(x2, y_start, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12) + 6, 0, 255);
    lightsWidgets[light_index].communication_type = SPINBOX_CreateEx(x2, y_start + 1 * y_step, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12) + 7, 1, 3);
    lightsWidgets[light_index].local_pin = SPINBOX_CreateEx(x2, y_start + 2 * y_step, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12) + 8, 0, 32);
    lightsWidgets[light_index].sleep_time = SPINBOX_CreateEx(x2, y_start + 3 * y_step, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12) + 9, 0, 255);
    lightsWidgets[light_index].button_external = SPINBOX_CreateEx(x2, y_start + 4 * y_step, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (light_index * 12) + 10, 0, 3);
    lightsWidgets[light_index].tiedToMainLight = CHECKBOX_Create(x2, y_start + 5 * y_step, 130, 20, 0, ID_LightsModbusRelay + (light_index * 12) + 11, WM_CF_SHOW);
    lightsWidgets[light_index].rememberBrightness = CHECKBOX_Create(x2, y_start + 5 * y_step + 23, 145, 20, 0, ID_LightsModbusRelay + (light_index * 12) + 12, WM_CF_SHOW);

    // KORAK 3: Postavljamo početne vrijednosti widgeta ISKLJUČIVO
    // pozivanjem novih GETTER funkcija iz `lights` API-ja.
    // Display modul više ne zna za postojanje `config` strukture.
    SPINBOX_SetEdge(lightsWidgets[light_index].relay, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].relay, LIGHT_GetRelay(handle));
    SPINBOX_SetEdge(lightsWidgets[light_index].iconID, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[light_index].iconID, LIGHT_GetIconID(handle));
    // ... i tako za sve ostale gettere ...
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


    // Dinamičke labele
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);

    // Prva kolona labela
    GUI_GotoXY(x1 + 100 + 10, y_start + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, ((light_index + 1) < 10 ? 1 : 2));
    GUI_GotoXY(x1 + 100 + 10, y_start + 10 + 12);
    GUI_DispString("RELAY");

    GUI_GotoXY(x1 + 100 + 10, y_start + 1 * y_step + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, ((light_index + 1) < 10 ? 1 : 2));
    GUI_GotoXY(x1 + 100 + 10, y_start + 1 * y_step + 10 + 12);
    GUI_DispString("ICON");

    GUI_GotoXY(x1 + 100 + 10, y_start + 2 * y_step + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, ((light_index + 1) < 10 ? 1 : 2));
    GUI_GotoXY(x1 + 100 + 10, y_start + 2 * y_step + 10 + 12);
    GUI_DispString("ON ID");

    GUI_GotoXY(x1 + 100 + 10, y_start + 3 * y_step + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, ((light_index + 1) < 10 ? 1 : 2));
    GUI_GotoXY(x1 + 100 + 10, y_start + 3 * y_step + 10 + 12);
    GUI_DispString("ON ID DELAY");

    GUI_GotoXY(x1 + 100 + 10, y_start + 4 * y_step + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, ((light_index + 1) < 10 ? 1 : 2));
    GUI_GotoXY(x1 + 100 + 10, y_start + 4 * y_step + 10 + 12);
    GUI_DispString("HOUR ON");

    GUI_GotoXY(x1 + 100 + 10, y_start + 5 * y_step + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, ((light_index + 1) < 10 ? 1 : 2));
    GUI_GotoXY(x1 + 100 + 10, y_start + 5 * y_step + 10 + 12);
    GUI_DispString("MINUTE ON");

    // Druga kolona labela
    GUI_GotoXY(x2 + 100 + 10, y_start + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, ((light_index + 1) < 10 ? 1 : 2));
    GUI_GotoXY(x2 + 100 + 10, y_start + 10 + 12);
    GUI_DispString("DELAY OFF");

    GUI_GotoXY(x2 + 100 + 10, y_start + 1 * y_step + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, ((light_index + 1) < 10 ? 1 : 2));
    GUI_GotoXY(x2 + 100 + 10, y_start + 1 * y_step + 10 + 12);
    GUI_DispString("COMM. TYPE");

    GUI_GotoXY(x2 + 100 + 10, y_start + 2 * y_step + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, ((light_index + 1) < 10 ? 1 : 2));
    GUI_GotoXY(x2 + 100 + 10, y_start + 2 * y_step + 10 + 12);
    GUI_DispString("LOCAL PIN");

    GUI_GotoXY(x2 + 100 + 10, y_start + 3 * y_step + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, ((light_index + 1) < 10 ? 1 : 2));
    GUI_GotoXY(x2 + 100 + 10, y_start + 3 * y_step + 10 + 12);
    GUI_DispString("SLEEP TIME");

    GUI_GotoXY(x2 + 100 + 10, y_start + 4 * y_step + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(light_index + 1, ((light_index + 1) < 10 ? 1 : 2));
    GUI_GotoXY(x2 + 100 + 10, y_start + 4 * y_step + 10 + 12);
    GUI_DispString("BUTTON EXT.");

    // Navigacioni dugmadi
    hBUTTON_Next = BUTTON_Create(410, 180, 60, 30, ID_Next, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Next, "NEXT");

    hBUTTON_Ok = BUTTON_Create(410, 230, 60, 30, ID_Ok, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

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
 * @brief Inicijalizuje šesti ekran podešavanja (razne postavke).
 * @note Kreira widgete za podešavanje Device ID-a, vremena za kretanje zavjesa,
 * ponašanja screensavera, te opcije za noćni tajmer i dugmad za
 * restart i vraćanje na podrazumevane vrednosti.
 */
static void DSP_InitSet6Scrn(void)
{
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    // Kreiranje SPINBOX-a za Device ID.
    hDEV_ID = SPINBOX_CreateEx(10, 10, 110, 40, 0, WM_CF_SHOW, ID_DEV_ID, 1, 254);
    SPINBOX_SetEdge(hDEV_ID, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hDEV_ID, tfifa);

    // Kreiranje SPINBOX-a za vrijeme kretanja zavjesa.
    hCurtainsMoveTime = SPINBOX_CreateEx(10, 60, 110, 40, 0, WM_CF_SHOW, ID_CurtainsMoveTime, 0, 60);
    SPINBOX_SetEdge(hCurtainsMoveTime, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hCurtainsMoveTime, Curtain_GetMoveTime());

    // Kreiranje CHECKBOX-ova za screensaver i noćni tajmer.
    hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH = CHECKBOX_Create(10, 110, 205, 20, 0, ID_LEAVE_SCRNSVR_AFTER_TOUCH, WM_CF_SHOW);
    CHECKBOX_SetTextColor(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, GUI_GREEN);
    CHECKBOX_SetText(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, "ONLY LEAVE SCRNSVR AFTER TOUCH");
    CHECKBOX_SetState(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, g_display_settings.leave_scrnsvr_on_release);

    hCHKBX_LIGHT_NIGHT_TIMER = CHECKBOX_Create(10, 140, 170, 20, 0, ID_LIGHT_NIGHT_TIMER, WM_CF_SHOW);
    CHECKBOX_SetTextColor(hCHKBX_LIGHT_NIGHT_TIMER, GUI_GREEN);
    CHECKBOX_SetText(hCHKBX_LIGHT_NIGHT_TIMER, "LiGHT OFF TIMER AFTER 20h");
    CHECKBOX_SetState(hCHKBX_LIGHT_NIGHT_TIMER, g_display_settings.light_night_timer_enabled);

    // Kreiranje dugmadi za vraćanje na podrazumevane vrednosti i restart.
    hBUTTON_SET_DEFAULTS = BUTTON_Create(10, 190, 80, 30, ID_SET_DEFAULTS, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_SET_DEFAULTS, "SET DEFAULTS");
    hBUTTON_SYSRESTART = BUTTON_Create(10, 230, 80, 30, ID_SYSRESTART, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_SYSRESTART, "RESTART");

    // Iscrtavanje labela.
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);
    GUI_GotoXY(10 + 110 + 10, 10 + 10);
    GUI_DispString("DEVICE");
    GUI_GotoXY(10 + 110 + 10, 10 + 10 + 12);
    GUI_DispString("BUS ID");
    GUI_GotoXY(10 + 110 + 10, 60 + 10);
    GUI_DispString("CURTAINS");
    GUI_GotoXY(10 + 110 + 10, 60 + 10 + 12);
    GUI_DispString("MOVE TIME");

    // Kreiranje navigacionih dugmadi.
    hBUTTON_Next = BUTTON_Create(410, 180, 60, 30, ID_Next, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_Create(410, 230, 60, 30, ID_Ok, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

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
    WM_DeleteWindow(hBUTTON_Next);
    WM_DeleteWindow(hBUTTON_Ok);
}
/**
 * @brief Obrada događaja pritiska na ekranu, u zavisnosti od aktivnog ekrana.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 * @param click_flag Pokazivač na flag koji se postavlja ako treba generisati klik.
 * @note Ova funkcija služi kao dispečer, prosljeđujući događaj pritiska
 * odgovarajućoj funkciji za specifičan ekran.
 */
static void HandleTouchPressEvent(GUI_PID_STATE * pTS, uint8_t *click_flag) {
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
    // Posebna provjera za zonu na ekranu za resetovanje glavnih prekidača.
    else if((pTS->x > 100) && (pTS->y > 100) && (pTS->x < 400) && (pTS->y < 272) && (screen == SCREEN_RESET_MENU_SWITCHES)) {
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
static void HandlePress_SelectScreen1(GUI_PID_STATE * pTS, uint8_t *click_flag) {
    // Definisanje zona dodira baziranih na dinamičkim vrijednostima
    const uint16_t xSeparator = DRAWING_AREA_WIDTH;
    const uint16_t xMidLine = xSeparator / 2;
    const uint16_t yMidLine = 136;

    // NOVO: Definišemo dodirne zone za sve ikone (uključujući dinamičku)
    const GUI_BITMAP* iconLights = &bmSijalicaOff;
    const GUI_BITMAP* iconThermostat = &bmTermometar;
    const GUI_BITMAP* iconCurtains = &bmblindMedium;
    const GUI_BITMAP* iconDefroster = &bmdefrosterico;
    const uint16_t xCenterLeft = xMidLine / 2;
    const uint16_t yCenterTop = yMidLine / 2;
    const uint16_t yCenterBottom = yMidLine + (272 - yMidLine) / 2;
    const uint16_t xCenterRight = xMidLine + (xSeparator - xMidLine) / 2;

    const uint16_t xLightsZoneStart = xCenterLeft - (iconLights->XSize / 2);
    const uint16_t yLightsZoneStart = yCenterTop - (iconLights->YSize / 2);
    const uint16_t xThermostatZoneStart = xCenterRight - (iconThermostat->XSize / 2);
    const uint16_t yThermostatZoneStart = yCenterTop - (iconThermostat->YSize / 2);
    const uint16_t xCurtainsZoneStart = xCenterLeft - (iconCurtains->XSize / 2);
    const uint16_t yCurtainsZoneStart = yCenterBottom - (iconCurtains->YSize / 2);
    const uint16_t xDynamicIconZoneStart = xCenterRight - (iconDefroster->XSize / 2);
    const uint16_t yDynamicIconZoneStart = yCenterBottom - (iconDefroster->YSize / 2);

    const uint16_t xNextButton = 400;
    const uint16_t yNextButtonTop = 159;

    if (pTS->x < xSeparator) {
        if (pTS->y < yMidLine) { // Gornja polovina
            if (pTS->x < xMidLine) { // Gornji lijevi ugao: Svjetla
                screen = SCREEN_LIGHTS;
            } else { // Gornji desni ugao: Termostat
                screen = SCREEN_THERMOSTAT;
            }
        } else { // Donja polovina
            if (pTS->x < xMidLine) { // Donji lijevi ugao: Zavjese
                screen = SCREEN_CURTAINS;
                Curtain_ResetSelection();
            } else { // Donji desni ugao: Dinamička ikona (kvadrant 4)
                // NOVO: Ispravna logika za dinamički odabir i izvršenje komande
                switch(g_display_settings.selected_control_mode) {
                case MODE_DEFROSTER:
                    if(Defroster_isActive()) {
                        Defroster_Off();
                    } else {
                        Defroster_On();
                    }
                    dynamicIconUpdateFlag  = 1; // Ažurira fleg za ponovno iscrtavanje ikone
                    *click_flag = 1;
                    break;
                case MODE_VENTILATOR:
                    if(VENTILATOR_IS_ACTIVE()) {
                        Ventilator_Off();
                    } else {
                        Ventilator_On(false); // Eksterni okidač, bez odgode
                    }
                    dynamicIconUpdateFlag  = 1; // Ažurira fleg za ponovno iscrtavanje ikone
                    *click_flag = 1;
                    break;
                case MODE_OFF:
                    // Ne radi ništa
                    break;
                }
            }
        }
    } else if (pTS->x > xNextButton && pTS->y > yNextButtonTop) { // Dugme "NEXT"
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
static void HandlePress_SelectScreen2(GUI_PID_STATE * pTS, uint8_t *click_flag) {

    // --- Definisanje zona dodira baziranih na dinamičkim vrijednostima ---
    const uint16_t xSeparator = 380;
    const uint16_t xLine1 = DRAWING_AREA_WIDTH / 3;
    const uint16_t xLine2 = (DRAWING_AREA_WIDTH / 3) * 2;

    // Vertikalne granice touch zone za ikone
    const uint16_t yTouchZoneTop = 80;
    const uint16_t yTouchZoneBottom = 200;

    // Horizontalna granica za NEXT dugme
    const uint16_t xNextButton = xSeparator;
    const uint16_t yNextButton = 159;

    if(pTS->x < xSeparator) {
        // Provjera da li je dodir unutar vertikalne zone za ikone i tekst
        if((pTS->y > yTouchZoneTop) && (pTS->y < yTouchZoneBottom)) {
            // Provjera horizontalne zone (kolona)
            if(pTS->x < xLine1) {
                screen = SCREEN_CLEAN;
                menu_clean = 0; // Resetuj fleg za clean screen
            } else if(pTS->x < xLine2) {
                screen = SCREEN_QR_CODE;
                qr_code_draw_id = QR_CODE_WIFI_ID;
                shouldDrawScreen = 1;
            } else { // Treća kolona
                screen = SCREEN_QR_CODE;
                qr_code_draw_id = QR_CODE_APP_ID;
                shouldDrawScreen = 1;
            }
        }
    } else if(pTS->x > xNextButton && pTS->y > yNextButton) {
        // Provjera dodira na NEXT dugme
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
    // === POČETAK IZMJENE: Definicija zona dodira kao lokalnih konstanti ===
    //
    // Umjesto globalnih #define makroa, koordinate za "hitbox" svakog dugmeta
    // definišemo ovdje, unutar funkcije koja ih jedina i koristi.

    // Zona za POVEĆANJE temperature (+)
    const int BTN_INC_X0 = 200;
    const int BTN_INC_Y0 = 90;
    const int BTN_INC_X1 = BTN_INC_X0 + 120;
    const int BTN_INC_Y1 = BTN_INC_Y0 + 179;

    // Zona za SMANJENJE temperature (-)
    const int BTN_DEC_X0 = 0;
    const int BTN_DEC_Y0 = 90;
    const int BTN_DEC_X1 = BTN_DEC_X0 + 120;
    const int BTN_DEC_Y1 = BTN_DEC_Y0 + 179;

    // Zona za ON/OFF (dugi pritisak)
    const int BTN_ONOFF_X0 = 400;
    const int BTN_ONOFF_Y0 = 150;
    const int BTN_ONOFF_Y1 = 190;
    // === KRAJ IZMJENE ===

    // Provjera da li je dodir unutar zone za POVEĆANJE temperature
    if((pTS->x > BTN_INC_X0) && (pTS->y > BTN_INC_Y0) && (pTS->x < BTN_INC_X1) && (pTS->y < BTN_INC_Y1)) {
        *click_flag = 1; // Signaliziraj da treba generisati "klik" zvuk
        btninc = 1;      // Postavi fleg da je dugme "+" pritisnuto
    }
    // Provjera da li je dodir unutar zone za SMANJENJE temperature
    else if((pTS->x > BTN_DEC_X0) && (pTS->y > BTN_DEC_Y0) && (pTS->x < BTN_DEC_X1) && (pTS->y < BTN_DEC_Y1)) {
        *click_flag = 1; // Signaliziraj "klik"
        btndec = 1;      // Postavi fleg da je dugme "-" pritisnuto
    }
    // Provjera da li je dodir unutar zone za ON/OFF
    else if((pTS->x > BTN_ONOFF_X0) && (pTS->y > BTN_ONOFF_Y0) && (pTS->y < BTN_ONOFF_Y1)) {
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
 * @note Detektuje dodir na ikonu specifičnog svjetla. Ako svjetlo nije binarno (dimer/RGB),
 * pokreće tajmer za dugi pritisak kako bi se omogućio ulazak u meni za podešavanja.
 * Refaktorisana je da koristi isključivo `lights` API.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 * @param click_flag Pokazivač na fleg koji se postavlja ako treba generisati zvučni signal.
 */
static void HandlePress_LightsScreen(GUI_PID_STATE * pTS, uint8_t *click_flag)
{
    // Definišemo konstante za dimenzije "hitbox-a" ikonica radi čitljivosti.
    const int ICON_WIDTH = 80;
    const int ICON_HEIGHT = 120; // Uključuje i prostor za tekst ispod ikonice

    // Resetujemo globalne varijable stanja na početku obrade dodira.
    light_selectedIndex = LIGHTS_MODBUS_SIZE + 1;
    light_settingsTimerStart = 0;

    // Logika za dinamički raspored i detekciju dodira (nepromijenjena).
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

        // ...i kroz ikonice u trenutnom redu
        for(uint8_t i_light = 0; i_light < lightsInRow; ++i_light) {
            int x = (currentLightsMenuSpaceBetween * ((i_light % lightsInRow) + 1)) + (80 * (i_light % lightsInRow));

            // Provjeravamo da li koordinate dodira upadaju u "hitbox" trenutne ikonice.
            if((pTS->x > x) && (pTS->x < (x + ICON_WIDTH)) && (pTS->y > y) && (pTS->y < (y + ICON_HEIGHT))) {

                *click_flag = 1; // Signaliziraj "klik"
                light_selectedIndex = lightsInRowSum + i_light; // Zabilježi koje je svjetlo pritisnuto

                // =======================================================================
                // === POČETAK REFAKTORISANJA ===
                //
                // KORAK 1: Dobijamo "handle" za odabrano svjetlo koristeći novi API.
                LIGHT_Handle* handle = LIGHTS_GetInstance(light_selectedIndex);

                if (handle) {
                    // KORAK 2: Koristimo API funkciju `LIGHT_isBinary()` sa dobijenim handle-om.
                    // Više ne pristupamo `lights_modbus` nizu direktno.
                    if(!LIGHT_isBinary(handle)) {
                        // Ako svjetlo NIJE binarno (tj. dimer ili RGB), pokrećemo tajmer
                        // za detekciju dugog pritiska, koji vodi na ekran za podešavanje.
                        light_settingsTimerStart = HAL_GetTick();
                    }
                }
                // === KRAJ REFAKTORISANJA ===
                // =======================================================================

                // Resetujemo noćni tajmer pozivom API funkcije.
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
 * @note Rukuje pritiskom na strelice za pomicanje zavjesa "GORE" i "DOLJE",
 * kao i dugmadima za prebacivanje između pojedinačnih zavjesa i grupe.
 */
static void HandlePress_CurtainsScreen(GUI_PID_STATE * pTS, uint8_t *click_flag) {
    if(pTS->x < 400) {
        uint8_t local_curtainMenuCurtainLength = 120;
        uint8_t direction = CURTAIN_STOP;

        // Provjera smjera kretanja
        if ((pTS->x > (200 - (local_curtainMenuCurtainLength / 2))) && (pTS->x < (200 + (local_curtainMenuCurtainLength / 2)))) {
            direction = (pTS->y < 136) ? CURTAIN_UP : CURTAIN_DOWN;
            *click_flag = 1;
            shouldDrawScreen = 1;
        }

        if (direction != CURTAIN_STOP) {
            // Zamenjeno celim blokom koda pozivom nove funkcije
            Curtain_HandleTouchLogic(direction);
        } else if((Curtains_getCount() > 1) && (pTS->y > 192)) {
            // Logika za navigaciju (sa ispravnim prebacivanjem na sledeću/prethodnu roletnu)
            if(pTS->x > 320) { // Sljedeća roletna
                if (curtain_selected < Curtains_getCount()) { // Provjeri da li smo na opciji "SVE"
                    Curtain_Select(curtain_selected + 1);
                } else {
                    Curtain_Select(0); // Vrati se na prvu roletnu
                }
                shouldDrawScreen = 1;
                *click_flag = 1;
            }
            else if(pTS->x < 80) { // Prethodna roletna
                if(curtain_selected > 0) {
                    Curtain_Select(curtain_selected - 1);
                } else {
                    Curtain_Select(Curtains_getCount()); // Vrati se na opciju "SVE"
                }
                shouldDrawScreen = 1;
                *click_flag = 1;
            }
        }
    }
}

/**
 * @brief Obrada događaja pritiska za ekran "Light Settings".
 * @note Ova funkcija omogućava direktno podešavanje svjetline i boje svjetla
 * dodirom na gradijent i spektar boja. Refaktorisana je da koristi
 * isključivo `lights` API za provjeru tipa svjetla i postavljanje novih vrijednosti.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 */
static void HandlePress_LightSettingsScreen(GUI_PID_STATE * pTS)
{
    // --- KORAK 1: Definišemo sve potrebne konstante na početku funkcije ---

    // A) Konstante za pozicioniranje (Layout)
    const int centerX = LCD_GetXSize() / 2;
    const int centerY = LCD_GetYSize() / 2;
    const int sliderWidth = bmblackWhiteGradient.XSize;
    const int sliderHeight = bmblackWhiteGradient.YSize;
    const int sliderX0 = centerX - (sliderWidth / 2);
    const int sliderY0 = centerY - (sliderHeight / 2);
    const int PALETTE_X0 = (DRAWING_AREA_WIDTH - bmcolorSpectrum.XSize) / 2;
    const int PALETTE_Y0 = 180;
    const int PALETTE_WIDTH = bmcolorSpectrum.XSize;
    const int PALETTE_HEIGHT = bmcolorSpectrum.YSize;
    const int WHITE_SQUARE_SIZE = 60;
    const int WHITE_SQUARE_X0 = centerX - (WHITE_SQUARE_SIZE / 2);
    const int WHITE_SQUARE_Y0 = sliderY0 - WHITE_SQUARE_SIZE - 10;

    // B) Logičke konstante za slajder
    const uint8_t NO_BRIGHTNESS_CHANGE = 255;
    const uint8_t MAX_BRIGHTNESS = 100;
    const uint8_t MIDDLE_ZONE_MIN = 1;
    const uint8_t MIDDLE_ZONE_MAX = 99;
    const float ZONE_ZERO_LIMIT_PERCENT = 0.04f;    // 4% zone na početku slajdera (za vrijednost 0)
    const float ZONE_FULL_LIMIT_PERCENT = 0.96f;    // 4% zone na kraju slajdera (za vrijednost 100)
    const float MIDDLE_ZONE_PERCENT = ZONE_FULL_LIMIT_PERCENT - ZONE_ZERO_LIMIT_PERCENT; // Širina srednje zone (92%)
    const float BRIGHTNESS_STEPS_IN_MIDDLE_ZONE = (float)(MIDDLE_ZONE_MAX - MIDDLE_ZONE_MIN);

    // C) Logičke konstante za boju
    const uint32_t WHITE_COLOR = 0x00FFFFFF; // Format je 0xBBGGRR
    const uint32_t NO_COLOR_CHANGE = 0;

    // Inicijalizujemo varijable za detekciju promjene
    uint8_t new_brightness = NO_BRIGHTNESS_CHANGE;
    uint32_t new_color = NO_COLOR_CHANGE;

    // --- KORAK 2: Provjeravamo da li smo u RGB modu koristeći novi API ---
    bool is_rgb_mode = false;
    if (light_selectedIndex == LIGHTS_MODBUS_SIZE) { // Ako su odabrana "SVA SVJETLA"
        is_rgb_mode = lights_allSelected_hasRGB;
    } else { // Ako je odabrano jedno svjetlo
        LIGHT_Handle* handle = LIGHTS_GetInstance(light_selectedIndex);
        if (handle) {
            is_rgb_mode = LIGHT_isRGB(handle);
        }
    }

    // --- KORAK 3: Detekcija dodira na GUI elemente ---
    if (is_rgb_mode && (pTS->x >= WHITE_SQUARE_X0) && (pTS->x < (WHITE_SQUARE_X0 + WHITE_SQUARE_SIZE)) &&
            (pTS->y >= WHITE_SQUARE_Y0) && (pTS->y < (WHITE_SQUARE_Y0 + WHITE_SQUARE_SIZE))) {
        // Dodir je na bijelom kvadratu.
        new_color = WHITE_COLOR;
    }
    else if ((pTS->x >= sliderX0) && (pTS->x < (sliderX0 + sliderWidth)) &&
             (pTS->y >= sliderY0) && (pTS->y < (sliderY0 + sliderHeight))) {
        // Dodir je na slajderu za svjetlinu.
        g_high_precision_mode = true; // Aktiviraj visoku preciznost za glatko pomjeranje

        // --- Kompletan kod za izračunavanje svjetline iz pozicije dodira ---
        int relative_touch_x = pTS->x - sliderX0;
        const int zone_zero_limit = (int)(sliderWidth * ZONE_ZERO_LIMIT_PERCENT);
        const int zone_full_limit = (int)(sliderWidth * ZONE_FULL_LIMIT_PERCENT);

        if (relative_touch_x < zone_zero_limit) {
            // Zona 1: Prvih 4% širine slajdera -> Svjetlina = 0
            new_brightness = 0;
        } else if (relative_touch_x >= zone_full_limit) {
            // Zona 3: Posljednjih 4% širine slajdera -> Svjetlina = 100
            new_brightness = MAX_BRIGHTNESS;
        } else {
            // Zona 2: Srednja zona (92%) za vrijednosti od 1 do 99
            int relative_middle_zone_x = relative_touch_x - zone_zero_limit;
            float middle_zone_width = sliderWidth * MIDDLE_ZONE_PERCENT;
            float percentage = relative_middle_zone_x / middle_zone_width;
            new_brightness = MIDDLE_ZONE_MIN + (uint8_t)(percentage * BRIGHTNESS_STEPS_IN_MIDDLE_ZONE);
        }
    }
    else if (is_rgb_mode && (pTS->x >= PALETTE_X0) && (pTS->x < (PALETTE_X0 + PALETTE_WIDTH)) &&
             (pTS->y >= PALETTE_Y0) && (pTS->y < (PALETTE_Y0 + PALETTE_HEIGHT))) {
        // Dodir je na paleti boja.
        // Uzimamo boju piksela i maskiramo alpha kanal da osiguramo 0x00BBGGRR format.
        new_color = LCD_GetPixelColor(pTS->x, pTS->y) & 0x00FFFFFF;
    }

    // --- KORAK 4: Primjena detektovanih promjena koristeći novi API ---
    if (new_brightness != NO_BRIGHTNESS_CHANGE || new_color != NO_COLOR_CHANGE) {

        if (light_selectedIndex == LIGHTS_MODBUS_SIZE) { // Slučaj: Sva svjetla
            // Petlja kroz sva svjetla i primjenjuje promjene samo na ona koja su vezana za glavni prekidač.
            for(uint8_t i = 0; i < LIGHTS_getCount(); i++) {
                LIGHT_Handle* handle = LIGHTS_GetInstance(i);
                if (handle && LIGHT_isTiedToMainLight(handle) && !LIGHT_isBinary(handle)) {
                    if (new_brightness != NO_BRIGHTNESS_CHANGE) {
                        LIGHT_SetBrightness(handle, new_brightness);
                    } else if (LIGHT_isRGB(handle) && new_color != NO_COLOR_CHANGE) {
                        LIGHT_SetColor(handle, new_color);
                    }
                }
            }
        } else { // Slučaj: Pojedinačno svjetlo
            LIGHT_Handle* handle = LIGHTS_GetInstance(light_selectedIndex);
            if (handle) {
                if (new_brightness != NO_BRIGHTNESS_CHANGE) {
                    LIGHT_SetBrightness(handle, new_brightness);
                } else if (LIGHT_isRGB(handle) && new_color != NO_COLOR_CHANGE) {
                    LIGHT_SetColor(handle, new_color);
                }
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
