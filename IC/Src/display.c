/**
 ******************************************************************************
 * Project          : KuceDzevadova
 ******************************************************************************
 *
 *
 ******************************************************************************
 */


#if (__DISPH__ != FW_BUILD)
#error "display header version mismatch"
#endif
/* Include  ------------------------------------------------------------------*/
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
/* Private Define ------------------------------------------------------------*/

typedef struct
{
    SPINBOX_Handle  relay, offTime, iconID, controllerID_on, controllerID_on_delay, on_hour, on_minute, communication_type, local_pin, sleep_time, button_external;
    CHECKBOX_Handle  tiedToMainLight, rememberBrightness;
}
Light_Modbus_settingsWidgets;

#define GUI_REFRESH_TIME                100U    // refresh gui 10 time in second
#define DATE_TIME_REFRESH_TIME          1000U   // refresh date & time info every 1 sec. 
#define SETTINGS_MENU_ENABLE_TIME       3456U   // press and holde upper left corrner for period to enter setup menu
#define WFC_TOUT                        8765U   // 9 sec. weather display timeout   
#define BUTTON_RESPONSE_TIME            1234U   // button response delay till all onewire device update state
#define DISPIMG_TIME_MULTI              30000U  // 30 sec. min time increment for image display time * 255 max
#define SETTINGS_MENU_TIMEOUT           59000U  // 1 min. settings menu timeout
#define WFC_CHECK_TOUT                  (SECONDS_PER_HOUR * 1000U) // check weather forecast data validity every hour
#define KEYPAD_SIGNAL_TIME              3000
#define KEYPAD_UNLOCK_TIME              30000
#define DISPMSG_TIME                    45U     // time to display message on event * GUI_REFRESH_TIME
#define EVENT_ONOFF_TOUT                500    // on/off event touch max. time 1s  
#define VALUE_STEP_TOUT                 15      // light value chage time
#define DISP_BRGHT_MAX                  80
#define DISP_BRGHT_MIN                  5

#define LIGHTS_MODBUS_PER_SETTINGS      1
#define LIGHTS_MODBUS_PER_ROW           3

#define VENTILATOR_ON_TIMER_DURATION    15

#define QR_CODE_COUNT                   2
#define QR_CODE_LENGTH                  50

#define CLR_DARK_BLUE                   GUI_MAKE_COLOR(0x613600)
#define CLR_LIGHT_BLUE                  GUI_MAKE_COLOR(0xaa7d67)
#define CLR_BLUE                        GUI_MAKE_COLOR(0x855a41)
#define CLR_LEMON                       GUI_MAKE_COLOR(0x00d6d3)

#define BTN_SETTINGS_X0                 0
#define BTN_SETTINGS_Y0                 0
#define BTN_SETTINGS_X1                 100
#define BTN_SETTINGS_Y1                 100

#define BTN_DEC_X0                      0
#define BTN_DEC_Y0                      90
#define BTN_DEC_X1                      (BTN_DEC_X0 + 120)
#define BTN_DEC_Y1                      (BTN_DEC_Y0 + 179)

#define BTN_INC_X0                      200
#define BTN_INC_Y0                      90
#define BTN_INC_X1                      (BTN_INC_X0 + 120)
#define BTN_INC_Y1                      (BTN_INC_Y0 + 179)

#define BTN_OK_X0                       269
#define BTN_OK_Y0                       135
#define BTN_OK_X1                       (BTN_OK_X0  + 200)
#define BTN_OK_Y1                       (BTN_OK_Y0  + 134)

#define SP_H_POS                        200
#define SP_V_POS                        150
#define CLOCK_H_POS                     240
#define CLOCK_V_POS                     136

#define ID_Ok                           0x803
#define ID_Next                         0x805

#define ID_AmbientNtcOffset             0x830
#define ID_MaxSetpoint                  0x831
#define ID_MinSetpoint                  0x832
#define ID_DisplayHighBrightness        0x833
#define ID_DisplayLowBrightness         0x834
#define ID_ScrnsvrTimeout               0x835
#define ID_ScrnsvrEnableHour            0x836
#define ID_ScrnsvrDisableHour           0x837
#define ID_ScrnsvrClkColour             0x838
#define ID_ScrnsvrLogoClockColour       0x839
#define ID_Hour                         0x83A
#define ID_Minute                       0x83B
#define ID_Day                          0x83C
#define ID_Month                        0x83D
#define ID_Year                         0x83E
#define ID_WeekDay                      0x83F

#define ID_Scrnsvr                      0x850
#define ID_ScrnsvrClock                 0x851
#define ID_ScrnsvrLogoClock             0x852
#define ID_RTcfg                        0x853
#define ID_RCcfg                        0x854

#define ID_ThstControl                  0x860
#define ID_FanControl                   0x861
#define ID_ThstMaxSetPoint              0x862
#define ID_ThstMinSetPoint              0x863
#define ID_FanDiff                      0x864
#define ID_FanLowBand                   0x865
#define ID_FanHiBand                    0x866

#define ID_DEV_ID                       0x870
#define ID_DIM_LIGHT1                   0x871
#define ID_DIM_LIGHT2                   0x872
#define ID_DIM_LIGHT3                   0x873
#define ID_BIN_MAIN1                    0x874
#define ID_BIN_LED1                     0x875
#define ID_BIN_LED2                     0x876
#define ID_BIN_LED3                     0x877
#define ID_BIN_OUT1                     0x878

#define ID2_DIM_LIGHT1                  0x880
#define ID2_DIM_LIGHT2                  0x881
#define ID2_DIM_LIGHT3                  0x882
#define ID2_BIN_MAIN1                   0x883
#define ID2_BIN_LED1                    0x884
#define ID2_BIN_LED2                    0x885
#define ID2_BIN_LED3                    0x886
#define ID2_BIN_OUT1                    0x887

#define ID_ThstRelay1                   0x888
#define ID_ThstRelay2                   0x889
#define ID_ThstRelay3                   0x88A
#define ID_ThstRelay4                   0x88B
#define ID_ThstRelay3Delay              0x88C
#define ID_ThstRelay4Delay              0x88D
#define ID_NTC_Offset                   0x88E

#define ID_VentilatorRelay              0x88F
#define ID_VentilatorDelayOn            0x890
#define ID_VentilatorDelayOff           0x891
#define ID_VentilatorUseDelayOn         0x892
#define ID_VentilatorUseDelayOff        0x893

#define ID_CurtainsRelay                0x894  //holds 30 bytes for 15 curtains
#define ID_CurtainsMoveTime             0x8B2

#define ID_LightsModbusRelay            0x8B3  //holds 195 bytes for 15 lights

#define ID_SYSRESTART                   0x976

#define ID_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH       0x977
#define ID_LIGHT_NIGHT_TIMER            0x978
#define ID_THST_GROUP                   0x979
#define ID_THST_MASTER                  0x97A

#define ID_DEFROSTER_CYCLE_TIME         0x97B
#define ID_DEFROSTER_ACTIVE_TIME        0x97C
#define ID_DEFROSTER_PIN                0x97D

#define ID_SET_DEFAULTS                 0x97E


/* Private Type --------------------------------------------------------------*/
BUTTON_Handle   hBUTTON_Increase;
BUTTON_Handle   hBUTTON_Decrease;
BUTTON_Handle   hBUTTON_Ok;
BUTTON_Handle   hBUTTON_Next;

SPINBOX_Handle  hSPNBX_MaxSetpoint;                         //  set thermostat user maximum setpoint value
SPINBOX_Handle  hSPNBX_MinSetpoint;                         //  set thermostat user minimum setpoint value

RADIO_Handle    hThstControl;
RADIO_Handle    hFanControl;
SPINBOX_Handle  hThstMaxSetPoint;
SPINBOX_Handle  hThstMinSetPoint;
SPINBOX_Handle  hFanDiff;
SPINBOX_Handle  hFanLowBand;
SPINBOX_Handle  hFanHiBand;
SPINBOX_Handle  hThstGroup;
CHECKBOX_Handle hThstMaster;


SPINBOX_Handle  hDEV_ID;
SPINBOX_Handle  hDIM_LIGHT1;
SPINBOX_Handle  hDIM_LIGHT2;
SPINBOX_Handle  hDIM_LIGHT3;
SPINBOX_Handle  hBIN_MAIN1;
SPINBOX_Handle  hBIN_LED1;
SPINBOX_Handle  hBIN_LED2;
SPINBOX_Handle  hBIN_LED3;
SPINBOX_Handle  hBIN_OUT1;

SPINBOX_Handle  hDIM2_LIGHT1;
SPINBOX_Handle  hDIM2_LIGHT2;
SPINBOX_Handle  hDIM2_LIGHT3;
SPINBOX_Handle  hBIN2_MAIN1;
SPINBOX_Handle  hBIN2_LED1;
SPINBOX_Handle  hBIN2_LED2;
SPINBOX_Handle  hBIN2_LED3;
SPINBOX_Handle  hBIN2_OUT1;

SPINBOX_Handle  hSPNBX_DisplayHighBrightness;               //  lcd display backlight led brightness level for activ user interface (high level)
SPINBOX_Handle  hSPNBX_DisplayLowBrightness;                //  lcd display backlight led brightness level for activ screensaver (low level)
SPINBOX_Handle  hSPNBX_ScrnsvrTimeout;                      //  start screensaver (value x 10 s) after last touch event or disable screensaver for 0
SPINBOX_Handle  hSPNBX_ScrnsvrEnableHour;                   //  when to start display big digital clock for unused thermostat display
SPINBOX_Handle  hSPNBX_ScrnsvrDisableHour;                  //  when to stop display big digital clock but to just dimm thermostat user interfaca
SPINBOX_Handle  hSPNBX_ScrnsvrClockColour;                  //  set colour for full display screensaver digital clock digits
SPINBOX_Handle  hSPNBX_Hour;
SPINBOX_Handle  hSPNBX_Minute;
SPINBOX_Handle  hSPNBX_Day;
SPINBOX_Handle  hSPNBX_Month;
SPINBOX_Handle  hSPNBX_Year;
CHECKBOX_Handle hCHKBX_ScrnsvrClock;
DROPDOWN_Handle hDRPDN_WeekDay;

SPINBOX_Handle  hVentilatorRelay;
SPINBOX_Handle  hVentilatorDelayOn;
SPINBOX_Handle  hVentilatorDelayOff;
CHECKBOX_Handle hVentilatorUseDelayOn;
CHECKBOX_Handle hVentilatorUseDelayOff;

SPINBOX_Handle  hCurtainsRelay[CURTAINS_SIZE * 2];
SPINBOX_Handle  hCurtainsMoveTime;

SPINBOX_Handle  hLightsModbusRelay[LIGHTS_MODBUS_SIZE];
SPINBOX_Handle  hLightsModbusOffTime[LIGHTS_MODBUS_SIZE];
CHECKBOX_Handle  hLightsModbusTiedToMainLight[LIGHTS_MODBUS_SIZE];
Light_Modbus_settingsWidgets lightsWidgets[LIGHTS_MODBUS_SIZE];

BUTTON_Handle hBUTTON_SET_DEFAULTS;
BUTTON_Handle hBUTTON_SYSRESTART;

CHECKBOX_Handle hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH;
CHECKBOX_Handle hCHKBX_LIGHT_NIGHT_TIMER;

Defroster_settingsWidgets defroster_settingWidgets;


static uint32_t clk_clrs[COLOR_BSIZE] = {                   //  selectable screensaver clock colours
    GUI_GRAY, GUI_RED, GUI_BLUE, GUI_GREEN, GUI_CYAN, GUI_MAGENTA,
    GUI_YELLOW, GUI_LIGHTGRAY, GUI_LIGHTRED, GUI_LIGHTBLUE, GUI_LIGHTGREEN,
    GUI_LIGHTCYAN, GUI_LIGHTMAGENTA, GUI_LIGHTYELLOW, GUI_DARKGRAY, GUI_DARKRED,
    GUI_DARKBLUE,  GUI_DARKGREEN, GUI_DARKCYAN, GUI_DARKMAGENTA, GUI_DARKYELLOW,
    GUI_WHITE, GUI_BROWN, GUI_ORANGE, CLR_DARK_BLUE, CLR_LIGHT_BLUE, CLR_BLUE, CLR_LEMON
};


static const char * _acContent[] = {
    "PON",
    "UTO",
    "SRI",
    "CET",
    "PET",
    "SUB",
    "NED"
};
LIGHT_CtrlTypeDef LIGHT_Ctrl1;
LIGHT_CtrlTypeDef LIGHT_Ctrl2;
/* Private Macro   --------------------------------------------------------- */
/* Private Variable ----------------------------------------------------------*/
uint32_t rtctmr, dispfl;
uint32_t clean_tmr = 0;
uint32_t lightInitRequestTime = 0;
uint32_t thermostatOnOffTouch_timer = 0;
uint8_t btn_ok_state, btnset = 0, old_min=60;
uint8_t low_bcklght, high_bcklght, light_ldr;
uint8_t btn_increase_state, btn_increase_old_state;
uint8_t btn_decrease_state, btn_decrease_old_state;
uint8_t scrnsvr_ena_hour, scrnsvr_dis_hour, disp_rot;
uint8_t scrnsvr_clk_clr, scrnsvr_semiclk_clr, scrnsvr_tout;
uint8_t menu_clean = 0, clrtmr = 0;
uint8_t lightInitRequestSend = 0;
uint8_t t = 0;
char logbuf[128];
static uint32_t scrnsvr_tmr;
static uint8_t menu_dim, menu_rel123, menu_out1;
uint8_t menu_thst, screen, ctrl1, ctrl2, ctrl3;
static uint8_t btninc, _btninc, btndec, _btndec, menu_lc;
uint8_t settingsChanged = 0;
uint8_t curtainSettingMenu = 0, curtainMenu = 0, curtainMenuCurtainCount = 1, curtainMenuStartIndex = 0, curtainMenuCurtainLength = 120, curtainMenuSpaceBetween = 0, curtain_selected = 0;
uint32_t light_settingsTimerStart = 0;
uint8_t lightsModbusSettingsMenu = 0, light_selectedIndex = LIGHTS_MODBUS_SIZE + 1, lights_allSelected_hasRGB = 0, lightsMenuSpaceBetween = (400 - (80 * LIGHTS_MODBUS_PER_ROW)) / (LIGHTS_MODBUS_PER_ROW - 1 + 2);
uint8_t shouldDrawScreen = 1;
uint8_t bOnlyLeaveScreenSaverAfterTouch = 0;
uint32_t ventilatorOnDelayTimer_Start = 0;
uint32_t everyMinuteTimerStart = 0;

uint8_t qr_codes[QR_CODE_COUNT][QR_CODE_LENGTH] = {0}, qr_code_draw_id = 0;

enum Languages {BOS = 0, ENG} language = ENG;


#define LANGUAGES_NUM 2
char langText[40];

void lng(uint8_t t)
{
    switch(language)
    {
    case ENG:  // english
    {
        switch(t)
        {
        case 1:
            strcpy(langText, "ALARM");
            break;
        case 2:
            strcpy(langText, "THERMOSTAT");
            break;
        case 3:
            strcpy(langText, "CURTAINS");
            break;
        case 4:
            strcpy(langText, "NEXT");
            break;
        case 5:
            strcpy(langText, "TV");
            break;
        case 6:
            strcpy(langText, "CLEAN");
            break;
        case 7:
            strcpy(langText, "SETTINGS");
            break;
        case 8:
            strcpy(langText, "Hours");
            break;
        case 9:
            strcpy(langText, "Minutes");
            break;
        case 10:
            strcpy(langText, "RESET");
            break;
        case 11:
            strcpy(langText, "ACTIVATE");
            break;
        case 12:
            strcpy(langText, "ALARM TIME");
            break;
        case 13:
            strcpy(langText, "DISPLAY CLEAN TIME:");
            break;
        case 14:
            strcpy(langText, "ENTER PASSWORD");
            break;
        case 15:
            strcpy(langText, "PASSWORD CORRECT");
            break;
        case 16:
            strcpy(langText, "WRONG PASSWORD");
            break;
        case 17:
            strcpy(langText, "ENG");
            break;
        case 18:
            strcpy(langText, "MUSIC");
            break;
        case 19:
            strcpy(langText, "LIGHT");
            break;
        case 20:
            strcpy(langText, "LIGHTS");
            break;
        case 21:
            strcpy(langText, "BLINDS");
            break;
        case 22:
            strcpy(langText, "BED");
            break;
        case 23:
            strcpy(langText, "HALLWAY");
            break;
        case 24:
            strcpy(langText, "WC");
            break;
        case 25:
            strcpy(langText, "TERRACE");
            break;
        case 26:
            strcpy(langText, "KITCHEN");
            break;
        case 27:
            strcpy(langText, "STAIRS");
            break;
        case 28:
            strcpy(langText, "LIVING R. 1");
            break;
        case 29:
            strcpy(langText, "LIVING R. 2");
            break;
        case 30:
            strcpy(langText, "LIVING R. 3");
            break;
        case 31:
            strcpy(langText, "TERR. L.");
            break;
        case 32:
            strcpy(langText, "TERR. R.");
            break;
        case 33:
            strcpy(langText, "SIDE WIN.");
            break;
        case 34:
            strcpy(langText, "WINDOWS");
            break;
        case 35:
            strcpy(langText, "FACADE");
            break;
        case 36:
            strcpy(langText, "BEDROOM");
            break;
        case 37:
            strcpy(langText, "BEDROOM 1");
            break;
        case 38:
            strcpy(langText, "BEDROOM 2");
            break;
        case 39:
            strcpy(langText, "TERRACE 1");
            break;
        case 40:
            strcpy(langText, "TERRACE 2");
            break;
        case 41:
            strcpy(langText, "LIVING\nROOM 1");
            break;
        case 42:
            strcpy(langText, "LIVING\nROOM 2");
            break;
        case 43:
            strcpy(langText, "POOL 1");
            break;
        case 44:
            strcpy(langText, "POOL 2");
            break;
        case 45:
            strcpy(langText, "POOL 3");
            break;
        case 46:
            strcpy(langText, "LEFT");
            break;
        case 47:
            strcpy(langText, "MIDDLE");
            break;
        case 48:
            strcpy(langText, "RIGHT");
            break;
        case 49:
            strcpy(langText, "LIVING ");
            break;
        case 50:
            strcpy(langText, "ALL");
            break;
        case 51:
            strcpy(langText, "Wi-Fi");
            break;
        case 52:
            strcpy(langText, "APP");
            break;
        case 53:
            strcpy(langText, "DEFROSTER");
            break;
        }
        break;
    }
    case BOS:  // BiH
    {
        switch(t)
        {
        case 1:
            strcpy(langText, "ALARM");
            break;
        case 2:
            strcpy(langText, "TERMOSTAT");
            break;
        case 3:
            strcpy(langText, "ZAVJESE");
            break;
        case 4:
            strcpy(langText, "SLJEDECE");
            break;
        case 5:
            strcpy(langText, "TV");
            break;
        case 6:
            strcpy(langText, "CISCENJE");
            break;
        case 7:
            strcpy(langText, "POSTAVKE");
            break;
        case 8:
            strcpy(langText, "Sati");
            break;
        case 9:
            strcpy(langText, "Minute");
            break;
        case 10:
            strcpy(langText, "PONISTI");
            break;
        case 11:
            strcpy(langText, "AKTIVIRAJ");
            break;
        case 12:
            strcpy(langText, "VRIJEME ALARMA");
            break;
        case 13:
            strcpy(langText, "VRIJEME BRISANJA EKRANA:");
            break;
        case 14:
            strcpy(langText, "UNESI SIFRU");
            break;
        case 15:
            strcpy(langText, "SIFRA TACNA");
            break;
        case 16:
            strcpy(langText, "POGRESNA SIFRA");
            break;
        case 17:
            strcpy(langText, "BOS");
            break;
        case 18:
            strcpy(langText, "MUZIKA");
            break;
        case 19:
            strcpy(langText, "SVJETLO");
            break;
        case 20:
            strcpy(langText, "SVJETLA");
            break;
        case 21:
            strcpy(langText, "ROLETNE");
            break;
        case 22:
            strcpy(langText, "SPAVACA");
            break;
        case 23:
            strcpy(langText, "HODNIK");
            break;
        case 24:
            strcpy(langText, "WC");
            break;
        case 25:
            strcpy(langText, "TERASA");
            break;
        case 26:
            strcpy(langText, "KUHINJA");
            break;
        case 27:
            strcpy(langText, "STEP.");
            break;  //stepenice
        case 28:
            strcpy(langText, "DNEVNI B. 1");
            break;
        case 29:
            strcpy(langText, "DNEVNI B. 2");
            break;
        case 30:
            strcpy(langText, "DNEVNI B. 3");
            break;
        case 31:
            strcpy(langText, "TER. L.");
            break;
        case 32:
            strcpy(langText, "TER. R.");
            break;
        case 33:
            strcpy(langText, "BOČ. PRO.");
            break;
        case 34:
            strcpy(langText, "PROZORI");
            break;
        case 35:
            strcpy(langText, "FASADA");
            break;
        case 36:
            strcpy(langText, "SPAVACA");
            break;
        case 37:
            strcpy(langText, "SPAVACA 1");
            break;
        case 38:
            strcpy(langText, "SPAVACA 2");
            break;
        case 39:
            strcpy(langText, "TERASA 1");
            break;
        case 40:
            strcpy(langText, "TERASA 2");
            break;
        case 41:
            strcpy(langText, "LIVING\nROOM 1");
            break;
        case 42:
            strcpy(langText, "LIVING\nROOM 2");
            break;
        case 43:
            strcpy(langText, "BAZEN 1");
            break;
        case 44:
            strcpy(langText, "BAZEN 2");
            break;
        case 45:
            strcpy(langText, "BAZEN 3");
            break;
        case 46:
            strcpy(langText, "LIJEVE");
            break;
        case 47:
            strcpy(langText, "SREDNJE");
            break;
        case 48:
            strcpy(langText, "DESNE");
            break;
        case 49:
            strcpy(langText, "DNEVNI ");
            break;
        case 50:
            strcpy(langText, "SVE");
            break;
        case 51:
            strcpy(langText, "Wi-Fi");
            break;
        case 52:
            strcpy(langText, "APP");
            break;
        case 53:
            strcpy(langText, "ODMRZIVAC");
            break;
        }
        break;
    }
        /*case MONT:  // crnogorski
        {
            switch(t)
            {
                case 1: strcpy(langText, "ALARM"); break;
                case 2: strcpy(langText, "KLIMA / GRIJANJE"); break;
                case 3: strcpy(langText, "ZAVJESE"); break;
                case 4: strcpy(langText, "SLJEDECE"); break;
                case 5: strcpy(langText, "TV"); break;
                case 6: strcpy(langText, "CISCENJE"); break;
                case 7: strcpy(langText, "POSTAVKE"); break;
                case 8: strcpy(langText, "Sati"); break;
                case 9: strcpy(langText, "Minute"); break;
                case 10: strcpy(langText, "PONISTI"); break;
                case 11: strcpy(langText, "AKTIVIRAJ"); break;
                case 12: strcpy(langText, "VRIJEME ALARMA"); break;
                case 13: strcpy(langText, "VRIJEME BRISANJA EKRANA:"); break;
                case 14: strcpy(langText, "UNESI SIFRU"); break;
                case 15: strcpy(langText, "SIFRA TACNA"); break;
                case 16: strcpy(langText, "POGRESNA SIFRA"); break;
                case 17: strcpy(langText, "CRNOGORSKI"); break;
                case 18: strcpy(langText, "MUZIKA"); break;
                case 19: strcpy(langText, "SVJETLO"); break;
            }
            break;
        }*/
        /*case GER:
        {
            switch(t)
            {
                case 1: strcpy(langText, "ALARM"); break;
                case 2: strcpy(langText, "KLIMA"); break;
                case 3: strcpy(langText, "VORHAGE"); break;
                case 4: strcpy(langText, "NACHSTE"); break;
                case 5: strcpy(langText, "TV"); break;
                case 6: strcpy(langText, "SAUBER"); break;
                case 7: strcpy(langText, "EINSTELLUNGEN"); break;
                case 8: strcpy(langText, "Std"); break;
                case 9: strcpy(langText, "Protokoll"); break;
                case 10: strcpy(langText, "ZURUCKSETZEN"); break;
                case 11: strcpy(langText, "AKTIVIEREN"); break;
                case 12: strcpy(langText, "WECKZEIT"); break;
                case 13: strcpy(langText, "REINIGUNGSZEIT ANZEIGEN:"); break;  // upitno
                case 14: strcpy(langText, "PASSWORT EINGEBEN"); break;
                case 15: strcpy(langText, "PASSWORT RICHTIG"); break;
                case 16: strcpy(langText, "FALSCHES PASSWORT"); break;
                case 17: strcpy(langText, "GER"); break;
                case 18: strcpy(langText, "MUSIK"); break;
                case 19: strcpy(langText, "BELEUCHTUNG"); break;
                case 20: strcpy(langText, "VENTILATOR"); break;
                case 21: strcpy(langText, "WECKER"); break;
                case 22: strcpy(langText, "HEIMAT"); break;
            }

            break;
        }*/
    }

}



uint8_t readPins()
{
    uint8_t pin = 0, retPins = 0;

    for(unsigned short int i = 1U; i < 8; i++)
    {
        switch(i)
        {
        /*case 1:
            pin = Pin3IsSet();
            break;

        case 2:
            pin = Pin4IsSet();
            break;

        case 3:
            pin = Pin5IsSet();
            break;

        case 4:
            pin = Pin6IsSet();
            break;

        case 5:
            pin = Pin7IsSet();
            break;

        case 6:
            pin = Pin8IsSet();
            break;

        default:
            pin = 0;
            break;*/

        case 1:
            pin = IsLight1Active();
            break;

        case 2:
            pin = IsLight2Active();
            break;

        case 3:
            pin = IsLight3Active();
            break;

        case 4:
            pin = IsBuzzerActiv();
            break;

        case 5:
            pin = IsLight5Active();
            break;

        case 6:
            pin = IsLight6Active();
            break;

        default:
            pin = 0;
            break;
        }

        retPins = ((retPins | pin) << 1U);
    }

    pin = 0;  //Pin8IsSet();

    retPins = (retPins | pin);

    return retPins;
}

bool ReadPin(const uint8_t pin)
{
    switch(pin)
    {
    case 1:
        return IsLight1Active();
    //break;

    case 2:
    {
        return IsLight2Active();
        //break;
    }

    case 3:
        return IsLight3Active();
    //break;

    case 4:
        return IsLight4Active();
    //break;

    case 5:
        return IsLight5Active();
    //break;

    case 6:
        return IsLight6Active();
    //break;

    default:
        break;
    }

    return false;
}

void SetPin(uint8_t pin, uint8_t pinVal)
{
    switch(pin)
    {
    case 1:
        if(pinVal) Light1On();
        else Light1Off();
        break;

    case 2:
    {
        if(pinVal) Light2On();
        else Light2Off();
        /*if(pinVal) t = 1; else t = 0;
        screen = 7;*/

        break;
    }

    case 3:
        if(pinVal) Light3On();
        else Light3Off();
        break;

    case 4:
        if(pinVal) Light4On();
        else Light4Off();
        break;

    case 5:
        if(pinVal) Light5On();
        else Light5Off();
        break;

    case 6:
        if(pinVal) Light6On();
        else Light6Off();
        break;

    default:
        break;
    }
}

/* Private Macro -------------------------------------------------------------*/
/* Private Function Prototype ------------------------------------------------*/
static void DISPDateTime(void);
static void DSP_InitSet1Scrn(void);
static void DSP_InitSet2Scrn(void);
static void DSP_InitSet3Scrn(void);
static void DSP_InitSet4Scrn(void);
static void DSP_InitSet5Scrn(void);
static void DSP_InitSet6Scrn(void);
static void DSP_InitSet7Scrn(void);
static void DSP_InitSet8Scrn(void);
static void DSP_KillSet1Scrn(void);
static void DSP_KillSet2Scrn(void);
static void DSP_KillSet3Scrn(void);
static void DSP_KillSet4Scrn(void);
static void DSP_KillSet5Scrn(void);
static void DSP_KillSet6Scrn(void);
static void DSP_KillSet7Scrn(void);
static void DSP_KillSet8Scrn(void);
static uint8_t DISPMenuSettings(uint8_t btn);
static void SaveLightController(LIGHT_CtrlTypeDef* lc, uint16_t addr);
static void ReadLightController(LIGHT_CtrlTypeDef* lc, uint16_t addr);
void Curtain_Select(const uint8_t curtain);
uint8_t Curtain_getSelected(void);
bool Curtain_areAllSelected(void);
void Curtain_ResetSelection(void);
void DrawEquilateralTriangle(const int16_t x, const int16_t y, const int16_t length, double rotation, const GUI_COLOR color, const GUI_COLOR fillColor);
/* Program Code  -------------------------------------------------------------*/
/**
  * @brief
  * @param
  * @retval
  */
void DISP_Init(void)
{
    uint8_t len;

    GUI_Init();
    GUI_PID_SetHook(PID_Hook);
    WM_MULTIBUF_Enable(1);
    GUI_UC_SetEncodeUTF8();
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_Exec();

    ReadLightController(&LIGHT_Ctrl1, EE_CTRL1);
    ReadLightController(&LIGHT_Ctrl2, EE_CTRL2);
//    Ventilator_Init(&ventilator, EE_VENTILATOR);
    EE_ReadBuffer(&bOnlyLeaveScreenSaverAfterTouch, EE_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, 1);
    EE_ReadBuffer(&len, EE_QR_CODE1, 1); // uzmi dužinu qr koda
    EE_ReadBuffer(&qr_codes[0][0], EE_QR_CODE1+1, len); // učitaj qr kod
    EE_ReadBuffer(&len, EE_QR_CODE2, 1);
    EE_ReadBuffer(&qr_codes[1][0], EE_QR_CODE2+1, len);
    everyMinuteTimerStart = HAL_GetTick();
}
/**
  * @brief
  * @param
  * @retval
  */
void DISP_Service(void)
{
    char dbuf[8];
    uint8_t ebuf[8];
    static int i = 0, s = 0, r = 0;
    static uint32_t onoff_tmr = 0, dimm_tmr = 0, dir = 0;
    static uint32_t value_step_tmr = 0;
    static uint32_t out1_tmr = 0;
    static uint32_t refresh_tmr = 0, guitmr = 0;
    static uint8_t c1= 0, c2= 0, c3= 0;
    static uint8_t thsta = 0, lcsta = 0, fwmsg = 2;
    //
    //  GUI REDRAW AND TOUCH SCREEN REFRESH
    //
    if ((HAL_GetTick() - guitmr) >= 100) {
        guitmr = HAL_GetTick();
        GUI_Exec();
    }

    if (IsFwUpdateActiv()) {
        if (!fwmsg) {
            fwmsg = 1;
            GUI_MULTIBUF_BeginEx(1);
            GUI_Clear();
            GUI_SetFont(GUI_FONT_24B_1);
            GUI_SetColor(GUI_ORANGE);
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetTextAlign(GUI_TA_HCENTER|GUI_TA_VCENTER);
            GUI_GotoXY(240, 135);
            GUI_DispString("FIRMWARE UPDATE RUN\n\rPLEASE WAIT");
            GUI_MULTIBUF_EndEx(1);
            DISPResetScrnsvr();
        }
        return;
    } else if(fwmsg == 1) {
        fwmsg = 0;
        scrnsvr_tmr = 0;
    } else if(fwmsg == 2) {
        fwmsg = 0;
        GUI_MULTIBUF_BeginEx(1);
        GUI_Clear();
        GUI_SetPenSize(9);
        GUI_SetColor(GUI_RED);
        GUI_DrawEllipse(240, 136, 50, 50);
        GUI_DrawLine(400,20,450,20);
        GUI_DrawLine(400,40,450,40);
        GUI_DrawLine(400,60,450,60);
        GUI_MULTIBUF_EndEx(1);
    }

    switch(screen) {
        //
        //  MAIN LIGHT CONTROL ON/OFF/DIMM
        //
        case SCREEN_MAIN:
        {
            //
            //  TOUCH EVENT TIME SHORT(ON/OFF)/LONG(DIMM)
            //
            switch(i) {
            //
            //  FIRST TOUCH EVENT IS ON/OFF
            //
            case 0:
            {
                t = 1;

                for(uint8_t i = 0; i < Lights_Modbus_getCount(); i++)
                {
                    if(Light_Modbus_isTiedToMainLight(lights_modbus + i)  &&  Light_Modbus_isNewValueOn(lights_modbus + i))
                    {
                        t = 0;
                        break;
                    }
                }

                //
                //  MAIN LIGHT CONTROL STATE ON/OFF
                //
                switch(t)
                {
                //
                //  MAIN LIGHT SWITCH ON POSSITION
                //
                case 0:
                {
                    i = 1; // START TOUCH PRESSED EVENT TIMER
    //                            t = 1; // CHECK TOUCH EVENT FIRST
    //                            Light1On();
    //                            LIGHT_Ctrl1.Main1.value = 1;
                    GUI_MULTIBUF_BeginEx(1);
                    GUI_Clear();
                    GUI_SetPenSize(9);
                    GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);
                    GUI_DrawLine(400,20,450,20);
                    GUI_DrawLine(400,40,450,40);
                    GUI_DrawLine(400,60,450,60);
                    GUI_SetColor(GUI_GREEN);
                    GUI_DrawEllipse(240,136,50,50);
                    GUI_MULTIBUF_EndEx(1);
                    onoff_tmr = HAL_GetTick();
                    break;
                }
                //
                //  MAIN LIGHT SWITCH OFF POSITION
                //
                case 1:
                default:
                {
                    i = 4; // DO NOT ENTER HERE UNTIL TOUCH SCREEN UNPRESSED EVENT
    //                            t = 0; // CHECK TOUCH EVENT FIRST
    //                            Light1Off();
    //                            LIGHT_Ctrl1.Main1.value = 0;
                    GUI_MULTIBUF_BeginEx(1);
                    GUI_Clear();
                    GUI_SetPenSize(9);
                    GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);
                    GUI_DrawLine(400,20,450,20);
                    GUI_DrawLine(400,40,450,40);
                    GUI_DrawLine(400,60,450,60);
                    GUI_SetColor(GUI_RED);
                    GUI_DrawEllipse(240,136,50,50);
                    GUI_MULTIBUF_EndEx(1);
                    onoff_tmr = HAL_GetTick();
                    break;
                }
                }

                i = 0;
                screen = SCREEN_RESET_MENU_SWITCHES;
                break;

                if (r) {
                    screen = SCREEN_RESET_MENU_SWITCHES;
                    r = 0; // SKEEP FOR MENU TIMEOUT CALL
                } else {
                    if      (t == 0) {
                        LIGHT_Ctrl1.Led1.value = 0;
                        LIGHT_Ctrl1.Led2.value = 0;
                        LIGHT_Ctrl1.Led3.value = 0;
                        LIGHT_Ctrl1.Light1.value = 0;
                        LIGHT_Ctrl1.Light2.value = 0;
                        LIGHT_Ctrl1.Light3.value = 0;
                    } else if (t == 1) {
                        LIGHT_Ctrl1.Light1.value = 0xFF;
                        LIGHT_Ctrl1.Light2.value = 0xFF;
                        LIGHT_Ctrl1.Light3.value = 0xFF/3;
                    }
                    ScrnsvrReset();
                    DISPSetBrightnes(DISP_BRGHT_MAX);
                }
                break;
            }
            //
            //  TOUCH EVENT TIME TRESHOLD
            //
            case 1:
            {
                if ((HAL_GetTick() -  onoff_tmr) >= EVENT_ONOFF_TOUT) {
                    ++i;    // touch event last over 1s, switch to light increase/decrease state
                    value_step_tmr = HAL_GetTick(); // load change step timer
                }
                break;
            }
            //
            //  LONG TOUCH IS MAIN LIGHT DIMMER CONTROL
            //
            case 2:
            {
                if ((HAL_GetTick() -  value_step_tmr) >= VALUE_STEP_TOUT) {
                    value_step_tmr = HAL_GetTick();
                    //
                    //  DIMMER INCREASE/DECREASE
                    //
                    switch(s)
                    {
                    //
                    //  DIMMER INCREASE
                    //
                    case 0:
                    {
                        if (LIGHT_Ctrl1.Light1.value < 0xFF) ++LIGHT_Ctrl1.Light1.value;
                        if (LIGHT_Ctrl1.Light2.value < 0xFF) ++LIGHT_Ctrl1.Light2.value;
                        if (LIGHT_Ctrl1.Light3.value < 0xFF) ++LIGHT_Ctrl1.Light3.value;
                        if ((LIGHT_Ctrl1.Light1.value == 0xFF)
                                &&  (LIGHT_Ctrl1.Light2.value == 0xFF)
                                &&  (LIGHT_Ctrl1.Light3.value == 0xFF)) {
                            s = 1;
                        }
                        break;
                    }
                    //
                    //  DIMMER DECREASE
                    //
                    case 1:
                    default:
                    {
                        if (LIGHT_Ctrl1.Light1.value) --LIGHT_Ctrl1.Light1.value;
                        if (LIGHT_Ctrl1.Light2.value) --LIGHT_Ctrl1.Light2.value;
                        if (LIGHT_Ctrl1.Light3.value) --LIGHT_Ctrl1.Light3.value;
                        if (!LIGHT_Ctrl1.Light1.value
                                &&  !LIGHT_Ctrl1.Light2.value
                                &&  !LIGHT_Ctrl1.Light3.value) {
                            s = 0;
                        }
                        break;
                    }
                    }
                }
                break;
            }
            }
            scrnsvr_tmr = HAL_GetTick();
            menu_thst = 0;
            menu_dim = 0;
            menu_rel123 = 0;
            menu_out1 = 0;
            menu_lc = 0;
            old_min = 60;
            rtctmr = 0;
            break;
        }
        //
        //  CONTROL SELECT
        //
        case SCREEN_CONTROL_SELECT:
        {
            if (menu_lc == 0) {
                ++menu_lc;

                GUI_MULTIBUF_BeginEx(1);

                GUI_SelectLayer(0);
                GUI_Clear();
                GUI_SelectLayer(1);
                GUI_SetBkColor(GUI_TRANSPARENT);
                GUI_Clear();

                GUI_SetPenSize(9);
                GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);

                GUI_DrawLine(400,20,450,20);
                GUI_DrawLine(400,40,450,40);
                GUI_DrawLine(400,60,450,60);

                GUI_DrawLine(380,10,380,262);

                GUI_DrawLine(30,136,350,136);
                GUI_DrawLine(190,20,190,252);

                GUI_DrawBitmap(&bmnext, 385,159);

                GUI_DrawBitmap(&bmSijalicaOff, 55, 10);
                GUI_DrawBitmap(&bmTermometar, 245, 15);
                GUI_DrawBitmap(&bmblindMedium, 55, 150);
                if(Defroster_isActive()) GUI_DrawBitmap(&bmdefrosterOn, 240, 155);
                else GUI_DrawBitmap(&bmdefroster, 240, 155);



                GUI_SetFont(GUI_FONT_24B_1);
                GUI_SetColor(GUI_ORANGE);
                GUI_SetTextMode(GUI_TM_TRANS);

                GUI_SetTextAlign(GUI_TA_HCENTER|GUI_TA_VCENTER);
                GUI_GotoXY(95, 110);
                lng(20);
                GUI_DispString(langText);

                GUI_SetTextAlign(GUI_TA_HCENTER|GUI_TA_VCENTER);
                GUI_GotoXY(285, 110);
                lng(2);
                GUI_DispString(langText);

                GUI_SetTextAlign(GUI_TA_HCENTER|GUI_TA_VCENTER);
                GUI_GotoXY(95, 250);
                lng(21);
                GUI_DispString(langText);

                GUI_SetTextAlign(GUI_TA_HCENTER|GUI_TA_VCENTER);
                GUI_GotoXY(285, 250);
                lng(53);
                GUI_DispString(langText);



                GUI_MULTIBUF_EndEx(1);

                menu_thst = 0;
                menu_dim = 0;
                menu_rel123 = 0;
                menu_out1 = 0;
            }
            else if(menu_lc == 1)
            {
                if(ctrl1)
                {
                    ctrl1 = 0; // hajd opet
                    GUI_MULTIBUF_BeginEx(1);
                    GUI_ClearRect(240,155,bmdefroster.XSize,bmdefroster.YSize);
                    if(Defroster_isActive()) GUI_DrawBitmap(&bmdefrosterOn, 240, 155);
                    else GUI_DrawBitmap(&bmdefroster, 240, 155);
                    GUI_MULTIBUF_EndEx(1);
                }
            }
            break;
        }
        //
        //  THERMOSTAT
        //
        case SCREEN_THERMOSTAT:
        {
            GUI_MULTIBUF_BeginEx(1);

            if      (menu_thst == 0) {
                ++menu_thst;

                GUI_MULTIBUF_BeginEx(0);
                GUI_SelectLayer(0);
                GUI_SetColor(GUI_BLACK);
                GUI_Clear();
                GUI_BMP_Draw(&thstat, 0, 0);

                GUI_SetPenSize(9);
                GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);

                GUI_ClearRect(380, 0, 480, 100);

                GUI_DrawLine(400,20,450,20);
                GUI_DrawLine(400,40,450,40);
                GUI_DrawLine(400,60,450,60);

                GUI_ClearRect(350, 80, 480, 180);
                GUI_ClearRect(310, 180, 420, 205);
                GUI_MULTIBUF_EndEx(0);

                GUI_SelectLayer(1);
                GUI_SetBkColor(GUI_TRANSPARENT);
                GUI_Clear();
                DISPSetPoint(); // show setpoint temperature
                DISPDateTime(); // show clock time
                MVUpdateSet(); // show room temperature
                menu_dim = 0;
                menu_rel123 = 0;
                menu_out1 = 0;
                menu_lc = 0;
            } else if (menu_thst == 1) {
                /************************************/
                /*      SETPOINT  VALUE  INCREASED  */
                /************************************/
                if      ( btninc&& !_btninc) {
                    _btninc = 1;
                    Thermostat_SP_Temp_Increment();
                    SaveThermostatController(&thst, EE_THST1);
                    DISPSetPoint();
                }
                else if (!btninc &&  _btninc) _btninc = 0;
                /************************************/
                /*      SETPOINT  VALUE  DECREASED  */
                /************************************/
                if      (btndec && !_btndec) {
                    _btndec = 1;
                    Thermostat_SP_Temp_Decrement();
                    SaveThermostatController(&thst, EE_THST1);
                    DISPSetPoint();
                }
                else if (!btndec &&  _btndec) _btndec = 0;
                /** ==========================================================================*/
                /**   R E W R I T E   A N D   S A V E   N E W   S E T P O I N T   V A L U E   */
                /** ==========================================================================*/
                if  (IsMVUpdateActiv()) {
                    MVUpdateReset();
                    GUI_ClearRect(410, 185, 480, 235);
                    GUI_ClearRect(310, 230, 480, 255);
                    if(IsTempRegActiv()) GUI_SetColor(GUI_GREEN);
                    else GUI_SetColor(GUI_RED);
                    GUI_SetFont(GUI_FONT_32B_1);
                    GUI_GotoXY(410, 170);
                    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);
                    if(IsTempRegActiv()) GUI_DispString("ON");
                    else GUI_DispString("OFF");
                    GUI_GotoXY(310, 242);
                    GUI_SetFont(GUI_FONT_20_1);
                    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);
                    /*if (IsTempRegHeating()){
                        GUI_SetColor(GUI_RED);
                        GUI_DispString("HEATING");
                        GUI_GotoXY(415, 220);
                        GUI_SetColor(GUI_WHITE);
                    } else if (IsTempRegCooling()){
                        GUI_SetColor(GUI_BLUE);
                        GUI_DispString("COOLING");
                        GUI_GotoXY(415, 197);
                        GUI_SetColor(GUI_ORANGE);
                    }*/
                    GUI_SetColor(GUI_WHITE);
                    GUI_GotoXY(415, 220);
                    GUI_SetFont(GUI_FONT_24_1);
                    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);
                    GUI_DispSDec(thst.mv_temp /10, 3);
                    GUI_DispString("°c");
                }
                /** ==========================================================================*/
                /**             W R I T E    D A T E    &    T I M E    S C R E E N           */
                /** ==========================================================================*/
                if ((HAL_GetTick() - rtctmr) >= DATE_TIME_REFRESH_TIME) {
                    rtctmr = HAL_GetTick();// screansaver clock time update
                    if(++refresh_tmr > 10) { // regular intervals
                        refresh_tmr = 0;// refresh room temperature
                        if (!IsScrnsvrActiv()) MVUpdateSet();
                    }

                    if(IsRtcTimeValid()) {
                        HAL_RTC_GetTime(&hrtc, &rtctm, RTC_FORMAT_BCD);
                        HAL_RTC_GetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);
                        HEX2STR(dbuf, &rtctm.Hours);
                        dbuf[2] = ':';
                        HEX2STR(&dbuf[3], &rtctm.Minutes);
                        dbuf[5]  = NUL;
                        GUI_SetFont(GUI_FONT_32_1);
                        GUI_SetColor(GUI_WHITE);
                        GUI_SetTextMode(GUI_TM_TRANS);
                        GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);
                        GUI_GotoXY(5, 245);
                        GUI_MULTIBUF_BeginEx(1);
                        GUI_ClearRect(0, 220, 100, 270);
                        GUI_DispString(dbuf);
                        GUI_MULTIBUF_EndEx(1);
                    }
                }
            }

            GUI_MULTIBUF_EndEx(1);


            if(thermostatOnOffTouch_timer)
            {
                DISPResetScrnsvr();

                if((HAL_GetTick() - thermostatOnOffTouch_timer) > (2 * 1000))
                {
                    thst.hasInfoChanged = true; // obavjesti ostale
                    thermostatOnOffTouch_timer = 0;
                    menu_thst = 0;

                    if(IsTempRegActiv())
                    {
                        thst.fan_speed = 0;
                        TempRegOff();
                    }
                    else
                    {
                        TempRegHeating();
                    }
                    
                    SaveThermostatController(&thst, EE_THST1);
                }
            }


            break;
        }
        //
        //  RETURN TO FIRST MENU
        //
        case SCREEN_RETURN_TO_FIRST:
        {
            GUI_SelectLayer(0);
            GUI_Clear();
            GUI_SelectLayer(1);
            GUI_SetBkColor(GUI_TRANSPARENT);
            GUI_Clear();
            DISPSetBrightnes(DISP_BRGHT_MIN);
            screen = SCREEN_MAIN;
            i = 0;  // SET SWITCH TO ENTER ON/OFF TOUCH EVENT
            r = 1;  // SKEEP BACKLIGHT LED MAX BRIGHTNESS
    //            if (t == 0) t = 1; // TOGGLE NEXT TOUCH EVENT
    //            else t = 0;
            menu_thst = 0;
            menu_dim = 0;
            menu_rel123 = 0;
            menu_out1 = 0;
            menu_lc = 0;
            menu_clean = 0;
            lcsta = 0;
            thsta = 0;
            curtainSettingMenu = 0;
            curtainMenu = 0;
            lightsModbusSettingsMenu = 0;
            light_selectedIndex = LIGHTS_MODBUS_SIZE + 1;
            lights_allSelected_hasRGB = false;
            shouldDrawScreen = 1;
            break;
        }
        //
        //  SETTINGS MENU 1
        //
        case SCREEN_SETTINGS_1:
        {
            /** ==========================================================================*/
            /**    S E T T I N G S     M E N U     U S E R     I N P U T   U P D A T E    */
            /** ==========================================================================*/


            if (thst.th_ctrl != RADIO_GetValue(hThstControl)) {
                thst.th_ctrl  = RADIO_GetValue(hThstControl);
                ++thsta;
            }
            else if (thst.fan_ctrl != RADIO_GetValue(hFanControl)) {
                thst.fan_ctrl  = RADIO_GetValue(hFanControl);
                ++thsta;
            }
            else if (thst.sp_max != SPINBOX_GetValue(hThstMaxSetPoint)) {
                Thermostat_Set_SP_Max(SPINBOX_GetValue(hThstMaxSetPoint));
                SPINBOX_SetValue(hThstMaxSetPoint, thst.sp_max);
                ++thsta;
            }
            else if (thst.sp_min != SPINBOX_GetValue(hThstMinSetPoint)) {
                Thermostat_Set_SP_Min(SPINBOX_GetValue(hThstMinSetPoint));
                SPINBOX_SetValue(hThstMinSetPoint, thst.sp_min);
                ++thsta;
            }
            else if (thst.fan_diff != SPINBOX_GetValue(hFanDiff)) {
                thst.fan_diff  = SPINBOX_GetValue(hFanDiff);
                ++thsta;
            }
            else if (thst.fan_loband != SPINBOX_GetValue(hFanLowBand)) {
                thst.fan_loband  = SPINBOX_GetValue(hFanLowBand);
                ++thsta;
            }
            else if (thst.fan_hiband != SPINBOX_GetValue(hFanHiBand)) {
                thst.fan_hiband  = SPINBOX_GetValue(hFanHiBand);
                ++thsta;
            }
            else if(thst.group != SPINBOX_GetValue(hThstGroup))
            {
                thst.group = SPINBOX_GetValue(hThstGroup);
                thsta = 1;
            }
            else if(thst.master != CHECKBOX_IsChecked(hThstMaster))
            {
                thst.master = CHECKBOX_IsChecked(hThstMaster);
                thsta = 1;
            }


            if (BUTTON_IsPressed(hBUTTON_Ok))
            {
                if(thsta)
                {
                    SaveThermostatController(&thst, EE_THST1);
                    thst.hasInfoChanged = true;
                }
                thsta = 0;
                DSP_KillSet1Scrn();
                screen = SCREEN_RETURN_TO_FIRST;
            }
            else if (BUTTON_IsPressed(hBUTTON_Next))
            {
                DSP_KillSet1Scrn();
                DSP_InitSet3Scrn();
                screen = SCREEN_SETTINGS_3;
            }
            break;
        }
        //
        //  SETTINGS MENU 2
        //
        case SCREEN_SETTINGS_2:
        {
            /*if (LIGHT_Ctrl1.modbusLight.index != SPINBOX_GetValue(hBIN_MAIN1)){
                LIGHT_Ctrl1.modbusLight.index  = SPINBOX_GetValue(hBIN_MAIN1);
                if (LIGHT_Ctrl1.Main1.index == 0) SPINBOX_SetValue(hBIN2_MAIN1, 0);
                ++lcsta;
            }*/
            if (LIGHT_Ctrl1.Led1.index != SPINBOX_GetValue(hBIN_LED1)) {
                LIGHT_Ctrl1.Led1.index  = SPINBOX_GetValue(hBIN_LED1);
                if (LIGHT_Ctrl1.Led1.index == 0) SPINBOX_SetValue(hBIN2_LED1, 0);
                ++lcsta;
            }
            if (LIGHT_Ctrl1.Led2.index != SPINBOX_GetValue(hBIN_LED2)) {
                LIGHT_Ctrl1.Led2.index  = SPINBOX_GetValue(hBIN_LED2);
                if (LIGHT_Ctrl1.Led2.index == 0) SPINBOX_SetValue(hBIN2_LED2, 0);
                ++lcsta;
            }
            if (LIGHT_Ctrl1.Led3.index != SPINBOX_GetValue(hBIN_LED3)) {
                LIGHT_Ctrl1.Led3.index  = SPINBOX_GetValue(hBIN_LED3);
                if (LIGHT_Ctrl1.Led3.index == 0) SPINBOX_SetValue(hBIN2_LED3, 0);
                ++lcsta;
            }
            if (LIGHT_Ctrl1.Out1.index != SPINBOX_GetValue(hBIN_OUT1)) {
                LIGHT_Ctrl1.Out1.index  = SPINBOX_GetValue(hBIN_OUT1);
                if (LIGHT_Ctrl1.Out1.index == 0) SPINBOX_SetValue(hBIN2_OUT1,0);
                ++lcsta;
            }
            if (LIGHT_Ctrl1.Light1.index != SPINBOX_GetValue(hDIM_LIGHT1)) {
                LIGHT_Ctrl1.Light1.index  = SPINBOX_GetValue(hDIM_LIGHT1);
                ++lcsta;
            }
            if (LIGHT_Ctrl1.Light2.index != SPINBOX_GetValue(hDIM_LIGHT2)) {
                LIGHT_Ctrl1.Light2.index  = SPINBOX_GetValue(hDIM_LIGHT2);
                ++lcsta;
            }
            if (LIGHT_Ctrl1.Light3.index != SPINBOX_GetValue(hDIM_LIGHT3)) {
                LIGHT_Ctrl1.Light3.index  = SPINBOX_GetValue(hDIM_LIGHT3);
                ++lcsta;
            }
            if (LIGHT_Ctrl2.Main1.index != SPINBOX_GetValue(hBIN2_MAIN1)) {
                LIGHT_Ctrl2.Main1.index  = SPINBOX_GetValue(hBIN2_MAIN1);
                ++lcsta;
            }
            if (LIGHT_Ctrl2.Led1.index != SPINBOX_GetValue(hBIN2_LED1)) {
                LIGHT_Ctrl2.Led1.index  = SPINBOX_GetValue(hBIN2_LED1);
                ++lcsta;
            }
            if (LIGHT_Ctrl2.Led2.index != SPINBOX_GetValue(hBIN2_LED2)) {
                LIGHT_Ctrl2.Led2.index  = SPINBOX_GetValue(hBIN2_LED2);
                ++lcsta;
            }
            if (LIGHT_Ctrl2.Led3.index != SPINBOX_GetValue(hBIN2_LED3)) {
                LIGHT_Ctrl2.Led3.index  = SPINBOX_GetValue(hBIN2_LED3);
                ++lcsta;
            }
            if (LIGHT_Ctrl2.Out1.index != SPINBOX_GetValue(hBIN2_OUT1)) {
                LIGHT_Ctrl2.Out1.index  = SPINBOX_GetValue(hBIN2_OUT1);
                ++lcsta;
            }

            if (BUTTON_IsPressed(hBUTTON_Ok)) {
                if (thsta) {
                    thsta = 0;
                    SaveThermostatController(&thst,  EE_THST1);
                }
                if (lcsta) {
                    lcsta = 0;
                    SaveLightController(&LIGHT_Ctrl1, EE_CTRL1);
                    SaveLightController(&LIGHT_Ctrl2, EE_CTRL2);
                }
                ebuf[0] = low_bcklght;
                ebuf[1] = high_bcklght;
                ebuf[2] = scrnsvr_tout;
                ebuf[3] = scrnsvr_ena_hour;
                ebuf[4] = scrnsvr_dis_hour;
                ebuf[5] = scrnsvr_clk_clr;
                if(IsScrnsvrClkActiv()) ebuf[6] = 1;
                else ebuf[6] = 0;
                EE_WriteBuffer(ebuf, EE_DISP_LOW_BCKLGHT, 7);
                DSP_KillSet2Scrn();
                screen = SCREEN_RETURN_TO_FIRST;
            } else if (BUTTON_IsPressed(hBUTTON_Next)) {
                DSP_KillSet2Scrn();
                DSP_InitSet3Scrn();
                screen = SCREEN_SETTINGS_3;
            }
            break;
        }
        //
        //  SETTINGS MENU 3
        //
        case SCREEN_SETTINGS_3:
        {
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
            if (rtcdt.WeekDay != Dec2Bcd(DROPDOWN_GetSel(hDRPDN_WeekDay)+1)) {
                rtcdt.WeekDay  = Dec2Bcd(DROPDOWN_GetSel(hDRPDN_WeekDay)+1);
                HAL_RTC_SetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);
                RtcTimeValidSet();
            }
            if (scrnsvr_clk_clr != SPINBOX_GetValue(hSPNBX_ScrnsvrClockColour)) {
                scrnsvr_clk_clr = SPINBOX_GetValue(hSPNBX_ScrnsvrClockColour);
                GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);
                GUI_FillRect(340, 51, 430, 59);
            }
            if (CHECKBOX_GetState(hCHKBX_ScrnsvrClock) == 1) ScrnsvrClkSet();
            else ScrnsvrClkReset();
            high_bcklght = SPINBOX_GetValue(hSPNBX_DisplayHighBrightness);
            low_bcklght = SPINBOX_GetValue(hSPNBX_DisplayLowBrightness);
            scrnsvr_tout = SPINBOX_GetValue(hSPNBX_ScrnsvrTimeout);
            scrnsvr_ena_hour = SPINBOX_GetValue(hSPNBX_ScrnsvrEnableHour);
            scrnsvr_dis_hour = SPINBOX_GetValue(hSPNBX_ScrnsvrDisableHour);
            scrnsvr_clk_clr = SPINBOX_GetValue(hSPNBX_ScrnsvrClockColour);
            if (BUTTON_IsPressed(hBUTTON_Ok)) {
                if (thsta) {
                    thsta = 0;
                    SaveThermostatController(&thst,  EE_THST1);
                }
                if (lcsta) {
                    lcsta = 0;
                    SaveLightController(&LIGHT_Ctrl1, EE_CTRL1);
                    SaveLightController(&LIGHT_Ctrl2, EE_CTRL2);
                }
                ebuf[0] = low_bcklght;
                ebuf[1] = high_bcklght;
                ebuf[2] = scrnsvr_tout;
                ebuf[3] = scrnsvr_ena_hour;
                ebuf[4] = scrnsvr_dis_hour;
                ebuf[5] = scrnsvr_clk_clr;
                if(IsScrnsvrClkActiv()) ebuf[6] = 1;
                else ebuf[6] = 0;
                EE_WriteBuffer(ebuf, EE_DISP_LOW_BCKLGHT, 7);
                EE_WriteBuffer(&tfifa, EE_TFIFA, 1);
                DSP_KillSet3Scrn();
                screen = SCREEN_RETURN_TO_FIRST;
            } else if (BUTTON_IsPressed(hBUTTON_Next)) {
                DSP_KillSet3Scrn();
                DSP_InitSet5Scrn();
                screen = SCREEN_SETTINGS_5;
            }
            break;
        }
        //
        //  CLEAN
        //
        case SCREEN_CLEAN:
        {
            if (menu_clean == 0) {
                ++menu_clean;
                GUI_Clear();
                clrtmr = 60;
            } else if (menu_clean == 1) {
                if (HAL_GetTick()-clean_tmr >= 1000) {

                    clean_tmr = HAL_GetTick();

                    DISPResetScrnsvr();

                    GUI_MULTIBUF_BeginEx(1);

                    GUI_ClearRect(0,50,480,200);
                    if (clrtmr > 5) {
                        GUI_SetColor(GUI_GREEN);
                    }
                    else {
                        GUI_SetColor(GUI_RED);
                        BuzzerOn();
                        HAL_Delay(1);
                        BuzzerOff();
                    }
                    GUI_SetFont(GUI_FONT_32_1);
                    GUI_SetTextMode(GUI_TM_TRANS);
                    GUI_SetTextAlign(GUI_TA_HCENTER|GUI_TA_VCENTER);
                    GUI_GotoXY(240, 80);
                    lng(13);
                    GUI_DispString(langText);
                    GUI_SetFont(GUI_FONT_D64);
                    GUI_SetTextMode(GUI_TM_TRANS);
                    GUI_SetTextAlign(GUI_TA_HCENTER|GUI_TA_VCENTER);
                    GUI_GotoXY(240, 156);
                    GUI_DispDecMin(clrtmr);

                    GUI_MULTIBUF_EndEx(1);

                    if (clrtmr) --clrtmr;
                    else screen = SCREEN_RETURN_TO_FIRST;
                }
            }

            break;
        }
        //
        //  SETTINGS MENU 4
        //
        case SCREEN_SETTINGS_4:
        {
            /*if(Ventilator_getRelay(&ventilator) != SPINBOX_GetValue(hVentilatorRelay))
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
            }*/



            if (BUTTON_IsPressed(hBUTTON_Ok))
            {
                if(settingsChanged)
                {
    //                    Ventilator_Save(&ventilator, EE_VENTILATOR);
                    settingsChanged = 0;
                }
                DSP_KillSet4Scrn();
                screen = SCREEN_RETURN_TO_FIRST;
            }
            else if (BUTTON_IsPressed(hBUTTON_Next))
            {
                DSP_KillSet4Scrn();
                DSP_InitSet5Scrn();
                screen = SCREEN_SETTINGS_5;
            }
            break;
        }
        //
        //  SETTINGS MENU 5
        //
        case SCREEN_SETTINGS_5:
        {
            for(uint8_t i = curtainSettingMenu * 4; i < (((CURTAINS_SIZE - (curtainSettingMenu * 4)) >= 4) ? ((curtainSettingMenu * 4) + 4) : CURTAINS_SIZE); i++)
            {
                if((Curtain_GetRelayUp(curtains + i) != SPINBOX_GetValue(hCurtainsRelay[i * 2])) || (Curtain_GetRelayDown(curtains + i) != SPINBOX_GetValue(hCurtainsRelay[(i * 2) + 1])))
                {
                    settingsChanged = 1;

                    Curtain_SetRelayUp(curtains + i, SPINBOX_GetValue(hCurtainsRelay[i * 2]));
                    Curtain_SetRelayDown(curtains + i, SPINBOX_GetValue(hCurtainsRelay[(i * 2) + 1]));
    //                        curtains[i].relayUp = SPINBOX_GetValue(hCurtainsRelay[i * 2]);
    //                        curtains[i].relayDown = SPINBOX_GetValue(hCurtainsRelay[(i * 2) + 1]);
                }
            }


            if (BUTTON_IsPressed(hBUTTON_Ok))
            {

                if(settingsChanged)
                {
                    Curtains_Save();
                    settingsChanged = 0;
                }
                DSP_KillSet5Scrn();
                screen = SCREEN_RETURN_TO_FIRST;
            }
            else if (BUTTON_IsPressed(hBUTTON_Next))
            {
                if((CURTAINS_SIZE - ((curtainSettingMenu + 1) * 4)) > 0)
                {
                    DSP_KillSet5Scrn();
                    ++curtainSettingMenu;
                    DSP_InitSet5Scrn();
                }
                else
                {
                    if(settingsChanged)
                    {
                        Curtains_Save();
                        settingsChanged = 0;
                    }

                    DSP_KillSet5Scrn();
                    curtainSettingMenu = 0;

                    DSP_InitSet6Scrn();
                    screen = SCREEN_SETTINGS_6;
                }
            }
            break;
        }
        //
        //  SETTINGS MENU 6
        //
        case SCREEN_SETTINGS_6:
        {
            GUI_MULTIBUF_BeginEx(1);


            for(uint8_t i = lightsModbusSettingsMenu * LIGHTS_MODBUS_PER_SETTINGS; i < (((LIGHTS_MODBUS_SIZE - (lightsModbusSettingsMenu * LIGHTS_MODBUS_PER_SETTINGS)) >= LIGHTS_MODBUS_PER_SETTINGS) ? ((lightsModbusSettingsMenu * LIGHTS_MODBUS_PER_SETTINGS) + LIGHTS_MODBUS_PER_SETTINGS) : LIGHTS_MODBUS_SIZE); i++)
            {
                if(Light_Modbus_GetRelay(lights_modbus + i) != SPINBOX_GetValue(lightsWidgets[i].relay))
                {
                    settingsChanged = 1;

                    Light_Modbus_SetRelay(lights_modbus + i, SPINBOX_GetValue(lightsWidgets[i].relay));
                }
                else if(Light_Modbus_GetIconID(lights_modbus + i) != SPINBOX_GetValue(lightsWidgets[i].iconID))
                {
                    settingsChanged = 1;

                    Light_Modbus_SetIcon(lights_modbus + i, SPINBOX_GetValue(lightsWidgets[i].iconID));
                }
                else if(lights_modbus[i].controllerID_on != SPINBOX_GetValue(lightsWidgets[i].controllerID_on))
                {
                    settingsChanged = 1;

                    lights_modbus[i].controllerID_on = SPINBOX_GetValue(lightsWidgets[i].controllerID_on);
                }
                else if(Light_Modbus_GetOnDelayTime(lights_modbus + i) != SPINBOX_GetValue(lightsWidgets[i].controllerID_on_delay))
                {
                    settingsChanged = 1;

                    Light_Modbus_SetOnDelayTime(lights_modbus + i, SPINBOX_GetValue(lightsWidgets[i].controllerID_on_delay));
                }
                else if(Light_Modbus_GetOffTime(lights_modbus + i) != SPINBOX_GetValue(lightsWidgets[i].offTime))
                {
                    settingsChanged = 1;

                    Light_Modbus_SetOffTime(lights_modbus + i, SPINBOX_GetValue(lightsWidgets[i].offTime));
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
                else if(Light_Modbus_isTiedToMainLight(lights_modbus + i) != CHECKBOX_GetState(lightsWidgets[i].tiedToMainLight))
                {
                    settingsChanged = 1;

                    if(CHECKBOX_GetState(lightsWidgets[i].tiedToMainLight)) Light_Modbus_TieToMainLight(lights_modbus + i);
                    else Light_Modbus_UntieFromMainLight(lights_modbus + i);
                }
                else if(Light_Modbus_isBrightnessRemembered(lights_modbus + i) != CHECKBOX_GetState(lightsWidgets[i].rememberBrightness))
                {
                    settingsChanged = 1;

                    Light_Modbus_RememberBrightnessSet(lights_modbus + i, CHECKBOX_GetState(lightsWidgets[i].rememberBrightness));
                }

                // drawing light assumes there's one light per screen
                GUI_ClearRect(380, 0, 480, 100);
                GUI_DrawBitmap(Light_Modbus_GetIcon(lights_modbus + i), 480 - Light_Modbus_GetIcon(lights_modbus + i)->XSize, 0);
            }




            if (BUTTON_IsPressed(hBUTTON_Ok))
            {
                if(settingsChanged)
                {
                    Lights_Modbus_Save();
                    settingsChanged = 0;
                }
                DSP_KillSet6Scrn();
                screen = SCREEN_RETURN_TO_FIRST;
            }
            else if (BUTTON_IsPressed(hBUTTON_Next))
            {
                if((LIGHTS_MODBUS_SIZE - ((lightsModbusSettingsMenu + 1) * LIGHTS_MODBUS_PER_SETTINGS)) > 0)
                {
                    DSP_KillSet6Scrn();
                    ++lightsModbusSettingsMenu;
                    DSP_InitSet6Scrn();
                }
                else
                {
                    if(settingsChanged)
                    {
                        Lights_Modbus_Save();
                        settingsChanged = 0;
                    }

                    DSP_KillSet6Scrn();
                    lightsModbusSettingsMenu = 0;

                    DSP_InitSet7Scrn();
                    screen = SCREEN_SETTINGS_7;
                }
            }

            GUI_MULTIBUF_EndEx(1);

            break;
        }
        //
        //  LIGHTS
        //
        case SCREEN_LIGHTS:
        {
            if(shouldDrawScreen)
            {
                shouldDrawScreen = 0;

                GUI_MULTIBUF_BeginEx(1);

                GUI_Clear();
                GUI_SetPenSize(9);
                GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);
                GUI_DrawLine(400,20,450,20);
                GUI_DrawLine(400,40,450,40);
                GUI_DrawLine(400,60,450,60);



                int y = (Lights_Modbus_Rows_getCount() > 1) ? 10 : 86;
                uint8_t lightsInRowSum = 0;

                for(uint8_t row = 0; row < Lights_Modbus_Rows_getCount(); ++row)
                {
                    uint8_t lightsInRow = Lights_Modbus_getCount();

                    if(Lights_Modbus_getCount() > 3)
                    {
                        if(Lights_Modbus_getCount() == 4) lightsInRow = 2;
                        else if(Lights_Modbus_getCount() == 5) lightsInRow = (row > 0) ? 2 : 3;
                        else lightsInRow = 3;
                    }

                    uint8_t lightsMenuSpaceBetween = (400 - (80 * lightsInRow)) / (lightsInRow - 1 + 2);

                    for(uint8_t i = 0; i < lightsInRow; ++i)
                    {
                        const LIGHT_Modbus_CmdTypeDef* light = lights_modbus + lightsInRowSum + i;

                        int x = (lightsMenuSpaceBetween * ((i % lightsInRow) + 1)) + (80 * (i % lightsInRow));

                        GUI_DrawBitmap(Light_Modbus_GetIcon(light), x, y);

                        /*if(Light_Modbus_isActive(light)) GUI_DrawBitmap(Light_Modbus_GetIcon(light), x, y);
                        else GUI_DrawBitmap(Light_Modbus_GetIcon(light), x, y);*/

                        /*GUI_SetFont(GUI_FONT_24B_1);
                        GUI_SetColor(GUI_ORANGE);
                        GUI_SetTextMode(GUI_TM_TRANS);

                        GUI_SetTextAlign(GUI_TA_HCENTER|GUI_TA_VCENTER);
                        GUI_GotoXY(x + 45, y + 100);

                        GUI_DispDec((row + 1) * (i + 1), 1);*/
                    }

                    lightsInRowSum += lightsInRow;
                    y += 130;
                }


                GUI_MULTIBUF_EndEx(1);
            }

            break;
        }
        //
        //  CURTAINS
        //
        case SCREEN_CURTAINS:
        {
            if(shouldDrawScreen)
            {
                shouldDrawScreen = 0;

                GUI_MULTIBUF_BeginEx(1);

                GUI_Clear();
                GUI_SetPenSize(9);
                GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);

                GUI_DrawLine(400,20,450,20);
                GUI_DrawLine(400,40,450,40);
                GUI_DrawLine(400,60,450,60);

    //                GUI_SetColor(GUI_RED);
    //                GUI_DrawLine(380,10,380,262);


                if(Curtains_getCount() > 1)
                {
                    GUI_DrawBitmap(&bmprevious, 0, 192);
                    GUI_DrawBitmap(&bmnext, 320, 192);
                }


                GUI_ClearRect(0, 0, 70, 70);

                GUI_SetColor(GUI_WHITE);
                GUI_SetFont(GUI_FONT_D48);
                GUI_SetTextMode(GUI_TM_TRANS);
                GUI_SetTextAlign(GUI_TA_HCENTER|GUI_TA_VCENTER);

                if(!Curtain_areAllSelected()) GUI_DispDecAt(Curtain_getSelected() + 1, 50, 50, ((Curtain_getSelected() + 1) < 10) ? 1 : 2);


                curtainMenuSpaceBetween = (380 - (curtainMenuCurtainLength * curtainMenuCurtainCount)) / (curtainMenuCurtainCount - 1 + 2);

                GUI_SetColor(GUI_WHITE);

                GUI_DrawLine(curtainMenuSpaceBetween,      136,      curtainMenuSpaceBetween + curtainMenuCurtainLength,     136);

                DrawEquilateralTriangle(curtainMenuSpaceBetween,     136 - 20,    curtainMenuCurtainLength, 0, GUI_RED, (((Curtain_areAllSelected()) && Curtains_isNewDirectionUp()) || ((!Curtain_areAllSelected()) && Curtain_isNewDirectionUp(curtains + Curtain_getSelected()))) ? GUI_RED : 0);
                DrawEquilateralTriangle(curtainMenuSpaceBetween + curtainMenuCurtainLength,     136 + 20,    curtainMenuCurtainLength, 180, GUI_BLUE, (((Curtain_areAllSelected()) && Curtains_isNewDirectionDown()) || ((!Curtain_areAllSelected()) && Curtain_isNewDirectionDown(curtains + Curtain_getSelected()))) ? GUI_BLUE : 0);


                GUI_MULTIBUF_EndEx(1);
            }

            break;
        }
        //
        //  SETTINGS MENU 7
        //
        case SCREEN_SETTINGS_7:
        {
            if(BUTTON_IsPressed(hBUTTON_SET_DEFAULTS))
            {
                SetDefault();
            }
            else if(BUTTON_IsPressed(hBUTTON_SYSRESTART))
            {
                SYSRestart();
            }
            else
            {
                if (tfifa != SPINBOX_GetValue(hDEV_ID))
                {
                    tfifa = SPINBOX_GetValue(hDEV_ID);
                    settingsChanged = 1;
                }
                else if(Curtain_GetMoveTime() != SPINBOX_GetValue(hCurtainsMoveTime))
                {
                    Curtain_SetMoveTime(SPINBOX_GetValue(hCurtainsMoveTime));
                    settingsChanged = 1;
                }
                else if(bOnlyLeaveScreenSaverAfterTouch != CHECKBOX_GetState(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH))
                {
                    bOnlyLeaveScreenSaverAfterTouch = CHECKBOX_GetState(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH);
                    settingsChanged = 1;
                }
                else if(LightNightTimer_isEnabled != CHECKBOX_GetState(hCHKBX_LIGHT_NIGHT_TIMER))
                {
                    LightNightTimer_isEnabled = CHECKBOX_GetState(hCHKBX_LIGHT_NIGHT_TIMER);
                    settingsChanged = 1;
                }
            }


            if(BUTTON_IsPressed(hBUTTON_Ok))
            {
                if(settingsChanged)
                {
                    Curtains_Save();
                    EE_WriteBuffer(&tfifa, EE_TFIFA, 1);
                    EE_WriteBuffer(&bOnlyLeaveScreenSaverAfterTouch, EE_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, 1);
                    settingsChanged = 0;
                }
                DSP_KillSet7Scrn();
                screen = SCREEN_RETURN_TO_FIRST;
            }
            else if (BUTTON_IsPressed(hBUTTON_Next))
            {
                if(settingsChanged)
                {
                    Curtains_Save();
                    EE_WriteBuffer(&tfifa, EE_TFIFA, 1);
                    EE_WriteBuffer(&bOnlyLeaveScreenSaverAfterTouch, EE_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, 1);
                    settingsChanged = 0;
                }


                DSP_KillSet7Scrn();
                DSP_InitSet8Scrn();
                screen = SCREEN_SETTINGS_8;
            }

            break;
        }
        //
        //  SELECT SCREEN 2
        //
        case SCREEN_SELECT_SCREEN_2:
        {
            if(shouldDrawScreen)
            {
                shouldDrawScreen = 0;

                GUI_MULTIBUF_BeginEx(1);

                GUI_SelectLayer(0);
                GUI_Clear();
                GUI_SelectLayer(1);
                GUI_SetBkColor(GUI_TRANSPARENT);
                GUI_Clear();

                GUI_SetPenSize(9);
                GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);

                GUI_DrawLine(400,20,450,20);
                GUI_DrawLine(400,40,450,40);
                GUI_DrawLine(400,60,450,60);

                GUI_DrawLine(380,10,380,262);

                GUI_DrawLine(126,60,126,212);
                GUI_DrawLine(252,60,252,212);

                GUI_DrawBitmap(&bmnext, 385, 159);

                GUI_DrawBitmap(&bmCLEAN, 18, 76);
                GUI_DrawBitmap(&bmwifi, 146, 76);
                GUI_DrawBitmap(&bmmobilePhone, 290, 76);



                GUI_SetFont(GUI_FONT_24B_1);
                GUI_SetColor(GUI_ORANGE);
                GUI_SetTextMode(GUI_TM_TRANS);


                GUI_SetTextAlign(GUI_TA_HCENTER|GUI_TA_VCENTER);
                GUI_GotoXY(63, 176);
                lng(6);
                GUI_DispString(langText);

                GUI_SetTextAlign(GUI_TA_HCENTER|GUI_TA_VCENTER);
                GUI_GotoXY(189, 176);
                lng(51);
                GUI_DispString(langText);

                GUI_SetTextAlign(GUI_TA_HCENTER|GUI_TA_VCENTER);
                GUI_GotoXY(315, 176);
                lng(52);
                GUI_DispString(langText);

                GUI_MULTIBUF_EndEx(1);
            }

            break;
        }
        //
        //  QR CODE
        //
        case SCREEN_QR_CODE:
        {
            if(shouldDrawScreen)
            {
                shouldDrawScreen = 0;

                GUI_MULTIBUF_BeginEx(1);

                GUI_Clear();

                GUI_SetPenSize(9);
                GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);
                GUI_DrawLine(400,20,450,20);
                GUI_DrawLine(400,40,450,40);
                GUI_DrawLine(400,60,450,60);

                GUI_HMEM hqr = GUI_QR_Create((char*)QR_Code_Get(qr_code_draw_id), 8, GUI_QR_ECLEVEL_M, 0);
                GUI_QR_INFO qrInfo;
                GUI_QR_GetInfo(hqr, &qrInfo);

                GUI_SetColor(GUI_WHITE);
                GUI_FillRect(0, 0, qrInfo.Size + 20, qrInfo.Size + 20);

                GUI_QR_Draw(hqr, 10, 10);
                GUI_QR_Delete(hqr);


                GUI_MULTIBUF_EndEx(1);
            }

            break;
        }
        //
        //  LIGHT SETTINGS
        //
        case SCREEN_LIGHT_SETTINGS:
        {
            if(shouldDrawScreen)
            {
                shouldDrawScreen = 0;

                GUI_MULTIBUF_BeginEx(1);

                GUI_SelectLayer(0);
                GUI_Clear();
                GUI_SelectLayer(1);
                GUI_SetBkColor(GUI_TRANSPARENT);
                GUI_Clear();
                
                GUI_DrawBitmap(&bmprevious, 0, 0);

                GUI_SetPenSize(9);
                GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);

                GUI_DrawLine(400,20,450,20);
                GUI_DrawLine(400,40,450,40);
                GUI_DrawLine(400,60,450,60);

                //GUI_DrawRect(20, 20, 100, 100);
                //GUI_SetColor(Light_Modbus_GetColor(lights_modbus + light_selectedIndex));
                //GUI_FillRect(20, 20, 100, 100);


                if(((light_selectedIndex == LIGHTS_MODBUS_SIZE) && (!lights_allSelected_hasRGB)) || Light_Modbus_isDimmer(lights_modbus + light_selectedIndex))
                {
                    GUI_DrawBitmap(&bmblackWhiteGradient, 20, 110);
                }
                else if(((light_selectedIndex == LIGHTS_MODBUS_SIZE) && lights_allSelected_hasRGB) || Light_Modbus_isRGB(lights_modbus + light_selectedIndex))
                {
                    GUI_SetColor(GUI_WHITE);
                    GUI_FillRect(200, 20, 280, 100);

                    GUI_DrawBitmap(&bmblackWhiteGradient, 20, 110);
                    GUI_DrawBitmap(&bmcolorSpectrum, 20, 180);
                }

                GUI_MULTIBUF_EndEx(1);
            }

            break;
        }
        //
        //  SETTINGS MENU 8
        //
        case SCREEN_SETTINGS_8:
        {
            if(defroster.cycleTime != SPINBOX_GetValue(defroster_settingWidgets.cycleTime))
            {
                Defroster_SetCycleTime(SPINBOX_GetValue(defroster_settingWidgets.cycleTime));
                settingsChanged = 1;
            }
            else if(defroster.activeTime != SPINBOX_GetValue(defroster_settingWidgets.activeTime))
            {
                Defroster_SetActiveTime(SPINBOX_GetValue(defroster_settingWidgets.activeTime));
                settingsChanged = 1;
            }
            else if(defroster.pin != SPINBOX_GetValue(defroster_settingWidgets.pin))
            {
                defroster.pin = SPINBOX_GetValue(defroster_settingWidgets.pin);
                settingsChanged = 1;
            }




            if(BUTTON_IsPressed(hBUTTON_Ok))
            {
                if(settingsChanged)
                {
                    Defroster_Save();
                    settingsChanged = 0;
                }
                DSP_KillSet8Scrn();
                screen = SCREEN_RETURN_TO_FIRST;
            }
            else if (BUTTON_IsPressed(hBUTTON_Next))
            {
                if(settingsChanged)
                {
                    Defroster_Save();
                    settingsChanged = 0;
                }


                DSP_KillSet8Scrn();
                DSP_InitSet1Scrn();
                screen = SCREEN_SETTINGS_1;
            }

            break;
        }
        //
        //  RESET MENU SWITCHES
        //
        case SCREEN_RESET_MENU_SWITCHES:
        {
            if(LightNightTimer_StartTime)
            {
                GUI_MULTIBUF_BeginEx(1);

                const uint8_t dispTime = (((LIGHT_NIGHT_TIMER_DURATION * 1000) - (HAL_GetTick() - LightNightTimer_StartTime)) / 1000);

                GUI_SetColor(GUI_WHITE);
                GUI_SetFont(GUI_FONT_D32);
                GUI_SetTextMode(GUI_TM_TRANS);
                GUI_SetTextAlign(GUI_TA_HCENTER|GUI_TA_VCENTER);

                GUI_ClearRect(220, 116, 265, 156);
                GUI_DispDecAt(dispTime + 1, 240, 136, (((dispTime / 10) < 2) ? (dispTime / 10) : -1) + 1);

                GUI_MULTIBUF_EndEx(1);
            }
        }
        default:
        {
            i = 0;
            menu_lc = 0;
            menu_thst = 0;
            menu_dim = 0;
            menu_rel123 = 0;
            menu_out1 = 0;
            break;
        }
    }





    if((HAL_GetTick() - everyMinuteTimerStart) >= (60 * 1000))
    {
        everyMinuteTimerStart = HAL_GetTick();

        for(uint8_t i = 0; i < Lights_Modbus_getCount(); i++)
        {
            if(Light_Modbus_isTimeOnEnabled(lights_modbus + i) && Light_Modbus_isTimeToTurnOn(lights_modbus + i))
            {
                Light_Modbus_On(lights_modbus + i);

                if(screen == SCREEN_LIGHTS) shouldDrawScreen = 1;
                else if((screen == SCREEN_RESET_MENU_SWITCHES) || (screen == SCREEN_MAIN)) screen = SCREEN_RETURN_TO_FIRST;
            }
        }
    }




    if(light_settingsTimerStart && ((HAL_GetTick() - light_settingsTimerStart) >= (2 * 1000)))
    {
        light_settingsTimerStart = 0;
        screen = SCREEN_LIGHT_SETTINGS;
        shouldDrawScreen = 1;
    }





    if (!IsScrnsvrActiv()) {
        if ((HAL_GetTick() - scrnsvr_tmr) >= (uint32_t)(scrnsvr_tout*1000))
        {
            uint8_t saveBrightness = 0;
            
            if      (screen == SCREEN_SETTINGS_1) DSP_KillSet1Scrn();
            else if (screen == SCREEN_SETTINGS_2) DSP_KillSet2Scrn();
            else if (screen == SCREEN_SETTINGS_3)DSP_KillSet3Scrn();
            else if (screen == SCREEN_SETTINGS_4)DSP_KillSet4Scrn();
            else if (screen == SCREEN_SETTINGS_5)DSP_KillSet5Scrn();
            else if (screen == SCREEN_SETTINGS_6)DSP_KillSet6Scrn();
            else if (screen == SCREEN_SETTINGS_7)DSP_KillSet7Scrn();
            else if (screen == SCREEN_SETTINGS_8)DSP_KillSet8Scrn();
            
            for(uint8_t i = 0; i < Lights_Modbus_getCount(); i++)
            {
                if(lights_modbus[i].saveBrightness)
                {
                    lights_modbus[i].saveBrightness = 0;
                    saveBrightness = 1;
                }
            }
            
            if(saveBrightness)
            {
                saveBrightness = 0;
                Lights_Modbus_Save();
            }
            
            DISPSetBrightnes(low_bcklght);
            ScrnsvrInitReset();
            ScrnsvrSet();
            screen = SCREEN_RETURN_TO_FIRST;
        }
    }
    //
    //  CHECK IF ENABLED RELAY 4 AUTO SHUTDOWN TIMER
    //
    if (out1_tmr) {
        if ((HAL_GetTick() - out1_tmr) >= (uint32_t)(SECONDS_PER_HOUR*4000)) {
            LIGHT_Ctrl1.Out1.value = 0;
            out1_tmr = 0;
        }
    }
    //
    //  DISPLAY SETTINGS MENU
    //
    if (DISPMenuSettings(btnset)&&(screen < SCREEN_SETTINGS_1)) {

        Lights_Modbus_Off();
        Curtains_Stop();
        Defroster_Off();

        DSP_InitSet1Scrn();
        screen = SCREEN_SETTINGS_1;
    }

    if (HAL_GetTick() - rtctmr >= 1000) {
        rtctmr = HAL_GetTick();
        if (screen < SCREEN_CONTROL_SELECT) DISPDateTime();
        if (!IsScrnsvrActiv()) MVUpdateSet();
    }
}
/**
  * @brief
  * @param
  * @retval
  */
static void DSP_InitSet1Scrn(void)
{
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    hThstControl = RADIO_CreateEx(10, 20, 150, 80, 0,WM_CF_SHOW, 0, ID_ThstControl, 3, 20);
    RADIO_SetTextColor(hThstControl, GUI_GREEN);
    RADIO_SetText(hThstControl, "OFF",     0);
    RADIO_SetText(hThstControl, "COOLING", 1);
    RADIO_SetText(hThstControl, "HEATING", 2);
    RADIO_SetValue(hThstControl, thst.th_ctrl);

    hFanControl = RADIO_CreateEx(10, 150, 150, 80, 0,WM_CF_SHOW, 0, ID_FanControl, 2, 20);
    RADIO_SetTextColor(hFanControl, GUI_GREEN);
    RADIO_SetText(hFanControl, "ON / OFF", 0);
    RADIO_SetText(hFanControl, "3 SPEED", 1);
    RADIO_SetValue(hFanControl, thst.fan_ctrl);

    hThstMaxSetPoint = SPINBOX_CreateEx(110, 20, 90, 30, 0, WM_CF_SHOW, ID_ThstMaxSetPoint, THST_SP_MIN, THST_SP_MAX);
    SPINBOX_SetEdge(hThstMaxSetPoint, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstMaxSetPoint, thst.sp_max);

    hThstMinSetPoint = SPINBOX_CreateEx(110, 70, 90, 30, 0, WM_CF_SHOW, ID_ThstMinSetPoint, THST_SP_MIN, THST_SP_MAX);
    SPINBOX_SetEdge(hThstMinSetPoint, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstMinSetPoint, thst.sp_min);

    hFanDiff = SPINBOX_CreateEx(110, 150, 90, 30, 0, WM_CF_SHOW, ID_FanDiff, 0, 10);
    SPINBOX_SetEdge(hFanDiff, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hFanDiff, thst.fan_diff);

    hFanLowBand = SPINBOX_CreateEx(110, 190, 90, 30, 0, WM_CF_SHOW, ID_FanLowBand, 0, 50);
    SPINBOX_SetEdge(hFanLowBand, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hFanLowBand, thst.fan_loband);

    hFanHiBand = SPINBOX_CreateEx(110, 230, 90, 30, 0, WM_CF_SHOW, ID_FanHiBand, 0, 100);
    SPINBOX_SetEdge(hFanHiBand, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hFanHiBand, thst.fan_hiband);

    hThstGroup = SPINBOX_CreateEx(320, 20, 100, 40, 0, WM_CF_SHOW, ID_THST_GROUP, 0, 254);
    SPINBOX_SetEdge(hThstGroup, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstGroup, thst.group);

    hThstMaster = CHECKBOX_Create(320, 70, 170, 20, 0, ID_THST_MASTER, WM_CF_SHOW);
    CHECKBOX_SetTextColor(hThstMaster, GUI_GREEN);
    CHECKBOX_SetText(hThstMaster, "Master");
    CHECKBOX_SetState(hThstMaster, thst.master);







    hBUTTON_Next = BUTTON_Create(340, 180, 130, 30, ID_Next, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Next, "NEXT");

    hBUTTON_Ok = BUTTON_Create(340, 230, 130, 30, ID_Ok, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

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
  * @brief
  * @param
  * @retval
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
  * @brief
  * @param
  * @retval
  */
static void DSP_InitSet2Scrn(void)
{
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    hBIN_MAIN1 = SPINBOX_CreateEx(10, 50, 90, 30, 0, WM_CF_SHOW, ID_BIN_MAIN1, 0, 512);
    SPINBOX_SetEdge(hBIN_MAIN1, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hBIN_MAIN1, LIGHT_Ctrl1.modbusLight.index);

    hBIN_LED1 = SPINBOX_CreateEx(10, 90, 90, 30, 0, WM_CF_SHOW, ID_BIN_LED1, 0, 24);
    SPINBOX_SetEdge(hBIN_LED1, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hBIN_LED1, LIGHT_Ctrl1.Led1.index);

    hBIN_LED2 = SPINBOX_CreateEx(10, 130, 90, 30, 0, WM_CF_SHOW, ID_BIN_LED2, 0, 24);
    SPINBOX_SetEdge(hBIN_LED2, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hBIN_LED2, LIGHT_Ctrl1.Led2.index);

    hBIN_LED3 = SPINBOX_CreateEx(10, 170, 90, 30, 0, WM_CF_SHOW, ID_BIN_LED3, 0, 24);
    SPINBOX_SetEdge(hBIN_LED3, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hBIN_LED3, LIGHT_Ctrl1.Led3.index);

    hBIN_OUT1 = SPINBOX_CreateEx(10, 210, 90, 30, 0, WM_CF_SHOW, ID_BIN_OUT1, 0, 24);
    SPINBOX_SetEdge(hBIN_OUT1, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hBIN_OUT1, LIGHT_Ctrl1.Out1.index);

    hDIM_LIGHT1 = SPINBOX_CreateEx(340, 50, 90, 30, 0, WM_CF_SHOW, ID_DIM_LIGHT1, 0, 32);
    SPINBOX_SetEdge(hDIM_LIGHT1, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hDIM_LIGHT1, LIGHT_Ctrl1.Light1.index);

    hDIM_LIGHT2 = SPINBOX_CreateEx(340, 90, 90, 30, 0, WM_CF_SHOW, ID_DIM_LIGHT2, 0, 32);
    SPINBOX_SetEdge(hDIM_LIGHT2, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hDIM_LIGHT2, LIGHT_Ctrl1.Light2.index);

    hDIM_LIGHT3 = SPINBOX_CreateEx(340, 130, 90, 30, 0, WM_CF_SHOW, ID_DIM_LIGHT3, 0, 32);
    SPINBOX_SetEdge(hDIM_LIGHT3, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hDIM_LIGHT3, LIGHT_Ctrl1.Light3.index);


    hBIN2_MAIN1 = SPINBOX_CreateEx(170, 50, 90, 30, 0, WM_CF_SHOW, ID2_BIN_MAIN1, 0, 24);
    SPINBOX_SetEdge(hBIN2_MAIN1, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hBIN2_MAIN1, LIGHT_Ctrl2.Main1.index);

    hBIN2_LED1 = SPINBOX_CreateEx(170, 90, 90, 30, 0, WM_CF_SHOW, ID2_BIN_LED1, 0, 24);
    SPINBOX_SetEdge(hBIN2_LED1, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hBIN2_LED1, LIGHT_Ctrl2.Led1.index);

    hBIN2_LED2 = SPINBOX_CreateEx(170, 130, 90, 30, 0, WM_CF_SHOW, ID2_BIN_LED2, 0, 24);
    SPINBOX_SetEdge(hBIN2_LED2, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hBIN2_LED2, LIGHT_Ctrl2.Led2.index);

    hBIN2_LED3 = SPINBOX_CreateEx(170, 170, 90, 30, 0, WM_CF_SHOW, ID2_BIN_LED3, 0, 24);
    SPINBOX_SetEdge(hBIN2_LED3, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hBIN2_LED3, LIGHT_Ctrl2.Led3.index);

    hBIN2_OUT1 = SPINBOX_CreateEx(170, 210, 90, 30, 0, WM_CF_SHOW, ID2_BIN_OUT1, 0, 24);
    SPINBOX_SetEdge(hBIN2_OUT1, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hBIN2_OUT1, LIGHT_Ctrl2.Out1.index);

    hBUTTON_Next = BUTTON_Create(340, 180, 130, 30, ID_Next, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Next, "NEXT");

    hBUTTON_Ok = BUTTON_Create(340, 230, 130, 30, ID_Ok, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);

    GUI_GotoXY(110, 54);
    GUI_DispString("MAIN SW.");
    GUI_GotoXY(110, 66);
    GUI_DispString("RELAY1");

    GUI_GotoXY(110, 94);
    GUI_DispString("LED1 SW.");
    GUI_GotoXY(110, 106);
    GUI_DispString("RELAY1");

    GUI_GotoXY(110, 134);
    GUI_DispString("LED2 SW.");
    GUI_GotoXY(110, 146);
    GUI_DispString("RELAY1");

    GUI_GotoXY(110, 174);
    GUI_DispString("LED3 SW.");
    GUI_GotoXY(110, 186);
    GUI_DispString("RELAY1");

    GUI_GotoXY(110, 214);
    GUI_DispString("OUT1 SW.");
    GUI_GotoXY(110, 226);
    GUI_DispString("RELAY1");

    GUI_GotoXY(270, 54);
    GUI_DispString("MAIN SW.");
    GUI_GotoXY(270, 66);
    GUI_DispString("RELAY2");

    GUI_GotoXY(270, 94);
    GUI_DispString("LED1 SW.");
    GUI_GotoXY(270, 106);
    GUI_DispString("RELAY2");

    GUI_GotoXY(270, 134);
    GUI_DispString("LED2 SW.");
    GUI_GotoXY(270, 146);
    GUI_DispString("RELAY2");

    GUI_GotoXY(270, 174);
    GUI_DispString("LED3 SW.");
    GUI_GotoXY(270, 186);
    GUI_DispString("RELAY2");

    GUI_GotoXY(270, 214);
    GUI_DispString("OUT1 SW.");
    GUI_GotoXY(270, 226);
    GUI_DispString("RELAY2");

    GUI_GotoXY(440, 54);
    GUI_DispString("LIGHT1");
    GUI_GotoXY(440, 66);
    GUI_DispString("DIMMER");

    GUI_GotoXY(440, 94);
    GUI_DispString("LIGHT2");
    GUI_GotoXY(440, 106);
    GUI_DispString("DIMMER");

    GUI_GotoXY(440, 134);
    GUI_DispString("LIGHT3");
    GUI_GotoXY(440, 146);
    GUI_DispString("DIMMER");

    GUI_MULTIBUF_EndEx(1);
}
/**
  * @brief
  * @param
  * @retval
  */
static void DSP_KillSet2Scrn(void)
{
    WM_DeleteWindow(hBIN_MAIN1);
    WM_DeleteWindow(hBIN_LED1);
    WM_DeleteWindow(hBIN_LED2);
    WM_DeleteWindow(hBIN_LED3);
    WM_DeleteWindow(hBIN_OUT1);
    WM_DeleteWindow(hDIM_LIGHT1);
    WM_DeleteWindow(hDIM_LIGHT2);
    WM_DeleteWindow(hDIM_LIGHT3);
    WM_DeleteWindow(hBIN2_MAIN1);
    WM_DeleteWindow(hBIN2_LED1);
    WM_DeleteWindow(hBIN2_LED2);
    WM_DeleteWindow(hBIN2_LED3);
    WM_DeleteWindow(hBIN2_OUT1);
    WM_DeleteWindow(hBUTTON_Ok);
    WM_DeleteWindow(hBUTTON_Next);
}
/**
  * @brief
  * @param
  * @retval
  */
static void DSP_InitSet3Scrn(void)
{
    int i;
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);
    HAL_RTC_GetTime(&hrtc, &rtctm, RTC_FORMAT_BCD);
    HAL_RTC_GetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);
    hSPNBX_DisplayHighBrightness = SPINBOX_CreateEx(10, 20, 90, 30, 0, WM_CF_SHOW, ID_DisplayHighBrightness, 1, 90);
    SPINBOX_SetEdge(hSPNBX_DisplayHighBrightness, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_DisplayHighBrightness, high_bcklght);
    hSPNBX_DisplayLowBrightness = SPINBOX_CreateEx(10, 60, 90, 30, 0, WM_CF_SHOW, ID_DisplayLowBrightness, 1, 90);
    SPINBOX_SetEdge(hSPNBX_DisplayLowBrightness, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_DisplayLowBrightness, low_bcklght);
    hSPNBX_ScrnsvrTimeout = SPINBOX_CreateEx(10, 130, 90, 30, 0, WM_CF_SHOW, ID_ScrnsvrTimeout, 1, 240);
    SPINBOX_SetEdge(hSPNBX_ScrnsvrTimeout, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_ScrnsvrTimeout, scrnsvr_tout);
    hSPNBX_ScrnsvrEnableHour = SPINBOX_CreateEx(10, 170, 90, 30, 0, WM_CF_SHOW, ID_ScrnsvrEnableHour, 0, 23);
    SPINBOX_SetEdge(hSPNBX_ScrnsvrEnableHour, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_ScrnsvrEnableHour, scrnsvr_ena_hour);
    hSPNBX_ScrnsvrDisableHour = SPINBOX_CreateEx(10, 210, 90, 30, 0, WM_CF_SHOW, ID_ScrnsvrDisableHour, 0, 23);
    SPINBOX_SetEdge(hSPNBX_ScrnsvrDisableHour, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_ScrnsvrDisableHour, scrnsvr_dis_hour);
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
    hSPNBX_ScrnsvrClockColour = SPINBOX_CreateEx(340, 20, 90, 30, 0, WM_CF_SHOW, ID_ScrnsvrClkColour, 1, COLOR_BSIZE);
    SPINBOX_SetEdge(hSPNBX_ScrnsvrClockColour, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hSPNBX_ScrnsvrClockColour, scrnsvr_clk_clr);
    hCHKBX_ScrnsvrClock = CHECKBOX_Create(340, 70, 110, 20, 0, ID_ScrnsvrClock, WM_CF_SHOW);
    CHECKBOX_SetTextColor(hCHKBX_ScrnsvrClock, GUI_GREEN);
    CHECKBOX_SetText(hCHKBX_ScrnsvrClock, "SCREENSAVER");
    if(IsScrnsvrClkActiv()) CHECKBOX_SetState(hCHKBX_ScrnsvrClock, 1);
    else CHECKBOX_SetState(hCHKBX_ScrnsvrClock, 0);
    hDRPDN_WeekDay = DROPDOWN_CreateEx(340, 100, 130, 100, 0, WM_CF_SHOW, DROPDOWN_CF_AUTOSCROLLBAR, ID_WeekDay);
    for (i = 0; i < GUI_COUNTOF(_acContent); i++) {
        DROPDOWN_AddString(hDRPDN_WeekDay, *(_acContent + i));
    }
    DROPDOWN_SetSel(hDRPDN_WeekDay, rtcdt.WeekDay-1);
    hBUTTON_Next = BUTTON_Create(340, 180, 130, 30, ID_Next, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Next, "NEXT");
    hBUTTON_Ok = BUTTON_Create(340, 230, 130, 30, ID_Ok, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");
    GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);
    GUI_FillRect(340, 51, 430, 59);
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);
    /************************************/
    /* DISPLAY BACKLIGHT LED BRIGHTNESS */
    /************************************/
    GUI_DrawHLine   ( 15,   5, 160);
    GUI_GotoXY      ( 10,   5);
    GUI_DispString  ("DISPLAY BACKLIGHT");
    GUI_GotoXY      (110,  35);
    GUI_DispString  ("HIGH");
    GUI_GotoXY      (110,  75);
    GUI_DispString  ("LOW");
    GUI_DrawHLine   ( 15, 185, 320);
    GUI_GotoXY      (190,   5);
    /************************************/
    /*          SET        TIME         */
    /************************************/
    GUI_DispString  ("SET TIME");
    GUI_GotoXY      (290,  35);
    GUI_DispString  ("HOUR");
    GUI_GotoXY      (290,  75);
    GUI_DispString  ("MINUTE");
    GUI_DrawHLine   ( 15, 335, 475);
    /************************************/
    /*    SET SCREENSAVER CLOCK COOLOR  */
    /************************************/
    GUI_GotoXY      (340,   5);
    GUI_DispString  ("SET COLOR");
    GUI_GotoXY      (440, 26);
    GUI_DispString  ("FULL");
    GUI_GotoXY      (440, 38);
    GUI_DispString  ("CLOCK");
    /************************************/
    /*      SCREENSAVER     OPTION      */
    /************************************/
    GUI_DrawHLine   (125,   5, 160);
    GUI_GotoXY      ( 10, 115);
    GUI_DispString  ("SCREENSAVER OPTION");
    GUI_GotoXY      (110, 145);
    GUI_DispString  ("TIMEOUT");
    GUI_GotoXY      (110, 176);
    GUI_DispString  ("ENABLE");
    GUI_GotoXY      (110, 188);
    GUI_DispString  ("HOUR");
    GUI_GotoXY      (110, 216);
    GUI_DispString  ("DISABLE");
    GUI_GotoXY      (110, 228);
    GUI_DispString  ("HOUR");
    /************************************/
    /*          SET        DATE         */
    /************************************/
    GUI_DrawHLine   (125, 185, 320);
    GUI_GotoXY      (190, 115);
    GUI_DispString  ("SET DATE");
    GUI_GotoXY      (290, 145);
    GUI_DispString  ("DAY");
    GUI_GotoXY      (290, 185);
    GUI_DispString  ("MONTH");
    GUI_GotoXY      (290, 225);
    GUI_DispString  ("YEAR");
    GUI_MULTIBUF_EndEx(1);
}
/**
  * @brief
  * @param
  * @retval
  */
static void DSP_KillSet3Scrn(void)
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
  * @brief
  * @param
  * @retval
  */

static void DSP_InitSet4Scrn(void)
{
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    /*hVentilatorRelay = SPINBOX_CreateEx(10, 20, 110, 40, 0, WM_CF_SHOW, ID_VentilatorRelay, 0, 512);
    SPINBOX_SetEdge(hVentilatorRelay, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hVentilatorRelay, Ventilator_getRelay(&ventilator));

    hVentilatorDelayOn = SPINBOX_CreateEx(10, 70, 110, 40, 0, WM_CF_SHOW, ID_VentilatorDelayOn, 0, 100);
    SPINBOX_SetEdge(hVentilatorDelayOn, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hVentilatorDelayOn, Ventilator_getDelayOnTime(&ventilator));

    hVentilatorDelayOff = SPINBOX_CreateEx(10, 120, 110, 40, 0, WM_CF_SHOW, ID_VentilatorDelayOff, 1, 100);
    SPINBOX_SetEdge(hVentilatorDelayOff, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hVentilatorDelayOff, Ventilator_getDelayOffTime(&ventilator));



    hVentilatorUseDelayOn = CHECKBOX_Create(200, 80, 110, 20, 0, ID_VentilatorUseDelayOn, WM_CF_SHOW);
    CHECKBOX_SetTextColor(hVentilatorUseDelayOn, GUI_GREEN);
    CHECKBOX_SetText(hVentilatorUseDelayOn, "USE DELAY ON");
    CHECKBOX_SetState(hVentilatorUseDelayOn, Ventilator_getDelayOnUse(&ventilator));

    hVentilatorUseDelayOff = CHECKBOX_Create(200, 130, 110, 20, 0, ID_VentilatorUseDelayOff, WM_CF_SHOW);
    CHECKBOX_SetTextColor(hVentilatorUseDelayOff, GUI_GREEN);
    CHECKBOX_SetText(hVentilatorUseDelayOff, "USE DELAY OFF");
    CHECKBOX_SetState(hVentilatorUseDelayOff, Ventilator_getDelayOffUse(&ventilator));*/




    hBUTTON_Next = BUTTON_Create(410, 180, 60, 30, ID_Next, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Next, "NEXT");

    hBUTTON_Ok = BUTTON_Create(410, 230, 60, 30, ID_Ok, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    GUI_MULTIBUF_EndEx(1);
}

/**
  * @brief
  * @param
  * @retval
  */
static void DSP_KillSet4Scrn(void)
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
  * @brief
  * @param
  * @retval
  */

static void DSP_InitSet5Scrn(void)
{
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    int x = 10, y = 20;

    for(uint8_t i = curtainSettingMenu * 4; i < (((CURTAINS_SIZE - (curtainSettingMenu * 4)) >= 4) ? ((curtainSettingMenu * 4) + 4) : CURTAINS_SIZE); i++)
    {
        hCurtainsRelay[i * 2] = SPINBOX_CreateEx(x, y, 110, 40, 0, WM_CF_SHOW, ID_CurtainsRelay + (i * 2), 0, 512);
        SPINBOX_SetEdge(hCurtainsRelay[i * 2], SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(hCurtainsRelay[i * 2], Curtain_GetRelayUp(curtains + i));

        hCurtainsRelay[(i * 2) + 1] = SPINBOX_CreateEx(x, y + 50, 110, 40, 0, WM_CF_SHOW, ID_CurtainsRelay + (i * 2) + 1, 0, 512);
        SPINBOX_SetEdge(hCurtainsRelay[(i * 2) + 1], SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(hCurtainsRelay[(i * 2) + 1], Curtain_GetRelayDown(curtains + i));



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

        if((i % 4) == 1)
        {
            x = 200;
            y = 20;
        }
        else y += 50 * 2;
    }

    hBUTTON_Next = BUTTON_Create(410, 180, 60, 30, ID_Next, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Next, "NEXT");

    hBUTTON_Ok = BUTTON_Create(410, 230, 60, 30, ID_Ok, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    GUI_MULTIBUF_EndEx(1);
}

/**
  * @brief
  * @param
  * @retval
  */
static void DSP_KillSet5Scrn(void)
{
//    for(uint8_t i = curtainSettingMenu * 4; i < (((CURTAINS_SIZE - (curtainSettingMenu * 4)) >= 4) ? 4 : (CURTAINS_SIZE - (curtainSettingMenu * 4))); i++)
    for(uint8_t i = curtainSettingMenu * 4; i < (((CURTAINS_SIZE - (curtainSettingMenu * 4)) >= 4) ? ((curtainSettingMenu * 4) + 4) : CURTAINS_SIZE); i++)
    {
        WM_DeleteWindow(hCurtainsRelay[i * 2]);
        WM_DeleteWindow(hCurtainsRelay[(i * 2) + 1]);
    }
    WM_DeleteWindow(hBUTTON_Next);
    WM_DeleteWindow(hBUTTON_Ok);
}


/**
  * @brief
  * @param
  * @retval
  */

static void DSP_InitSet6Scrn(void)
{
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    int x = 10, y = 5;

    for(uint8_t i = lightsModbusSettingsMenu * LIGHTS_MODBUS_PER_SETTINGS; i < (((LIGHTS_MODBUS_SIZE - (lightsModbusSettingsMenu * LIGHTS_MODBUS_PER_SETTINGS)) >= LIGHTS_MODBUS_PER_SETTINGS) ? ((lightsModbusSettingsMenu * LIGHTS_MODBUS_PER_SETTINGS) + LIGHTS_MODBUS_PER_SETTINGS) : LIGHTS_MODBUS_SIZE); i++)
    {
        /*hLightsModbusRelay[i] = SPINBOX_CreateEx(x, y, 90, 30, 0, WM_CF_SHOW, ID_LightsModbusRelay + i, 0, 512);
        SPINBOX_SetEdge(hLightsModbusRelay[i], SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(hLightsModbusRelay[i], Light_Modbus_GetRelay(lights_modbus + i));

        hLightsModbusOffTime[i] = SPINBOX_CreateEx(x, y + 33, 90, 30, 0, WM_CF_SHOW, ID_LightsModbusOffTime + i, 0, 9);
        SPINBOX_SetEdge(hLightsModbusOffTime[i], SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(hLightsModbusOffTime[i], Light_Modbus_GetOffTime(lights_modbus + i));

        hLightsModbusTiedToMainLight[i] = CHECKBOX_Create(x, y + 66, 130, 15, 0, ID_LightModbusTiedToMainLight + i, WM_CF_SHOW);
        CHECKBOX_SetTextColor(hLightsModbusTiedToMainLight[i], GUI_GREEN);
        CHECKBOX_SetText(hLightsModbusTiedToMainLight[i], "TIED TO MAIN LIGHT");
        CHECKBOX_SetState(hLightsModbusTiedToMainLight[i], Light_Modbus_isTiedToMainLight(lights_modbus + i));*/



        lightsWidgets[i].relay = SPINBOX_CreateEx(x, y, 100, 40, 0, WM_CF_SHOW, ID_LightsModbusRelay * i, 0, 512);
        SPINBOX_SetEdge(lightsWidgets[i].relay, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].relay, Light_Modbus_GetRelay(lights_modbus + i));

        lightsWidgets[i].iconID = SPINBOX_CreateEx(x, y + 43, 100, 40, 0, WM_CF_SHOW, (ID_LightsModbusRelay * i) + 1, 0, LIGHT_ICON_COUNT - 1);
        SPINBOX_SetEdge(lightsWidgets[i].iconID, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].iconID, lights_modbus[i].iconID);

        lightsWidgets[i].controllerID_on = SPINBOX_CreateEx(x, y + 86, 100, 40, 0, WM_CF_SHOW, (ID_LightsModbusRelay * i) + 2, 0, 512);
        SPINBOX_SetEdge(lightsWidgets[i].controllerID_on, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].controllerID_on, lights_modbus[i].controllerID_on);

        lightsWidgets[i].controllerID_on_delay = SPINBOX_CreateEx(x, y + 129, 100, 40, 0, WM_CF_SHOW, (ID_LightsModbusRelay * i) + 3, 0, 512);
        SPINBOX_SetEdge(lightsWidgets[i].controllerID_on_delay, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].controllerID_on_delay, lights_modbus[i].controllerID_on_delay);

        lightsWidgets[i].on_hour = SPINBOX_CreateEx(x, y + 172, 100, 40, 0, WM_CF_SHOW, (ID_LightsModbusRelay * i) + 4, 0, 23);
        SPINBOX_SetEdge(lightsWidgets[i].on_hour, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].on_hour, lights_modbus[i].on_hour);

        lightsWidgets[i].on_minute = SPINBOX_CreateEx(x, y + 215, 100, 40, 0, WM_CF_SHOW, (ID_LightsModbusRelay * i) + 5, 0, 59);
        SPINBOX_SetEdge(lightsWidgets[i].on_minute, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].on_minute, lights_modbus[i].on_minute);

        GUI_SetColor(GUI_WHITE);
        GUI_SetFont(GUI_FONT_13_1);
        GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);

        GUI_GotoXY(x + 100 + 10, y + 10);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 100 + 10, y + 10 + 12);
        GUI_DispString("RELAY");

        GUI_GotoXY(x + 100 + 10, y + 43 + 10);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 100 + 10, y + 43 + 10 + 12);
        GUI_DispString("ICON");

        GUI_GotoXY(x + 100 + 10, y + 86 + 10);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 100 + 10, y + 86 + 10 + 12);
        GUI_DispString("ON ID");

        GUI_GotoXY(x + 100 + 10, y + 129 + 10);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 100 + 10, y + 129 + 10 + 12);
        GUI_DispString("ON ID DELAY");

        GUI_GotoXY(x + 100 + 10, y + 172 + 10);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 100 + 10, y + 172 + 10 + 12);
        GUI_DispString("HOUR ON");

        GUI_GotoXY(x + 100 + 10, y + 215 + 10);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 100 + 10, y + 215 + 10 + 12);
        GUI_DispString("MINUTE ON");

        x = 200;
        y = 5;

        lightsWidgets[i].offTime = SPINBOX_CreateEx(x, y, 100, 40, 0, WM_CF_SHOW, (ID_LightsModbusRelay * i) + 6, 0, 254);
        SPINBOX_SetEdge(lightsWidgets[i].offTime, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].offTime, Light_Modbus_GetOffTime(lights_modbus + i));

        lightsWidgets[i].communication_type = SPINBOX_CreateEx(x, y + 43, 100, 40, 0, WM_CF_SHOW, (ID_LightsModbusRelay * i) + 7, 1, 3);
        SPINBOX_SetEdge(lightsWidgets[i].communication_type, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].communication_type, lights_modbus[i].communication_type);

        lightsWidgets[i].local_pin = SPINBOX_CreateEx(x, y + 86, 100, 40, 0, WM_CF_SHOW, (ID_LightsModbusRelay * i) + 8, 0, 6);
        SPINBOX_SetEdge(lightsWidgets[i].local_pin, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].local_pin, lights_modbus[i].local_pin);

        lightsWidgets[i].sleep_time = SPINBOX_CreateEx(x, y + 129, 100, 40, 0, WM_CF_SHOW, (ID_LightsModbusRelay * i) + 9, 0, 254);
        SPINBOX_SetEdge(lightsWidgets[i].sleep_time, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].sleep_time, lights_modbus[i].sleep_time);

        lightsWidgets[i].button_external = SPINBOX_CreateEx(x, y + 172, 100, 40, 0, WM_CF_SHOW, (ID_LightsModbusRelay * i) + 10, 0, 4);
        SPINBOX_SetEdge(lightsWidgets[i].button_external, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].button_external, lights_modbus[i].button_external);

        lightsWidgets[i].tiedToMainLight = CHECKBOX_Create(x, y + 215, 130, 20, 0, (ID_LightsModbusRelay * i) + 11, WM_CF_SHOW);
        CHECKBOX_SetTextColor(lightsWidgets[i].tiedToMainLight, GUI_GREEN);
        CHECKBOX_SetText(lightsWidgets[i].tiedToMainLight, "TIED TO MAIN LIGHT");
        CHECKBOX_SetState(lightsWidgets[i].tiedToMainLight, Light_Modbus_isTiedToMainLight(lights_modbus + i));

        lightsWidgets[i].rememberBrightness = CHECKBOX_Create(x, y + 238, 145, 20, 0, (ID_LightsModbusRelay * i) + 12, WM_CF_SHOW);
        CHECKBOX_SetTextColor(lightsWidgets[i].rememberBrightness, GUI_GREEN);
        CHECKBOX_SetText(lightsWidgets[i].rememberBrightness, "REMEMBER BRIGHTNESS");
        CHECKBOX_SetState(lightsWidgets[i].rememberBrightness, Light_Modbus_isBrightnessRemembered(lights_modbus + i));

        GUI_GotoXY(x + 100 + 10, y + 10);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 100 + 10, y + 10 + 12);
        GUI_DispString("DELAY OFF");

        GUI_GotoXY(x + 100 + 10, y + 43 + 10);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 100 + 10, y + 43 + 10 + 12);
        GUI_DispString("COMM. TYPE");

        GUI_GotoXY(x + 100 + 10, y + 86 + 10);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 100 + 10, y + 86 + 10 + 12);
        GUI_DispString("LOCAL PIN");

        GUI_GotoXY(x + 100 + 10, y + 129 + 10);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 100 + 10, y + 129 + 10 + 12);
        GUI_DispString("SLEEP TIME");

        GUI_GotoXY(x + 100 + 10, y + 172 + 10);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 100 + 10, y + 172 + 10 + 12);
        GUI_DispString("BUTTON EXT.");






        /*lightsWidgets[i].offTime = SPINBOX_CreateEx(x, y + 33, 90, 30, 0, WM_CF_SHOW, (ID_LightsModbusRelay * i) + 1, 0, 9);
        SPINBOX_SetEdge(lightsWidgets[i].offTime, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].offTime, Light_Modbus_GetOffTime(lights_modbus + i));

        lightsWidgets[i].tiedToMainLight = CHECKBOX_Create(x, y + 66, 130, 15, 0, (ID_LightsModbusRelay * i) + 2, WM_CF_SHOW);
        CHECKBOX_SetTextColor(lightsWidgets[i].tiedToMainLight, GUI_GREEN);
        CHECKBOX_SetText(lightsWidgets[i].tiedToMainLight, "TIED TO MAIN LIGHT");
        CHECKBOX_SetState(lightsWidgets[i].tiedToMainLight, Light_Modbus_isTiedToMainLight(lights_modbus + i));*/

//        hCurtainsRelay[(i * 2) + 1] = SPINBOX_CreateEx(x, y + 50, 110, 40, 0, WM_CF_SHOW, ID_CurtainsRelay + (i * 2) + 1, 0, 512);
//        SPINBOX_SetEdge(hCurtainsRelay[(i * 2) + 1], SPINBOX_EDGE_CENTER);
//        SPINBOX_SetValue(hCurtainsRelay[(i * 2) + 1], Curtain_GetRelayDown(curtains + i));



        /*GUI_SetColor(GUI_WHITE);
        GUI_SetFont(GUI_FONT_13_1);
        GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);

        GUI_GotoXY(x + 90 + 10, y + 8);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 90 + 10, y + 8 + 12);
        GUI_DispString("RELAY");

        GUI_GotoXY(x + 90 + 10, y + 33 + 8);
        GUI_DispString("FAN ON");
        GUI_GotoXY(x + 90 + 10, y + 33 + 8 + 12);
        GUI_DispString("TIME");*/

//        GUI_GotoXY(x + 110 + 10, y + 50 + 8);
//        GUI_DispString("CURTAIN ");
//        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
//        GUI_GotoXY(x + 110 + 10, y + 50 + 8 + 12);
//        GUI_DispString("RELAY DOWN");

        /*if((i % LIGHTS_MODBUS_PER_SETTINGS) == 2)
        {
            x = 200;
            y = 5;
        }
        else y += 43 * 2;*/
    }

    hBUTTON_Next = BUTTON_Create(410, 180, 60, 30, ID_Next, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Next, "NEXT");

    hBUTTON_Ok = BUTTON_Create(410, 230, 60, 30, ID_Ok, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    GUI_MULTIBUF_EndEx(1);
}

/**
  * @brief
  * @param
  * @retval
  */
static void DSP_KillSet6Scrn(void)
{
    for(uint8_t i = lightsModbusSettingsMenu * LIGHTS_MODBUS_PER_SETTINGS; i < (((LIGHTS_MODBUS_SIZE - (lightsModbusSettingsMenu * LIGHTS_MODBUS_PER_SETTINGS)) >= LIGHTS_MODBUS_PER_SETTINGS) ? ((lightsModbusSettingsMenu * LIGHTS_MODBUS_PER_SETTINGS) + LIGHTS_MODBUS_PER_SETTINGS) : LIGHTS_MODBUS_SIZE); i++)
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
  * @brief
  * @param
  * @retval
  */

static void DSP_InitSet7Scrn(void)
{
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);


    hDEV_ID = SPINBOX_CreateEx(10, 10, 110, 40, 0, WM_CF_SHOW, ID_DEV_ID, 1, 254);
    SPINBOX_SetEdge(hDEV_ID, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hDEV_ID, tfifa);

    hCurtainsMoveTime = SPINBOX_CreateEx(10, 60, 110, 40, 0, WM_CF_SHOW, ID_CurtainsMoveTime, 0, 60);
    SPINBOX_SetEdge(hCurtainsMoveTime, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hCurtainsMoveTime, Curtain_GetMoveTime());

    hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH = CHECKBOX_Create(10, 110, 205, 20, 0, ID_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, WM_CF_SHOW);
    CHECKBOX_SetTextColor(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, GUI_GREEN);
    CHECKBOX_SetText(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, "ONLY LEAVE SCRNSVR AFTER TOUCH");
    CHECKBOX_SetState(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, bOnlyLeaveScreenSaverAfterTouch);

    hCHKBX_LIGHT_NIGHT_TIMER = CHECKBOX_Create(10, 140, 170, 20, 0, ID_LIGHT_NIGHT_TIMER, WM_CF_SHOW);
    CHECKBOX_SetTextColor(hCHKBX_LIGHT_NIGHT_TIMER, GUI_GREEN);
    CHECKBOX_SetText(hCHKBX_LIGHT_NIGHT_TIMER, "LiGHT OFF TIMER AFTER 20h");
    CHECKBOX_SetState(hCHKBX_LIGHT_NIGHT_TIMER, LightNightTimer_isEnabled);





    hBUTTON_SET_DEFAULTS = BUTTON_Create(10, 190, 80, 30, ID_SET_DEFAULTS, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_SET_DEFAULTS, "SET DEFAULTS");

    hBUTTON_SYSRESTART = BUTTON_Create(10, 230, 80, 30, ID_SYSRESTART, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_SYSRESTART, "RESTART");



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



    hBUTTON_Next = BUTTON_Create(410, 180, 60, 30, ID_Next, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Next, "NEXT");

    hBUTTON_Ok = BUTTON_Create(410, 230, 60, 30, ID_Ok, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    GUI_MULTIBUF_EndEx(1);
}

/**
  * @brief
  * @param
  * @retval
  */
static void DSP_KillSet7Scrn(void)
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




static void DSP_InitSet8Scrn(void)
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



    defroster_settingWidgets.cycleTime = SPINBOX_CreateEx(10, 10, 110, 40, 0, WM_CF_SHOW, ID_DEFROSTER_CYCLE_TIME, 0, 254);
    SPINBOX_SetEdge(defroster_settingWidgets.cycleTime, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(defroster_settingWidgets.cycleTime, defroster.cycleTime);

    GUI_GotoXY(130, 20);
    GUI_DispString("DEFROSTER");
    GUI_GotoXY(130, 32);
    GUI_DispString("CYCLE TIME");




    defroster_settingWidgets.activeTime = SPINBOX_CreateEx(10, 60, 110, 40, 0, WM_CF_SHOW, ID_DEFROSTER_ACTIVE_TIME, 0, 254);
    SPINBOX_SetEdge(defroster_settingWidgets.activeTime, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(defroster_settingWidgets.activeTime, defroster.activeTime);

    GUI_GotoXY(130, 70);
    GUI_DispString("DEFROSTER");
    GUI_GotoXY(130, 82);
    GUI_DispString("ACTIVE TIME");



    defroster_settingWidgets.pin = SPINBOX_CreateEx(10, 110, 110, 40, 0, WM_CF_SHOW, ID_DEFROSTER_PIN, 0, 6);
    SPINBOX_SetEdge(defroster_settingWidgets.pin, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(defroster_settingWidgets.pin, defroster.pin);

    GUI_GotoXY(130, 120);
    GUI_DispString("DEFROSTER");
    GUI_GotoXY(130, 132);
    GUI_DispString("PIN");








    hBUTTON_Next = BUTTON_Create(410, 180, 60, 30, ID_Next, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Next, "NEXT");

    hBUTTON_Ok = BUTTON_Create(410, 230, 60, 30, ID_Ok, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");

    GUI_MULTIBUF_EndEx(1);
}

static void DSP_KillSet8Scrn(void)
{
    WM_DeleteWindow(defroster_settingWidgets.cycleTime);
    WM_DeleteWindow(defroster_settingWidgets.activeTime);
    WM_DeleteWindow(defroster_settingWidgets.pin);
    WM_DeleteWindow(hBUTTON_Next);
    WM_DeleteWindow(hBUTTON_Ok);
}



/**
  * @brief  Display Backlight LED brightnes control
  * @param  brightnes_high_level
  * @retval DISP sreensvr_tmr loaded with system_tmr value
  */
void DISPSetBrightnes(uint8_t val)
{
    if      (val < DISP_BRGHT_MIN) val = DISP_BRGHT_MIN;
    else if (val > DISP_BRGHT_MAX) val = DISP_BRGHT_MAX;
    __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, (uint16_t) (val * 10U));
}
/**
  * @brief  Display Date and Time in deifferent font size colour and position
  * @param  Flags: IsRtcTimeValid, IsScrnsvrActiv, BUTTON_Dnd, BUTTON_BTNMaid,
            BTNSosReset, IsScrnsvrSemiClkActiv, IsScrnsvrClkActiv
  * @retval None
  */
static void DISPDateTime(void)
{
    char dbuf[32];
    static uint8_t old_day = 0;
    if(!IsRtcTimeValid()) return; // nothing to display untill system rtc validated
    HAL_RTC_GetTime(&hrtc, &rtctm, RTC_FORMAT_BCD);
    HAL_RTC_GetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);
    /************************************/
    /*   CHECK IS SCREENSAVER ENABLED   */
    /************************************/
    if(scrnsvr_ena_hour>=scrnsvr_dis_hour) {
        if      (Bcd2Dec(rtctm.Hours)>=scrnsvr_ena_hour) ScrnsvrEnable();
        else if (Bcd2Dec(rtctm.Hours)<scrnsvr_dis_hour) ScrnsvrEnable();
        else if (IsScrnsvrEnabled())ScrnsvrDisable(), screen = SCREEN_RETURN_TO_FIRST;
    } else if(scrnsvr_ena_hour<scrnsvr_dis_hour) {
        if((Bcd2Dec(rtctm.Hours<scrnsvr_dis_hour))&&(Bcd2Dec(rtctm.Hours)>=scrnsvr_ena_hour)) ScrnsvrEnable();
        else if(IsScrnsvrEnabled()) ScrnsvrDisable(), screen = SCREEN_RETURN_TO_FIRST;
    }
    /************************************/
    /*      DISPLAY  DATE  &  TIME      */
    /************************************/
    if (IsScrnsvrActiv()&&IsScrnsvrEnabled()&&IsScrnsvrClkActiv()) {
        if(!IsScrnsvrInitActiv()||(old_day!=rtcdt.WeekDay)) {
            ScrnsvrInitSet();
            GUI_MULTIBUF_BeginEx(0);
            GUI_SelectLayer(0);
            GUI_Clear();
            GUI_MULTIBUF_EndEx(0);
            GUI_MULTIBUF_BeginEx(1);
            GUI_SelectLayer(1);
            GUI_SetBkColor(GUI_TRANSPARENT);
            GUI_Clear();
            old_min=60U;
            old_day=rtcdt.WeekDay;
            GUI_SetPenSize(9);
            GUI_SetColor(GUI_GREEN);
//            GUI_DrawLine(400,20,450,20);
//            GUI_DrawLine(400,40,450,40);
//            GUI_DrawLine(400,60,450,60);
            GUI_MULTIBUF_EndEx(1);
        }
        HEX2STR(dbuf,&rtctm.Hours);
        if(rtctm.Seconds&1) dbuf[2]=':';
        else dbuf[2]=' ';
        HEX2STR(&dbuf[3],&rtctm.Minutes);
        GUI_GotoXY(CLOCK_H_POS, CLOCK_V_POS);
        GUI_SetColor(clk_clrs[scrnsvr_clk_clr]);
        GUI_SetFont(GUI_FONT_D80);
        GUI_SetTextAlign(GUI_TA_HCENTER|GUI_TA_VCENTER);
        GUI_MULTIBUF_BeginEx(1);
        GUI_ClearRect(0,80,480,192);
        GUI_ClearRect(0,220,100,270);
        GUI_DispString(dbuf);
        if(rtcdt.WeekDay==0) rtcdt.WeekDay=7;
        if(rtcdt.WeekDay==1) memcpy(dbuf,"  Monday  ",10);
        else if(rtcdt.WeekDay==2) memcpy(dbuf," Tuestday ",10);
        else if(rtcdt.WeekDay==3) memcpy(dbuf,"Wednesday ",10);
        else if(rtcdt.WeekDay==4) memcpy(dbuf,"Thurstday ",10);
        else if(rtcdt.WeekDay==5) memcpy(dbuf,"  Friday  ",10);
        else if(rtcdt.WeekDay==6) memcpy(dbuf," Saturday ",10);
        else if(rtcdt.WeekDay==7) memcpy(dbuf,"  Sunday  ",10);
        HEX2STR(&dbuf[10],&rtcdt.Date);
        if(rtcdt.Month==1) memcpy(&dbuf[12],". January ",10);
        else if(rtcdt.Month==2) memcpy(&dbuf[12],". February",10);
        else if(rtcdt.Month==3) memcpy(&dbuf[12],".  March  ",10);
        else if(rtcdt.Month==4) memcpy(&dbuf[12],".  April  ",10);
        else if(rtcdt.Month==5) memcpy(&dbuf[12],".   May   ",10);
        else if(rtcdt.Month==6) memcpy(&dbuf[12],".   June  ",10);
        else if(rtcdt.Month==7) memcpy(&dbuf[12],".   July  ",10);
        else if(rtcdt.Month==8) memcpy(&dbuf[12],". August  ",10);
        else if(rtcdt.Month==9) memcpy(&dbuf[12],".September",10);
        else if(rtcdt.Month==0x10) memcpy(&dbuf[12],". October ",10);
        else if(rtcdt.Month==0x11) memcpy(&dbuf[12],". November",10);
        else if(rtcdt.Month==0x12) memcpy(&dbuf[12],". December",10);
        memcpy(&dbuf[22]," 20",3U);
        HEX2STR(&dbuf[25],&rtcdt.Year);
        dbuf[27]='.';
        dbuf[28]=NUL;
        GUI_SetFont(GUI_FONT_24B_1);
        GUI_GotoXY(CLOCK_H_POS,CLOCK_V_POS+70);
        GUI_SetTextAlign(GUI_TA_HCENTER|GUI_TA_VCENTER);
        GUI_DispString(dbuf);
        GUI_MULTIBUF_EndEx(1);
    } else if(old_min != rtctm.Minutes) {
        old_min = rtctm.Minutes;
        HEX2STR(dbuf,&rtctm.Hours);
        dbuf[2]=':';
        HEX2STR(&dbuf[3],&rtctm.Minutes);
        GUI_SetFont(GUI_FONT_32_1);
        GUI_SetColor(GUI_WHITE);
        GUI_SetTextMode(GUI_TM_TRANS);
        GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);
        GUI_MULTIBUF_BeginEx(1);
        GUI_GotoXY(5,245);
        GUI_ClearRect(0,220,100,270);
        GUI_DispString(dbuf);
        GUI_MULTIBUF_EndEx(1);
    }

    if (old_day != rtcdt.WeekDay) {
        old_day  = rtcdt.WeekDay;
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, rtcdt.Date);
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR3, rtcdt.Month);
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR4, rtcdt.WeekDay);
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR5, rtcdt.Year);
    }
}
/**
  * @brief  Display Backlight LED brightnes control
  * @param  brightnes_high_level
  * @retval DISP sreensvr_tmr loaded with system_tmr value
  */
void DISPResetScrnsvr(void)
{
    if(IsScrnsvrActiv() && IsScrnsvrEnabled()) screen = SCREEN_RETURN_TO_FIRST;
    ScrnsvrReset();
    ScrnsvrInitReset();
    scrnsvr_tmr = HAL_GetTick();
    scrnsvr_tout = SCRNSVR_TOUT;
    DISPSetBrightnes(high_bcklght);
}
/**
  * @brief
  * @param
  * @retval
  */
void DISPSetPoint(void)
{
    GUI_MULTIBUF_BeginEx(1);
    GUI_ClearRect(SP_H_POS - 5, SP_V_POS - 5, SP_H_POS + 120, SP_V_POS + 85);
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_D48);
    GUI_SetTextMode(GUI_TM_NORMAL);
    GUI_SetTextAlign(GUI_TA_RIGHT);
    GUI_GotoXY(SP_H_POS, SP_V_POS);
    GUI_DispDec(thst.sp_temp , 2);
    GUI_MULTIBUF_EndEx(1);
}
/**
  * @brief
  * @param
  * @retval
  */
void PID_Hook(GUI_PID_STATE * pTS)
{
    static uint8_t release = 0;
    uint8_t click = 0;
    
    if (pTS->Pressed  == 1U)
    {
        pTS->Layer = 1U;
        release = 1;
        

        if ((pTS->x > 400) && (pTS->y < 60) && ((screen <= SCREEN_CONTROL_SELECT) || (screen == SCREEN_THERMOSTAT) || ((screen > SCREEN_SETTINGS_6) && (screen != SCREEN_SETTINGS_7)))) {
            click = 1;
            if      (screen  < SCREEN_CONTROL_SELECT) screen = SCREEN_CONTROL_SELECT;
            else if ((screen == SCREEN_CONTROL_SELECT) || (screen == SCREEN_THERMOSTAT) || ((screen > SCREEN_SETTINGS_6) && (screen != SCREEN_SETTINGS_7))) screen = SCREEN_RETURN_TO_FIRST;
        }
        else if(screen == SCREEN_CONTROL_SELECT)
        {
            if(pTS->x < 400)
            {
                if(pTS->y < 136)
                {
                    if(pTS->x < 190)
                    {
                        screen = SCREEN_LIGHTS;
                        shouldDrawScreen = 1;
                    }
                    else
                    {
                        screen = SCREEN_THERMOSTAT;
                    }
                }
                else
                {
                    if(pTS->x < 190)
                    {
                        screen = SCREEN_CURTAINS;
                        Curtain_ResetSelection();
                        shouldDrawScreen = 1;
                    }
                    else
                    {
                        /*if(language == (LANGUAGES_NUM - 1))
                        {
                            language = BOS;
                        }
                        else
                        {
                            language++;
                        }

                        menu_lc = 0;*/


                        if(Defroster_isActive())
                        {
                            Defroster_Off();
                        }
                        else
                        {
                            Defroster_On();
                        }
                        ctrl1 = 1;
                    }
                }
            }
            else if(pTS->y > 159)
            {
                shouldDrawScreen = 1;
                menu_lc = 0;
                screen = SCREEN_SELECT_SCREEN_2;
            }

            if(screen != SCREEN_CONTROL_SELECT) click = 1;
        }
        else if(screen == SCREEN_THERMOSTAT)
        {
            if((pTS->x > BTN_INC_X0)&&(pTS->y > BTN_INC_Y0)&&(pTS->x < BTN_INC_X1)&&(pTS->y < BTN_INC_Y1))
            {
                click = 1;
                btninc = 1;
            }
            else if((pTS->x > BTN_DEC_X0)&&(pTS->y > BTN_DEC_Y0)&&(pTS->x < BTN_DEC_X1)&&(pTS->y < BTN_DEC_Y1)) {
                click = 1;
                btndec = 1;
            }
            else if((pTS->x > 400) && (pTS->y > 150) && (pTS->y < 190))
            {
                click = 1;
                thermostatOnOffTouch_timer = HAL_GetTick();
                if(!thermostatOnOffTouch_timer) thermostatOnOffTouch_timer = 1;
            }
        }
        else if(screen == SCREEN_LIGHTS) //lights
        {
            light_selectedIndex = LIGHTS_MODBUS_SIZE + 1; // clear selected light

            int y = (Lights_Modbus_Rows_getCount() > 1) ? 10 : 86;
            uint8_t lightsInRowSum = 0;

            for(uint8_t row = 0; row < Lights_Modbus_Rows_getCount(); ++row)
            {
                uint8_t lightsInRow = Lights_Modbus_getCount();

                if(Lights_Modbus_getCount() > 3)
                {
                    if(Lights_Modbus_getCount() == 4) lightsInRow = 2;
                    else if(Lights_Modbus_getCount() == 5) lightsInRow = (row > 0) ? 2 : 3;
                    else lightsInRow = 3;
                }

                uint8_t lightsMenuSpaceBetween = (400 - (80 * lightsInRow)) / (lightsInRow - 1 + 2);

                for(uint8_t i = 0; i < lightsInRow; ++i)
                {
                    int x = (lightsMenuSpaceBetween * ((i % lightsInRow) + 1)) + (80 * (i % lightsInRow));

                    if((pTS->x > x) && (pTS->x < (x + 80)) && (pTS->y > y) && (pTS->y < (y + 120)))
                    {
                        click = 1;
                        //Light_Modbus_Flip(lights_modbus + lightsInRowSum + i);
                        light_selectedIndex = lightsInRowSum + i;
                        if(!Light_Modbus_isBinary(lights_modbus + light_selectedIndex))
                        {
                            light_settingsTimerStart = HAL_GetTick();
                        }
                        LightNightTimer_StartTime = 0;
                        break;
                    }
                }

                lightsInRowSum += lightsInRow;
                y += 130;
            }





            /*for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; i++)
            {
                int x = (lightsMenuSpaceBetween * ((i % LIGHTS_MODBUS_PER_ROW) + 1)) + (80 * (i % LIGHTS_MODBUS_PER_ROW));

                if((pTS->x > x) && (pTS->x < (x + 80)))
                {
                    click = 1;
                    Light_Modbus_Flip(lights_modbus + ((pTS->y > 136) ? LIGHTS_MODBUS_PER_ROW + i : i));
                    break;
                }
            }*/
        }
        else if(screen == SCREEN_CURTAINS) //curtains
        {
            if(pTS->x < 400)
            {
                if((pTS->x > (200 - (curtainMenuCurtainLength / 2))) && (pTS->x < (200 + (curtainMenuCurtainLength / 2))))
                {
                    if(Curtain_areAllSelected())
                    {
                        Curtains_MoveSignal((pTS->y < 136) ? CURTAIN_UP : CURTAIN_DOWN);
                    }
                    else
                    {
                        Curtain_MoveSignal(curtains + Curtain_getSelected(), (pTS->y < 136) ? CURTAIN_UP : CURTAIN_DOWN);
                    }

                    click = 1;
                }
                else if((Curtains_getCount() > 1) && (pTS->y > 192))
                {
                    if(pTS->x > 320)
                    {
                        if(Curtain_areAllSelected())
                        {
                            Curtain_Select(0);
                        }
                        else
                        {
                            Curtain_Select(Curtain_getSelected() + 1);
                        }

                        shouldDrawScreen = 1;
                        click = 1;
                    }
                    else if(pTS->x < 80)
                    {
                        if(!Curtain_getSelected())
                        {
                            Curtain_Select(Curtains_getCount());
                        }
                        else
                        {
                            Curtain_Select(Curtain_getSelected() - 1);
                        }

                        shouldDrawScreen = 1;
                        click = 1;
                    }
                }
            }
        }
        else if(screen == SCREEN_SELECT_SCREEN_2)   // select screen 2
        {
            if(pTS->x < 400)
            {
                if((pTS->y > 116) && (pTS->y < 216))
                {
                    if(pTS->x < 126)
                    {
                        screen = SCREEN_CLEAN;
                    }
                    else if(pTS->x < 252)
                    {
                        screen = SCREEN_QR_CODE;
                        qr_code_draw_id = QR_CODE_WIFI_ID;
                        shouldDrawScreen = 1;
                    }
                    else
                    {
                        screen = SCREEN_QR_CODE;
                        qr_code_draw_id = QR_CODE_APP_ID;
                        shouldDrawScreen = 1;
                    }
                }
            }
            else if(pTS->y > 159)
            {
                screen = SCREEN_CONTROL_SELECT;
                menu_lc = 0;
                shouldDrawScreen = 1;
            }
        }
        else if(screen == SCREEN_LIGHT_SETTINGS)  // light settings
        {
            uint8_t brightness = 255;
            GUI_COLOR color = 0;
            
            if((pTS->x < 80) && (pTS->y < 80))
            {
                screen = SCREEN_LIGHTS;
                shouldDrawScreen = true;
                light_selectedIndex = LIGHTS_MODBUS_SIZE + 1;
                lights_allSelected_hasRGB = false;
            }
            else
            {
                if((((light_selectedIndex == LIGHTS_MODBUS_SIZE) && (lights_allSelected_hasRGB)) || Light_Modbus_isRGB(lights_modbus + light_selectedIndex)) && (pTS->x >= 200) && (pTS->x <= 280) && (pTS->y >= 20) && (pTS->y <= 100))
                {
                    color = GUI_WHITE;
                }
                else if((pTS->x >= 20) && (pTS->x <= 460))
                {
                    if((pTS->y >= 110) && (pTS->y <= 170))      brightness = ((pTS->x - 20) / (float)bmblackWhiteGradient.XSize) * 100;
                    else if((((light_selectedIndex == LIGHTS_MODBUS_SIZE) && (lights_allSelected_hasRGB)) || Light_Modbus_isRGB(lights_modbus + light_selectedIndex)) && (pTS->y >= 180) && (pTS->y <= 240))     color = LCD_GetPixelColor(pTS->x, pTS->y);
                    shouldDrawScreen = 1;
                }


                if(light_selectedIndex == LIGHTS_MODBUS_SIZE)
                {
                    for(uint8_t i = 0; i < Lights_Modbus_getCount(); i++)
                    {
                        if(Light_Modbus_isTiedToMainLight(lights_modbus + i) && (!Light_Modbus_isBinary(lights_modbus + i)))
                        {
                            if(brightness != 255)
                            {
                                Light_Modbus_SetBrightness(lights_modbus + i, brightness);
                            }
                            else if(Light_Modbus_isRGB(lights_modbus + i) && color)
                            {
                                Light_Modbus_SetColor(lights_modbus + i, color);
                            }
                        }
                    }
                }
                else
                {
                    if(brightness != 255)
                    {
                        Light_Modbus_SetBrightness(lights_modbus + light_selectedIndex, brightness);
                    }
                    else if(Light_Modbus_isRGB(lights_modbus + light_selectedIndex) && color)
                    {
                        Light_Modbus_SetColor(lights_modbus + light_selectedIndex, color);
                    }
                }
            }
        }
        else if((pTS->x > 100)&&(pTS->y > 100)&&(pTS->x < 400)&&(pTS->y < 272)&&(screen == SCREEN_RESET_MENU_SWITCHES))
        {
            if((!bOnlyLeaveScreenSaverAfterTouch) || (bOnlyLeaveScreenSaverAfterTouch && (!IsScrnsvrActiv())))
            {
                light_selectedIndex = LIGHTS_MODBUS_SIZE + 1;

                for(uint8_t i = 0; i < Lights_Modbus_getCount(); i++)
                {
                    if(Light_Modbus_isTiedToMainLight(lights_modbus + i) && (!Light_Modbus_isBinary(lights_modbus + i)))
                    {
                        light_selectedIndex = LIGHTS_MODBUS_SIZE;
                        if(Light_Modbus_isRGB(lights_modbus + i)) lights_allSelected_hasRGB = true;
                    }
                }



                if(light_selectedIndex == LIGHTS_MODBUS_SIZE)
                {
                    light_settingsTimerStart = HAL_GetTick();
                }





//                if(LightNightTimer_isEnabled && (!LightNightTimer_StartTime) && (!(((Bcd2Dec(rtctm.Hours) > 6) && (Bcd2Dec(rtctm.Hours) < 20)))))
//                {
//                    uint8_t isAnyActive = 0;
//
//                    for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; ++i)
//                    {
//                        if(Light_Modbus_isTiedToMainLight(lights_modbus + i) && Light_Modbus_isActive(lights_modbus + i))
//                        {
//                            isAnyActive = 1;
//                            break;
//                        }
//                    }
//
//                    if(isAnyActive) LightNightTimer_StartTime = HAL_GetTick();
//                    if(!LightNightTimer_StartTime) LightNightTimer_StartTime = 1;
//                }
//                else
//                {
//                    LightNightTimer_StartTime = 0;
//
//                    for(uint8_t i = 0, isActive = 0; i < LIGHTS_MODBUS_SIZE; ++i)
//                    {
//                        if(Light_Modbus_isTiedToMainLight(lights_modbus + i))
//                        {
//                            if(Light_Modbus_isActive(lights_modbus + i))
//                            {
//                                isActive = 1;
//
//                                /*if(LightNightTimer_isEnabled && (!LightNightTimer_StartTime) && (!(((Bcd2Dec(rtctm.Hours) > 6) && (Bcd2Dec(rtctm.Hours) < 20)))))
//                                {
//                                    LightNightTimer_StartTime = HAL_GetTick();
//                                    if(!LightNightTimer_StartTime) LightNightTimer_StartTime = 1;
//                                    i = LIGHTS_MODBUS_SIZE - 1;
//                                }
//                                else
//                                {*/
////                                    LightNightTimer_StartTime = 0;
//                                    Light_Modbus_Off(lights_modbus + i);
////                                }
//                            }
//                        }
//
//                        if((i == (LIGHTS_MODBUS_SIZE - 1)) && (!isActive))
//                        {
////                            LightNightTimer_StartTime = 0;
//
//                            for(uint8_t j = 0; j < LIGHTS_MODBUS_SIZE; j++)
//                            {
//                                if(Light_Modbus_isTiedToMainLight(lights_modbus + j)) Light_Modbus_On(lights_modbus + j);
//                            }
//                        }
//                    }
//                }
            }

            /*if(t)
            {
                t = 0;
            }
            else
            {
                t = 1;
            }*/
        }

        if ((pTS->x > 380)&&(pTS->y > 5)&&(pTS->x < 475)&&(pTS->y < 100)) {
            btnset = 1;
        }

        if (click) {
            BuzzerOn();
            HAL_Delay(1);
            BuzzerOff();
        }
    }
    else
    {
        if(release)
        {
            release = 0;
            
            if(screen == SCREEN_MAIN) screen = SCREEN_RESET_MENU_SWITCHES;
            else if(screen == SCREEN_LIGHTS)
            {
                if(Light_Modbus_isBinary(lights_modbus + light_selectedIndex) || ((!Light_Modbus_isBinary(lights_modbus + light_selectedIndex)) && light_settingsTimerStart))
                {
                    light_settingsTimerStart = 0;
                    Light_Modbus_Flip(lights_modbus + light_selectedIndex);
                }

            }
            else if(screen == SCREEN_RESET_MENU_SWITCHES)
            {
                if((pTS->x > 100) && (pTS->y > 100) && (pTS->x < 400) && (pTS->y < 272) && ((light_selectedIndex == (LIGHTS_MODBUS_SIZE + 1)) || ((light_selectedIndex == LIGHTS_MODBUS_SIZE) && light_settingsTimerStart)))
                {
                    uint8_t isAnyLightActive = 0;

                    click = 1;
                    screen = SCREEN_MAIN;

                    light_settingsTimerStart = 0;

                    for(uint8_t i = 0; i < Lights_Modbus_getCount(); ++i)
                    {
                        if(Light_Modbus_isTiedToMainLight(lights_modbus + i) && Light_Modbus_isActive(lights_modbus + i))
                        {
                            isAnyLightActive = 1;
                            break;
                        }
                    }


                    if((isAnyLightActive)
                            && LightNightTimer_isEnabled
                            && (!LightNightTimer_StartTime)
                            && (!((Bcd2Dec(rtctm.Hours) > 6) && (Bcd2Dec(rtctm.Hours) < 20))))
                    {
                        LightNightTimer_StartTime = HAL_GetTick();
                        if(!LightNightTimer_StartTime) LightNightTimer_StartTime = 1;
                    }
                    else
                    {
                        LightNightTimer_StartTime = 0;

                        for(uint8_t i = 0; i < Lights_Modbus_getCount(); ++i)
                        {
                            if(Light_Modbus_isTiedToMainLight(lights_modbus + i))
                            {
                                if(isAnyLightActive)
                                {
                                    Light_Modbus_Off(lights_modbus + i);
                                }
                                else
                                {
                                    /*if(Light_Modbus_isOffTimeEnabled(lights_modbus + i))
                                    {
                                        uint32_t currentTickTime = HAL_GetTick();
                                        if(!currentTickTime) currentTickTime = 1;
                                        Ventilator_SetOnDelayTimer(currentTickTime);
                                    }*/

                                    Light_Modbus_On(lights_modbus + i);
                                }
                            }




                            /*if(Light_Modbus_isTiedToMainLight(lights_modbus + i))
                            {
                                if(Light_Modbus_isActive(lights_modbus + i))
                                {
                                    isActive = 1;
                                    Light_Modbus_Off(lights_modbus + i);
                                }
                            }

                            if((i == (LIGHTS_MODBUS_SIZE - 1)) && (!isActive))
                            {
                                for(uint8_t j = 0; j < LIGHTS_MODBUS_SIZE; j++)
                                {
                                    if(Light_Modbus_isTiedToMainLight(lights_modbus + j)) Light_Modbus_On(lights_modbus + j);
                                }
                            }*/
                        }
                    }
                }
            }
        
            btnset = 0;
            btndec = 0U;
            btninc = 0U;
            ctrl1 = 0;
            ctrl2 = 0;
            ctrl3 = 0;
            thermostatOnOffTouch_timer = 0;
        }
    }
    DISPResetScrnsvr();
}
/**
  * @brief
  * @param
  * @retval
  */
static uint8_t DISPMenuSettings(uint8_t btn)
{
    static uint8_t last_state = 0U;
    static uint32_t menu_tmr = 0U;
    if      ((btn == 1U) && (last_state == 0U)) {
        last_state = 1U;
        menu_tmr = HAL_GetTick();
    } else if ((btn == 1U) && (last_state == 1U)) {
        if((HAL_GetTick() - menu_tmr) >= SETTINGS_MENU_ENABLE_TIME) {
            last_state = 0U;
            return (1U);
        }
    } else if ((btn == 0U) && (last_state == 1U)) last_state = 0U;
    return (0U);
}
/**
  * @brief
  * @param
  * @retval
  */
void DISP_UpdateLog(const char *pbuf)
{
    static char displog[6][128];
    int i = 5;
    GUI_ClearRect(120, 80, 480, 240);
    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_TOP);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_SetFont(&GUI_Font16B_1);
    GUI_SetColor(GUI_WHITE);
    do {
        ZEROFILL(displog[i], COUNTOF(displog[i]));
        strcpy(displog[i], displog[i-1]);
        GUI_DispStringAt(displog[i], 125, 200-(i*20));
    } while(--i);
    GUI_SetColor(GUI_YELLOW);
    ZEROFILL(displog[0], COUNTOF(displog[0]));
    strcpy(displog[0], pbuf);
    GUI_DispStringAt(displog[0], 125, 200);
    GUI_Exec();
}
/**
  * @brief
  * @param
  * @retval
  */
static void SaveLightController(LIGHT_CtrlTypeDef* lc, uint16_t addr)
{
    uint8_t buf[8];
    buf[0] = lc->Main1.index;
    buf[1] = lc->Led1.index;
    buf[2] = lc->Led2.index;
    buf[3] = lc->Led3.index;
    buf[4] = lc->Out1.index;
    buf[5] = lc->Light1.index;
    buf[6] = lc->Light2.index;
    buf[7] = lc->Light3.index;
    EE_WriteBuffer(buf, addr, 8);
    EE_WriteBuffer((uint8_t*)(&(lc->modbusLight.index)), addr + 8, 2);
}
/**
  * @brief
  * @param
  * @retval
  */
static void ReadLightController(LIGHT_CtrlTypeDef* lc, uint16_t addr)
{
    uint8_t buf[8];
    EE_ReadBuffer(buf, addr, 8);
    lc->Main1.index     = buf[0];
    lc->Led1.index      = buf[1];
    lc->Led2.index      = buf[2];
    lc->Led3.index      = buf[3];
    lc->Out1.index      = buf[4];
    lc->Light1.index    = buf[5];
    lc->Light2.index    = buf[6];
    lc->Light3.index    = buf[7];
    EE_ReadBuffer((uint8_t*)(&(lc->modbusLight.index)), addr + 8, 2);
}


void Curtain_Select(const uint8_t curtain)
{
    curtain_selected = curtain;
}

uint8_t Curtain_getSelected()
{
    return curtain_selected;
}

bool Curtain_areAllSelected()
{
    return curtain_selected == Curtains_getCount();
}

void Curtain_ResetSelection()
{
    curtain_selected = Curtains_getCount();
}




bool QR_Code_isDataLengthShortEnough(uint8_t dataLength)
{
    return dataLength < QR_CODE_LENGTH;
}


bool QR_Code_willDataFit(const uint8_t *data)
{
    return QR_Code_isDataLengthShortEnough(strlen((char*)data));
}



uint8_t* QR_Code_Get(const uint8_t qrCodeID)
{
    return qr_codes[qrCodeID - 1];
}



void QR_Code_Set(const uint8_t qrCodeID, const uint8_t *data)
{
    if(QR_Code_willDataFit(data))
    {
        sprintf((char*)(qr_codes[qrCodeID - 1]), "%s", (char*)data);
    }
}






void DrawEquilateralTriangle(const int16_t x, const int16_t y, const int16_t length, double rotation, const GUI_COLOR color, const GUI_COLOR fillColor)
{
    rotation = (rotation * 3.141592654) / 180;

    GUI_POINT point[3] = {{x, y}, {x + length, y}, {x + (length / 2), y - (length - length / 4)} /*{x + (length / 2), y - ((sqrt(3) * length) / 2)}*/};
    const double co = cos(rotation), si = sin(rotation);

    point[1].x = (co * (point[1].x - x)) - (si * (point[1].y - y)) + x;
    point[1].y = (si * (point[1].x - x)) + (co * (point[1].y - y)) + y;

    point[2].x = (co * (point[2].x - x)) - (si * (point[2].y - y)) + x;
    point[2].y = (si * (point[2].x - x)) + (co * (point[2].y - y)) + y;

//    GUI_POINT pr = {x, y};
//    GUI_RotatePolygon(point, &pr, 2, rotation);

    GUI_SetColor(color);

    GUI_DrawPolygon(point, 3, 0, 0);

    if(fillColor)
    {
        GUI_SetColor(fillColor);
        GUI_FillPolygon(point, 3, 0, 0);
    }

}
/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/

