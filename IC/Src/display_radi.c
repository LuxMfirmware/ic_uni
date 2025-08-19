/**
 ******************************************************************************
 * @file   display.c
 * @author Gemini
 * @brief  Modul za upravljanje korisničkim interfejsom (GUI).
 *
 * @note
 * Ovaj fajl sadrži kompletnu logiku za iscrtavanje svih ekrana, obradu
 * korisničkog unosa putem ekrana osjetljivog na dodir (touch screen),
 * i upravljanje stanjima aplikacije vezanim za GUI. Koristi STemWin
 * biblioteku za kreiranje grafičkih elemenata.
 ******************************************************************************
 */

/* Provjera verzije build-a kako bi se osigurala konzistentnost sa header fajlom */
#if (__DISPH__ != FW_BUILD)
#error "display header version mismatch"
#endif

/*============================================================================*/
/* UKLJUČENI FAJLOVI (INCLUDES)                                               */
/*============================================================================*/
#include "png.h"
#include "main.h"
#include "rs485.h"
#include "logger.h"
#include "display.h"
#include "thermostat.h"
#include "curtain.h"
#include "defroster.h"
#include "stm32746g.h"
#include "stm32746g_ts.h"
#include "stm32746g_qspi.h"
#include "stm32746g_sdram.h"
#include "stm32746g_eeprom.h"

/*============================================================================*/
/* PRIVATNE DEFINICIJE I MAKROI                                               */
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

/* --- Vremenske konstante --- */
#define GUI_REFRESH_TIME                100U    // Period osvježavanja GUI-ja (u ms), 10 puta u sekundi.
#define DATE_TIME_REFRESH_TIME          1000U   // Period osvježavanja prikaza datuma i vremena (svake sekunde).
#define SETTINGS_MENU_ENABLE_TIME       3456U   // Vrijeme držanja pritiska za ulazak u meni za podešavanja (u ms).
#define SETTINGS_MENU_TIMEOUT           59000U  // Timeout za automatski izlazak iz menija za podešavanja (u ms).
#define WFC_CHECK_TOUT                  (SECONDS_PER_HOUR * 1000U) // Provjera vremenske prognoze (nije implementirano).
#define EVENT_ONOFF_TOUT                500     // Maksimalno vrijeme za on/off dodir (u ms).
#define VALUE_STEP_TOUT                 15      // Vrijeme za promjenu vrijednosti (npr. dimovanje) (u ms).
#define VENTILATOR_ON_TIMER_DURATION    15      // Trajanje tajmera za ventilator (u sekundama).

/* --- Konfiguracija ekrana i prikaza --- */
#define DISP_BRGHT_MAX                  80      // Maksimalna svjetlina pozadinskog osvjetljenja.
#define DISP_BRGHT_MIN                  5       // Minimalna svjetlina pozadinskog osvjetljenja.
#define LANGUAGES_NUM                   2       // Ukupan broj podržanih jezika.
#define LIGHTS_MODBUS_PER_SETTINGS      1       // Broj svjetala prikazanih po jednom ekranu u podešavanjima.
#define LIGHTS_MODBUS_PER_ROW           3       // Broj svjetala u jednom redu na glavnom ekranu za svjetla.
#define QR_CODE_COUNT                   2       // Broj podržanih QR kodova (WiFi i App).
#define QR_CODE_LENGTH                  50      // Maksimalna dužina stringa za QR kod.

/* --- Definicije boja --- */
#define CLR_DARK_BLUE                   GUI_MAKE_COLOR(0x613600)
#define CLR_LIGHT_BLUE                  GUI_MAKE_COLOR(0xaa7d67)
#define CLR_BLUE                        GUI_MAKE_COLOR(0x855a41)
#define CLR_LEMON                       GUI_MAKE_COLOR(0x00d6d3)

// Makroi za pozicije termostata i sata
#define SP_H_POS                        200     // Horizontalna pozicija za SetPoint
#define SP_V_POS                        150     // Vertikalna pozicija za SetPoint
#define CLOCK_H_POS                     240     // Horizontalna pozicija za sat
#define CLOCK_V_POS                     136     // Vertikalna pozicija za sat

// Makroi za pozicije dugmadi za povecanje/smanjenje u termostatu
#define BTN_DEC_X0                      0       // X koordinata početka dugmeta za smanjenje
#define BTN_DEC_Y0                      90      // Y koordinata početka dugmeta za smanjenje
#define BTN_DEC_X1                      (BTN_DEC_X0 + 120) // X koordinata kraja dugmeta za smanjenje
#define BTN_DEC_Y1                      (BTN_DEC_Y0 + 179) // Y koordinata kraja dugmeta za smanjenje
#define BTN_INC_X0                      200     // X koordinata početka dugmeta za povećanje
#define BTN_INC_Y0                      90      // Y koordinata početka dugmeta za povećanje
#define BTN_INC_X1                      (BTN_INC_X0 + 120) // X koordinata kraja dugmeta za povećanje
#define BTN_INC_Y1                      (BTN_INC_Y0 + 179) // Y koordinata kraja dugmeta za povećanje

// ID-jevi za GUI widgete
#define ID_Ok                           0x803   // ID dugmeta "OK" ili "SAVE"
#define ID_Next                         0x805   // ID dugmeta "NEXT"
#define ID_MaxSetpoint                  0x831   // ID za Spinbox maksimalne zadane temperature
#define ID_MinSetpoint                  0x832   // ID za Spinbox minimalne zadane temperature
#define ID_DisplayHighBrightness        0x833   // ID za Spinbox visoke svjetline ekrana
#define ID_DisplayLowBrightness         0x834   // ID za Spinbox niske svjetline ekrana
#define ID_ScrnsvrTimeout               0x835   // ID za Spinbox screensaver timeouta
#define ID_ScrnsvrEnableHour            0x836   // ID za Spinbox sata aktivacije screensavera
#define ID_ScrnsvrDisableHour           0x837   // ID za Spinbox sata deaktivacije screensavera
#define ID_ScrnsvrClkColour             0x838   // ID za Spinbox boje sata na screensaveru
#define ID_Hour                         0x83A   // ID za Spinbox sata
#define ID_Minute                       0x83B   // ID za Spinbox minute
#define ID_Day                          0x83C   // ID za Spinbox dana
#define ID_Month                        0x83D   // ID za Spinbox mjeseca
#define ID_Year                         0x83E   // ID za Spinbox godine
#define ID_WeekDay                      0x83F   // ID za Dropdown dan u sedmici
#define ID_ScrnsvrClock                 0x851   // ID za Checkbox screensaver sata
#define ID_ThstControl                  0x860   // ID za Radio dugme kontrole termostata
#define ID_FanControl                   0x861   // ID za Radio dugme kontrole ventilatora
#define ID_FanDiff                      0x864   // ID za Spinbox diferencijala ventilatora
#define ID_FanLowBand                   0x865   // ID za Spinbox opsega niske brzine ventilatora
#define ID_FanHiBand                    0x866   // ID za Spinbox opsega visoke brzine ventilatora
#define ID_DEV_ID                       0x870   // ID za Spinbox ID-a uređaja
#define ID_VentilatorRelay              0x88F   // ID za Spinbox releja ventilatora
#define ID_VentilatorDelayOn            0x890   // ID za Spinbox odgode paljenja ventilatora
#define ID_VentilatorDelayOff           0x891   // ID za Spinbox odgode gašenja ventilatora
#define ID_VentilatorUseDelayOn         0x892   // ID za Checkbox korištenja odgode paljenja
#define ID_VentilatorUseDelayOff        0x893   // ID za Checkbox korištenja odgode gašenja
#define ID_CurtainsRelay                0x894   // Bazni ID za Spinboxe releja zavjesa
#define ID_CurtainsMoveTime             0x8B2   // ID za Spinbox vremena kretanja zavjesa
#define ID_LightsModbusRelay            0x8B3   // Bazni ID za Spinboxe releja svjetala
#define ID_SYSRESTART                   0x976   // ID dugmeta za restart sistema
#define ID_LEAVE_SCRNSVR_AFTER_TOUCH    0x977   // ID za Checkbox ponašanja screensavera
#define ID_LIGHT_NIGHT_TIMER            0x978   // ID za Checkbox noćnog tajmera za svjetla
#define ID_THST_GROUP                   0x979   // ID za Spinbox grupe termostata
#define ID_THST_MASTER                  0x97A   // ID za Checkbox master moda termostata
#define ID_DEFROSTER_CYCLE_TIME         0x97B   // ID za Spinbox vremena ciklusa odmrzivača
#define ID_DEFROSTER_ACTIVE_TIME        0x97C   // ID za Spinbox aktivnog vremena odmrzivača
#define ID_DEFROSTER_PIN                0x97D   // ID za Spinbox GPIO pina odmrzivača
#define ID_SET_DEFAULTS                 0x97E   // ID dugmeta za postavljanje podrazumijevanih vrednosti

/*============================================================================*/
/* HANDLE-OVI ZA GUI WIDGET-E                                                 */
/*============================================================================*/

BUTTON_Handle   hBUTTON_Ok;                         // Handle za dugme "OK" ili "SAVE"
BUTTON_Handle   hBUTTON_Next;                       // Handle za dugme "NEXT"
BUTTON_Handle   hBUTTON_SET_DEFAULTS;               // Handle za dugme "SET DEFAULTS"
BUTTON_Handle   hBUTTON_SYSRESTART;                 // Handle za dugme "RESTART"
RADIO_Handle    hThstControl;                       // Handle za radio dugmad kontrole termostata
RADIO_Handle    hFanControl;                        // Handle za radio dugmad kontrole ventilatora
SPINBOX_Handle  hThstMaxSetPoint;                   // Handle za spinbox maksimalne zadane temperature
SPINBOX_Handle  hThstMinSetPoint;                   // Handle za spinbox minimalne zadane temperature
SPINBOX_Handle  hFanDiff;                           // Handle za spinbox diferencijala ventilatora
SPINBOX_Handle  hFanLowBand;                        // Handle za spinbox opsega niske brzine ventilatora
SPINBOX_Handle  hFanHiBand;                         // Handle za spinbox opsega visoke brzine ventilatora
SPINBOX_Handle  hThstGroup;                         // Handle za spinbox grupe termostata
CHECKBOX_Handle hThstMaster;                        // Handle za checkbox master moda termostata
SPINBOX_Handle  hSPNBX_DisplayHighBrightness;       // Handle za spinbox visoke svjetline ekrana
SPINBOX_Handle  hSPNBX_DisplayLowBrightness;        // Handle za spinbox niske svjetline ekrana
SPINBOX_Handle  hSPNBX_ScrnsvrTimeout;              // Handle za spinbox screensaver timeouta
SPINBOX_Handle  hSPNBX_ScrnsvrEnableHour;           // Handle za spinbox sata aktivacije screensavera
SPINBOX_Handle  hSPNBX_ScrnsvrDisableHour;          // Handle za spinbox sata deaktivacije screensavera
SPINBOX_Handle  hSPNBX_ScrnsvrClockColour;          // Handle za spinbox boje sata na screensaveru
SPINBOX_Handle  hSPNBX_Hour;                        // Handle za spinbox sata
SPINBOX_Handle  hSPNBX_Minute;                      // Handle za spinbox minute
SPINBOX_Handle  hSPNBX_Day;                         // Handle za spinbox dana
SPINBOX_Handle  hSPNBX_Month;                       // Handle za spinbox mjeseca
SPINBOX_Handle  hSPNBX_Year;                        // Handle za spinbox godine
CHECKBOX_Handle hCHKBX_ScrnsvrClock;                // Handle za checkbox screensaver sata
DROPDOWN_Handle hDRPDN_WeekDay;                     // Handle za dropdown dan u sedmici
SPINBOX_Handle  hVentilatorRelay;                   // Handle za spinbox releja ventilatora
SPINBOX_Handle  hVentilatorDelayOn;                 // Handle za spinbox odgode paljenja ventilatora
SPINBOX_Handle  hVentilatorDelayOff;                // Handle za spinbox odgode gašenja ventilatora
CHECKBOX_Handle hVentilatorUseDelayOn;              // Handle za checkbox korištenja odgode paljenja
CHECKBOX_Handle hVentilatorUseDelayOff;             // Handle za checkbox korištenja odgode gašenja
SPINBOX_Handle  hCurtainsRelay[CURTAINS_SIZE * 2];  // Niz handle-ova za spinboxove releja zavjesa
SPINBOX_Handle  hCurtainsMoveTime;                  // Handle za spinbox vremena kretanja zavjesa
LIGHT_settingsWidgets lightsWidgets[LIGHTS_MODBUS_SIZE]; // Niz struktura za postavke svjetala
SPINBOX_Handle  hDEV_ID;                            // Handle za spinbox ID-a uređaja
CHECKBOX_Handle hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH; // Handle za checkbox screensaver ponašanja
CHECKBOX_Handle hCHKBX_LIGHT_NIGHT_TIMER;           // Handle za checkbox noćnog tajmera za svjetla
Defroster_settingsWidgets defroster_settingWidgets; // Struktura za widgete odmrzivača

/*============================================================================*/
/* STATIČKE VARIJABLE NA NIVOU MODULA                                         */
/*============================================================================*/

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
uint32_t dispfl;                                    // Glavni fleg registar za GUI stanja.
uint32_t rtctmr;                                    // Tajmer za periodično osvježavanje vremena.
uint32_t thermostatOnOffTouch_timer = 0;            // Tajmer za detekciju dugog pritiska na termostatu.
uint32_t scrnsvr_tmr;                               // Tajmer za praćenje neaktivnosti korisnika.
uint32_t light_settingsTimerStart = 0;              // Tajmer za detekciju dugog pritiska za ulazak u podešavanja svjetla.
uint32_t everyMinuteTimerStart = 0;                 // Tajmer koji se okida svake minute.
uint32_t onoff_tmr = 0;                             // Tajmer za logiku on/off dodira.
uint32_t value_step_tmr = 0;                        // Tajmer za postepenu promjenu vrijednosti (dimovanje).
uint32_t refresh_tmr = 0;                           // Tajmer za periodično osvježavanje ekrana termostata.
static bool touch_in_menu_zone = false;             // Fleg, true ako je dodir počeo u zoni menija.
uint8_t screen;                                     // Trenutno aktivni ekran (iz eScreen enuma).
uint8_t shouldDrawScreen = 1;                       // Fleg, ako je 1, ekran će biti ponovo iscrtan.
uint8_t menu_clean = 0;                             // Stanje menija za čišćenje.
uint8_t menu_lc = 0;                                // Stanje menija za odabir kontrole.
uint8_t menu_thst = 0;                              // Stanje menija termostata.
uint8_t curtainSettingMenu = 0;                     // Trenutna stranica u meniju za podešavanje zavjesa.
uint8_t lightsModbusSettingsMenu = 0;               // Trenutna stranica u meniju za podešavanje svjetala.
uint8_t curtain_selected = 0;                       // Indeks trenutno odabrane zavjese.
uint8_t light_selectedIndex = LIGHTS_MODBUS_SIZE + 1; // Indeks odabranog svjetla.
uint8_t lights_allSelected_hasRGB = 0;              // Fleg, true ako među odabranim svjetlima ima RGB.
uint8_t settingsChanged = 0;                        // Fleg, true ako je došlo do promjene u podešavanjima.
uint8_t thsta = 0;                                  // Fleg za promjene u podešavanjima termostata.
uint8_t lcsta = 0;                                  // Fleg za promjene u podešavanjima svjetala.
uint8_t btnset = 0;                                 // Fleg za detekciju dugog pritiska za ulazak u meni.
uint8_t btninc, _btninc;                            // Flegovi za dugme "increase".
uint8_t btndec, _btndec;                            // Flegovi za dugme "decrease".
uint8_t old_min = 60;                               // Prethodna vrijednost minuta, za detekciju promjene.
static uint8_t old_day = 0;                         // Prethodna vrijednost dana, za detekciju promjene.
uint8_t low_bcklght;                                // Vrijednost niske svjetline ekrana.
uint8_t high_bcklght;                               // Vrijednost visoke svjetline ekrana.
uint8_t scrnsvr_ena_hour;                           // Sat kada se screensaver automatski aktivira.
uint8_t scrnsvr_dis_hour;                           // Sat kada se screensaver automatski deaktivira.
uint8_t scrnsvr_clk_clr;                            // Boja sata na screensaver-u.
uint8_t scrnsvr_tout;                               // Timeout za screensaver u sekundama.
uint8_t bOnlyLeaveScreenSaverAfterTouch = 0;        // Fleg, ako je 1, screensaver se gasi samo dodirom.
enum Languages language = ENG;                      // Trenutno odabrani jezik.
uint8_t qr_codes[QR_CODE_COUNT][QR_CODE_LENGTH] = {0}; // Bafer za čuvanje stringova QR kodova.
uint8_t qr_code_draw_id = 0;                        // ID QR koda koji treba iscrtati.
char logbuf[128];                                   // Bafer za ispis log poruka na ekran.
uint8_t ctrl1 = 0;                                  // Fleg za ažuriranje ikonice odmrzivača (postavlja se u PID_Hook).
uint32_t clean_tmr = 0;                             // Tajmer za ekran za čišćenje.
uint8_t clrtmr = 0;                                 // Fleg za odbrojavanje na ekranu za ciscenje.

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
    { "SVJETLA", "LIGHTS" },                        // TXT_BLINDS
    { "ROLETNE", "BLINDS" },                        // TXT_BED
    { "SPAVACA", "BED" },                           // TXT_HALLWAY
    { "HODNIK", "HALLWAY" },                        // TXT_WC
    { "WC", "WC" },                                 // TXT_TERRACE
    { "TERASA", "TERRACE" },                        // TXT_KITCHEN
    { "KUHINJA", "KITCHEN" },                       // TXT_STAIRS
    { "STEP.", "STAIRS" },                          // TXT_LIVING_R_1
    { "DNEVNI B. 1", "LIVING R. 1" },               // TXT_LIVING_R_2
    { "DNEVNI B. 2", "LIVING R. 2" },               // TXT_LIVING_R_3
    { "DNEVNI B. 3", "LIVING R. 3" },               // TXT_TERR_L
    { "TER. L.", "TERR. L." },                      // TXT_TERR_R
    { "TER. R.", "TERR. R." },                      // TXT_SIDE_WIN
    { "BOČ. PRO.", "SIDE WIN." },                   // TXT_WINDOWS
    { "PROZORI", "WINDOWS" },                       // TXT_FACADE
    { "FASADA", "FACADE" },                         // TXT_BEDROOM
    { "BEDROOM", "BEDROOM" },                       // TXT_BEDROOM_1
    { "BEDROOM 1", "BEDROOM 1" },                   // TXT_BEDROOM_2
    { "BEDROOM 2", "BEDROOM 2" },                   // TXT_TERRACE_1
    { "TERRACE 1", "TERRACE 1" },                   // TXT_TERRACE_2
    { "TERRACE 2", "TERRACE 2" },                   // TXT_LIVING_ROOM_1
    { "LIVING\nROOM 1", "LIVING\nROOM 1" },         // TXT_LIVING_ROOM_2
    { "LIVING\nROOM 2", "LIVING\nROOM 2" },         // TXT_POOL_1
    { "BAZEN 1", "POOL 1" },                        // TXT_POOL_2
    { "BAZEN 2", "POOL 2" },                        // TXT_POOL_3
    { "BAZEN 3", "POOL 3" },                        // TXT_LEFT
    { "LIJEVE", "LEFT" },                           // TXT_MIDDLE
    { "SREDNJE", "MIDDLE" },                        // TXT_RIGHT
    { "DESNE", "RIGHT" },                           // TXT_LIVING
    { "DNEVNI ", "LIVING " },                       // TXT_ALL
    { "SVE", "ALL" },                               // TXT_WIFI
    { "Wi-Fi", "Wi-Fi" },                           // TXT_APP
    { "APP", "APP" },                               // TXT_DEFROSTER
    { "ODMRZIVAC", "DEFROSTER" },                   // TXT_SAVE
    { "SPASI", "SAVE" }                             // TXT_FIRMWARE_UPDATE
};
/*============================================================================*/
/* PROTOTIPOVI PRIVATNIH FUNKCIJA                                             */
/*============================================================================*/

static void DISPDateTime(void);
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
static uint8_t DISPMenuSettings(uint8_t btn);
static uint8_t Service_HandleFirmwareUpdate(void);
static void Service_MainScreen(void);
static void Service_ControlSelectScreen(void);
static void Service_ThermostatScreen(void);
static void Service_ReturnToFirst(void);
static void Service_CleanScreen(void);
static void Service_SettingsScreen_1(void);
static void Service_SettingsScreen_2(void);
static void Service_SettingsScreen_3(void);
static void Service_SettingsScreen_4(void);
static void Service_SettingsScreen_5(void);
static void Service_SettingsScreen_6(void);
static void Service_SettingsScreen_7(void);
static void Service_LightsScreen(void);
static void Service_CurtainsScreen(void);
static void Service_SelectScreen2(void);
static void Service_QrCodeScreen(void);
static void Service_LightSettingsScreen(void);
static void Service_ResetMenuSwitches(void);
static void Handle_PeriodicEvents(void);
static void HandleTouchPressEvent(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandleTouchReleaseEvent(GUI_PID_STATE * pTS);
static void HandlePress_ControlSelectScreen(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_ThermostatScreen(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_LightsScreen(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_CurtainsScreen(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_SelectScreen2(GUI_PID_STATE * pTS, uint8_t *click_flag);
static void HandlePress_LightSettingsScreen(GUI_PID_STATE * pTS);
static void HandlePress_ResetMenuSwitchesScreenArea(GUI_PID_STATE * pTS);
static void HandleRelease_MainScreenLogic(GUI_PID_STATE * pTS);
static void HandleRelease_LightsScreenLogic(GUI_PID_STATE * pTS);
static void HandleRelease_ResetMenuSwitchesScreenArea(GUI_PID_STATE * pTS);

/*============================================================================*/
/* IMPLEMENTACIJA JAVNIH FUNKCIJA (GRUPA 1/12)                                */
/*============================================================================*/

/**
 * @brief Inicijalizuje GUI sistem.
 * @note Poziva se jednom na početku programa iz main() funkcije.
 * Inicijalizuje STemWin, postavlja hook za touch, učitava
 * parametre iz EEPROM-a.
 */
void DISP_Init(void)
{
    uint8_t len;

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
    // Izvršavanje inicijalnih komandi za iscrtavanje
    GUI_Exec();

    // Učitavanje konfiguracije iz EEPROM-a
    EE_ReadBuffer(&bOnlyLeaveScreenSaverAfterTouch, EE_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, 1);

    // Učitavanje prvog QR koda
    EE_ReadBuffer(&len, EE_QR_CODE1, 1); // Prvo učitaj dužinu stringa
    if (len < QR_CODE_LENGTH) { // Provjera da ne prekoračimo bafer
        EE_ReadBuffer(&qr_codes[0][0], EE_QR_CODE1 + 1, len);
    }

    // Učitavanje drugog QR koda
    EE_ReadBuffer(&len, EE_QR_CODE2, 1);
    if (len < QR_CODE_LENGTH) {
        EE_ReadBuffer(&qr_codes[1][0], EE_QR_CODE2 + 1, len);
    }

    // Pokretanje tajmera koji se okida svake minute
    everyMinuteTimerStart = HAL_GetTick();
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
    case SCREEN_CONTROL_SELECT:
        Service_ControlSelectScreen();
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
    case SCREEN_LIGHTS:
        Service_LightsScreen();
        break;
    case SCREEN_CURTAINS:
        Service_CurtainsScreen();
        break;
    case SCREEN_SELECT_SCREEN_2:
        Service_SelectScreen2();
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
        menu_thst = 0;
        break;
    }

    // Upravljanje periodičnim događajima i tajmerima (npr. screensaver)
    Handle_PeriodicEvents();

    // Provjera da li treba ući u meni za podešavanja (dugi pritisak)
    if (DISPMenuSettings(btnset) && (screen < SCREEN_SETTINGS_1)) {
        // Prije ulaska u podešavanja, isključi sve aktivne uređaje
        LIGHTS_Off();
        Curtains_Stop();
        Defroster_Off();
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
        return language_strings[t][language];
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
    // Koristi se višestruko baferovanje za iscrtavanje bez treperenja
    GUI_MULTIBUF_BeginEx(1);
    // Očisti pravougaonik gdje se nalazi vrijednost
    GUI_ClearRect(SP_H_POS - 5, SP_V_POS - 5, SP_H_POS + 120, SP_V_POS + 85);
    // Postavi boju, font i poravnanje
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_D48);
    GUI_SetTextMode(GUI_TM_NORMAL);
    GUI_SetTextAlign(GUI_TA_RIGHT);
    GUI_GotoXY(SP_H_POS, SP_V_POS);
    // Ispiši vrijednost zadate temperature
    GUI_DispDec(thst.sp_temp , 2);
    // Završi operaciju iscrtavanja
    GUI_MULTIBUF_EndEx(1);
}

/**
 * @brief Resetuje tajmer za screensaver i postavlja visoku svjetlinu ekrana.
 * @note Ova funkcija se poziva nakon svakog dodira na ekran.
 */
void DISPResetScrnsvr(void)
{
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
    scrnsvr_tout = SCRNSVR_TOUT;
    // Postavi visoku (normalnu) svjetlinu ekrana
    DISPSetBrightnes(high_bcklght);
}

/**
 * @brief Hook funkcija za obradu događaja sa ekrana osjetljivog na dodir.
 * @note  Ovo je centralna tačka za obradu korisničkog unosa. STemWin poziva
 * ovu funkciju svaki put kada detektuje promjenu stanja dodira.
 * @param pTS Pointer na strukturu sa stanjem dodira (koordinate, pritisnut/otpušten).
 */
void PID_Hook(GUI_PID_STATE * pTS)
{
    static uint8_t release = 0; // Fleg koji prati da li je dodir bio pritisnut i čeka otpuštanje.
    uint8_t click = 0;          // Fleg koji signalizira da treba generisati zvučni signal (klik).

    // --- Obrada pritiska na ekran ---
    if (pTS->Pressed == 1U) {
        pTS->Layer = 1U; // Događaj se odnosi na gornji sloj (layer 1)
        release = 1;     // Postavi fleg da se čeka otpuštanje

        // Provjera da li je dodir unutar zone menija (gornji desni ugao)
        if ((pTS->x > 400) && (pTS->y < 80)) {
            touch_in_menu_zone = true; // Postavi fleg da je dodir počeo u zoni menija
            click = 1;                 // Svaki dodir u ovoj zoni generiše zvučni signal

            // --- Logika za KRATKI PRITISAK (povratak na prethodni ekran) ---
            switch(screen) {
            // Sa ovih ekrana, vraćamo se na glavni meni za odabir kontrole
            case SCREEN_THERMOSTAT:
            case SCREEN_LIGHTS:
            case SCREEN_CURTAINS:
            case SCREEN_SELECT_SCREEN_2:
                screen = SCREEN_CONTROL_SELECT;
                menu_lc = 0; // Forsira ponovno iscrtavanje menija
                break;

            // Sa glavnog menija za odabir, vraćamo se na početni ekran
            case SCREEN_CONTROL_SELECT:
                screen = SCREEN_RETURN_TO_FIRST;
                break;

            // Sa ekrana za QR kod, vraćamo se na meni sa kojeg je pozvan
            case SCREEN_QR_CODE:
                screen = SCREEN_SELECT_SCREEN_2;
                shouldDrawScreen = 1;
                break;

            // Sa ekrana za podešavanje svjetla, vraćamo se na ekran svjetala
            case SCREEN_LIGHT_SETTINGS:
                screen = SCREEN_LIGHTS;
                shouldDrawScreen = 1;
                break;

            // Sa glavnog ekrana idemo na meni za odabir
            case SCREEN_MAIN:
                screen = SCREEN_CONTROL_SELECT;
                break;

            // Sa bilo kojeg ekrana podešavanja, vraćamo se na početni ekran
            case SCREEN_SETTINGS_1:
            case SCREEN_SETTINGS_2:
            case SCREEN_SETTINGS_3:
            case SCREEN_SETTINGS_4:
            case SCREEN_SETTINGS_5:
            case SCREEN_SETTINGS_6:
            case SCREEN_SETTINGS_7:
                screen = SCREEN_RETURN_TO_FIRST;
                break;
            }

            // --- Logika za DUGI PRITISAK (ulazak u podešavanja) ---
            // Postavljanjem ovog flega, funkcija DISPMenuSettings() će početi mjeriti vrijeme.
            btnset = 1;

        } else {
            // Ako dodir NIJE u zoni menija, obradi ga regularno
            touch_in_menu_zone = false;
            HandleTouchPressEvent(pTS, &click);
        }

        // Generiši zvučni signal ako je potrebno
        if (click) {
            BuzzerOn();
            HAL_Delay(1); // Kratak impuls
            BuzzerOff();
            click = 0; // Resetuj click fleg
        }
    }
    // --- Obrada otpuštanja dodira ---
    else
    {
        if(release) {
            release = 0; // Resetuj fleg za otpuštanje
            // Pozovi funkciju za obradu logike otpuštanja
            HandleTouchReleaseEvent(pTS);
            // Resetuj fleg za zonu menija nakon kompletnog ciklusa (pritisak-otpuštanje)
            touch_in_menu_zone = false;
        }
    }
    // Svaki dodir resetuje tajmer za screensaver
    DISPResetScrnsvr();
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


/*============================================================================*/
/* IMPLEMENTACIJA STATIČKIH (PRIVATNIH) FUNKCIJA                              */
/*============================================================================*/

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
        GUI_SetPenSize(9);
        GUI_SetColor(GUI_RED); // Boja će biti dinamički mijenjana kasnije
        GUI_DrawEllipse(240, 136, 50, 50);
        GUI_DrawLine(400,20,450,20);
        GUI_DrawLine(400,40,450,40);
        GUI_DrawLine(400,60,450,60);
        GUI_MULTIBUF_EndEx(1);
    }
    return 0; // Ažuriranje nije aktivno
}

/**
 * @brief Servisira glavni ekran.
 * @note  Ova funkcija se poziva u petlji kada je `screen == SCREEN_MAIN`.
 * Resetuje flegove menija i iscrtava osnovne elemente glavnog ekrana.
 */
static void Service_MainScreen(void)
{
    // Resetuje interne flagove za menije
    menu_thst = 0;
    menu_lc = 0;
    old_min = 60; // Forsira update vremena pri sljedećem pozivu DISPDateTime()
    rtctmr = 0;

    // Resetuj screensaver tajmer svaki put kad smo na glavnom ekranu
    scrnsvr_tmr = HAL_GetTick();

    // Iscrtaj osnovnu grafiku ako je potrebno (shouldDrawScreen fleg)
    GUI_MULTIBUF_BeginEx(1);
    GUI_Clear();

    // Iscrtaj hamburger meni ikonicu
    GUI_SetPenSize(9);
    GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);
    GUI_DrawLine(400,20,450,20);
    GUI_DrawLine(400,40,450,40);
    GUI_DrawLine(400,60,450,60);

    // Provjeri stanje svjetala povezanih sa glavnim prekidačem da bi se odredila boja kruga
    uint8_t anyLightOn = 0;
    for(uint8_t light_idx = 0; light_idx < LIGHTS_getCount(); light_idx++) {
        if(LIGHT_isTiedToMainLight(lights_modbus + light_idx) && LIGHT_isNewValueOn(lights_modbus + light_idx)) {
            anyLightOn = 1;
            break;
        }
    }

    // Postavi boju kruga (zelena ako je upaljeno, crvena ako je ugašeno)
    if (anyLightOn) {
        GUI_SetColor(GUI_GREEN);
    } else {
        GUI_SetColor(GUI_RED);
    }
    GUI_DrawEllipse(240,136,50,50);

    GUI_MULTIBUF_EndEx(1);
}

/**
 * @brief Iscrtava ekran za odabir kontrole (Svjetla, Termostat, Zavjese, Odmrzivač).
 */
static void Service_ControlSelectScreen(void)
{
    // Iscrtaj ekran samo jednom, kada se prvi put uđe u njega
    if (menu_lc == 0) {
        menu_lc = 1; // Postavi fleg da je ekran iscrtan

        GUI_MULTIBUF_BeginEx(1);
        GUI_SelectLayer(0);
        GUI_Clear();
        GUI_SelectLayer(1);
        GUI_SetBkColor(GUI_TRANSPARENT);
        GUI_Clear();

        // --- Iscrtavanje linija i ikonica ---
        GUI_SetPenSize(9);
        GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);
        GUI_DrawLine(400, 20, 450, 20);
        GUI_DrawLine(400, 40, 450, 40);
        GUI_DrawLine(400, 60, 450, 60);
        GUI_DrawLine(380, 10, 380, 262);
        GUI_DrawLine(30, 136, 350, 136);
        GUI_DrawLine(190, 20, 190, 252);

        // --- Iscrtavanje bitmapa (ikonica) ---
        GUI_DrawBitmap(&bmnext, 385, 159);
        GUI_DrawBitmap(&bmSijalicaOff, 55, 10);
        GUI_DrawBitmap(&bmTermometar, 245, 15);
        GUI_DrawBitmap(&bmblindMedium, 55, 150);
        if(Defroster_isActive()) {
            GUI_DrawBitmap(&bmdefrostericoOn, 240, 155);
        } else {
            GUI_DrawBitmap(&bmdefrosterico, 240, 155);
        }

        // --- Ispisivanje teksta ---
        GUI_SetFont(GUI_FONT_24B_1);
        GUI_SetColor(GUI_ORANGE);
        GUI_SetTextMode(GUI_TM_TRANS);
        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt(lng(TXT_LIGHTS), 95, 110);
        GUI_DispStringAt(lng(TXT_THERMOSTAT), 285, 110);
        GUI_DispStringAt(lng(TXT_BLINDS), 95, 250);
        GUI_DispStringAt(lng(TXT_DEFROSTER), 285, 250);

        GUI_MULTIBUF_EndEx(1);

        menu_thst = 0; // Resetuj fleg za termostat ekran
    }
    // Ako je ekran već iscrtan, provjeri da li treba ažurirati ikonicu za odmrzivač
    else if (menu_lc == 1) {
        // `ctrl1` fleg se postavlja u PID_Hook kada se pritisne dugme za odmrzivač
        if (ctrl1) {
            ctrl1 = 0; // Resetuj fleg
            GUI_MULTIBUF_BeginEx(1);
            GUI_ClearRect(240, 155, bmdefrosterico.XSize, bmdefrosterico.YSize);
            if(Defroster_isActive()) {
                GUI_DrawBitmap(&bmdefrostericoOn, 240, 155);
            } else {
                GUI_DrawBitmap(&bmdefrosterico, 240, 155);
            }
            GUI_MULTIBUF_EndEx(1);
        }
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
    // Ažuriranje prikaza termostata se radi unutar jedne multibuffer transakcije.
    GUI_MULTIBUF_BeginEx(1);

    if (menu_thst == 0) {
        // Ako je ovo prvi put da ulazimo na ekran, iscrtavamo kompletnu pozadinu.
        menu_thst = 1;

        GUI_MULTIBUF_BeginEx(0);
        GUI_SelectLayer(0);
        GUI_SetColor(GUI_BLACK);
        GUI_Clear();
        // Iscrtavanje pozadinske bitmap slike termostata.
        GUI_BMP_Draw(&thstat, 0, 0);
        // Iscrtavanje hamburger meni ikonice u gornjem desnom uglu.
        GUI_SetPenSize(9);
        GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);
        GUI_ClearRect(380, 0, 480, 100);
        GUI_DrawLine(400,20,450,20);
        GUI_DrawLine(400,40,450,40);
        GUI_DrawLine(400,60,450,60);
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
    } else if (menu_thst == 1) {
        // Ako je ekran već iscrtan, obrađujemo samo promjene stanja.

        // Logika za povećanje/smanjenje zadane temperature.
        if (btninc && !_btninc) {
            _btninc = 1;
            Thermostat_SP_Temp_Increment();
            SaveThermostatController(&thst, EE_THST1);
            DISPSetPoint();
        } else if (!btninc && _btninc) {
            _btninc = 0;
        }

        if (btndec && !_btndec) {
            _btndec = 1;
            Thermostat_SP_Temp_Decrement();
            SaveThermostatController(&thst, EE_THST1);
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
            if(IsTempRegActiv()) {
                GUI_SetColor(GUI_GREEN);
            } else {
                GUI_SetColor(GUI_RED);
            }

            GUI_SetFont(GUI_FONT_32B_1);
            GUI_GotoXY(410, 170);
            GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);
            if(IsTempRegActiv()) {
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
            GUI_DispSDec(thst.mv_temp / 10, 3);
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
            menu_thst = 0;
            if(IsTempRegActiv()) {
                thst.fan_speed = 0;
                TempRegOff();
            } else {
                TempRegHeating();
            }
            SaveThermostatController(&thst, EE_THST1);
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
    // Očisti oba grafička sloja.
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();

    // Postavi minimalnu svjetlinu ekrana.
    DISPSetBrightnes(DISP_BRGHT_MIN);

    // Postavi aktivni ekran na glavni meni.
    screen = SCREEN_MAIN;

    // Resetovanje svih flagova i brojača.
    menu_thst = 0;
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
            GUI_ClearRect(0, 50, 480, 200);

            // Promjena boje teksta i zvučni signal za posljednjih 5 sekundi.
            GUI_SetColor((clrtmr > 5) ? GUI_GREEN : GUI_RED);
            if (clrtmr <= 5) {
                BuzzerOn();
                HAL_Delay(1);
                BuzzerOff();
            }

            // Iscrtavanje teksta "VRIJEME BRISANJA EKRANA" i preostalog vremena.
            GUI_SetFont(GUI_FONT_32_1);
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
            GUI_DispStringAt(lng(TXT_DISPLAY_CLEAN_TIME), 240, 80);
            GUI_SetFont(GUI_FONT_D64);
            GUI_DispDecAt(clrtmr, 240, 156, 0);

            GUI_MULTIBUF_EndEx(1);

            if (clrtmr) {
                --clrtmr;
            } else {
                // Vrati se na početni ekran kad tajmer istekne.
                screen = SCREEN_RETURN_TO_FIRST;
            }
        }
    }
}

/**
 * @brief Servisira prvi ekran podešavanja (kontrola termostata i ventilatora).
 * @note Ova funkcija detektuje promjene na GUI widgetima i ažurira
 * globalne varijable. Kada korisnik pritisne 'SAVE' ili 'NEXT',
 * promjene se upisuju u EEPROM.
 */
static void Service_SettingsScreen_1(void)
{
    // Detekcija promjena na RADIO i SPINBOX widgetima.
    if (thst.th_ctrl != RADIO_GetValue(hThstControl)) {
        thst.th_ctrl = RADIO_GetValue(hThstControl);
        ++thsta;
    }
    else if (thst.fan_ctrl != RADIO_GetValue(hFanControl)) {
        thst.fan_ctrl = RADIO_GetValue(hFanControl);
        ++thsta;
    }
    else if (thst.sp_max != SPINBOX_GetValue(hThstMaxSetPoint)) {
        Thermostat_Set_SP_Max(SPINBOX_GetValue(hThstMaxSetPoint));
        SPINBOX_SetValue(hThstMaxSetPoint, thst.sp_max); // Ažuriraj widget nakon validacije
        ++thsta;
    }
    else if (thst.sp_min != SPINBOX_GetValue(hThstMinSetPoint)) {
        Thermostat_Set_SP_Min(SPINBOX_GetValue(hThstMinSetPoint));
        SPINBOX_SetValue(hThstMinSetPoint, thst.sp_min); // Ažuriraj widget nakon validacije
        ++thsta;
    }
    else if (thst.fan_diff != SPINBOX_GetValue(hFanDiff)) {
        thst.fan_diff = SPINBOX_GetValue(hFanDiff);
        ++thsta;
    }
    else if (thst.fan_loband != SPINBOX_GetValue(hFanLowBand)) {
        thst.fan_loband = SPINBOX_GetValue(hFanLowBand);
        ++thsta;
    }
    else if (thst.fan_hiband != SPINBOX_GetValue(hFanHiBand)) {
        thst.fan_hiband = SPINBOX_GetValue(hFanHiBand);
        ++thsta;
    }
    else if (thst.group != SPINBOX_GetValue(hThstGroup)) {
        thst.group = SPINBOX_GetValue(hThstGroup);
        thsta = 1;
    }
    else if (thst.master != CHECKBOX_IsChecked(hThstMaster)) {
        thst.master = CHECKBOX_IsChecked(hThstMaster);
        thsta = 1;
    }

    // Obrada pritiska na dugmad "SAVE" ili "NEXT"
    if (BUTTON_IsPressed(hBUTTON_Ok)) {
        if (thsta) {
            SaveThermostatController(&thst, EE_THST1); // Sačuvaj promjene u EEPROM
            thst.hasInfoChanged = true;
        }
        thsta = 0;
        DSP_KillSet1Scrn(); // Uništavanje widgeta trenutnog ekrana
        screen = SCREEN_RETURN_TO_FIRST;
    } else if (BUTTON_IsPressed(hBUTTON_Next)) {
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
    if (scrnsvr_clk_clr != SPINBOX_GetValue(hSPNBX_ScrnsvrClockColour)) {
        scrnsvr_clk_clr = SPINBOX_GetValue(hSPNBX_ScrnsvrClockColour);
        GUI_FillRect(340, 51, 430, 59);
    }
    // Ažuriranje statusa screensavera.
    if (CHECKBOX_GetState(hCHKBX_ScrnsvrClock) == 1) {
        ScrnsvrClkSet();
    } else {
        ScrnsvrClkReset();
    }

    // Ažuriranje ostalih konfiguracionih varijabli.
    high_bcklght = SPINBOX_GetValue(hSPNBX_DisplayHighBrightness);
    low_bcklght = SPINBOX_GetValue(hSPNBX_DisplayLowBrightness);
    scrnsvr_tout = SPINBOX_GetValue(hSPNBX_ScrnsvrTimeout);
    scrnsvr_ena_hour = SPINBOX_GetValue(hSPNBX_ScrnsvrEnableHour);
    scrnsvr_dis_hour = SPINBOX_GetValue(hSPNBX_ScrnsvrDisableHour);
    scrnsvr_clk_clr = SPINBOX_GetValue(hSPNBX_ScrnsvrClockColour);

    // Obrada pritiska na dugmad "SAVE" i "NEXT".
    if (BUTTON_IsPressed(hBUTTON_Ok)) {
        if (thsta) {
            thsta = 0;
            SaveThermostatController(&thst, EE_THST1);
        }
        if (lcsta) {
            lcsta = 0;
        }

        // Upisivanje svih promijenjenih postavki u EEPROM.
        ebuf[0] = low_bcklght;
        ebuf[1] = high_bcklght;
        ebuf[2] = scrnsvr_tout;
        ebuf[3] = scrnsvr_ena_hour;
        ebuf[4] = scrnsvr_dis_hour;
        ebuf[5] = scrnsvr_clk_clr;
        ebuf[6] = IsScrnsvrClkActiv();
        EE_WriteBuffer(ebuf, EE_DISP_LOW_BCKLGHT, 7);

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
    // Provjera promjena na widgetima. Originalni kod je imao komentarisane linije,
    // što implicira da je funkcionalnost bila planirana, ali nije u potpunosti implementirana.
    // Zadržavamo strukturu za buduću implementaciju.

    /*
    if(Ventilator_getRelay(&ventilator) != SPINBOX_GetValue(hVentilatorRelay))
    {
        Ventilator_setRelay(&ventilator, SPINBOX_GetValue(hVentilatorRelay));
        settingsChanged = 1;
    }
    else if(Ventilator_getDelayOnTime(&ventilator) != SPINBOX_GetValue(hVentilatorDelayOn))
    {
        Ventilator_setDelayOnTime(&ventilator, SPINBOX_GetValue(hVentilatorDelayOn));
        settingsChanged = 1;
    }
    else if(Ventilator_getDelayOffTime(&ventilator) != SPINBOX_GetValue(hVentilatorDelayOff))
    {
        Ventilator_setDelayOffTime(&ventilator, SPINBOX_GetValue(hVentilatorDelayOff));
        settingsChanged = 1;
    }
    else if(Ventilator_getDelayOnUse(&ventilator) != CHECKBOX_GetState(hVentilatorUseDelayOn))
    {
        Ventilator_setDelayOnUse(&ventilator, CHECKBOX_GetState(hVentilatorUseDelayOn));
        settingsChanged = 1;
    }
    else if(Ventilator_getDelayOffUse(&ventilator) != CHECKBOX_GetState(hVentilatorUseDelayOff))
    {
        Ventilator_setDelayOffUse(&ventilator, CHECKBOX_GetState(hVentilatorUseDelayOff));
        settingsChanged = 1;
    }
    */

    if (BUTTON_IsPressed(hBUTTON_Ok)) {
        if(settingsChanged) {
            // Pretpostavlja se da postoji funkcija za spremanje postavki ventilatora.
            // Ventilator_Save(&ventilator, EE_VENTILATOR);
            settingsChanged = 0;
        }
        DSP_KillSet3Scrn();
        screen = SCREEN_RETURN_TO_FIRST;
    } else if (BUTTON_IsPressed(hBUTTON_Next)) {
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
 * @note Ova funkcija dinamički obrađuje promjene u postavkama svjetala,
 * ažurira stanje widgeta i ponovo iscrtava ikonu kada je to potrebno.
 */
static void Service_SettingsScreen_5(void)
{
    GUI_MULTIBUF_BeginEx(1);

    uint8_t i = lightsModbusSettingsMenu * LIGHTS_MODBUS_PER_SETTINGS;

    // Dohvaćanje handle-ova widgeta.
    lightsWidgets[i].relay = (SPINBOX_Handle)WM_GetDialogItem(WM_GetDesktopWindow(), ID_LightsModbusRelay + (i * 12));
    lightsWidgets[i].iconID = (SPINBOX_Handle)WM_GetDialogItem(WM_GetDesktopWindow(), ID_LightsModbusRelay + (i * 12) + 1);
    lightsWidgets[i].controllerID_on = (SPINBOX_Handle)WM_GetDialogItem(WM_GetDesktopWindow(), ID_LightsModbusRelay + (i * 12) + 2);
    lightsWidgets[i].controllerID_on_delay = (SPINBOX_Handle)WM_GetDialogItem(WM_GetDesktopWindow(), ID_LightsModbusRelay + (i * 12) + 3);
    lightsWidgets[i].on_hour = (SPINBOX_Handle)WM_GetDialogItem(WM_GetDesktopWindow(), ID_LightsModbusRelay + (i * 12) + 4);
    lightsWidgets[i].on_minute = (SPINBOX_Handle)WM_GetDialogItem(WM_GetDesktopWindow(), ID_LightsModbusRelay + (i * 12) + 5);
    lightsWidgets[i].offTime = (SPINBOX_Handle)WM_GetDialogItem(WM_GetDesktopWindow(), ID_LightsModbusRelay + (i * 12) + 6);
    lightsWidgets[i].communication_type = (SPINBOX_Handle)WM_GetDialogItem(WM_GetDesktopWindow(), ID_LightsModbusRelay + (i * 12) + 7);
    lightsWidgets[i].local_pin = (SPINBOX_Handle)WM_GetDialogItem(WM_GetDesktopWindow(), ID_LightsModbusRelay + (i * 12) + 8);
    lightsWidgets[i].sleep_time = (SPINBOX_Handle)WM_GetDialogItem(WM_GetDesktopWindow(), ID_LightsModbusRelay + (i * 12) + 9);
    lightsWidgets[i].button_external = (SPINBOX_Handle)WM_GetDialogItem(WM_GetDesktopWindow(), ID_LightsModbusRelay + (i * 12) + 10);
    lightsWidgets[i].tiedToMainLight = (CHECKBOX_Handle)WM_GetDialogItem(WM_GetDesktopWindow(), ID_LightsModbusRelay + (i * 12) + 11);
    lightsWidgets[i].rememberBrightness = (CHECKBOX_Handle)WM_GetDialogItem(WM_GetDesktopWindow(), ID_LightsModbusRelay + (i * 12) + 12);

    // Provjera promjena na widgetima i ažuriranje globalne strukture
    if(LIGHT_GetRelay(lights_modbus + i) != SPINBOX_GetValue(lightsWidgets[i].relay))
    {
        settingsChanged = 1;
        LIGHT_SetRelay(lights_modbus + i, SPINBOX_GetValue(lightsWidgets[i].relay));
    }
    else if(lights_modbus[i].iconID != SPINBOX_GetValue(lightsWidgets[i].iconID))
    {
        settingsChanged = 1;
        lights_modbus[i].iconID = SPINBOX_GetValue(lightsWidgets[i].iconID);
        // Iscrtavanje nove ikonice nakon promjene
        GUI_ClearRect(380, 0, 480, 100);
        GUI_DrawBitmap(LIGHT_GetIcon(lights_modbus + i), 480 - LIGHT_GetIcon(lights_modbus + i)->XSize, 0);
    }
    else if(lights_modbus[i].controllerID_on != SPINBOX_GetValue(lightsWidgets[i].controllerID_on))
    {
        settingsChanged = 1;
        lights_modbus[i].controllerID_on = SPINBOX_GetValue(lightsWidgets[i].controllerID_on);
    }
    else if(LIGHT_GetOnDelayTime(lights_modbus + i) != SPINBOX_GetValue(lightsWidgets[i].controllerID_on_delay))
    {
        settingsChanged = 1;
        LIGHT_SetOnDelayTime(lights_modbus + i, SPINBOX_GetValue(lightsWidgets[i].controllerID_on_delay));
    }
    else if(LIGHT_GetOffTime(lights_modbus + i) != SPINBOX_GetValue(lightsWidgets[i].offTime))
    {
        settingsChanged = 1;
        LIGHT_SetOffTime(lights_modbus + i, SPINBOX_GetValue(lightsWidgets[i].offTime));
    }
    else if(lights_modbus[i].on_hour != SPINBOX_GetValue(lightsWidgets[i].on_hour))
    {
        settingsChanged = 1;
        lights_modbus[i].on_hour = SPINBOX_GetValue(lightsWidgets[i].on_hour);
    }
    else if(lights_modbus[i].on_minute != SPINBOX_GetValue(lightsWidgets[i].on_minute))
    {
        settingsChanged = 1;
        lights_modbus[i].on_minute = SPINBOX_GetValue(lightsWidgets[i].on_minute);
    }
    else if(lights_modbus[i].communication_type != SPINBOX_GetValue(lightsWidgets[i].communication_type))
    {
        settingsChanged = 1;
        lights_modbus[i].communication_type = SPINBOX_GetValue(lightsWidgets[i].communication_type);
    }
    else if(lights_modbus[i].local_pin != SPINBOX_GetValue(lightsWidgets[i].local_pin))
    {
        settingsChanged = 1;
        lights_modbus[i].local_pin = SPINBOX_GetValue(lightsWidgets[i].local_pin);
    }
    else if(lights_modbus[i].sleep_time != SPINBOX_GetValue(lightsWidgets[i].sleep_time))
    {
        settingsChanged = 1;
        lights_modbus[i].sleep_time = SPINBOX_GetValue(lightsWidgets[i].sleep_time);
    }
    else if(lights_modbus[i].button_external != SPINBOX_GetValue(lightsWidgets[i].button_external))
    {
        settingsChanged = 1;
        lights_modbus[i].button_external = SPINBOX_GetValue(lightsWidgets[i].button_external);
    }
    else if(LIGHT_isTiedToMainLight(lights_modbus + i) != CHECKBOX_GetState(lightsWidgets[i].tiedToMainLight))
    {
        settingsChanged = 1;
        if(CHECKBOX_GetState(lightsWidgets[i].tiedToMainLight)) LIGHT_TieToMainLight(lights_modbus + i);
        else LIGHT_UntieFromMainLight(lights_modbus + i);
    }
    else if(LIGHT_isBrightnessRemembered(lights_modbus + i) != CHECKBOX_GetState(lightsWidgets[i].rememberBrightness))
    {
        settingsChanged = 1;
        LIGHT_RememberBrightnessSet(lights_modbus + i, CHECKBOX_GetState(lightsWidgets[i].rememberBrightness));
    }


    // Ažuriranje ikonice sa strane (posljednji prikazani widget).
    GUI_ClearRect(380, 0, 480, 100);
    GUI_DrawBitmap(LIGHT_GetIcon(lights_modbus + i), 480 - LIGHT_GetIcon(lights_modbus + i)->XSize, 0);

    // Logika za dugmad OK i NEXT
    if (BUTTON_IsPressed(hBUTTON_Ok))
    {
        if(settingsChanged)
        {
            LIGHTS_Save();
            settingsChanged = 0;
        }
        DSP_KillSet5Scrn();
        screen = SCREEN_RETURN_TO_FIRST;
    }
    else if (BUTTON_IsPressed(hBUTTON_Next))
    {
        if((LIGHTS_MODBUS_SIZE - ((lightsModbusSettingsMenu + 1) * LIGHTS_MODBUS_PER_SETTINGS)) > 0)
        {
            DSP_KillSet5Scrn();
            ++lightsModbusSettingsMenu;
            DSP_InitSet5Scrn();
        }
        else
        {
            if(settingsChanged)
            {
                LIGHTS_Save();
                settingsChanged = 0;
            }

            DSP_KillSet5Scrn();
            lightsModbusSettingsMenu = 0;

            DSP_InitSet6Scrn();
            screen = SCREEN_SETTINGS_6;
        }
    }

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
        } else if(Curtain_GetMoveTime() != SPINBOX_GetValue(hCurtainsMoveTime)) {
            Curtain_SetMoveTime(SPINBOX_GetValue(hCurtainsMoveTime));
            settingsChanged = 1;
        } else if(bOnlyLeaveScreenSaverAfterTouch != CHECKBOX_GetState(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH)) {
            bOnlyLeaveScreenSaverAfterTouch = CHECKBOX_GetState(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH);
            settingsChanged = 1;
        } else if(LightNightTimer_isEnabled != CHECKBOX_GetState(hCHKBX_LIGHT_NIGHT_TIMER)) {
            LightNightTimer_isEnabled = CHECKBOX_GetState(hCHKBX_LIGHT_NIGHT_TIMER);
            settingsChanged = 1;
        }
    }

    if(BUTTON_IsPressed(hBUTTON_Ok)) {
        if(settingsChanged) {
            Curtains_Save();
            EE_WriteBuffer(&tfifa, EE_TFIFA, 1);
            EE_WriteBuffer(&bOnlyLeaveScreenSaverAfterTouch, EE_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, 1);
            EE_WriteBuffer(&LightNightTimer_isEnabled, EE_LIGHT_NIGHT_TIMER, 1);
            settingsChanged = 0;
        }
        DSP_KillSet6Scrn();
        screen = SCREEN_RETURN_TO_FIRST;
    } else if (BUTTON_IsPressed(hBUTTON_Next)) {
        if(settingsChanged) {
            Curtains_Save();
            EE_WriteBuffer(&tfifa, EE_TFIFA, 1);
            EE_WriteBuffer(&bOnlyLeaveScreenSaverAfterTouch, EE_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, 1);
            EE_WriteBuffer(&LightNightTimer_isEnabled, EE_LIGHT_NIGHT_TIMER, 1);
            settingsChanged = 0;
        }
        DSP_KillSet6Scrn();
        DSP_InitSet7Scrn();
        screen = SCREEN_SETTINGS_7;
    }
}

/**
 * @brief Servisira sedmi ekran podešavanja (Defroster).
 * @note Ova funkcija obrađuje postavke za ciklus i aktivno vrijeme odmrzivača.
 * Promjene se spremaju u EEPROM nakon pritiska na "SAVE".
 */
static void Service_SettingsScreen_7(void)
{
    // Provjera promjena na widgetima.
    if(defroster.cycleTime != SPINBOX_GetValue(defroster_settingWidgets.cycleTime)) {
        Defroster_SetCycleTime(SPINBOX_GetValue(defroster_settingWidgets.cycleTime));
        settingsChanged = 1;
    } else if(defroster.activeTime != SPINBOX_GetValue(defroster_settingWidgets.activeTime)) {
        Defroster_SetActiveTime(SPINBOX_GetValue(defroster_settingWidgets.activeTime));
        settingsChanged = 1;
    } else if(defroster.pin != SPINBOX_GetValue(defroster_settingWidgets.pin)) {
        defroster.pin = SPINBOX_GetValue(defroster_settingWidgets.pin);
        settingsChanged = 1;
    }

    if(BUTTON_IsPressed(hBUTTON_Ok)) {
        if(settingsChanged) {
            Defroster_Save(); // Spremi postavke odmrzivača.
            settingsChanged = 0;
        }
        DSP_KillSet7Scrn();
        screen = SCREEN_RETURN_TO_FIRST;
    } else if (BUTTON_IsPressed(hBUTTON_Next)) {
        if(settingsChanged) {
            Defroster_Save();
            settingsChanged = 0;
        }
        DSP_KillSet7Scrn();
        DSP_InitSet1Scrn(); // Prelazak na prvi ekran podešavanja.
        screen = SCREEN_SETTINGS_1;
    }
}
/**
 * @brief Servisira ekran sa svjetlima.
 * @note Ova funkcija dinamički iscrtava ikone svjetala na osnovu broja
 * i rasporeda definisanog u konfiguraciji. Iscrtavanje se obavlja samo
 * kada je to neophodno, koristeći 'shouldDrawScreen' flag.
 */
static void Service_LightsScreen(void)
{
    // Provjera da li je potrebno ponovno iscrtavanje cijelog ekrana.
    if(shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);

        GUI_Clear();
        // Iscrtavanje hamburger meni ikonice.
        GUI_SetPenSize(9);
        GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);
        GUI_DrawLine(400,20,450,20);
        GUI_DrawLine(400,40,450,40);
        GUI_DrawLine(400,60,450,60);

        // Dinamički izračunavanje pozicije ikonica.
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

            for(uint8_t idx = 0; idx < lightsInRow; ++idx) {
                const LIGHT_Modbus_CmdTypeDef* light = lights_modbus + lightsInRowSum + idx;
                int x = (currentLightsMenuSpaceBetween * ((idx % lightsInRow) + 1)) + (80 * (idx % lightsInRow));
                GUI_DrawBitmap(LIGHT_GetIcon(light), x, y);
            }
            lightsInRowSum += lightsInRow;
            y += 130;
        }
        GUI_MULTIBUF_EndEx(1);
    }
}

/**
 * @brief Servisira ekran sa zavjesama.
 * @note Prikazuje vizualizaciju zavjesa sa dugmadima za gore/dolje i prebacivanje
 * između pojedinačnih zavjesa ili svih odjednom.
 */
static void Service_CurtainsScreen(void)
{
    if(shouldDrawScreen) {
        shouldDrawScreen = 0;

        GUI_MULTIBUF_BeginEx(1);

        GUI_Clear();
        // Iscrtavanje hamburger meni ikonice.
        GUI_SetPenSize(9);
        GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);
        GUI_DrawLine(400,20,450,20);
        GUI_DrawLine(400,40,450,40);
        GUI_DrawLine(400,60,450,60);

        // Iscrtavanje dugmadi za prebacivanje između zavjesa, ako ih ima više od jedne.
        if(Curtains_getCount() > 1) {
            GUI_DrawBitmap(&bmprevious, 0, 192);
            GUI_DrawBitmap(&bmnext, 320, 192);
        }

        GUI_ClearRect(0, 0, 70, 70);

        GUI_SetColor(GUI_WHITE);
        GUI_SetFont(GUI_FONT_D48);
        GUI_SetTextMode(GUI_TM_TRANS);
        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);

        // Prikaz broja odabrane zavjese ili "SVE".
        if(!Curtain_areAllSelected()) {
            GUI_DispDecAt(Curtain_getSelected() + 1, 50, 50, ((Curtain_getSelected() + 1) < 10) ? 1 : 2);
        }

        // Crtanje strelica za upravljanje zavjesama.
        uint8_t curtainMenuSpaceBetween_local = (380 - (120 * 1)) / (1 - 1 + 2);
        GUI_SetColor(GUI_WHITE);

        GUI_DrawLine(curtainMenuSpaceBetween_local, 136, curtainMenuSpaceBetween_local + 120, 136);

        // Definiranje tačaka za trouglove
        const GUI_POINT aBlindsUp[] = {
            {0, 90},
            {180, 90},
            {90, 0}
        };
        const GUI_POINT aBlindsDown[] = {
            {0, 0},
            {180, 0},
            {90, 90}
        };

        // Crtanje gornjeg trougla za 'GORE'.
        GUI_SetColor(GUI_RED);
        GUI_DrawPolygon(aBlindsUp, 3, 140, 20);

        // Crtanje donjeg trougla za 'DOLJE'.
        GUI_SetColor(GUI_BLUE);
        GUI_DrawPolygon(aBlindsDown, 3, 140, 150);

        GUI_MULTIBUF_EndEx(1);
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

        // Iscrtavanje hamburger meni ikonice i linija.
        GUI_SetPenSize(9);
        GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);

        GUI_DrawLine(400,20,450,20);
        GUI_DrawLine(400,40,450,40);
        GUI_DrawLine(400,60,450,60);

        GUI_DrawLine(380,10,380,262);

        GUI_DrawLine(126,60,126,212);
        GUI_DrawLine(252,60,252,212);

        // Iscrtavanje ikonica.
        GUI_DrawBitmap(&bmnext, 385, 159);
        GUI_DrawBitmap(&bmCLEAN, 18, 76);
        GUI_DrawBitmap(&bmwifi, 146, 76);
        GUI_DrawBitmap(&bmmobilePhone, 290, 76);

        // Ispisivanje teksta.
        GUI_SetFont(GUI_FONT_24B_1);
        GUI_SetColor(GUI_ORANGE);
        GUI_SetTextMode(GUI_TM_TRANS);

        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt(lng(TXT_CLEAN), 63, 176);

        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt(lng(TXT_WIFI), 189, 176);

        GUI_SetTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
        GUI_DispStringAt(lng(TXT_APP), 315, 176);

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
        GUI_SetPenSize(9);
        GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);
        GUI_DrawLine(400,20,450,20);
        GUI_DrawLine(400,40,450,40);
        GUI_DrawLine(400,60,450,60);

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

        // Iscrtavanje hamburger meni ikonice.
        GUI_SetPenSize(9);
        GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);

        GUI_DrawLine(400,20,450,20);
        GUI_DrawLine(400,40,450,40);
        GUI_DrawLine(400,60,450,60);

        // Prikaz kontrola na osnovu tipa svjetla (dimmer ili RGB).
        if(((light_selectedIndex == LIGHTS_MODBUS_SIZE) && (!lights_allSelected_hasRGB)) || LIGHT_isDimmer(lights_modbus + light_selectedIndex)) {
            GUI_DrawBitmap(&bmblackWhiteGradient, 20, 110);
        } else if(((light_selectedIndex == LIGHTS_MODBUS_SIZE) && lights_allSelected_hasRGB) || LIGHT_isRGB(lights_modbus + light_selectedIndex)) {
            GUI_SetColor(GUI_WHITE);
            GUI_FillRect(200, 20, 280, 100);
            GUI_DrawBitmap(&bmblackWhiteGradient, 20, 110);
            GUI_DrawBitmap(&bmcolorSpectrum, 20, 180);
        }

        GUI_MULTIBUF_EndEx(1);
    }
}
/**
 * @brief Servisira ekran za resetovanje glavnih prekidača/menija.
 * @note Ova funkcija prvenstveno prikazuje odbrojavanje ako je aktivan
 * Light Night Timer.
 */
static void Service_ResetMenuSwitches(void)
{
    // Provjera da li je Light Night Timer pokrenut.
    if(LightNightTimer_StartTime) {
        GUI_MULTIBUF_BeginEx(1);

        // Izračunavanje preostalog vremena za prikaz.
        const uint8_t dispTime = (((LIGHT_NIGHT_TIMER_DURATION * 1000) - (HAL_GetTick() - LightNightTimer_StartTime)) / 1000);

        GUI_SetColor(GUI_WHITE);
        GUI_SetFont(GUI_FONT_D32);
        GUI_SetTextMode(GUI_TM_TRANS);
        GUI_SetTextAlign(GUI_TA_HCENTER|GUI_TA_VCENTER);

        // Ažuriranje prikaza odbrojavanja na sredini ekrana.
        GUI_ClearRect(220, 116, 265, 156);
        GUI_DispDecAt(dispTime + 1, 240, 136, (((dispTime / 10) < 2) ? (dispTime / 10) : -1) + 1);

        GUI_MULTIBUF_EndEx(1);
    }
}


/**
 * @brief Upravlja svim periodičnim događajima i tajmerima.
 * @note Ova funkcija je srž logike za pozadinske procese koji se ne odnose
 * direktno na trenutni ekran. Uključuje logiku screensavera, tajmera za
 * noćno osvjetljenje i ažuriranje vremena.
 */
static void Handle_PeriodicEvents(void)
{
    static uint32_t out1_tmr = 0; // Lokalni statički tajmer za specifičnu logiku.

    // Tajmer koji se izvršava svake minute. Koristi se za provjeru automatskog paljenja svjetala.
    if ((HAL_GetTick() - everyMinuteTimerStart) >= (60 * 1000)) {
        everyMinuteTimerStart = HAL_GetTick();
        for (uint8_t i = 0; i < LIGHTS_getCount(); i++) {
            if (LIGHT_isTimeOnEnabled(lights_modbus + i) && LIGHT_isTimeToTurnOn(lights_modbus + i)) {
                LIGHT_On(lights_modbus + i);
                if (screen == SCREEN_LIGHTS) {
                    shouldDrawScreen = 1;
                } else if((screen == SCREEN_RESET_MENU_SWITCHES) || (screen == SCREEN_MAIN)) {
                    screen = SCREEN_RETURN_TO_FIRST;
                }
            }
        }
    }

    // Tajmer za ulazak u mod za podešavanje svjetla nakon dugog pritiska.
    if (light_settingsTimerStart && ((HAL_GetTick() - light_settingsTimerStart) >= (2 * 1000))) {
        light_settingsTimerStart = 0;
        screen = SCREEN_LIGHT_SETTINGS;
        shouldDrawScreen = 1;
    }

    // Screensaver tajmer.
    if (!IsScrnsvrActiv()) {
        if ((HAL_GetTick() - scrnsvr_tmr) >= (uint32_t)(scrnsvr_tout * 1000)) {
            uint8_t saveBrightness = 0;
            // Gašenje svih aktivnih ekrana podešavanja.
            if (screen == SCREEN_SETTINGS_1)      DSP_KillSet1Scrn();
            else if (screen == SCREEN_SETTINGS_2) DSP_KillSet2Scrn();
            else if (screen == SCREEN_SETTINGS_3) DSP_KillSet3Scrn();
            else if (screen == SCREEN_SETTINGS_4) DSP_KillSet4Scrn();
            else if (screen == SCREEN_SETTINGS_5) DSP_KillSet5Scrn();
            else if (screen == SCREEN_SETTINGS_6) DSP_KillSet6Scrn();
            else if (screen == SCREEN_SETTINGS_7) DSP_KillSet7Scrn();

            // Logika za čuvanje svjetline ako je opcija aktivna.
            for(uint8_t i = 0; i < LIGHTS_getCount(); i++) {
                if(lights_modbus[i].saveBrightness) {
                    lights_modbus[i].saveBrightness = 0;
                    saveBrightness = 1;
                }
            }
            if(saveBrightness) LIGHTS_Save();

            // Aktivacija screensaver-a.
            DISPSetBrightnes(low_bcklght);
            ScrnsvrInitReset();
            ScrnsvrSet();
            screen = SCREEN_RETURN_TO_FIRST;
        }
    }

    // Tajmer za automatsko gašenje releja 4 (nije potpuno implementirano).
    if (out1_tmr) {
        if ((HAL_GetTick() - out1_tmr) >= (uint32_t)(SECONDS_PER_HOUR * 4000)) {
            out1_tmr = 0;
        }
    }

    // Ažuriranje vremena na ekranu.
    if (HAL_GetTick() - rtctmr >= 1000) {
        rtctmr = HAL_GetTick();
        if(++refresh_tmr > 10) {
            refresh_tmr = 0;
            if (!IsScrnsvrActiv()) MVUpdateSet();
        }
        if (screen < SCREEN_CONTROL_SELECT) DISPDateTime();
    }

    // Provera promene stanja svetala sa busa i forsiranje ažuriranja grafike.
    uint8_t anyTiedLightChanged = 0;
    for(uint8_t i = 0; i < LIGHTS_getCount(); i++) {
        if(LIGHT_isTiedToMainLight(lights_modbus + i) && LIGHT_hasStatusChanged(lights_modbus + i)) {
            anyTiedLightChanged = 1;
            break;
        }
    }

    if (anyTiedLightChanged) {
        shouldDrawScreen = 1;
        if(screen == SCREEN_RESET_MENU_SWITCHES || screen == SCREEN_MAIN) {
            screen = SCREEN_MAIN;
        }
    }
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

    if(!IsRtcTimeValid()) return;

    HAL_RTC_GetTime(&hrtc, &rtctm, RTC_FORMAT_BCD);
    HAL_RTC_GetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);

    // Provjera da li treba aktivirati screensaver na osnovu sata.
    if(scrnsvr_ena_hour >= scrnsvr_dis_hour) {
        if (Bcd2Dec(rtctm.Hours) >= scrnsvr_ena_hour || Bcd2Dec(rtctm.Hours) < scrnsvr_dis_hour) {
            ScrnsvrEnable();
        } else if (IsScrnsvrEnabled()) {
            ScrnsvrDisable();
            screen = SCREEN_RETURN_TO_FIRST;
        }
    } else if(scrnsvr_ena_hour < scrnsvr_dis_hour) {
        if (Bcd2Dec(rtctm.Hours) >= scrnsvr_ena_hour && Bcd2Dec(rtctm.Hours) < scrnsvr_dis_hour) {
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
        GUI_GotoXY(CLOCK_H_POS, CLOCK_V_POS);
        GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);
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
    RADIO_SetValue(hThstControl, thst.th_ctrl);

    // Kreiranje RADIO dugmadi za kontrolu brzine ventilatora.
    hFanControl = RADIO_CreateEx(10, 150, 150, 80, 0, WM_CF_SHOW, 0, ID_FanControl, 2, 20);
    RADIO_SetTextColor(hFanControl, GUI_GREEN);
    RADIO_SetText(hFanControl, "ON / OFF", 0);
    RADIO_SetText(hFanControl, "3 SPEED", 1);
    RADIO_SetValue(hFanControl, thst.fan_ctrl);

    // Kreiranje SPINBOX-ova za maksimalnu i minimalnu zadanu temperaturu.
    hThstMaxSetPoint = SPINBOX_CreateEx(110, 20, 90, 30, 0, WM_CF_SHOW, ID_MaxSetpoint, THST_SP_MIN, THST_SP_MAX);
    SPINBOX_SetEdge(hThstMaxSetPoint, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstMaxSetPoint, thst.sp_max);
    hThstMinSetPoint = SPINBOX_CreateEx(110, 70, 90, 30, 0, WM_CF_SHOW, ID_MinSetpoint, THST_SP_MIN, THST_SP_MAX);
    SPINBOX_SetEdge(hThstMinSetPoint, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstMinSetPoint, thst.sp_min);

    // Kreiranje SPINBOX-ova za postavke ventilatora.
    hFanDiff = SPINBOX_CreateEx(110, 150, 90, 30, 0, WM_CF_SHOW, ID_FanDiff, 0, 10);
    SPINBOX_SetEdge(hFanDiff, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hFanDiff, thst.fan_diff);
    hFanLowBand = SPINBOX_CreateEx(110, 190, 90, 30, 0, WM_CF_SHOW, ID_FanLowBand, 0, 50);
    SPINBOX_SetEdge(hFanLowBand, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hFanLowBand, thst.fan_loband);
    hFanHiBand = SPINBOX_CreateEx(110, 230, 90, 30, 0, WM_CF_SHOW, ID_FanHiBand, 0, 100);
    SPINBOX_SetEdge(hFanHiBand, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hFanHiBand, thst.fan_hiband);

    // Kreiranje SPINBOX-a i CHECKBOX-a za grupni rad termostata.
    hThstGroup = SPINBOX_CreateEx(320, 20, 100, 40, 0, WM_CF_SHOW, ID_THST_GROUP, 0, 254);
    SPINBOX_SetEdge(hThstGroup, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstGroup, thst.group);
    hThstMaster = CHECKBOX_Create(320, 70, 170, 20, 0, ID_THST_MASTER, WM_CF_SHOW);
    CHECKBOX_SetTextColor(hThstMaster, GUI_GREEN);
    CHECKBOX_SetText(hThstMaster, "Master");
    CHECKBOX_SetState(hThstMaster, thst.master);

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
    SPINBOX_SetValue(hSPNBX_DisplayHighBrightness, high_bcklght);
    hSPNBX_DisplayLowBrightness = SPINBOX_CreateEx(10, 60, 90, 30, 0, WM_CF_SHOW, ID_DisplayLowBrightness, 1, 90);
    SPINBOX_SetEdge(hSPNBX_DisplayLowBrightness, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_DisplayLowBrightness, low_bcklght);

    // Kreiranje widgeta za postavke screensavera.
    hSPNBX_ScrnsvrTimeout = SPINBOX_CreateEx(10, 130, 90, 30, 0, WM_CF_SHOW, ID_ScrnsvrTimeout, 1, 240);
    SPINBOX_SetEdge(hSPNBX_ScrnsvrTimeout, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_ScrnsvrTimeout, scrnsvr_tout);
    hSPNBX_ScrnsvrEnableHour = SPINBOX_CreateEx(10, 170, 90, 30, 0, WM_CF_SHOW, ID_ScrnsvrEnableHour, 0, 23);
    SPINBOX_SetEdge(hSPNBX_ScrnsvrEnableHour, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_ScrnsvrEnableHour, scrnsvr_ena_hour);
    hSPNBX_ScrnsvrDisableHour = SPINBOX_CreateEx(10, 210, 90, 30, 0, WM_CF_SHOW, ID_ScrnsvrDisableHour, 0, 23);
    SPINBOX_SetEdge(hSPNBX_ScrnsvrDisableHour, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_ScrnsvrDisableHour, scrnsvr_dis_hour);

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
    SPINBOX_SetValue(hSPNBX_ScrnsvrClockColour, scrnsvr_clk_clr);
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
    GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);
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

    // Kreiranje SPINBOX-ova za relej i odgode.
    hVentilatorRelay = SPINBOX_CreateEx(10, 20, 110, 40, 0, WM_CF_SHOW, ID_VentilatorRelay, 0, 512);
    SPINBOX_SetEdge(hVentilatorRelay, SPINBOX_EDGE_CENTER);
    // SPINBOX_SetValue(hVentilatorRelay, Ventilator_getRelay(&ventilator)); // Komentarisano u originalnom kodu

    hVentilatorDelayOn = SPINBOX_CreateEx(10, 70, 110, 40, 0, WM_CF_SHOW, ID_VentilatorDelayOn, 0, 100);
    SPINBOX_SetEdge(hVentilatorDelayOn, SPINBOX_EDGE_CENTER);
    // SPINBOX_SetValue(hVentilatorDelayOn, Ventilator_getDelayOnTime(&ventilator));

    hVentilatorDelayOff = SPINBOX_CreateEx(10, 120, 110, 40, 0, WM_CF_SHOW, ID_VentilatorDelayOff, 1, 100);
    SPINBOX_SetEdge(hVentilatorDelayOff, SPINBOX_EDGE_CENTER);
    // SPINBOX_SetValue(hVentilatorDelayOff, Ventilator_getDelayOffTime(&ventilator));

    // Kreiranje CHECKBOX-ova za aktiviranje odgoda.
    hVentilatorUseDelayOn = CHECKBOX_Create(200, 80, 110, 20, 0, ID_VentilatorUseDelayOn, WM_CF_SHOW);
    CHECKBOX_SetTextColor(hVentilatorUseDelayOn, GUI_GREEN);
    CHECKBOX_SetText(hVentilatorUseDelayOn, "USE DELAY ON");
    // CHECKBOX_SetState(hVentilatorUseDelayOn, Ventilator_getDelayOnUse(&ventilator));

    hVentilatorUseDelayOff = CHECKBOX_Create(200, 130, 110, 20, 0, ID_VentilatorUseDelayOff, WM_CF_SHOW);
    CHECKBOX_SetTextColor(hVentilatorUseDelayOff, GUI_GREEN);
    CHECKBOX_SetText(hVentilatorUseDelayOff, "USE DELAY OFF");
    // CHECKBOX_SetState(hVentilatorUseDelayOff, Ventilator_getDelayOffUse(&ventilator));

    // Kreiranje navigacionih dugmadi.
    hBUTTON_Next = BUTTON_Create(410, 180, 60, 30, ID_Next, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_Create(410, 230, 60, 30, ID_Ok, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    GUI_MULTIBUF_EndEx(1);
}

/**
 * @brief Briše GUI widgete sa trećeg ekrana podešavanja.
 * @note Briše sve widgete vezane za postavke ventilatora.
 */
static void DSP_KillSet3Scrn(void)
{
    WM_DeleteWindow(hVentilatorRelay);
    WM_DeleteWindow(hVentilatorDelayOn);
    WM_DeleteWindow(hVentilatorDelayOff);
    WM_DeleteWindow(hVentilatorUseDelayOn);
    WM_DeleteWindow(hVentilatorUseDelayOff);
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
        WM_DeleteWindow(hCurtainsRelay[i * 2]);
        WM_DeleteWindow(hCurtainsRelay[(i * 2) + 1]);
    }
    WM_DeleteWindow(hBUTTON_Next);
    WM_DeleteWindow(hBUTTON_Ok);
}

/**
 * @brief Inicijalizuje peti ekran podešavanja (Modbus svjetla).
 * @note Ova funkcija dinamički kreira sve GUI widgete za podešavanje svjetala.
 * Postavke su podijeljene u dvije kolone radi bolje preglednosti.
 */
static void DSP_InitSet5Scrn(void)
{
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    int x1 = 10, x2 = 200, y_start = 5, y_step = 43;
    uint8_t i = lightsModbusSettingsMenu * LIGHTS_MODBUS_PER_SETTINGS;

    // Kreiranje widgeta za postavke jednog svjetla.
    // Prva kolona
    lightsWidgets[i].relay = SPINBOX_CreateEx(x1, y_start, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (i * 12), 0, 512);
    SPINBOX_SetEdge(lightsWidgets[i].relay, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[i].relay, LIGHT_GetRelay(lights_modbus + i));

    lightsWidgets[i].iconID = SPINBOX_CreateEx(x1, y_start + 1 * y_step, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (i * 12) + 1, 0, LIGHT_ICON_COUNT - 1);
    SPINBOX_SetEdge(lightsWidgets[i].iconID, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[i].iconID, lights_modbus[i].iconID);

    lightsWidgets[i].controllerID_on = SPINBOX_CreateEx(x1, y_start + 2 * y_step, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (i * 12) + 2, 0, 512);
    SPINBOX_SetEdge(lightsWidgets[i].controllerID_on, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[i].controllerID_on, lights_modbus[i].controllerID_on);

    lightsWidgets[i].controllerID_on_delay = SPINBOX_CreateEx(x1, y_start + 3 * y_step, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (i * 12) + 3, 0, 512);
    SPINBOX_SetEdge(lightsWidgets[i].controllerID_on_delay, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[i].controllerID_on_delay, lights_modbus[i].controllerID_on_delay);

    lightsWidgets[i].on_hour = SPINBOX_CreateEx(x1, y_start + 4 * y_step, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (i * 12) + 4, 0, 512);
    SPINBOX_SetEdge(lightsWidgets[i].on_hour, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[i].on_hour, lights_modbus[i].on_hour);

    lightsWidgets[i].on_minute = SPINBOX_CreateEx(x1, y_start + 5 * y_step, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (i * 12) + 5, 0, 512);
    SPINBOX_SetEdge(lightsWidgets[i].on_minute, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[i].on_minute, lights_modbus[i].on_minute);

    // Druga kolona
    lightsWidgets[i].offTime = SPINBOX_CreateEx(x2, y_start, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (i * 12) + 6, 0, 512);
    SPINBOX_SetEdge(lightsWidgets[i].offTime, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[i].offTime, LIGHT_GetOffTime(lights_modbus + i));

    lightsWidgets[i].communication_type = SPINBOX_CreateEx(x2, y_start + 1 * y_step, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (i * 12) + 7, 1, 3);
    SPINBOX_SetEdge(lightsWidgets[i].communication_type, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[i].communication_type, lights_modbus[i].communication_type);

    lightsWidgets[i].local_pin = SPINBOX_CreateEx(x2, y_start + 2 * y_step, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (i * 12) + 8, 0, 512);
    SPINBOX_SetEdge(lightsWidgets[i].local_pin, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[i].local_pin, lights_modbus[i].local_pin);

    lightsWidgets[i].sleep_time = SPINBOX_CreateEx(x2, y_start + 3 * y_step, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (i * 12) + 9, 0, 512);
    SPINBOX_SetEdge(lightsWidgets[i].sleep_time, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[i].sleep_time, lights_modbus[i].sleep_time);

    lightsWidgets[i].button_external = SPINBOX_CreateEx(x2, y_start + 4 * y_step, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay + (i * 12) + 10, 0, 512);
    SPINBOX_SetEdge(lightsWidgets[i].button_external, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(lightsWidgets[i].button_external, lights_modbus[i].button_external);

    // Checkboxi
    lightsWidgets[i].tiedToMainLight = CHECKBOX_Create(x2, y_start + 5 * y_step, 130, 20, 0, ID_LightsModbusRelay + (i * 12) + 11, WM_CF_SHOW);
    CHECKBOX_SetTextColor(lightsWidgets[i].tiedToMainLight, GUI_GREEN);
    CHECKBOX_SetText(lightsWidgets[i].tiedToMainLight, "TIED TO MAIN LIGHT");
    CHECKBOX_SetState(lightsWidgets[i].tiedToMainLight, LIGHT_isTiedToMainLight(lights_modbus + i));

    lightsWidgets[i].rememberBrightness = CHECKBOX_Create(x2, y_start + 5 * y_step + 23, 145, 20, 0, ID_LightsModbusRelay + (i * 12) + 12, WM_CF_SHOW);
    CHECKBOX_SetTextColor(lightsWidgets[i].rememberBrightness, GUI_GREEN);
    CHECKBOX_SetText(lightsWidgets[i].rememberBrightness, "REMEMBER BRIGHTNESS");
    CHECKBOX_SetState(lightsWidgets[i].rememberBrightness, LIGHT_isBrightnessRemembered(lights_modbus + i));

    // Dinamičke labele
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT | GUI_TA_VCENTER);

    // Prva kolona labela
    GUI_GotoXY(x1 + 100 + 10, y_start + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
    GUI_GotoXY(x1 + 100 + 10, y_start + 10 + 12);
    GUI_DispString("RELAY");

    GUI_GotoXY(x1 + 100 + 10, y_start + 1 * y_step + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
    GUI_GotoXY(x1 + 100 + 10, y_start + 1 * y_step + 10 + 12);
    GUI_DispString("ICON");

    GUI_GotoXY(x1 + 100 + 10, y_start + 2 * y_step + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
    GUI_GotoXY(x1 + 100 + 10, y_start + 2 * y_step + 10 + 12);
    GUI_DispString("ON ID");

    GUI_GotoXY(x1 + 100 + 10, y_start + 3 * y_step + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
    GUI_GotoXY(x1 + 100 + 10, y_start + 3 * y_step + 10 + 12);
    GUI_DispString("ON ID DELAY");

    GUI_GotoXY(x1 + 100 + 10, y_start + 4 * y_step + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
    GUI_GotoXY(x1 + 100 + 10, y_start + 4 * y_step + 10 + 12);
    GUI_DispString("HOUR ON");

    GUI_GotoXY(x1 + 100 + 10, y_start + 5 * y_step + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
    GUI_GotoXY(x1 + 100 + 10, y_start + 5 * y_step + 10 + 12);
    GUI_DispString("MINUTE ON");

    // Druga kolona labela
    GUI_GotoXY(x2 + 100 + 10, y_start + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
    GUI_GotoXY(x2 + 100 + 10, y_start + 10 + 12);
    GUI_DispString("DELAY OFF");

    GUI_GotoXY(x2 + 100 + 10, y_start + 1 * y_step + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
    GUI_GotoXY(x2 + 100 + 10, y_start + 1 * y_step + 10 + 12);
    GUI_DispString("COMM. TYPE");

    GUI_GotoXY(x2 + 100 + 10, y_start + 2 * y_step + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
    GUI_GotoXY(x2 + 100 + 10, y_start + 2 * y_step + 10 + 12);
    GUI_DispString("LOCAL PIN");

    GUI_GotoXY(x2 + 100 + 10, y_start + 3 * y_step + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
    GUI_GotoXY(x2 + 100 + 10, y_start + 3 * y_step + 10 + 12);
    GUI_DispString("SLEEP TIME");

    GUI_GotoXY(x2 + 100 + 10, y_start + 4 * y_step + 10);
    GUI_DispString("LIGHT ");
    GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
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
    // Dinamičko brisanje widgeta na osnovu trenutne stranice
    for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; i++)
    {
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
    }
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
    CHECKBOX_SetState(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, bOnlyLeaveScreenSaverAfterTouch);

    hCHKBX_LIGHT_NIGHT_TIMER = CHECKBOX_Create(10, 140, 170, 20, 0, ID_LIGHT_NIGHT_TIMER, WM_CF_SHOW);
    CHECKBOX_SetTextColor(hCHKBX_LIGHT_NIGHT_TIMER, GUI_GREEN);
    CHECKBOX_SetText(hCHKBX_LIGHT_NIGHT_TIMER, "LiGHT OFF TIMER AFTER 20h");
    CHECKBOX_SetState(hCHKBX_LIGHT_NIGHT_TIMER, LightNightTimer_isEnabled);

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
 * @brief Inicijalizuje sedmi ekran podešavanja (Defroster).
 * @note Kreira widgete za podešavanje vremena ciklusa, aktivnog vremena
 * i GPIO pina za odmrzivač.
 */
static void DSP_InitSet7Scrn(void)
{
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);

    // Kreiranje SPINBOX-a za vrijeme ciklusa odmrzivača.
    defroster_settingWidgets.cycleTime = SPINBOX_CreateEx(10, 10, 110, 40, 0, WM_CF_SHOW, ID_DEFROSTER_CYCLE_TIME, 0, 254);
    SPINBOX_SetEdge(defroster_settingWidgets.cycleTime, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(defroster_settingWidgets.cycleTime, defroster.cycleTime);
    GUI_GotoXY(130, 20);
    GUI_DispString("DEFROSTER");
    GUI_GotoXY(130, 32);
    GUI_DispString("CYCLE TIME");

    // Kreiranje SPINBOX-a za aktivno vrijeme odmrzivača.
    defroster_settingWidgets.activeTime = SPINBOX_CreateEx(10, 60, 110, 40, 0, WM_CF_SHOW, ID_DEFROSTER_ACTIVE_TIME, 0, 254);
    SPINBOX_SetEdge(defroster_settingWidgets.activeTime, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(defroster_settingWidgets.activeTime, defroster.activeTime);
    GUI_GotoXY(130, 70);
    GUI_DispString("DEFROSTER");
    GUI_GotoXY(130, 82);
    GUI_DispString("ACTIVE TIME");

    // Kreiranje SPINBOX-a za GPIO pin.
    defroster_settingWidgets.pin = SPINBOX_CreateEx(10, 110, 110, 40, 0, WM_CF_SHOW, ID_DEFROSTER_PIN, 0, 6);
    SPINBOX_SetEdge(defroster_settingWidgets.pin, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(defroster_settingWidgets.pin, defroster.pin);
    GUI_GotoXY(130, 120);
    GUI_DispString("DEFROSTER");
    GUI_GotoXY(130, 132);
    GUI_DispString("PIN");

    // Kreiranje navigacionih dugmadi.
    hBUTTON_Next = BUTTON_Create(410, 180, 60, 30, ID_Next, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_Create(410, 230, 60, 30, ID_Ok, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    GUI_MULTIBUF_EndEx(1);
}

/**
 * @brief Briše GUI widgete sa sedmog ekrana podešavanja.
 * @note Briše sve widgete vezane za postavke odmrzivača.
 */
static void DSP_KillSet7Scrn(void)
{
    WM_DeleteWindow(defroster_settingWidgets.cycleTime);
    WM_DeleteWindow(defroster_settingWidgets.activeTime);
    WM_DeleteWindow(defroster_settingWidgets.pin);
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
    else if(screen == SCREEN_CONTROL_SELECT) {
        HandlePress_ControlSelectScreen(pTS, click_flag);
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
    else if(screen == SCREEN_SELECT_SCREEN_2) {
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
 * @brief Obrada događaja otpuštanja dodira na ekranu, u zavisnosti od ekrana.
 * @param pTS Pokazivač na strukturu sa stanjem dodira.
 * @note Ova funkcija prosljeđuje događaj otpuštanja logici za specifičan ekran,
 * a zatim resetuje sve opšte flagove.
 */
static void HandleTouchReleaseEvent(GUI_PID_STATE * pTS) {
    // Logika za obradu otpuštanja na glavnom ekranu, ako dodir nije u zoni menija.
    if(screen == SCREEN_MAIN && !touch_in_menu_zone) {
        HandleRelease_MainScreenLogic(pTS);
    }
    else if(screen == SCREEN_LIGHTS) {
        // Provjeravamo da li je odabrano svjetlo i da li je pritisak bio kratak (ne dug).
        if(light_selectedIndex < LIGHTS_MODBUS_SIZE + 1) {
            // Logika za dimabilna/RGB svjetla: prebaci stanje ako pritisak nije bio dug.
            if (!LIGHT_isBinary(lights_modbus + light_selectedIndex) && (HAL_GetTick() - light_settingsTimerStart) < 2000) {
                LIGHT_Flip(lights_modbus + light_selectedIndex);
            }
            // Logika za binarna svjetla: uvijek prebaci stanje na kratak klik.
            else if (LIGHT_isBinary(lights_modbus + light_selectedIndex)) {
                LIGHT_Flip(lights_modbus + light_selectedIndex);
            }
        }
        // Uvijek resetuj timer za podešavanje svjetla i indeks nakon puštanja.
        light_settingsTimerStart = 0;
        light_selectedIndex = LIGHTS_MODBUS_SIZE + 1;
    }
    else if(screen == SCREEN_RESET_MENU_SWITCHES) {
        HandleRelease_ResetMenuSwitchesScreenArea(pTS);
    }

    // Resetovanje svih kontrolnih flagova.
    btnset = 0;
    btndec = 0U;
    btninc = 0U;
    ctrl1 = 0;
    thermostatOnOffTouch_timer = 0;
    LightNightTimer_StartTime = 0; // Resetujemo i ovaj tajmer
}


/**
 * @brief Obrada događaja pritiska za ekran "Control Select".
 * @note Ova funkcija detektuje koji je od 4 glavna ekrana (Svjetla, Termostat, Zavjese, Odmrzivač)
 * ili dugme "NEXT" pritisnuto, te postavlja odgovarajuće stanje.
 */
static void HandlePress_ControlSelectScreen(GUI_PID_STATE * pTS, uint8_t *click_flag) {
    if(pTS->x < 400) {
        if(pTS->y < 136) { // Gornja polovina
            if(pTS->x < 190) { // Gornji lijevi ugao: Svjetla
                screen = SCREEN_LIGHTS;
                shouldDrawScreen = 1;
            } else { // Gornji desni ugao: Termostat
                screen = SCREEN_THERMOSTAT;
            }
        } else { // Donja polovina
            if(pTS->x < 190) { // Donji lijevi ugao: Zavjese
                screen = SCREEN_CURTAINS;
                Curtain_ResetSelection();
                shouldDrawScreen = 1;
            } else { // Donji desni ugao: Odmrzivač
                // Togglovanje stanja odmrzivača.
                if(Defroster_isActive()) {
                    Defroster_Off();
                } else {
                    Defroster_On();
                }
                ctrl1 = 1; // Flag za ažuriranje ikonice.
            }
        }
    } else if(pTS->y > 159) { // Dugme "NEXT"
        shouldDrawScreen = 1;
        menu_lc = 0;
        screen = SCREEN_SELECT_SCREEN_2;
    }

    if(screen != SCREEN_CONTROL_SELECT) {
        *click_flag = 1;
    }
}

/**
 * @brief Obrada događaja pritiska za ekran "Thermostat".
 * @note Rukuje dugmadima za povećanje i smanjenje zadane temperature,
 * kao i dugim pritiskom za paljenje/gašenje termostata.
 */
static void HandlePress_ThermostatScreen(GUI_PID_STATE * pTS, uint8_t *click_flag) {
    // Provjera pritiska na "increase" i "decrease" dugmad.
    if((pTS->x > BTN_INC_X0) && (pTS->y > BTN_INC_Y0) && (pTS->x < BTN_INC_X1) && (pTS->y < BTN_INC_Y1)) {
        *click_flag = 1;
        btninc = 1;
    } else if((pTS->x > BTN_DEC_X0) && (pTS->y > BTN_DEC_Y0) && (pTS->x < BTN_DEC_X1) && (pTS->y < BTN_DEC_Y1)) {
        *click_flag = 1;
        btndec = 1;
    }
    // Provjera pritiska za toggle ON/OFF.
    else if((pTS->x > 400) && (pTS->y > 150) && (pTS->y < 190)) {
        *click_flag = 1;
        thermostatOnOffTouch_timer = HAL_GetTick();
        if(!thermostatOnOffTouch_timer) {
            thermostatOnOffTouch_timer = 1;
        }
    }
}



/**
 * @brief Obrada događaja pritiska za ekran "Lights".
 * @note Detektuje dodir na ikonu specifičnog svjetla. Ako je svjetlo dimabilno,
 * pokreće tajmer za dugi pritisak kako bi se omogućio ulazak u meni za podešavanja.
 */
static void HandlePress_LightsScreen(GUI_PID_STATE * pTS, uint8_t *click_flag) {
    light_selectedIndex = LIGHTS_MODBUS_SIZE + 1;
    light_settingsTimerStart = 0;

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

        for(uint8_t i_light = 0; i_light < lightsInRow; ++i_light) {
            int x = (currentLightsMenuSpaceBetween * ((i_light % lightsInRow) + 1)) + (80 * (i_light % lightsInRow));

            if((pTS->x > x) && (pTS->x < (x + 80)) && (pTS->y > y) && (pTS->y < (y + 120))) {
                *click_flag = 1;
                light_selectedIndex = lightsInRowSum + i_light;

                // Ako je svjetlo dimabilno/RGB, pokreni tajmer za dugi pritisak.
                if(!LIGHT_isBinary(lights_modbus + light_selectedIndex)) {
                    light_settingsTimerStart = HAL_GetTick();
                }
                LightNightTimer_StartTime = 0;
                break;
            }
        }
        lightsInRowSum += lightsInRow;
        y += 130;
    }
}



/**
 * @brief Obrada događaja pritiska za ekran "Curtains".
 * @note Rukuje pritiskom na strelice za pomicanje zavjesa "GORE" i "DOLJE",
 * kao i dugmadima za prebacivanje između pojedinačnih zavjesa i grupe.
 */
static void HandlePress_CurtainsScreen(GUI_PID_STATE * pTS, uint8_t *click_flag) {
    if(pTS->x < 400) {
        uint8_t local_curtainMenuCurtainLength = 120;
        uint8_t local_curtainMenuCurtainCount = 1;

        if((pTS->x > (200 - (local_curtainMenuCurtainLength / 2))) && (pTS->x < (200 + (local_curtainMenuCurtainLength / 2)))) {
            // Kontrola kretanja zavjesa.
            if(Curtain_areAllSelected()) {
                Curtains_MoveSignal((pTS->y < 136) ? CURTAIN_UP : CURTAIN_DOWN);
            } else {
                Curtain_MoveSignal(curtains + Curtain_getSelected(), (pTS->y < 136) ? CURTAIN_UP : CURTAIN_DOWN);
            }
            *click_flag = 1;
        } else if((Curtains_getCount() > 1) && (pTS->y > 192)) {
            // Prebacivanje između zavjesa.
            if(pTS->x > 320) {
                if(Curtain_areAllSelected()) {
                    Curtain_Select(0);
                } else {
                    Curtain_Select(Curtain_getSelected() + 1);
                }
                shouldDrawScreen = 1;
                *click_flag = 1;
            } else if(pTS->x < 80) {
                if(!Curtain_getSelected()) {
                    Curtain_Select(Curtains_getCount());
                } else {
                    Curtain_Select(Curtain_getSelected() - 1);
                }
                shouldDrawScreen = 1;
                *click_flag = 1;
            }
        }
    }
}

/**
 * @brief Obrada događaja pritiska za ekran "Select Screen 2".
 * @note Odgovoran je za prelazak na ekrane za čišćenje, Wi-Fi QR kod i App QR kod.
 */
static void HandlePress_SelectScreen2(GUI_PID_STATE * pTS, uint8_t *click_flag) {
    if(pTS->x < 400) {
        if((pTS->y > 116) && (pTS->y < 216)) {
            if(pTS->x < 126) {
                screen = SCREEN_CLEAN;
            } else if(pTS->x < 252) {
                screen = SCREEN_QR_CODE;
                qr_code_draw_id = QR_CODE_WIFI_ID;
                shouldDrawScreen = 1;
            } else {
                screen = SCREEN_QR_CODE;
                qr_code_draw_id = QR_CODE_APP_ID;
                shouldDrawScreen = 1;
            }
        }
    } else if(pTS->y > 159) {
        screen = SCREEN_CONTROL_SELECT;
        menu_lc = 0;
        shouldDrawScreen = 1;
    }
    if(screen != SCREEN_SELECT_SCREEN_2) {
        *click_flag = 1;
    }
}

/**
 * @brief Obrada događaja pritiska za ekran "Light Settings".
 * @note Ova funkcija omogućava direktno podešavanje svjetline i boje svjetla
 * dodirom na gradijent i spektar boja.
 */
static void HandlePress_LightSettingsScreen(GUI_PID_STATE * pTS) {
    uint8_t brightness = 255;
    GUI_COLOR color = 0;

    if((pTS->x < 80) && (pTS->y < 80)) {
        screen = SCREEN_LIGHTS;
        shouldDrawScreen = true;
        light_selectedIndex = LIGHTS_MODBUS_SIZE + 1;
        lights_allSelected_hasRGB = false;
        return;
    }

    if((((light_selectedIndex == LIGHTS_MODBUS_SIZE) && (lights_allSelected_hasRGB)) || LIGHT_isRGB(lights_modbus + light_selectedIndex)) && (pTS->x >= 200) && (pTS->x <= 280) && (pTS->y >= 20) && (pTS->y <= 100)) {
        color = GUI_WHITE;
    } else if((pTS->x >= 20) && (pTS->x <= 460)) {
        if((pTS->y >= 110) && (pTS->y <= 170)) {
            brightness = (uint8_t)(((pTS->x - 20) / (float)bmblackWhiteGradient.XSize) * 100.0f);
        } else if((((light_selectedIndex == LIGHTS_MODBUS_SIZE) && (lights_allSelected_hasRGB)) || LIGHT_isRGB(lights_modbus + light_selectedIndex)) && (pTS->y >= 180) && (pTS->y <= 240)) {
            color = LCD_GetPixelColor(pTS->x, pTS->y);
        }
        shouldDrawScreen = 1;
    }

    if(light_selectedIndex == LIGHTS_MODBUS_SIZE) {
        for(uint8_t i_light = 0; i_light < LIGHTS_getCount(); i_light++) {
            if(LIGHT_isTiedToMainLight(lights_modbus + i_light) && (!LIGHT_isBinary(lights_modbus + i_light))) {
                if(brightness != 255) {
                    LIGHT_SetBrightness(lights_modbus + i_light, brightness);
                } else if(LIGHT_isRGB(lights_modbus + i_light) && color) {
                    LIGHT_SetColor(lights_modbus + i_light, color);
                }
            }
        }
    } else {
        if(brightness != 255) {
            LIGHT_SetBrightness(lights_modbus + light_selectedIndex, brightness);
        } else if(LIGHT_isRGB(lights_modbus + light_selectedIndex) && color) {
            LIGHT_SetColor(lights_modbus + light_selectedIndex, color);
        }
    }
}

/**
 * @brief Obrada događaja pritiska u zoni glavnog prekidača na ekranu.
 * @note Pokreće tajmer za dugi pritisak kako bi se ušlo u podešavanja
 * svjetla ako je odabrano svjetlo dimabilno.
 */
static void HandlePress_ResetMenuSwitchesScreenArea(GUI_PID_STATE * pTS) {
    if((!bOnlyLeaveScreenSaverAfterTouch) || (bOnlyLeaveScreenSaverAfterTouch && (!IsScrnsvrActiv()))) {
        light_selectedIndex = LIGHTS_MODBUS_SIZE + 1;

        for(uint8_t i_light = 0; i_light < LIGHTS_getCount(); i_light++) {
            if(LIGHT_isTiedToMainLight(lights_modbus + i_light) && (!LIGHT_isBinary(lights_modbus + i_light))) {
                light_selectedIndex = LIGHTS_MODBUS_SIZE;
                if(LIGHT_isRGB(lights_modbus + i_light)) {
                    lights_allSelected_hasRGB = true;
                }
            }
        }

        if(light_selectedIndex == LIGHTS_MODBUS_SIZE) {
            light_settingsTimerStart = HAL_GetTick();
        }
    }
}


/**
 * @brief Obrada otpuštanja dodira na glavnom ekranu.
 * @note Togglovanje stanja svih svjetala povezanih s glavnim prekidačem.
 */
static void HandleRelease_MainScreenLogic(GUI_PID_STATE * pTS) {
    uint8_t isAnyLightCurrentlyOn = 0;
    // Provjera da li je neki od glavnih svjetala uključen
    for(uint8_t i_light = 0; i_light < LIGHTS_getCount(); ++i_light) {
        if(LIGHT_isTiedToMainLight(lights_modbus + i_light) && LIGHT_isNewValueOn(lights_modbus + i_light)) {
            isAnyLightCurrentlyOn = 1;
            break;
        }
    }
    // Postavljanje novog stanja za sva glavna svjetla
    bool newStateIsOn = !isAnyLightCurrentlyOn;
    for(uint8_t i_light = 0; i_light < LIGHTS_getCount(); ++i_light) {
        if(LIGHT_isTiedToMainLight(lights_modbus + i_light)) {
            if (newStateIsOn) {
                LIGHT_On(lights_modbus + i_light);
            } else {
                LIGHT_Off(lights_modbus + i_light);
            }
        }
    }

    // Logika za noćni tajmer
    if (LightNightTimer_isEnabled && !LightNightTimer_StartTime && !((Bcd2Dec(rtctm.Hours) > 6) && (Bcd2Dec(rtctm.Hours) < 20))) {
        if (newStateIsOn) {
            LightNightTimer_StartTime = HAL_GetTick();
            if(!LightNightTimer_StartTime) {
                LightNightTimer_StartTime = 1;
            }
        }
    } else {
        LightNightTimer_StartTime = 0;
    }

    shouldDrawScreen = 1;
    screen = SCREEN_MAIN;
}
/**
 * @brief Obrada otpuštanja dodira na ekranu sa svjetlima.
 * @note Togglovanje stanja odabranog svjetla nakon dodira.
 */
static void HandleRelease_LightsScreenLogic(GUI_PID_STATE * pTS) {
    if(light_selectedIndex < LIGHTS_MODBUS_SIZE + 1) {
        if(LIGHT_isBinary(lights_modbus + light_selectedIndex) || ((!LIGHT_isBinary(lights_modbus + light_selectedIndex)) && light_settingsTimerStart)) {
            light_settingsTimerStart = 0;
            LIGHT_Flip(lights_modbus + light_selectedIndex);
        }
    }
}

/**
 * @brief Obrada otpuštanja dodira na ekranu za resetovanje.
 * @note Togglovanje stanja svih svjetala povezanih s glavnim prekidačem.
 */
static void HandleRelease_ResetMenuSwitchesScreenArea(GUI_PID_STATE * pTS) {
    uint8_t isAnyLightCurrentlyOn = 0;
    for(uint8_t i_light = 0; i_light < LIGHTS_getCount(); ++i_light) {
        if(LIGHT_isTiedToMainLight(lights_modbus + i_light) && LIGHT_isNewValueOn(lights_modbus + i_light)) {
            isAnyLightCurrentlyOn = 1;
            break;
        }
    }

    bool newStateIsOn = !isAnyLightCurrentlyOn;
    for(uint8_t i_light = 0; i_light < LIGHTS_getCount(); ++i_light) {
        if(LIGHT_isTiedToMainLight(lights_modbus + i_light)) {
            if (newStateIsOn) {
                LIGHT_On(lights_modbus + i_light);
            } else {
                LIGHT_Off(lights_modbus + i_light);
            }
        }
    }

    if (LightNightTimer_isEnabled && !LightNightTimer_StartTime && !((Bcd2Dec(rtctm.Hours) > 6) && (Bcd2Dec(rtctm.Hours) < 20))) {
        if (newStateIsOn) {
            LightNightTimer_StartTime = HAL_GetTick();
            if(!LightNightTimer_StartTime) {
                LightNightTimer_StartTime = 1;
            }
        }
    } else {
        LightNightTimer_StartTime = 0;
    }

    shouldDrawScreen = 1;
    screen = SCREEN_MAIN;
}
