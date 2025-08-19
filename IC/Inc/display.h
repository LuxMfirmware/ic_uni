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

// Definiše dostupne jezike
enum Languages {BOS = 0, ENG, LANGUAGE_COUNT};

// Definiše jedinstveni ID za svaki tekstualni string u aplikaciji
typedef enum {
    TXT_DUMMY = 0,
    TXT_ALARM,
    TXT_THERMOSTAT,
    TXT_CURTAINS,
    TXT_NEXT,
    TXT_TV,
    TXT_CLEAN,
    TXT_SETTINGS,
    TXT_HOURS,
    TXT_MINUTES,
    TXT_RESET,
    TXT_ACTIVATE,
    TXT_ALARM_TIME,
    TXT_DISPLAY_CLEAN_TIME,
    TXT_ENTER_PASSWORD,
    TXT_PASSWORD_CORRECT,
    TXT_WRONG_PASSWORD,
    TXT_LANGUAGE_NAME,
    TXT_MUSIC,
    TXT_LIGHT,
    TXT_LIGHTS,      // Iscpravljena pozicija za SVJETLA
    TXT_BLINDS,      // Ispravljena pozicija za ROLETNE
    TXT_BED,
    TXT_HALLWAY,
    TXT_WC,
    TXT_TERRACE,
    TXT_KITCHEN,
    TXT_STAIRS,
    TXT_LIVING_R_1,
    TXT_LIVING_R_2,
    TXT_LIVING_R_3,
    TXT_TERR_L,
    TXT_TERR_R,
    TXT_SIDE_WIN,
    TXT_WINDOWS,
    TXT_FACADE,
    TXT_BEDROOM,
    TXT_BEDROOM_1,
    TXT_BEDROOM_2,
    TXT_TERRACE_1,
    TXT_TERRACE_2,
    TXT_LIVING_ROOM_1,
    TXT_LIVING_ROOM_2,
    TXT_POOL_1,
    TXT_POOL_2,
    TXT_POOL_3,
    TXT_LEFT,
    TXT_MIDDLE,
    TXT_RIGHT,
    TXT_LIVING,
    TXT_ALL,
    TXT_WIFI,
    TXT_APP,
    TXT_DEFROSTER,
    TXT_SAVE,
    TXT_FIRMWARE_UPDATE,
    TXT_VENTILATOR,
    TEXT_COUNT
} TextID;


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
