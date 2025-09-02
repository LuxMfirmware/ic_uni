/**
 ******************************************************************************
 * @file    display.h
 * @author  Edin & Gemini
 * @version V2.1.0
 * @date    18.08.2025.
 * @brief   Javni API za GUI Display Modul.
 *
 * @note
 * Ovaj modul je zadužen za kompletnu logiku iscrtavanja korisnickog
 * interfejsa (GUI) i obradu unosa putem ekrana osjetljivog na dodir.
 * Koristi STemWin biblioteku za kreiranje grafickih elemenata.
 *
 * Upravlja razlicitim ekranima definisanim u `eScreen` enumu i pruža
 * podršku za internacionalizaciju (i18n) putem `lng()` funkcije.
 *
 * U skladu sa novom arhitekturom, ovaj modul više ne pristupa direktno
 * internim podacima drugih modula (npr. `lights_modbus`), vec iskljucivo
 * koristi njihove javne API funkcije.
 *
 * Definicija `Display_EepromSettings_t` strukture je javna kako bi
 * drugi moduli mogli da citaju globalne postavke displeja.
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


#ifndef __DISP_H__
#define __DISP_H__                           FW_BUILD // version

/* Includes ------------------------------------------------------------------*/
#include "main.h" // Ukljucen samo 'main.h' za osnovne tipove

/*============================================================================*/
/* DEFINICIJE ZA INTERNACIONALIZACIJU (i18n)                                  */
/*============================================================================*/
#define TS_LAYER                        1       ///< Svrha: GUI "sloj" (layer) na kojem se obraduju dodiri. Vrijednost: 1 (sloj iznad pozadinskog).

/**
 * @brief Definiše sve podržane jezike u sistemu.
 * @note BSHC je skracenica za bosanski/srpski/hrvatski/crnogorski.
 * Redoslijed mora tacno odgovarati redoslijedu kolona u `language_strings`
 * i `_acContent` tabelama u `translations.h`.
 */
enum Languages { BSHC = 0, ENG, GER, FRA, ITA, SPA, RUS, UKR, POL, CZE, SLO, LANGUAGE_COUNT };

/**
 * @brief Definiše jedinstveni ID za svaki tekstualni string u aplikaciji.
 * @note  Redoslijed je logicki organizovan radi lakšeg održavanja i
 * mora se 1-na-1 poklapati sa redoslijedom u `language_strings` tabeli.
 */
typedef enum {
    TXT_DUMMY = 0,
    // --- Glavni meniji ---
    TXT_LIGHTS,
    TXT_THERMOSTAT,
    TXT_BLINDS,
    TXT_DEFROSTER,
    TXT_VENTILATOR,
    TXT_CLEAN,
    TXT_WIFI,
    TXT_APP,
    // --- Opšti tekstovi ---
    TXT_ALL,
    TXT_SETTINGS,
    // --- Poruke i dugmad ---
    TXT_DISPLAY_CLEAN_TIME,
    TXT_FIRMWARE_UPDATE,
    // --- Dani u sedmici ---
    TXT_MONDAY,
    TXT_TUESDAY,
    TXT_WEDNESDAY,
    TXT_THURSDAY,
    TXT_FRIDAY,
    TXT_SATURDAY,
    TXT_SUNDAY,
    // --- Mjeseci ---
    TXT_MONTH_JAN,
    TXT_MONTH_FEB,
    TXT_MONTH_MAR,
    TXT_MONTH_APR,
    TXT_MONTH_MAY,
    TXT_MONTH_JUN,
    TXT_MONTH_JUL,
    TXT_MONTH_AUG,
    TXT_MONTH_SEP,
    TXT_MONTH_OCT,
    TXT_MONTH_NOV,
    TXT_MONTH_DEC,
    // --- Nazivi jezika ---
    TXT_LANGUAGE_NAME,
    // --- Primarni tekstovi za ikonice (prema todo.txt) ---
    TXT_LUSTER,
    TXT_SPOT,
    TXT_VISILICA,
    TXT_PLAFONJERA,
    TXT_ZIDNA,
    TXT_SLIKA,
    TXT_PODNA,
    TXT_STOLNA,
    TXT_LED_TRAKA,
    TXT_VENTILATOR_IKONA,
    TXT_FASADA,
    TXT_STAZA,
    TXT_REFLEKTOR,
    // --- Sekundarni tekstovi za ikonice (prema todo.txt) ---
    TXT_GLAVNI_SECONDARY,
    TXT_AMBIJENT_SECONDARY,
    TXT_TRPEZARIJA_SECONDARY,
    TXT_DNEVNA_SOBA_SECONDARY,
    TXT_LIJEVI_SECONDARY,
    TXT_DESNI_SECONDARY,
    TXT_CENTRALNI_SECONDARY,
    TXT_PREDNJI_SECONDARY,
    TXT_ZADNJI_SECONDARY,
    TXT_HODNIK_SECONDARY,
    TXT_KUHINJA_SECONDARY,
    TXT_IZNAD_SANKA_SECONDARY,
    TXT_IZNAD_STola_SECONDARY,
    TXT_PORED_KREVETA_1_SECONDARY,
    TXT_PORED_KREVETA_2_SECONDARY,
    TXT_GLAVNA_SECONDARY,
    TXT_SOBA_1_SECONDARY,
    TXT_SOBA_2_SECONDARY,
    TXT_KUPATILO_SECONDARY,
    TXT_LIJEVA_SECONDARY,
    TXT_DESNA_SECONDARY,
    TXT_GORE_SECONDARY,
    TXT_DOLE_SECONDARY,
    TXT_CITANJE_SECONDARY,
    TXT_OGLEDALO_SECONDARY,
    TXT_UGAO_SECONDARY,
    TXT_PORED_FOTELJE_SECONDARY,
    TXT_RADNI_STO_SECONDARY,
    TXT_NOCNA_1_SECONDARY,
    TXT_NOCNA_2_SECONDARY,
    TXT_ISPOD_ELEMENTA_SECONDARY,
    TXT_IZNAD_ELEMENTA_SECONDARY,
    TXT_ORMAR_SECONDARY,
    TXT_STEPENICE_SECONDARY,
    TXT_TV_SECONDARY,
    TXT_ULAZ_SECONDARY,
    TXT_TERASA_SECONDARY,
    TXT_BALKON_SECONDARY,
    TXT_ZADNJA_SECONDARY,
    TXT_PRILAZ_SECONDARY,
    TXT_DVORISTE_SECONDARY,
    TXT_DRVO_SECONDARY,
    TEXT_COUNT // Uvijek na kraju!
} TextID;


/**
 * @brief Definiše jedinstveni ID za svaki VIZUELNI TIP ikonice.
 * @note  Nazivi su izvedeni iz imena bitmap varijabli u Resource.h.
 * Redoslijed u ovom enumu MORA odgovarati redoslijedu u `light_modbus_images` nizu u `display.c`.
 */
typedef enum {
    ICON_BULB = 0,                  // Vrijednost = 0
    ICON_VENTILATOR_ICON,           // Vrijednost = 1
    ICON_CEILING_LED_FIXTURE,       // Vrijednost = 2
    ICON_CHANDELIER,                // Vrijednost = 3
    ICON_HANGING,                   // Vrijednost = 4
    ICON_LED_STRIP,                 // Vrijednost = 5
    ICON_SPOT_CONSOLE,              // Vrijednost = 6
    ICON_SPOT_SINGLE,               // Vrijednost = 7
    ICON_STAIRS,                    // Vrijednost = 8
    ICON_WALL,                      // Vrijednost = 9
    // Ovdje se mogu dodati i ostale ikonice iz todo.txt kada za njih budu kreirane bitmape
    // Npr. ICON_PICTURE_LIGHT, ICON_FLOOR_LAMP, itd.
    ICON_COUNT // Ukupan broj definisanih vizuelnih tipova ikonica
} IconID;


/**
 * @brief Struktura koja mapira jedan unos iz SPINBOX-a na vizuelni prikaz.
 * @note  Ovo je kljucna struktura za novi, fleksibilni sistem prikaza ikonica.
 * Niz ovih struktura ce biti definisan u `translations.h`.
 */
typedef struct {
    IconID visual_icon_id;      /**< Koju slicicu (bitmapu) treba iscrtati. */
    TextID primary_text_id;     /**< ID za gornji (primarni) tekstualni opis. */
    TextID secondary_text_id;   /**< ID za donji (sekundarni) tekstualni opis. */
} IconMapping_t;


#pragma pack(push, 1)
/**
 * @brief  Sadrži sve EEPROM postavke vezane za displej i GUI.
 * @note   Ova struktura se cuva i cita kao jedan blok iz EEPROM-a i zašticena
 * je magicnim brojem i CRC-om radi integriteta podataka.
 */
typedef struct
{
    uint16_t magic_number;              // "Potpis" za validaciju podataka.
    uint8_t  low_bcklght;               // Vrijednost niske svjetline ekrana.
    uint8_t  high_bcklght;              // Vrijednost visoke svjetline ekrana.
    uint8_t  scrnsvr_tout;              // Timeout za screensaver u sekundama.
    uint8_t  scrnsvr_ena_hour;          // Sat kada se screensaver automatski aktivira.
    uint8_t  scrnsvr_dis_hour;          // Sat kada se screensaver automatski deaktivira.
    uint8_t  scrnsvr_clk_clr;           // Boja sata na screensaver-u.
    bool     scrnsvr_on_off;            // Fleg da li je screensaver sat ukljucen.
    bool     leave_scrnsvr_on_release;  // Fleg, ako je true, screensaver se gasi samo nakon otpuštanja dodirom.
    uint8_t  language;                  // Odabrani jezik (BOS = 0, ENG = 1, ...).
    uint8_t  selected_control_mode;     // Odabrani mod za dinamicku ikonu (Defroster/Ventilator/Off)
    bool     light_night_timer_enabled; // Fleg za nocni tajmer svjetala.
    uint16_t crc;                       // CRC za provjeru integriteta.
} Display_EepromSettings_t;
#pragma pack(pop)

/// =======================================================================
// === NOVO: Automatsko generisanje `enum`-a za ID-jeve ===
//
// ULOGA: Ovaj blok koda koristi X-Macro tehniku da automatski kreira
// jedinstveni `enum` tip koji sadrži sve ID-jeve iz `settings_widgets.def`.
// Ovo je cistiji i sigurniji nacin od gomile `#define`-ova.
//
typedef enum {
    // 1. Privremeno redefinišemo makro WIDGET da generiše `ID = VRIJEDNOST,`
    #define WIDGET(id, val, comment) id = val,
    // 2. Ukljucujemo našu glavnu listu
    #include "settings_widgets.def"
    // 3. Odmah poništavamo našu privremenu definiciju
    #undef WIDGET
} SettingsWidgetID_e;


// =======================================================================
/* Exported types ------------------------------------------------------------*/

typedef enum{
    SCREEN_RESET_MENU_SWITCHES = 0,
    SCREEN_MAIN = 1,
    SCREEN_SELECT_1,
    SCREEN_SELECT_2,
    SCREEN_THERMOSTAT,
    SCREEN_LIGHTS,
    SCREEN_CURTAINS,
    SCREEN_LIGHT_SETTINGS,
    SCREEN_QR_CODE,
    SCREEN_CLEAN,
    SCREEN_PINPAD,
    SCREEN_RETURN_TO_FIRST,
    SCREEN_SETTINGS_1,
    SCREEN_SETTINGS_2,
    SCREEN_SETTINGS_3,
    SCREEN_SETTINGS_4,
    SCREEN_SETTINGS_5,
    SCREEN_SETTINGS_6,
    SCREEN_SETTINGS_7
}eScreen;

typedef enum{
	RELEASED    = 0,
	PRESSED     = 1,
    BUTTON_SHIT = 2
}BUTTON_StateTypeDef;

// Enumeracija za opcije u DROPDOWN meniju
typedef enum {
    MODE_OFF = 0,
    MODE_DEFROSTER,
    MODE_VENTILATOR,
    MODE_COUNT
} ControlMode;

/* Exported variables  -------------------------------------------------------*/
extern uint32_t dispfl;
extern uint8_t curtain_selected;
extern uint8_t screen, shouldDrawScreen;
extern Display_EepromSettings_t g_display_settings;

/* Expoted Macro   --------------------------------------------------------- */
#define DISPUpdateSet()                     (dispfl |=  (1U<<0))
#define DISPUpdateReset()                   (dispfl &=(~(1U<<0)))
#define IsDISPUpdateActiv()                 (dispfl &   (1U<<0))
#define DISPBldrUpdSet()                    (dispfl |=  (1U<<1))
#define DISPBldrUpdReset()		            (dispfl &=(~(1U<<1)))
#define IsDISPBldrUpdSetActiv()		        (dispfl &   (1U<<1))
#define DISPBldrUpdFailSet()			    (dispfl |=  (1U<<2))
#define DISPBldrUpdFailReset()	            (dispfl &=(~(1U<<2)))
#define IsDISPBldrUpdFailActiv()	        (dispfl &   (1U<<2))
#define DISPUpdProgMsgSet()                 (dispfl |=  (1U<<3))
#define DISPUpdProgMsgDel()                 (dispfl &=(~(1U<<3)))
#define IsDISPUpdProgMsgActiv()             (dispfl &   (1U<<3))
#define DISPFwrUpd()                        (dispfl |=  (1U<<4))
#define DISPFwrUpdDelete()                  (dispfl &=(~(1U<<4)))
#define IsDISPFwrUpdActiv()                 (dispfl &   (1U<<4))
#define DISPFwrUpdFail()                    (dispfl |=  (1U<<5))
#define DISPFwrUpdFailDelete()              (dispfl &=(~(1U<<5)))
#define IsDISPFwrUpdFailActiv()             (dispfl &   (1U<<5))
#define DISPFwUpdSet()                      (dispfl |=  (1U<<6))
#define DISPFwUpdReset()                    (dispfl &=(~(1U<<6)))
#define IsDISPFwUpdActiv()                  (dispfl &   (1U<<6))
#define DISPFwUpdFailSet()                  (dispfl |=  (1U<<7))
#define DISPFwUpdFailReset()                (dispfl &=(~(1U<<7)))
#define IsDISPFwUpdFailActiv()              (dispfl &   (1U<<7))
#define PWMErrorSet()                       (dispfl |=  (1U<<8)) 
#define PWMErrorReset()                     (dispfl &=(~(1U<<8)))
#define IsPWMErrorActiv()                   (dispfl &   (1U<<8))
#define DISPKeypadSet()                     (dispfl |=  (1U<<9))
#define DISPKeypadReset()                   (dispfl &=(~(1U<<9)))
#define IsDISPKeypadActiv()                 (dispfl &   (1U<<9))
#define DISPUnlockSet()                     (dispfl |=  (1U<<10))
#define DISPUnlockReset()                   (dispfl &=(~(1U<<10)))
#define IsDISPUnlockActiv()                 (dispfl &   (1U<<10))
#define DISPLanguageSet()                   (dispfl |=  (1U<<11))
#define DISPLanguageReset()                 (dispfl &=(~(1U<<11)))
#define IsDISPLanguageActiv()               (dispfl &   (1U<<11))
#define DISPSettingsInitSet()               (dispfl |=  (1U<<12))	
#define DISPSettingsInitReset()             (dispfl &=(~(1U<<12)))
#define IsDISPSetInitActiv()                (dispfl &   (1U<<12))
#define DISPRefreshSet()					(dispfl |=  (1U<<13))
#define DISPRefreshReset()                  (dispfl &=(~(1U<<13)))
#define IsDISPRefreshActiv()			    (dispfl &   (1U<<13))
#define ScreenInitSet()                     (dispfl |=  (1U<<14))
#define ScreenInitReset()                   (dispfl &=(~(1U<<14)))
#define IsScreenInitActiv()                 (dispfl &   (1U<<14))
#define RtcTimeValidSet()                   (dispfl |=  (1U<<15))
#define RtcTimeValidReset()                 (dispfl &=(~(1U<<15)))
#define IsRtcTimeValid()                    (dispfl &   (1U<<15))
#define SPUpdateSet()                       (dispfl |=  (1U<<16)) 
#define SPUpdateReset()                     (dispfl &=(~(1U<<16)))
#define IsSPUpdateActiv()                   (dispfl &   (1U<<16))
#define ScrnsvrSet()                        (dispfl |=  (1U<<17)) 
#define ScrnsvrReset()                      (dispfl &=(~(1U<<17)))
#define IsScrnsvrActiv()                    (dispfl &   (1U<<17))
#define ScrnsvrClkSet()                     (dispfl |=  (1U<<18))
#define ScrnsvrClkReset()                   (dispfl &=(~(1U<<18)))
#define IsScrnsvrClkActiv()                 (dispfl &   (1U<<18))
#define ScrnsvrSemiClkSet()                 (dispfl |=  (1U<<19))
#define ScrnsvrSemiClkReset()               (dispfl &=(~(1U<<19)))
#define IsScrnsvrSemiClkActiv()             (dispfl &   (1U<<19))
#define MVUpdateSet()                       (dispfl |=  (1U<<20)) 
#define MVUpdateReset()                     (dispfl &=(~(1U<<20)))
#define IsMVUpdateActiv()                   (dispfl &   (1U<<20))
#define ScrnsvrEnable()                     (dispfl |=  (1U<<21)) 
#define ScrnsvrDisable()                    (dispfl &=(~(1U<<21)))
#define IsScrnsvrEnabled()                  (dispfl &   (1U<<21))
#define ScrnsvrInitSet()                    (dispfl |=  (1U<<22)) 
#define ScrnsvrInitReset()                  (dispfl &=(~(1U<<22)))
#define IsScrnsvrInitActiv()                (dispfl &   (1U<<22))
#define BTNUpdSet()                         (dispfl |=  (1U<<23))
#define BTNUpdReset()                       (dispfl &=(~(1U<<23)))
#define IsBTNUpdActiv()                     (dispfl &   (1U<<23))
#define DISPCleaningSet()                   (dispfl |=  (1U<<24))
#define DISPCleaningReset()                 (dispfl &=(~(1U<<24)))
#define IsDISPCleaningActiv()               (dispfl &   (1U<<24))

/* Exported functions  -------------------------------------------------------*/
void DISP_Init(void);
void DISP_Service(void);
void DISPSetPoint(void);
const char* lng(uint8_t t);
void DISPResetScrnsvr(void);
void DISP_UpdateLog(const char *pbuf);
void DISP_SignalDynamicIconUpdate(void);
uint8_t DISP_GetThermostatMenuState(void);
uint8_t* QR_Code_Get(const uint8_t qrCodeID);
bool QR_Code_willDataFit(const uint8_t *data);
void DISP_SetThermostatMenuState(uint8_t state);
bool QR_Code_isDataLengthShortEnough(uint8_t dataLength);
void QR_Code_Set(const uint8_t qrCodeID, const uint8_t *data);

#endif
