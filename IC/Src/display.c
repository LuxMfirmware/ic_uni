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
#include "stm32746g.h"
#include "stm32746g_ts.h"
#include "stm32746g_qspi.h"
#include "stm32746g_sdram.h"
#include "stm32746g_eeprom.h"
/* Private Define ------------------------------------------------------------*/

typedef struct
{
    SPINBOX_Handle  relay, offTime, iconID, controllerID_on, controllerID_on_delay, on_hour, on_minute, communication_type, local_pin, sleep_time, button_external;
    CHECKBOX_Handle  tiedToMainLight;
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

#define LIGHT_NIGHT_TIMER_DURATION      15

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

#define ID_LightsModbusRelay            0x8B3  //holds 30 bytes for 15 lights

#define ID_SYSRESTART                   0x976

#define ID_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH       0x977
#define ID_LIGHT_NIGHT_TIMER            0x978


/* Private Type --------------------------------------------------------------*/
BUTTON_Handle   hBUTTON_Increase;
BUTTON_Handle   hBUTTON_Decrease;
BUTTON_Handle   hBUTTON_Ok;
BUTTON_Handle   hBUTTON_Next;

SPINBOX_Handle  hSPNBX_MaxSetpoint;                         //  set thermostat user maximum setpoint value
SPINBOX_Handle  hSPNBX_MinSetpoint;                         //  set thermostat user minimum setpoint value

/*RADIO_Handle    hThstControl;
RADIO_Handle    hFanControl;
SPINBOX_Handle  hThstMaxSetPoint;
SPINBOX_Handle  hThstMinSetPoint;
SPINBOX_Handle  hFanDiff;
SPINBOX_Handle  hFanLowBand;
SPINBOX_Handle  hFanHiBand;*/
SPINBOX_Handle hThstRelay1;
SPINBOX_Handle hThstRelay2;
SPINBOX_Handle hThstRelay3;
SPINBOX_Handle hThstRelay4;
SPINBOX_Handle hThstRelay3Delay;
SPINBOX_Handle hThstRelay4Delay;
SPINBOX_Handle hNTC_Offset;
SPINBOX_Handle hFanDiff;


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

BUTTON_Handle hBUTTON_SYSRESTART;

CHECKBOX_Handle hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH;
CHECKBOX_Handle hCHKBX_LIGHT_NIGHT_TIMER;


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
LIGHT_Modbus_CmdTypeDef lights_modbus[LIGHTS_MODBUS_SIZE];
uint32_t light_settingsTimerStart = 0;
uint8_t lights_count = 0, lightsModbusSettingsMenu = 0, lights_modbus_rows = 0, light_selectedIndex = LIGHTS_MODBUS_SIZE, lightsMenuSpaceBetween = (400 - (80 * LIGHTS_MODBUS_PER_ROW)) / (LIGHTS_MODBUS_PER_ROW - 1 + 2);
uint8_t shouldDrawScreen = 1;
uint8_t bOnlyLeaveScreenSaverAfterTouch = 0;
uint8_t LightNightTimer_isEnabled = 0;
uint32_t LightNightTimer_StartTime = 0;
uint32_t ventilatorOnDelayTimer_Start = 0;
uint32_t everyMinuteTimerStart = 0;
uint8_t isButtonActive_old = 0;

uint8_t qr_codes[QR_CODE_COUNT][QR_CODE_LENGTH] = {0}, qr_code_draw_id = 0;

GUI_CONST_STORAGE GUI_BITMAP* light_modbus_images[] = {&bmSijalicaOff, &bmSijalicaOn, &bmVENTILATOR_OFF, &bmVENTILATOR_ON};

enum Languages{BOS = 0, ENG} language = ENG;
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
                case 1: strcpy(langText, "ALARM"); break;
                case 2: strcpy(langText, "THERMOSTAT"); break;
                case 3: strcpy(langText, "CURTAINS"); break;
                case 4: strcpy(langText, "NEXT"); break;
                case 5: strcpy(langText, "TV"); break;
                case 6: strcpy(langText, "CLEAN"); break;
                case 7: strcpy(langText, "SETTINGS"); break;
                case 8: strcpy(langText, "Hours"); break;
                case 9: strcpy(langText, "Minutes"); break;
                case 10: strcpy(langText, "RESET"); break;
                case 11: strcpy(langText, "ACTIVATE"); break;
                case 12: strcpy(langText, "ALARM TIME"); break;
                case 13: strcpy(langText, "DISPLAY CLEAN TIME:"); break;
                case 14: strcpy(langText, "ENTER PASSWORD"); break;
                case 15: strcpy(langText, "PASSWORD CORRECT"); break;
                case 16: strcpy(langText, "WRONG PASSWORD"); break;
                case 17: strcpy(langText, "ENG"); break;
                case 18: strcpy(langText, "MUSIC"); break;
                case 19: strcpy(langText, "LIGHT"); break;
                case 20: strcpy(langText, "LIGHTS"); break;
                case 21: strcpy(langText, "BLINDS"); break;
                case 22: strcpy(langText, "BED"); break;
                case 23: strcpy(langText, "HALLWAY"); break;
                case 24: strcpy(langText, "WC"); break;
                case 25: strcpy(langText, "TERRACE"); break;
                case 26: strcpy(langText, "KITCHEN"); break;
                case 27: strcpy(langText, "STAIRS"); break;
                case 28: strcpy(langText, "LIVING R. 1"); break;
                case 29: strcpy(langText, "LIVING R. 2"); break;
                case 30: strcpy(langText, "LIVING R. 3"); break;
                case 31: strcpy(langText, "TERR. L."); break;
                case 32: strcpy(langText, "TERR. R."); break;
                case 33: strcpy(langText, "SIDE WIN."); break;
                case 34: strcpy(langText, "WINDOWS"); break;
                case 35: strcpy(langText, "FACADE"); break;
                case 36: strcpy(langText, "BEDROOM"); break;
                case 37: strcpy(langText, "BEDROOM 1"); break;
                case 38: strcpy(langText, "BEDROOM 2"); break;
                case 39: strcpy(langText, "TERRACE 1"); break;
                case 40: strcpy(langText, "TERRACE 2"); break;
                case 41: strcpy(langText, "LIVING\nROOM 1"); break;
                case 42: strcpy(langText, "LIVING\nROOM 2"); break;
                case 43: strcpy(langText, "POOL 1"); break;
                case 44: strcpy(langText, "POOL 2"); break;
                case 45: strcpy(langText, "POOL 3"); break;
                case 46: strcpy(langText, "LEFT"); break;
                case 47: strcpy(langText, "MIDDLE"); break;
                case 48: strcpy(langText, "RIGHT"); break;
                case 49: strcpy(langText, "LIVING "); break;
                case 50: strcpy(langText, "ALL"); break;
                case 51: strcpy(langText, "Wi-Fi"); break;
                case 52: strcpy(langText, "APP"); break;
            }
            break;
        }
        case BOS:  // BiH
        {
            switch(t)
            {
                case 1: strcpy(langText, "ALARM"); break;
                case 2: strcpy(langText, "TERMOSTAT"); break;
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
                case 17: strcpy(langText, "BOS"); break;
                case 18: strcpy(langText, "MUZIKA"); break;
                case 19: strcpy(langText, "SVJETLO"); break;
                case 20: strcpy(langText, "SVJETLA"); break;
                case 21: strcpy(langText, "ROLETNE"); break;
                case 22: strcpy(langText, "SPAVACA"); break;
                case 23: strcpy(langText, "HODNIK"); break;
                case 24: strcpy(langText, "WC"); break;
                case 25: strcpy(langText, "TERASA"); break;
                case 26: strcpy(langText, "KUHINJA"); break;
                case 27: strcpy(langText, "STEP."); break;  //stepenice
                case 28: strcpy(langText, "DNEVNI B. 1"); break;
                case 29: strcpy(langText, "DNEVNI B. 2"); break;
                case 30: strcpy(langText, "DNEVNI B. 3"); break;
                case 31: strcpy(langText, "TER. L."); break;
                case 32: strcpy(langText, "TER. R."); break;
                case 33: strcpy(langText, "BOČ. PRO."); break;
                case 34: strcpy(langText, "PROZORI"); break;
                case 35: strcpy(langText, "FASADA"); break;
                case 36: strcpy(langText, "SPAVACA"); break;
                case 37: strcpy(langText, "SPAVACA 1"); break;
                case 38: strcpy(langText, "SPAVACA 2"); break;
                case 39: strcpy(langText, "TERASA 1"); break;
                case 40: strcpy(langText, "TERASA 2"); break;
                case 41: strcpy(langText, "LIVING\nROOM 1"); break;
                case 42: strcpy(langText, "LIVING\nROOM 2"); break;
                case 43: strcpy(langText, "BAZEN 1"); break;
                case 44: strcpy(langText, "BAZEN 2"); break;
                case 45: strcpy(langText, "BAZEN 3"); break;
                case 46: strcpy(langText, "LIJEVE"); break;
                case 47: strcpy(langText, "SREDNJE"); break;
                case 48: strcpy(langText, "DESNE"); break;
                case 49: strcpy(langText, "DNEVNI "); break;
                case 50: strcpy(langText, "SVE"); break;
                case 51: strcpy(langText, "Wi-Fi"); break;
                case 52: strcpy(langText, "APP"); break;
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
            break;

        case 2:
        {
            return IsLight2Active();
            break;
        }

        case 3:
            return IsLight3Active();
            break;
                        
        case 4:
            return IsLight4Active();
            break;

        case 5:
            return IsLight5Active();
            break;

        case 6:
            return IsLight6Active();
            break;
        
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
            if(pinVal) Light1On(); else Light1Off();
            break;

        case 2:
        {
            if(pinVal) Light2On(); else Light2Off();
            /*if(pinVal) t = 1; else t = 0;
            screen = 7;*/
            
            break;
        }

        case 3:
            if(pinVal) Light3On(); else Light3Off();
            break;
                        
        case 4:
            if(pinVal) Light4On(); else Light4Off();
            break;

        case 5:
            if(pinVal) Light5On(); else Light5Off();
            break;

        case 6:
            if(pinVal) Light6On(); else Light6Off();
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
static void DSP_KillSet1Scrn(void);
static void DSP_KillSet2Scrn(void);
static void DSP_KillSet3Scrn(void);
static void DSP_KillSet4Scrn(void);
static void DSP_KillSet5Scrn(void);
static void DSP_KillSet6Scrn(void);
static void DSP_KillSet7Scrn(void);
static uint8_t DISPMenuSettings(uint8_t btn);
static void SaveLightController(LIGHT_CtrlTypeDef* lc, uint16_t addr);
static void ReadLightController(LIGHT_CtrlTypeDef* lc, uint16_t addr);
uint8_t Lights_Modbus_getCount();
uint8_t Lights_Modbus_Rows_getCount();
void Lights_Modbus_Init();
void Lights_Modbus_Save();
uint16_t Light_Modbus_GetRelay(const LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_SetRelay(LIGHT_Modbus_CmdTypeDef* const li, const uint16_t val);
void Light_Modbus_TieToMainLight(LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_UntieFromMainLight(LIGHT_Modbus_CmdTypeDef* const li);
bool Light_Modbus_isTiedToMainLight(const LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_On(LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_Off(LIGHT_Modbus_CmdTypeDef* const li);
bool Light_Modbus_isActive(const LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_Flip(LIGHT_Modbus_CmdTypeDef* const li);
uint8_t Light_Modbus_GetOnDelayTime(const LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_SetOnDelayTime(LIGHT_Modbus_CmdTypeDef* const li, const uint8_t val);
bool Light_Modbus_isOnDelayTimeEnabled(const LIGHT_Modbus_CmdTypeDef* const li);
uint32_t Light_Modbus_GetOnDelayTimeTimer(const LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_SetOnDelayTimeTimer(LIGHT_Modbus_CmdTypeDef* const li, const uint32_t val);
bool Light_Modbus_isOnDelayTimeTimerActive(const LIGHT_Modbus_CmdTypeDef* const li);
bool Light_Modbus_hasOnDelayTimeTimerExpired(const LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_OnDelayTimeTimerDeactivate(LIGHT_Modbus_CmdTypeDef* const li);
uint8_t Light_Modbus_GetOffTime(const LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_SetOffTime(LIGHT_Modbus_CmdTypeDef* const li, const uint8_t val);
bool Light_Modbus_isOffTimeEnabled(const LIGHT_Modbus_CmdTypeDef* const li);
uint32_t Light_Modbus_GetOffTimeTimer(const LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_SetOffTimeTimer(LIGHT_Modbus_CmdTypeDef* const li, const uint32_t val);
bool Light_Modbus_isOffTimeTimerActive(const LIGHT_Modbus_CmdTypeDef* const li);
bool Light_Modbus_hasOffTimeTimerExpired(const LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_OffTimeTimerDeactivate(LIGHT_Modbus_CmdTypeDef* const li);
bool Light_Modbus_isTimeOnEnabled(const LIGHT_Modbus_CmdTypeDef* const li);
bool Light_Modbus_isTimeToTurnOn(const LIGHT_Modbus_CmdTypeDef* const li);
void Lights_Modbus_Button_External_Press(LIGHT_Modbus_CmdTypeDef* const li);
GUI_CONST_STORAGE GUI_BITMAP* Light_Modbus_GetIcon(const LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_SetColor(LIGHT_Modbus_CmdTypeDef* const li, GUI_COLOR color);
void Light_Modbus_ResetColor(LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_SetBrightness(LIGHT_Modbus_CmdTypeDef* const li, uint8_t brightness);
void Light_Modbus_ResetBrightness(LIGHT_Modbus_CmdTypeDef* const li);
uint32_t Ventilator_GetOnDelayTimer();
void Ventilator_SetOnDelayTimer(const uint32_t val);
bool Ventilator_isOnDelayTimerActive();
bool Ventilator_hasOnDelayTimerExpired();
void Ventilator_OnDelayTimerDeactivate();
void Curtain_Select(const uint8_t curtain);
uint8_t Curtain_getSelected();
bool Curtain_areAllSelected();
void Curtain_ResetSelection();
void DrawEquilateralTriangle(const int16_t x, const int16_t y, const int16_t length, double rotation, const GUI_COLOR color, const GUI_COLOR fillColor);
/* Program Code  -------------------------------------------------------------*/
/**
  * @brief
  * @param
  * @retval
  */
void DISP_Init(void){
    GUI_Init();
    GUI_PID_SetHook(PID_Hook);
    WM_MULTIBUF_Enable(1);
    GUI_UC_SetEncodeUTF8();
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT); 
    GUI_Clear();
    ReadLightController(&LIGHT_Ctrl1, EE_CTRL1);
    ReadLightController(&LIGHT_Ctrl2, EE_CTRL2);
//    Ventilator_Init(&ventilator, EE_VENTILATOR);
    Lights_Modbus_Init();
    EE_ReadBuffer(&bOnlyLeaveScreenSaverAfterTouch, EE_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, 1);
    EE_ReadBuffer(&LightNightTimer_isEnabled, EE_LIGHT_NIGHT_TIMER, 1);
    everyMinuteTimerStart = HAL_GetTick();
    GUI_Exec();
}
/**
  * @brief
  * @param
  * @retval
  */
void DISP_Service(void){
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
    if ((HAL_GetTick() - guitmr) >= 100){
        guitmr = HAL_GetTick();
        GUI_Exec();
    }
    
    if (IsFwUpdateActiv()){
        if (!fwmsg){
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
    } else if(fwmsg == 1){
        fwmsg = 0;
        scrnsvr_tmr = 0;
    } else if(fwmsg == 2){
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
    
    switch(screen){
        //
        //  MAIN LIGHT CONTROL ON/OFF/DIMM
        //
        case 1:
        {
            //
            //  TOUCH EVENT TIME SHORT(ON/OFF)/LONG(DIMM)
            //
            switch(i){
                //
                //  FIRST TOUCH EVENT IS ON/OFF
                //
                case 0:
                {
                    t = 1;
                    
                    for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; i++)
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
                            GUI_SetColor(GUI_GREEN);
                            GUI_DrawEllipse(240,136,50,50);
                            GUI_DrawLine(400,20,450,20);
                            GUI_DrawLine(400,40,450,40);
                            GUI_DrawLine(400,60,450,60);
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
                            GUI_SetColor(GUI_RED);
                            GUI_DrawEllipse(240,136,50,50);
                            GUI_DrawLine(400,20,450,20);
                            GUI_DrawLine(400,40,450,40);
                            GUI_DrawLine(400,60,450,60);
                            GUI_MULTIBUF_EndEx(1);
                            onoff_tmr = HAL_GetTick();
                            break;
                        }
                    }
                    
                    i = 0;
                    screen = 0;
                    break;

                    if (r){
                        screen = 0;
                        r = 0; // SKEEP FOR MENU TIMEOUT CALL
                    } else {
                        if      (t == 0){
                            LIGHT_Ctrl1.Led1.value = 0;
                            LIGHT_Ctrl1.Led2.value = 0;
                            LIGHT_Ctrl1.Led3.value = 0;
                            LIGHT_Ctrl1.Light1.value = 0;
                            LIGHT_Ctrl1.Light2.value = 0;
                            LIGHT_Ctrl1.Light3.value = 0;
                        } else if (t == 1){
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
                    if ((HAL_GetTick() -  onoff_tmr) >= EVENT_ONOFF_TOUT){
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
                    if ((HAL_GetTick() -  value_step_tmr) >= VALUE_STEP_TOUT){
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
                                &&  (LIGHT_Ctrl1.Light3.value == 0xFF)){
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
                                &&  !LIGHT_Ctrl1.Light3.value){
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
        case 2:
        {
            if (menu_lc == 0){
                ++menu_lc;
                
                GUI_MULTIBUF_BeginEx(1);
                
                GUI_SelectLayer(0);
                GUI_Clear();
                GUI_SelectLayer(1);
                GUI_SetBkColor(GUI_TRANSPARENT); 
                GUI_Clear();
                
                GUI_SetPenSize(9);
                GUI_SetColor(GUI_RED);
                
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
                GUI_DrawBitmap(&bmwifi, 240, 160);
                
                
                
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
                lng(51);
                GUI_DispString(langText);
                
                
                
                GUI_MULTIBUF_EndEx(1);
                
                menu_thst = 0;
                menu_dim = 0;
                menu_rel123 = 0;
                menu_out1 = 0;    
            }           
            break;
        }
        //
        //  LIGHT DIMMER 0-100% CONTROL
        //
        case 3:
        {
            switch (menu_dim){
                case 0:
                {
                    ++menu_dim;
                    GUI_MULTIBUF_BeginEx(1);
                    GUI_Clear();
                    GUI_SetPenSize(5);
                    GUI_SetColor(GUI_RED);
                    GUI_DrawLine(120,220,120,272);
                    GUI_DrawLine(240,220,240,272);
                    GUI_DrawLine(360,220,360,272);
                    GUI_DrawBitmap(&bmSijalicicaOn,   65, 215);
                    GUI_DrawBitmap(&bmSijalicicaOff, 185, 215);
                    GUI_DrawBitmap(&bmSijalicicaOff, 305, 215);
                    GUI_DrawBitmap(&bmHome,          400, 190);
                    if (LIGHT_Ctrl1.Light1.value)   GUI_DrawBitmap(&bmSijalicaOn,  60, 60);
                    else                            GUI_DrawBitmap(&bmSijalicaOff, 60, 60);
                    if (LIGHT_Ctrl1.Light2.value)   GUI_DrawBitmap(&bmSijalicaOn, 200, 60);
                    else                            GUI_DrawBitmap(&bmSijalicaOff,200, 60);
                    if (LIGHT_Ctrl1.Light3.value)   GUI_DrawBitmap(&bmSijalicaOn, 340, 60);
                    else                            GUI_DrawBitmap(&bmSijalicaOff,340, 60);                    
                    GUI_SetFont(GUI_FONT_24B_1);
                    GUI_SetColor(GUI_ORANGE);
                    GUI_SetTextMode(GUI_TM_TRANS);
                    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);
                    GUI_GotoXY(5, 250);
                    GUI_DispString("LIGHT");
                    GUI_GotoXY(135, 250);
                    GUI_DispString("LED");
                    GUI_GotoXY(255, 250);
                    GUI_DispString("OUT");
                    GUI_SetTextAlign(GUI_TA_HORIZONTAL|GUI_TA_VCENTER);
                    GUI_GotoXY(100,40);
                    if  (LIGHT_Ctrl1.Light1.value) GUI_DispDecMin((LIGHT_Ctrl1.Light1.value*100)/255);
                    GUI_GotoXY(240,40);
                    if  (LIGHT_Ctrl1.Light2.value) GUI_DispDecMin((LIGHT_Ctrl1.Light2.value*100)/255);
                    GUI_GotoXY(380,40);
                    if  (LIGHT_Ctrl1.Light3.value) GUI_DispDecMin((LIGHT_Ctrl1.Light3.value*100)/255);
                    GUI_MULTIBUF_EndEx(1);
                    menu_thst = 0;
                    menu_rel123 = 0;
                    menu_out1 = 0;
                    menu_lc = 0;
                    c1 = 0;
                    break;
                }
                //
                //  WAIT FOR DISPLAY TOUCH TO SELECT DIMMER
                //
                case 1:
                {
                    if (ctrl1 || ctrl2 || ctrl3){
                        c1 = ctrl1;
                        c2 = ctrl2;
                        c3 = ctrl3;
                        menu_dim = 2;
                        dimm_tmr = HAL_GetTick();
                    }
                    break;
                }
                //
                //  LONG TOUCH WILL START DIMMER CONTROL
                //  SHORT ONE WILL TRIGGER ON/OFF CONTROL
                //
                case 2:
                {
                    if ((HAL_GetTick() -  dimm_tmr) >= 1000){
                        value_step_tmr = HAL_GetTick();
                        ++menu_dim;
                        c1 = 0;
                        c2 = 0;
                        c3 = 0;
                    }
                    else if (!ctrl1 && !ctrl2 && !ctrl3){
                        if      (c1 &&  LIGHT_Ctrl1.Light1.value) LIGHT_Ctrl1.Light1.value = 0;
                        else if (c1 && !LIGHT_Ctrl1.Light1.value) LIGHT_Ctrl1.Light1.value = 0xFF;
                        if      (c2 &&  LIGHT_Ctrl1.Light2.value) LIGHT_Ctrl1.Light2.value = 0;
                        else if (c2 && !LIGHT_Ctrl1.Light2.value) LIGHT_Ctrl1.Light2.value = 0xFF;
                        if      (c3 &&  LIGHT_Ctrl1.Light3.value) LIGHT_Ctrl1.Light3.value = 0;
                        else if (c3 && !LIGHT_Ctrl1.Light3.value) LIGHT_Ctrl1.Light3.value = 0xFF;
                        menu_dim = 0;
                        c1 = 0;
                        c2 = 0;
                        c3 = 0;                        
                    }
                    break;
                }

                case 3:
                {
                    if      (!ctrl1 && !ctrl2 && !ctrl3) menu_dim = 0;
                    else if ((HAL_GetTick() -  value_step_tmr) >= VALUE_STEP_TOUT){                        
                        value_step_tmr = HAL_GetTick();
                        if      (ctrl1){                            
                            GUI_MULTIBUF_BeginEx(1);
                            switch (dir){
                                case 0:
                                {
                                    if  (!LIGHT_Ctrl1.Light1.value) GUI_DrawBitmap(&bmSijalicaOn, 60, 60);
                                    if  (LIGHT_Ctrl1.Light1.value < 0xFF) LIGHT_Ctrl1.Light1.value++;
                                    else dir = 1;
                                    break;
                                }

                                case 1:
                                default:
                                {
                                    if  (LIGHT_Ctrl1.Light1.value) LIGHT_Ctrl1.Light1.value--;
                                    else {
                                        dir = 0;
                                        GUI_ClearRect(60,60,160,160);
                                        GUI_DrawBitmap(&bmSijalicaOff, 60, 60);
                                    }
                                    break;
                                }
                            }
                            GUI_ClearRect(100,30,200,50);
                            GUI_GotoXY(100,40);
                            GUI_SetTextAlign(GUI_TA_HORIZONTAL|GUI_TA_VCENTER);
                            GUI_DispDecMin((LIGHT_Ctrl1.Light1.value*100)/255);
                            GUI_MULTIBUF_EndEx(1);
                        }
                        else if (ctrl2){
                            GUI_MULTIBUF_BeginEx(1);
                            switch (dir){
                                case 0:
                                {
                                    if  (!LIGHT_Ctrl1.Light2.value) GUI_DrawBitmap(&bmSijalicaOn, 200, 60);
                                    if   (LIGHT_Ctrl1.Light2.value < 0xFF) LIGHT_Ctrl1.Light2.value++;
                                    else dir = 1;
                                    break;
                                }

                                case 1:
                                default:
                                {
                                    if  (LIGHT_Ctrl1.Light2.value) LIGHT_Ctrl1.Light2.value--;
                                    else{
                                        dir = 0;
                                        GUI_ClearRect(200,60,300,160);
                                        GUI_DrawBitmap(&bmSijalicaOff, 200, 60);
                                    }
                                    break;
                                }
                            }
                            GUI_ClearRect(220,30,320,50);
                            GUI_GotoXY(240,40);
                            GUI_SetTextAlign(GUI_TA_HORIZONTAL|GUI_TA_VCENTER);
                            GUI_DispDecMin((LIGHT_Ctrl1.Light2.value*100)/255);
                            GUI_MULTIBUF_EndEx(1);
                        }
                        else if (ctrl3){
                            GUI_MULTIBUF_BeginEx(1);
                            switch (dir){
                                case 0:
                                {
                                    if  (!LIGHT_Ctrl1.Light3.value) GUI_DrawBitmap(&bmSijalicaOn, 340, 60);
                                    if   (LIGHT_Ctrl1.Light3.value < 0xFF) LIGHT_Ctrl1.Light3.value++;
                                    else dir = 1;
                                    break;
                                }

                                case 1:
                                default:
                                {
                                    if  (LIGHT_Ctrl1.Light3.value) LIGHT_Ctrl1.Light3.value--;
                                    else{
                                        dir = 0;
                                        GUI_ClearRect(340,60,440,160);
                                        GUI_DrawBitmap(&bmSijalicaOff, 340, 60);
                                    }
                                    break;
                                }
                            }
                            GUI_ClearRect(340,30,440,50);
                            GUI_GotoXY(380,40);
                            GUI_SetTextAlign(GUI_TA_HORIZONTAL|GUI_TA_VCENTER);
                            GUI_DispDecMin((LIGHT_Ctrl1.Light3.value*100)/255);
                            GUI_MULTIBUF_EndEx(1);
                        }
                    }
                    break;
                }
            }
            break;
        }
        //
        //  LED 1-3 RELAY ON/OFF CONTROL
        //
        case 4:
        {
            if      (menu_rel123 == 0){
                ++menu_rel123;
                GUI_MULTIBUF_BeginEx(1);
                GUI_Clear();
                GUI_SetPenSize(5);
                GUI_SetColor(GUI_RED);
                GUI_DrawLine(120,220,120,272);
                GUI_DrawLine(240,220,240,272);
                GUI_DrawLine(360,220,360,272);
                GUI_DrawBitmap(&bmSijalicicaOff, 65,215);
                GUI_DrawBitmap(&bmSijalicicaOn, 185,215);
                GUI_DrawBitmap(&bmSijalicicaOff,305,215);
                GUI_DrawBitmap(&bmHome,         400,190);
                GUI_SetFont(GUI_FONT_24B_1);
                GUI_SetColor(GUI_ORANGE);
                GUI_SetTextMode(GUI_TM_TRANS);
                GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);
                GUI_GotoXY(5, 250);
                GUI_DispString("LIGHT");
                GUI_GotoXY(135, 250);
                GUI_DispString("LED");
                GUI_GotoXY(255, 250);
                GUI_DispString("OUT");
                if (LIGHT_Ctrl1.Led1.value) GUI_DrawBitmap(&bmSijalicaOn,  60,60);
                else                        GUI_DrawBitmap(&bmSijalicaOff, 60,60);
                if (LIGHT_Ctrl1.Led2.value) GUI_DrawBitmap(&bmSijalicaOn, 200,60);
                else                        GUI_DrawBitmap(&bmSijalicaOff,200,60);
                if (LIGHT_Ctrl1.Led3.value) GUI_DrawBitmap(&bmSijalicaOn, 340,60);
                else                        GUI_DrawBitmap(&bmSijalicaOff,340,60);
                GUI_MULTIBUF_EndEx(1);
                menu_thst=0;
                menu_dim =0;
                menu_out1=0;
                menu_lc = 0;
            } else if (menu_rel123 == 1){
                if      (ctrl1 &&  LIGHT_Ctrl1.Led1.value)  LIGHT_Ctrl1.Led1.value = 0;
                else if (ctrl1 && !LIGHT_Ctrl1.Led1.value)  LIGHT_Ctrl1.Led1.value = 1;
                if      (ctrl2 &&  LIGHT_Ctrl1.Led2.value)  LIGHT_Ctrl1.Led2.value = 0;
                else if (ctrl2 && !LIGHT_Ctrl1.Led2.value)  LIGHT_Ctrl1.Led2.value = 1;
                if      (ctrl3 &&  LIGHT_Ctrl1.Led3.value)  LIGHT_Ctrl1.Led3.value = 0;
                else if (ctrl3 && !LIGHT_Ctrl1.Led3.value)  LIGHT_Ctrl1.Led3.value = 1;
                if (ctrl1 || ctrl2 || ctrl3) menu_rel123++;
            } else if (menu_rel123 == 2){
                if (!ctrl1 && !ctrl2 && !ctrl3) menu_rel123 = 0;
            }
            break;
        }
        //
        //  LED 4 RELAY ON/OFF CONTROL
        //
        case 5:
        {
            if      (menu_out1 == 0){
                ++menu_out1;
                GUI_MULTIBUF_BeginEx(1);
                GUI_Clear();
                GUI_SetPenSize(5);
                GUI_SetColor(GUI_RED);
                GUI_DrawLine(120,220,120,272);
                GUI_DrawLine(240,220,240,272);
                GUI_DrawLine(360,220,360,272);
                if (LIGHT_Ctrl1.Out1.value) GUI_DrawBitmap(&bmSijalicaOn, 200,60);
                else                        GUI_DrawBitmap(&bmSijalicaOff,200,60);
                GUI_DrawBitmap(&bmSijalicicaOff,  65, 215);
                GUI_DrawBitmap(&bmSijalicicaOff, 185, 215);
                GUI_DrawBitmap(&bmSijalicicaOn,  305, 215);
                GUI_DrawBitmap(&bmHome,          400, 190);
                GUI_SetFont(GUI_FONT_24B_1);
                GUI_SetColor(GUI_ORANGE);
                GUI_SetTextMode(GUI_TM_TRANS);
                GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);
                GUI_GotoXY(5, 250);
                GUI_DispString("LIGHT");
                GUI_GotoXY(135, 250);
                GUI_DispString("LED");
                GUI_GotoXY(255, 250);
                GUI_DispString("OUT");
                GUI_MULTIBUF_EndEx(1);
                menu_thst = 0;
                menu_dim = 0;
                menu_rel123 = 0;
                menu_lc = 0;
            }
            else if (menu_out1 == 1){
                if (ctrl2){
                    if (!LIGHT_Ctrl1.Out1.value){
                        LIGHT_Ctrl1.Out1.value = 1;
                        out1_tmr = HAL_GetTick(); // load to start relay 4 shutdown timer
                    }
                    else if (LIGHT_Ctrl1.Out1.value){
                        LIGHT_Ctrl1.Out1.value = 0;
                        out1_tmr = 0; // clear to disable relay 4 shutdown  timer
                    }
                    ++menu_out1;
                }
            }
            else if (menu_out1 == 2){
                if (!ctrl2) menu_out1 = 0;
            }
            break;
        }
        //
        //  THERMOSTAT
        //
        case 6:
        {
            GUI_MULTIBUF_BeginEx(1);
            
            if      (menu_thst == 0){
                ++menu_thst;
                
                GUI_MULTIBUF_BeginEx(0);
                GUI_SelectLayer(0);
                GUI_SetColor(GUI_BLACK);
                GUI_Clear();
                GUI_BMP_Draw(&thstat, 0, 0);
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
            } else if (menu_thst == 1){
                /************************************/
                /*      SETPOINT  VALUE  INCREASED  */
                /************************************/
                if      ( btninc&& !_btninc){
                    _btninc = 1;
                    if (thst.sp_temp < thst.sp_max) ++thst.sp_temp;
                    SaveThermostatController(&thst, EE_THST1);
                    DISPSetPoint();
                }
                else if (!btninc &&  _btninc) _btninc = 0;
                /************************************/
                /*      SETPOINT  VALUE  DECREASED  */
                /************************************/
                if      (btndec && !_btndec){
                    _btndec = 1;
                    if (thst.sp_temp > thst.sp_min)  --thst.sp_temp;
                    SaveThermostatController(&thst, EE_THST1);
                    DISPSetPoint();
                }
                else if (!btndec &&  _btndec) _btndec = 0;
                /** ==========================================================================*/
                /**   R E W R I T E   A N D   S A V E   N E W   S E T P O I N T   V A L U E   */
                /** ==========================================================================*/
                if  (IsMVUpdateActiv()){
                    MVUpdateReset();
                    GUI_ClearRect(410, 185, 480, 235);
                    GUI_ClearRect(310, 230, 480, 255);
                    if(IsTempRegActiv()) GUI_SetColor(GUI_GREEN); else GUI_SetColor(GUI_RED);
                    GUI_SetFont(GUI_FONT_32B_1);
                    GUI_GotoXY(410, 170);
                    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);
                    if(IsTempRegActiv()) GUI_DispString("ON"); else GUI_DispString("OFF");
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
                if ((HAL_GetTick() - rtctmr) >= DATE_TIME_REFRESH_TIME){ 
                    rtctmr = HAL_GetTick();// screansaver clock time update
                    if(++refresh_tmr > 10){// regular intervals
                        refresh_tmr = 0;// refresh room temperature
                        if (!IsScrnsvrActiv()) MVUpdateSet(); 
                    }
                    
                    if(IsRtcTimeValid()){
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
                    thermostatOnOffTouch_timer = 0;
                    menu_thst = 0;
                    
                    if(IsTempRegActiv())
                    {
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
        case 7:
        {
            GUI_SelectLayer(0);
            GUI_Clear();
            GUI_SelectLayer(1);
            GUI_SetBkColor(GUI_TRANSPARENT); 
            GUI_Clear();
            DISPSetBrightnes(DISP_BRGHT_MIN);
            screen = 1;
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
            shouldDrawScreen = 1;
            break;
        }
        //
        //  SETTINGS MENU 1
        //
        case 8:
        {            
            /** ==========================================================================*/
            /**    S E T T I N G S     M E N U     U S E R     I N P U T   U P D A T E    */
            /** ==========================================================================*/
            
            
            if(thst.relay1 != SPINBOX_GetValue(hThstRelay1))
            {
                thst.relay1 = SPINBOX_GetValue(hThstRelay1);
                thsta = 1;
            }
            else if(thst.relay2 != SPINBOX_GetValue(hThstRelay2))
            {
                thst.relay2 = SPINBOX_GetValue(hThstRelay2);
                thsta = 1;
            }
            else if(thst.relay3 != SPINBOX_GetValue(hThstRelay3))
            {
                thst.relay3 = SPINBOX_GetValue(hThstRelay3);
                thsta = 1;
            }
            else if(thst.relay4 != SPINBOX_GetValue(hThstRelay4))
            {
                thst.relay4 = SPINBOX_GetValue(hThstRelay4);
                thsta = 1;
            }
            else if(thst.mv_offset != SPINBOX_GetValue(hNTC_Offset))
            {
                thst.mv_offset = SPINBOX_GetValue(hNTC_Offset);
                thsta = 1;
            }
            else if(thst.fan_diff != SPINBOX_GetValue(hFanDiff))
            {
                thst.fan_diff = SPINBOX_GetValue(hFanDiff);
                thsta = 1;
            }
            else if(thst.relay3Delay != SPINBOX_GetValue(hThstRelay3Delay))
            {
                thst.relay3Delay = SPINBOX_GetValue(hThstRelay3Delay);
                thsta = 1;
            }
            else if(thst.relay4Delay != SPINBOX_GetValue(hThstRelay4Delay))
            {
                thst.relay4Delay = SPINBOX_GetValue(hThstRelay4Delay);
                thsta = 1;
            }
            
            
            if (BUTTON_IsPressed(hBUTTON_Ok))
            {
                if(thsta) SaveThermostatController(&thst, EE_THST1);
                thsta = 0;
                DSP_KillSet1Scrn();
                screen = 7;
            }
            else if (BUTTON_IsPressed(hBUTTON_Next))
            {                
                DSP_KillSet1Scrn();
                DSP_InitSet2Scrn();
                screen = 9;
            }
            break;
        }
        //
        //  SETTINGS MENU 2
        //
        case 9:
        {
            if (tfifa != SPINBOX_GetValue(hDEV_ID)){
                tfifa = SPINBOX_GetValue(hDEV_ID);
                EE_WriteBuffer(&tfifa, EE_TFIFA, 1);
            }            
            if (LIGHT_Ctrl1.modbusLight.index != SPINBOX_GetValue(hBIN_MAIN1)){
                LIGHT_Ctrl1.modbusLight.index  = SPINBOX_GetValue(hBIN_MAIN1);
                if (LIGHT_Ctrl1.Main1.index == 0) SPINBOX_SetValue(hBIN2_MAIN1, 0);
                ++lcsta;
            }
            if (LIGHT_Ctrl1.Led1.index != SPINBOX_GetValue(hBIN_LED1)){
                LIGHT_Ctrl1.Led1.index  = SPINBOX_GetValue(hBIN_LED1);
                if (LIGHT_Ctrl1.Led1.index == 0) SPINBOX_SetValue(hBIN2_LED1, 0);
                ++lcsta;
            }
            if (LIGHT_Ctrl1.Led2.index != SPINBOX_GetValue(hBIN_LED2)){
                LIGHT_Ctrl1.Led2.index  = SPINBOX_GetValue(hBIN_LED2);
                if (LIGHT_Ctrl1.Led2.index == 0) SPINBOX_SetValue(hBIN2_LED2, 0);
                ++lcsta;
            }
            if (LIGHT_Ctrl1.Led3.index != SPINBOX_GetValue(hBIN_LED3)){
                LIGHT_Ctrl1.Led3.index  = SPINBOX_GetValue(hBIN_LED3);
                if (LIGHT_Ctrl1.Led3.index == 0) SPINBOX_SetValue(hBIN2_LED3, 0);
                ++lcsta;
            }
            if (LIGHT_Ctrl1.Out1.index != SPINBOX_GetValue(hBIN_OUT1)){
                LIGHT_Ctrl1.Out1.index  = SPINBOX_GetValue(hBIN_OUT1);
                if (LIGHT_Ctrl1.Out1.index == 0) SPINBOX_SetValue(hBIN2_OUT1,0);
                ++lcsta;
            }
            if (LIGHT_Ctrl1.Light1.index != SPINBOX_GetValue(hDIM_LIGHT1)){
                LIGHT_Ctrl1.Light1.index  = SPINBOX_GetValue(hDIM_LIGHT1);
                ++lcsta;
            }
            if (LIGHT_Ctrl1.Light2.index != SPINBOX_GetValue(hDIM_LIGHT2)){
                LIGHT_Ctrl1.Light2.index  = SPINBOX_GetValue(hDIM_LIGHT2);
                ++lcsta;
            }
            if (LIGHT_Ctrl1.Light3.index != SPINBOX_GetValue(hDIM_LIGHT3)){
                LIGHT_Ctrl1.Light3.index  = SPINBOX_GetValue(hDIM_LIGHT3);
                ++lcsta;
            }
            if (LIGHT_Ctrl2.Main1.index != SPINBOX_GetValue(hBIN2_MAIN1)){
                LIGHT_Ctrl2.Main1.index  = SPINBOX_GetValue(hBIN2_MAIN1);
                ++lcsta;
            }
            if (LIGHT_Ctrl2.Led1.index != SPINBOX_GetValue(hBIN2_LED1)){
                LIGHT_Ctrl2.Led1.index  = SPINBOX_GetValue(hBIN2_LED1);
                ++lcsta;
            }
            if (LIGHT_Ctrl2.Led2.index != SPINBOX_GetValue(hBIN2_LED2)){
                LIGHT_Ctrl2.Led2.index  = SPINBOX_GetValue(hBIN2_LED2);
                ++lcsta;
            }
            if (LIGHT_Ctrl2.Led3.index != SPINBOX_GetValue(hBIN2_LED3)){
                LIGHT_Ctrl2.Led3.index  = SPINBOX_GetValue(hBIN2_LED3);
                ++lcsta;
            }
            if (LIGHT_Ctrl2.Out1.index != SPINBOX_GetValue(hBIN2_OUT1)){
                LIGHT_Ctrl2.Out1.index  = SPINBOX_GetValue(hBIN2_OUT1);
                ++lcsta;
            }
            
            if (BUTTON_IsPressed(hBUTTON_Ok)){
                if (thsta){
                    thsta = 0;
                    SaveThermostatController(&thst,  EE_THST1);
                }
                if (lcsta){
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
                DSP_KillSet2Scrn();
                screen = 7;
            } else if (BUTTON_IsPressed(hBUTTON_Next)){                
                DSP_KillSet2Scrn();
                DSP_InitSet3Scrn();
                screen = 10;
            }
            break;
        }        
        //
        //  SETTINGS MENU 3
        //
        case 10:
        {
            if (rtctm.Hours != Dec2Bcd(SPINBOX_GetValue(hSPNBX_Hour))){
                rtctm.Hours = Dec2Bcd(SPINBOX_GetValue(hSPNBX_Hour));
                HAL_RTC_SetTime(&hrtc, &rtctm, RTC_FORMAT_BCD);
                RtcTimeValidSet();
            }            
            if (rtctm.Minutes != Dec2Bcd(SPINBOX_GetValue(hSPNBX_Minute))){
                rtctm.Minutes = Dec2Bcd(SPINBOX_GetValue(hSPNBX_Minute));
                HAL_RTC_SetTime(&hrtc, &rtctm, RTC_FORMAT_BCD);
                RtcTimeValidSet();
            }            
            if (rtcdt.Date != Dec2Bcd(SPINBOX_GetValue(hSPNBX_Day))){
                rtcdt.Date = Dec2Bcd(SPINBOX_GetValue(hSPNBX_Day));
                HAL_RTC_SetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);
                RtcTimeValidSet();
            }            
            if (rtcdt.Month != Dec2Bcd(SPINBOX_GetValue(hSPNBX_Month))){
                rtcdt.Month = Dec2Bcd(SPINBOX_GetValue(hSPNBX_Month));
                HAL_RTC_SetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);
                RtcTimeValidSet();
            }            
            if (rtcdt.Year != Dec2Bcd(SPINBOX_GetValue(hSPNBX_Year) - 2000)){
                rtcdt.Year = Dec2Bcd(SPINBOX_GetValue(hSPNBX_Year) - 2000);
                HAL_RTC_SetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);
                RtcTimeValidSet();
            }
            if (rtcdt.WeekDay != Dec2Bcd(DROPDOWN_GetSel(hDRPDN_WeekDay)+1)){
                rtcdt.WeekDay  = Dec2Bcd(DROPDOWN_GetSel(hDRPDN_WeekDay)+1);
                HAL_RTC_SetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);
                RtcTimeValidSet();
            }
            if (scrnsvr_clk_clr != SPINBOX_GetValue(hSPNBX_ScrnsvrClockColour)){
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
            if (BUTTON_IsPressed(hBUTTON_Ok)){
                if (thsta){
                    thsta = 0;
                    SaveThermostatController(&thst,  EE_THST1);
                }
                if (lcsta){
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
                screen = 7;
            } else if (BUTTON_IsPressed(hBUTTON_Next)){                
                DSP_KillSet3Scrn();
                DSP_InitSet5Scrn();
                screen = 13;
            }
            break;
        }
        //
        //  CLEAN
        //
        case 11:
        {
            if (menu_clean == 0){
                ++menu_clean;
                GUI_Clear();
                clrtmr = 60;
            }else if (menu_clean == 1){
                if (HAL_GetTick()-clean_tmr >= 1000){
                    
                    clean_tmr = HAL_GetTick();
                    
                    DISPResetScrnsvr();
                    
                    GUI_MULTIBUF_BeginEx(1);
                    
                    GUI_ClearRect(0,50,480,200);
                    if (clrtmr > 5){GUI_SetColor(GUI_GREEN); }else{GUI_SetColor(GUI_RED);BuzzerOn();HAL_Delay(1);BuzzerOff();}
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
                    else screen = 7;
                }
            }
            
            break;
        }        
        //
        //  SETTINGS MENU 4
        //
        case 12:
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
                screen = 7;
            }
            else if (BUTTON_IsPressed(hBUTTON_Next))
            {                
                DSP_KillSet4Scrn();
                DSP_InitSet5Scrn();
                screen = 13;
            }
            break;
        }
        //
        //  SETTINGS MENU 5
        //
        case 13:
        {
            
            if (BUTTON_IsPressed(hBUTTON_Ok))
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
                
                if(settingsChanged)
                {
                    Curtains_Save();
                    settingsChanged = 0;
                }
                DSP_KillSet5Scrn();
                screen = 7;
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
                    DSP_KillSet5Scrn();
                    curtainSettingMenu = 0;
                    
                    DSP_InitSet6Scrn();
                    screen = 14;
                }
            }
            break;
        }
        //
        //  SETTINGS MENU 6
        //
        case 14:
        {
            if (BUTTON_IsPressed(hBUTTON_Ok))
            {
                for(uint8_t i = lightsModbusSettingsMenu * LIGHTS_MODBUS_PER_SETTINGS; i < (((LIGHTS_MODBUS_SIZE - (lightsModbusSettingsMenu * LIGHTS_MODBUS_PER_SETTINGS)) >= LIGHTS_MODBUS_PER_SETTINGS) ? ((lightsModbusSettingsMenu * LIGHTS_MODBUS_PER_SETTINGS) + LIGHTS_MODBUS_PER_SETTINGS) : LIGHTS_MODBUS_SIZE); i++)
                {
                    if(Light_Modbus_GetRelay(lights_modbus + i) != SPINBOX_GetValue(lightsWidgets[i].relay))
                    {
                        settingsChanged = 1;
                        
                        Light_Modbus_SetRelay(lights_modbus + i, SPINBOX_GetValue(lightsWidgets[i].relay));
                    }
                    
                    if(lights_modbus[i].iconID != SPINBOX_GetValue(lightsWidgets[i].iconID))
                    {
                        settingsChanged = 1;
                        
                        lights_modbus[i].iconID = SPINBOX_GetValue(lightsWidgets[i].iconID);
                    }
                    
                    if(lights_modbus[i].controllerID_on != SPINBOX_GetValue(lightsWidgets[i].controllerID_on))
                    {
                        settingsChanged = 1;
                        
                        lights_modbus[i].controllerID_on = SPINBOX_GetValue(lightsWidgets[i].controllerID_on);
                    }
                    
                    if(Light_Modbus_GetOnDelayTime(lights_modbus + i) != SPINBOX_GetValue(lightsWidgets[i].controllerID_on_delay))
                    {
                        settingsChanged = 1;
                        
                        Light_Modbus_SetOnDelayTime(lights_modbus + i, SPINBOX_GetValue(lightsWidgets[i].controllerID_on_delay));
                    }
                    
                    if(Light_Modbus_GetOffTime(lights_modbus + i) != SPINBOX_GetValue(lightsWidgets[i].offTime))
                    {
                        settingsChanged = 1;
                        
                        Light_Modbus_SetOffTime(lights_modbus + i, SPINBOX_GetValue(lightsWidgets[i].offTime));
                    }
                    
                    if(lights_modbus[i].on_hour != SPINBOX_GetValue(lightsWidgets[i].on_hour))
                    {
                        settingsChanged = 1;
                        
                        lights_modbus[i].on_hour = SPINBOX_GetValue(lightsWidgets[i].on_hour);
                    }
                    
                    if(lights_modbus[i].on_minute != SPINBOX_GetValue(lightsWidgets[i].on_minute))
                    {
                        settingsChanged = 1;
                        
                        lights_modbus[i].on_minute = SPINBOX_GetValue(lightsWidgets[i].on_minute);
                    }
                    
                    if(lights_modbus[i].communication_type != SPINBOX_GetValue(lightsWidgets[i].communication_type))
                    {
                        settingsChanged = 1;
                        
                        lights_modbus[i].communication_type = SPINBOX_GetValue(lightsWidgets[i].communication_type);
                    }
                    
                    if(lights_modbus[i].local_pin != SPINBOX_GetValue(lightsWidgets[i].local_pin))
                    {
                        settingsChanged = 1;
                        
                        lights_modbus[i].local_pin = SPINBOX_GetValue(lightsWidgets[i].local_pin);
                    }
                    
                    if(lights_modbus[i].sleep_time != SPINBOX_GetValue(lightsWidgets[i].sleep_time))
                    {
                        settingsChanged = 1;
                        
                        lights_modbus[i].sleep_time = SPINBOX_GetValue(lightsWidgets[i].sleep_time);
                    }
                    
                    if(lights_modbus[i].button_external != SPINBOX_GetValue(lightsWidgets[i].button_external))
                    {
                        settingsChanged = 1;
                        
                        lights_modbus[i].button_external = SPINBOX_GetValue(lightsWidgets[i].button_external);
                    }
                    
                    if(Light_Modbus_isTiedToMainLight(lights_modbus + i) != CHECKBOX_GetState(lightsWidgets[i].tiedToMainLight))
                    {
                        settingsChanged = 1;
                        
                        if(CHECKBOX_GetState(lightsWidgets[i].tiedToMainLight)) Light_Modbus_TieToMainLight(lights_modbus + i);
                        else Light_Modbus_UntieFromMainLight(lights_modbus + i);
                    }
                }
                
                if(settingsChanged)
                {
                    Lights_Modbus_Save();
                    settingsChanged = 0;
                }
                DSP_KillSet6Scrn();
                screen = 7;
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
                    DSP_KillSet6Scrn();
                    lightsModbusSettingsMenu = 0;
                    
                    DSP_InitSet7Scrn();
                    screen = 17;
                }
            }
            break;
        }
        //
        //  LIGHTS
        //     
        case 15:
        {
            if(shouldDrawScreen)
            {
                shouldDrawScreen = 0;
                
                GUI_MULTIBUF_BeginEx(1);
                
                GUI_Clear();
                GUI_SetPenSize(9);
                GUI_SetColor(GUI_GREEN);
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
                        
                        if(Light_Modbus_isActive(light)) GUI_DrawBitmap(Light_Modbus_GetIcon(light), x, y);
                        else GUI_DrawBitmap(Light_Modbus_GetIcon(light), x, y);
                        
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
        case 16:
        {
            if(shouldDrawScreen)
            {
                shouldDrawScreen = 0;
                
                GUI_MULTIBUF_BeginEx(1);
                
                GUI_Clear();
                GUI_SetPenSize(9);
                GUI_SetColor(GUI_GREEN);
                
                GUI_DrawLine(400,20,450,20);
                GUI_DrawLine(400,40,450,40);
                GUI_DrawLine(400,60,450,60);
                
//                GUI_SetColor(GUI_RED);
//                GUI_DrawLine(380,10,380,262);
                
                
                GUI_DrawBitmap(&bmprevious, 0, 192);
                GUI_DrawBitmap(&bmnext, 320, 192);
                
                
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
        case 17:
        {
            if(BUTTON_IsPressed(hBUTTON_SYSRESTART))
            {
                SYSRestart();
            }
            else if(BUTTON_IsPressed(hBUTTON_Ok))
            {
                if(Curtain_GetMoveTime() != SPINBOX_GetValue(hCurtainsMoveTime))
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
                
                if(settingsChanged)
                {
                    Curtains_Save();
                    EE_WriteBuffer(&bOnlyLeaveScreenSaverAfterTouch, EE_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, 1);
                    EE_WriteBuffer(&LightNightTimer_isEnabled, EE_LIGHT_NIGHT_TIMER, 1);
                    settingsChanged = 0;
                }
                DSP_KillSet7Scrn();
                screen = 7;
            }
            else if (BUTTON_IsPressed(hBUTTON_Next))
            {
                DSP_KillSet7Scrn();
                DSP_InitSet1Scrn();
                screen = 8;
            }
            
            break;
        }
        //
        //  SELECT SCREEN 2
        //     
        case 18:
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
                GUI_SetColor(GUI_RED);
                
                GUI_DrawLine(400,20,450,20);
                GUI_DrawLine(400,40,450,40);
                GUI_DrawLine(400,60,450,60);
                
                GUI_DrawLine(380,10,380,262);
                
                GUI_DrawLine(190,20,190,252);
                
                GUI_DrawBitmap(&bmnext, 385,159);
                
                GUI_DrawBitmap(&bmmobilePhone, 70, 76);
                GUI_DrawBitmap(&bmCLEAN, 245, 76);
                
                
                
                GUI_SetFont(GUI_FONT_24B_1);
                GUI_SetColor(GUI_ORANGE);
                GUI_SetTextMode(GUI_TM_TRANS);
                
                
                GUI_SetTextAlign(GUI_TA_HCENTER|GUI_TA_VCENTER);
                GUI_GotoXY(95, 176);
                lng(52);
                GUI_DispString(langText);
                
                GUI_SetTextAlign(GUI_TA_HCENTER|GUI_TA_VCENTER);
                GUI_GotoXY(285, 176);
                lng(6);
                GUI_DispString(langText);
                
                GUI_MULTIBUF_EndEx(1);
            }
            
            break;
        }
        //
        //  QR CODE
        //     
        case 19:
        {
            if(shouldDrawScreen)
            {
                shouldDrawScreen = 0;
                
                GUI_MULTIBUF_BeginEx(1);
                
                GUI_Clear();
                
                GUI_SetColor(GUI_WHITE);
                GUI_FillRect(0, 0, 480, 272);
                
                GUI_SetPenSize(9);
                GUI_SetColor(GUI_GREEN);
                GUI_DrawLine(400,20,450,20);
                GUI_DrawLine(400,40,450,40);
                GUI_DrawLine(400,60,450,60);
                
                int siz = strlen((char*)QR_Code_Get(qr_code_draw_id));
                
                GUI_HMEM hqr = GUI_QR_Create((char*)QR_Code_Get(qr_code_draw_id), 8, GUI_QR_ECLEVEL_M, 0);
                GUI_QR_Draw(hqr, 10, 10);
                GUI_QR_Delete(hqr);
                
                
                GUI_MULTIBUF_EndEx(1);
            }
            
            break;
        }
        //
        //  LIGHT SETTINGS
        //     
        case 20:
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
                GUI_SetColor(GUI_RED);
                
                GUI_DrawLine(400,20,450,20);
                GUI_DrawLine(400,40,450,40);
                GUI_DrawLine(400,60,450,60);
                
                //GUI_DrawRect(20, 20, 100, 100);
                GUI_SetColor(lights_modbus[light_selectedIndex].color);
                GUI_FillRect(20, 20, 100, 100);
                
                GUI_DrawBitmap(&bmcolorSpectrum, 20, 110);
                GUI_DrawBitmap(&bmblackWhiteGradient, 20, 180);
                
                GUI_MULTIBUF_EndEx(1);
            }
            
            break;
        }
        //
        //  RESET MENU SWITCHES
        //        
        case 0:
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
    
    
    
    
    if((isButtonActive_old != IsButtonActive()) && (!isButtonActive_old))
    {
        for(uint8_t i = 0; i < Curtains_getCount(); i++)
        {
            Lights_Modbus_Button_External_Press(lights_modbus + i);
        }
        
        isButtonActive_old = IsButtonActive();
    }
    
    
    
    
    
    if((HAL_GetTick() - everyMinuteTimerStart) >= (60 * 1000))
    {
        everyMinuteTimerStart = HAL_GetTick();
        
        for(uint8_t i = 0; i < Lights_Modbus_getCount(); i++)
        {
            if(Light_Modbus_isTimeOnEnabled(lights_modbus + i) && Light_Modbus_isTimeToTurnOn(lights_modbus + i))
            {
                Light_Modbus_On(lights_modbus + i);
                
                if(screen == 15) shouldDrawScreen = 1;
                else if((screen == 0) || (screen == 1)) screen = 7;
            }
        }
    }
    
    
    
    
    
    if(light_settingsTimerStart && ((HAL_GetTick() - light_settingsTimerStart) >= (2 * 1000)))
    {
        light_settingsTimerStart = 0;
        screen = 20;
        shouldDrawScreen = 1;
    }
    
    
    
    
    
    if(Ventilator_isOnDelayTimerActive() && Ventilator_hasOnDelayTimerExpired())
    {
        Ventilator_OnDelayTimerDeactivate();
        
        for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; ++i)
        {
            if(Light_Modbus_isOffTimeEnabled(lights_modbus + i)) Light_Modbus_On(lights_modbus + i);
            if(screen == 15) shouldDrawScreen = 1;
        }
    }
    
    
    
    
    
    for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; ++i)
    {
        if(Light_Modbus_isOnDelayTimeTimerActive(lights_modbus + i) && Light_Modbus_hasOnDelayTimeTimerExpired(lights_modbus + i))
        {
            Light_Modbus_OnDelayTimeTimerDeactivate(lights_modbus + i);
            Light_Modbus_On(lights_modbus + i);
        }
        
        if(screen == 15) shouldDrawScreen = 1;
    }
    
    
    
    
    
    for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; ++i)
    {
        if(Light_Modbus_isOffTimeTimerActive(lights_modbus + i) && Light_Modbus_hasOffTimeTimerExpired(lights_modbus + i))
        {
            Light_Modbus_OffTimeTimerDeactivate(lights_modbus + i);
            Light_Modbus_Off(lights_modbus + i);
        }
        
        if(screen == 15) shouldDrawScreen = 1;
    }
    
    
    
    
    if(LightNightTimer_StartTime && ((HAL_GetTick() - LightNightTimer_StartTime) >= (LIGHT_NIGHT_TIMER_DURATION * 1000)))
    {
        LightNightTimer_StartTime = 0;
        
        for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; ++i)
        {
            if(Light_Modbus_isTiedToMainLight(lights_modbus + i) && Light_Modbus_isActive(lights_modbus + i))
            {
                Light_Modbus_Off(lights_modbus + i);
            }
        }
        
        if(screen == 0) screen = 1;
        shouldDrawScreen = 1;
    }
    
    
    
    
    
    if (!IsScrnsvrActiv()){
        if ((HAL_GetTick() - scrnsvr_tmr) >= (uint32_t)(scrnsvr_tout*1000)){
            if      (screen == 8) DSP_KillSet1Scrn();
            else if (screen == 9) DSP_KillSet2Scrn();
            else if (screen == 10)DSP_KillSet3Scrn();
            else if (screen == 12)DSP_KillSet4Scrn();
            else if (screen == 13)DSP_KillSet5Scrn();
            else if (screen == 14)DSP_KillSet6Scrn();
            else if (screen == 17)DSP_KillSet7Scrn();
            DISPSetBrightnes(low_bcklght);
            ScrnsvrInitReset();
            ScrnsvrSet();            
            screen = 7;
        }
    }
    //
    //  CHECK IF ENABLED RELAY 4 AUTO SHUTDOWN TIMER
    //
    if (out1_tmr){
        if ((HAL_GetTick() - out1_tmr) >= (uint32_t)(SECONDS_PER_HOUR*4000)){
            LIGHT_Ctrl1.Out1.value = 0;
            out1_tmr = 0;
        }
    }
    //
    //  DISPLAY SETTINGS MENU
    //
    if (DISPMenuSettings(btnset)&&(screen < 8)){
        DSP_InitSet1Scrn();
        screen = 8;
    }
    
    if (HAL_GetTick() - rtctmr >= 1000){
        rtctmr = HAL_GetTick();
        if (screen < 2) DISPDateTime();
        if (!IsScrnsvrActiv()) MVUpdateSet(); 
    }
}
/**
  * @brief
  * @param
  * @retval
  */
static void DSP_InitSet1Scrn(void){
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT); 
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    /*hThstControl = RADIO_CreateEx(10, 20, 150, 80, 0,WM_CF_SHOW, 0, ID_ThstControl, 3, 20);
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

    hThstMaxSetPoint = SPINBOX_CreateEx(110, 20, 90, 30, 0, WM_CF_SHOW, ID_ThstMaxSetPoint, 15, 40);
    SPINBOX_SetEdge(hThstMaxSetPoint, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstMaxSetPoint, thst.sp_max);

    hThstMinSetPoint = SPINBOX_CreateEx(110, 70, 90, 30, 0, WM_CF_SHOW, ID_ThstMinSetPoint, 15, 40);
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
    
    GUI_DrawHLine(12, 5, 320);
    GUI_DrawHLine(130, 5, 320);*/
    
    
    hThstRelay1 = SPINBOX_CreateEx(10, 20, 110, 40, 0, WM_CF_SHOW, ID_ThstRelay1, 0, 512);
    SPINBOX_SetEdge(hThstRelay1, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstRelay1, thst.relay1);
    
    hThstRelay2 = SPINBOX_CreateEx(10, 70, 110, 40, 0, WM_CF_SHOW, ID_ThstRelay2, 0, 512);
    SPINBOX_SetEdge(hThstRelay2, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstRelay2, thst.relay2);
    
    hThstRelay3 = SPINBOX_CreateEx(10, 120, 110, 40, 0, WM_CF_SHOW, ID_ThstRelay3, 0, 512);
    SPINBOX_SetEdge(hThstRelay3, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstRelay3, thst.relay3);
    
    hThstRelay4 = SPINBOX_CreateEx(10, 170, 110, 40, 0, WM_CF_SHOW, ID_ThstRelay4, 0, 512);
    SPINBOX_SetEdge(hThstRelay4, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstRelay4, thst.relay4);
    
    
    
    
    hNTC_Offset = SPINBOX_CreateEx(200, 20, 110, 40, 0, WM_CF_SHOW, ID_NTC_Offset, -100, 100);
    SPINBOX_SetEdge(hNTC_Offset, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hNTC_Offset, thst.mv_offset);
    
    /*hTempDiff = SPINBOX_CreateEx(10, 220, 90, 30, 0, WM_CF_SHOW, ID_TempDiff, 15, 40);
    SPINBOX_SetEdge(hTempDiff, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hTempDiff, );*/
    
    hFanDiff = SPINBOX_CreateEx(200, 70, 110, 40, 0, WM_CF_SHOW, ID_FanDiff, 0, 50);
    SPINBOX_SetEdge(hFanDiff, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hFanDiff, thst.fan_diff);
    
    hThstRelay3Delay = SPINBOX_CreateEx(200, 120, 110, 40, 0, WM_CF_SHOW, ID_ThstRelay3Delay, 0, 100);
    SPINBOX_SetEdge(hThstRelay3Delay, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstRelay3Delay, thst.relay3Delay);
    
    hThstRelay4Delay = SPINBOX_CreateEx(200, 170, 110, 40, 0, WM_CF_SHOW, ID_ThstRelay4Delay, 0, 100);
    SPINBOX_SetEdge(hThstRelay4Delay, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hThstRelay4Delay, thst.relay4Delay);
    
    
    
    
    hBUTTON_Next = BUTTON_Create(410, 180, 60, 30, ID_Next, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Next, "NEXT");

    hBUTTON_Ok = BUTTON_Create(410, 230, 60, 30, ID_Ok, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_Ok, "SAVE");
    
    
    
    
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);
    
    GUI_GotoXY(130, 28);
    GUI_DispString("RELAY");
    GUI_GotoXY(130, 40);
    GUI_DispString("1 ADDR");
    
    GUI_GotoXY(130, 78);
    GUI_DispString("RELAY");
    GUI_GotoXY(130, 90);
    GUI_DispString("2 ADDR");
    
    GUI_GotoXY(130, 128);
    GUI_DispString("RELAY");
    GUI_GotoXY(130, 140);
    GUI_DispString("3 ADDR");
    
    GUI_GotoXY(130, 178);
    GUI_DispString("RELAY");
    GUI_GotoXY(130, 190);
    GUI_DispString("4 ADDR");
    
    
    
    GUI_GotoXY(320, 28);
    GUI_DispString("NTC x0.1*C");
    GUI_GotoXY(320, 40);
    GUI_DispString("OFFSET");
    
    GUI_GotoXY(320, 78);
    GUI_DispString("TEMPERATURE");
    GUI_GotoXY(320, 90);
    GUI_DispString("DIFFERENCE x0.1*C");
    
    GUI_GotoXY(320, 128);
    GUI_DispString("DELAY");
    GUI_GotoXY(320, 140);
    GUI_DispString("ON x10s");
    
    GUI_GotoXY(320, 178);
    GUI_DispString("DELAY");
    GUI_GotoXY(320, 190);
    GUI_DispString("ON x10s");
    
    
    GUI_MULTIBUF_EndEx(1);
}
/**
  * @brief
  * @param
  * @retval
  */
static void DSP_KillSet1Scrn(void){
    /*WM_DeleteWindow(hThstControl);
    WM_DeleteWindow(hFanControl);
    WM_DeleteWindow(hThstMaxSetPoint);
    WM_DeleteWindow(hThstMinSetPoint);
    WM_DeleteWindow(hFanDiff);
    WM_DeleteWindow(hFanLowBand);
    WM_DeleteWindow(hFanHiBand);*/
    WM_DeleteWindow(hThstRelay1);
    WM_DeleteWindow(hThstRelay2);
    WM_DeleteWindow(hThstRelay3);
    WM_DeleteWindow(hThstRelay4);
    WM_DeleteWindow(hNTC_Offset);
    WM_DeleteWindow(hThstRelay3Delay);
    WM_DeleteWindow(hThstRelay4Delay);
    WM_DeleteWindow(hFanDiff);
    WM_DeleteWindow(hBUTTON_Ok);
    WM_DeleteWindow(hBUTTON_Next);
}
/**
  * @brief
  * @param
  * @retval
  */
static void DSP_InitSet2Scrn(void){
    GUI_SelectLayer(0);
    GUI_Clear();
    GUI_SelectLayer(1);
    GUI_SetBkColor(GUI_TRANSPARENT); 
    GUI_Clear();
    GUI_MULTIBUF_BeginEx(1);

    hDEV_ID = SPINBOX_CreateEx(10, 10, 90, 30, 0, WM_CF_SHOW, ID_DEV_ID, 1, 254);
    SPINBOX_SetEdge(hDEV_ID, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hDEV_ID, tfifa);
    
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
    
    GUI_GotoXY(110, 14);
    GUI_DispString("DEVICE");
    GUI_GotoXY(110, 26);
    GUI_DispString("BUS ID");
    
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
static void DSP_KillSet2Scrn(void){
    WM_DeleteWindow(hDEV_ID);
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
static void DSP_InitSet3Scrn(void){
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
static void DSP_KillSet3Scrn(void){
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
    
    int x = 30, y = 5;
    
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
        
        
        
        lightsWidgets[i].relay = SPINBOX_CreateEx(x, y, 90, 30, 0, WM_CF_SHOW, ID_LightsModbusRelay * i, 0, 512);
        SPINBOX_SetEdge(lightsWidgets[i].relay, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].relay, Light_Modbus_GetRelay(lights_modbus + i));
        
        lightsWidgets[i].iconID = SPINBOX_CreateEx(x, y + 33, 90, 30, 0, WM_CF_SHOW, (ID_LightsModbusRelay * i) + 1, 0, 512);
        SPINBOX_SetEdge(lightsWidgets[i].iconID, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].iconID, lights_modbus[i].iconID);
        
        lightsWidgets[i].controllerID_on = SPINBOX_CreateEx(x, y + 66, 90, 30, 0, WM_CF_SHOW, (ID_LightsModbusRelay * i) + 2, 0, 512);
        SPINBOX_SetEdge(lightsWidgets[i].controllerID_on, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].controllerID_on, lights_modbus[i].controllerID_on);
        
        lightsWidgets[i].controllerID_on_delay = SPINBOX_CreateEx(x, y + 99, 90, 30, 0, WM_CF_SHOW, (ID_LightsModbusRelay * i) + 3, 0, 512);
        SPINBOX_SetEdge(lightsWidgets[i].controllerID_on_delay, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].controllerID_on_delay, lights_modbus[i].controllerID_on_delay);
        
        lightsWidgets[i].offTime = SPINBOX_CreateEx(x, y + 132, 90, 30, 0, WM_CF_SHOW, (ID_LightsModbusRelay * i) + 4, 0, 512);
        SPINBOX_SetEdge(lightsWidgets[i].offTime, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].offTime, Light_Modbus_GetOffTime(lights_modbus + i));
        
        lightsWidgets[i].on_hour = SPINBOX_CreateEx(x, y + 165, 90, 30, 0, WM_CF_SHOW, (ID_LightsModbusRelay * i) + 5, 0, 512);
        SPINBOX_SetEdge(lightsWidgets[i].on_hour, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].on_hour, lights_modbus[i].on_hour);
        
        lightsWidgets[i].on_minute = SPINBOX_CreateEx(x, y + 198, 90, 30, 0, WM_CF_SHOW, (ID_LightsModbusRelay * i) + 6, 0, 512);
        SPINBOX_SetEdge(lightsWidgets[i].on_minute, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].on_minute, lights_modbus[i].on_minute);
        
        GUI_SetColor(GUI_WHITE);
        GUI_SetFont(GUI_FONT_13_1);
        GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);
        
        GUI_GotoXY(x + 90 + 10, y + 8);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 90 + 10, y + 8 + 12);
        GUI_DispString("RELAY");
        
        GUI_GotoXY(x + 90 + 10, y + 33 + 8);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 90 + 10, y + 33 + 8 + 12);
        GUI_DispString("ICON");
        
        GUI_GotoXY(x + 90 + 10, y + 66 + 8);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 90 + 10, y + 66 + 8 + 12);
        GUI_DispString("ON ID");
        
        GUI_GotoXY(x + 90 + 10, y + 99 + 8);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 90 + 10, y + 99 + 8 + 12);
        GUI_DispString("ON ID DELAY");
        
        GUI_GotoXY(x + 90 + 10, y + 132 + 8);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 90 + 10, y + 132 + 8 + 12);
        GUI_DispString("DELAY OFF");
        
        GUI_GotoXY(x + 90 + 10, y + 165 + 8);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 90 + 10, y + 165 + 8 + 12);
        GUI_DispString("HOUR ON");
        
        GUI_GotoXY(x + 90 + 10, y + 198 + 8);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 90 + 10, y + 198 + 8 + 12);
        GUI_DispString("MINUTE ON");
        
        x = 200;
        y = 5;
        
        lightsWidgets[i].communication_type = SPINBOX_CreateEx(x, y, 90, 30, 0, WM_CF_SHOW, (ID_LightsModbusRelay * i) + 7, 0, 3);
        SPINBOX_SetEdge(lightsWidgets[i].communication_type, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].communication_type, lights_modbus[i].communication_type);
        
        lightsWidgets[i].local_pin = SPINBOX_CreateEx(x, y + 33, 90, 30, 0, WM_CF_SHOW, (ID_LightsModbusRelay * i) + 8, 0, 512);
        SPINBOX_SetEdge(lightsWidgets[i].local_pin, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].local_pin, lights_modbus[i].local_pin);
        
        lightsWidgets[i].sleep_time = SPINBOX_CreateEx(x, y + 66, 90, 30, 0, WM_CF_SHOW, (ID_LightsModbusRelay * i) + 9, 0, 512);
        SPINBOX_SetEdge(lightsWidgets[i].sleep_time, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].sleep_time, lights_modbus[i].sleep_time);
        
        lightsWidgets[i].button_external = SPINBOX_CreateEx(x, y + 99, 90, 30, 0, WM_CF_SHOW, (ID_LightsModbusRelay * i) + 10, 0, 512);
        SPINBOX_SetEdge(lightsWidgets[i].button_external, SPINBOX_EDGE_CENTER);
        SPINBOX_SetValue(lightsWidgets[i].button_external, lights_modbus[i].button_external);
        
        lightsWidgets[i].tiedToMainLight = CHECKBOX_Create(x, y + 132, 130, 15, 0, (ID_LightsModbusRelay * i) + 11, WM_CF_SHOW);
        CHECKBOX_SetTextColor(lightsWidgets[i].tiedToMainLight, GUI_GREEN);
        CHECKBOX_SetText(lightsWidgets[i].tiedToMainLight, "TIED TO MAIN LIGHT");
        CHECKBOX_SetState(lightsWidgets[i].tiedToMainLight, Light_Modbus_isTiedToMainLight(lights_modbus + i));
        
        GUI_GotoXY(x + 90 + 10, y + 8);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 90 + 10, y + 8 + 12);
        GUI_DispString("COMM. TYPE");
        
        GUI_GotoXY(x + 90 + 10, y + 33 + 8);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 90 + 10, y + 33 + 8 + 12);
        GUI_DispString("LOCAL PIN");
        
        GUI_GotoXY(x + 90 + 10, y + 66 + 8);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 90 + 10, y + 66 + 8 + 12);
        GUI_DispString("SLEEP TIME");
        
        GUI_GotoXY(x + 90 + 10, y + 99 + 8);
        GUI_DispString("LIGHT ");
        GUI_DispDec(i + 1, ((i + 1) < 10 ? 1 : 2));
        GUI_GotoXY(x + 90 + 10, y + 99 + 8 + 12);
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
    
    
    hCurtainsMoveTime = SPINBOX_CreateEx(10, 20, 110, 40, 0, WM_CF_SHOW, ID_CurtainsMoveTime, 0, 60);
    SPINBOX_SetEdge(hCurtainsMoveTime, SPINBOX_EDGE_CENTER);
    SPINBOX_SetValue(hCurtainsMoveTime, Curtain_GetMoveTime());
    
    hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH = CHECKBOX_Create(10, 70, 205, 20, 0, ID_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, WM_CF_SHOW);
    CHECKBOX_SetTextColor(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, GUI_GREEN);
    CHECKBOX_SetText(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, "ONLY LEAVE SCRNSVR AFTER TOUCH");
    CHECKBOX_SetState(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH, bOnlyLeaveScreenSaverAfterTouch);
    
    hCHKBX_LIGHT_NIGHT_TIMER = CHECKBOX_Create(10, 100, 170, 20, 0, ID_LIGHT_NIGHT_TIMER, WM_CF_SHOW);
    CHECKBOX_SetTextColor(hCHKBX_LIGHT_NIGHT_TIMER, GUI_GREEN);
    CHECKBOX_SetText(hCHKBX_LIGHT_NIGHT_TIMER, "LiGHT OFF TIMER AFTER 20h");
    CHECKBOX_SetState(hCHKBX_LIGHT_NIGHT_TIMER, LightNightTimer_isEnabled);
    
    hBUTTON_SYSRESTART = BUTTON_Create(10, 230, 60, 30, ID_SYSRESTART, WM_CF_SHOW);
    BUTTON_SetText(hBUTTON_SYSRESTART, "RESTART");
    
    
    
    GUI_SetColor(GUI_WHITE);
    GUI_SetFont(GUI_FONT_13_1);
    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_VCENTER);
    
    GUI_GotoXY(130, 28);
    GUI_DispString("CURTAINS");
    GUI_GotoXY(130, 40);
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
    WM_DeleteWindow(hCurtainsMoveTime);
    WM_DeleteWindow(hCHKBX_ONLY_LEAVE_SCRNSVR_AFTER_TOUCH);
    WM_DeleteWindow(hCHKBX_LIGHT_NIGHT_TIMER);
    WM_DeleteWindow(hBUTTON_SYSRESTART);
    WM_DeleteWindow(hBUTTON_Next);
    WM_DeleteWindow(hBUTTON_Ok);
}



/**
  * @brief  Display Backlight LED brightnes control
  * @param  brightnes_high_level
  * @retval DISP sreensvr_tmr loaded with system_tmr value
  */
void DISPSetBrightnes(uint8_t val){
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
static void DISPDateTime(void){
    char dbuf[32];
    static uint8_t old_day = 0;
    if(!IsRtcTimeValid()) return; // nothing to display untill system rtc validated
    HAL_RTC_GetTime(&hrtc, &rtctm, RTC_FORMAT_BCD);
    HAL_RTC_GetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);
    /************************************/
    /*   CHECK IS SCREENSAVER ENABLED   */
    /************************************/ 
    if(scrnsvr_ena_hour>=scrnsvr_dis_hour){
        if      (Bcd2Dec(rtctm.Hours)>=scrnsvr_ena_hour) ScrnsvrEnable();
        else if (Bcd2Dec(rtctm.Hours)<scrnsvr_dis_hour) ScrnsvrEnable();
        else if (IsScrnsvrEnabled())ScrnsvrDisable(), screen = 7;
    }else if(scrnsvr_ena_hour<scrnsvr_dis_hour){
        if((Bcd2Dec(rtctm.Hours<scrnsvr_dis_hour))&&(Bcd2Dec(rtctm.Hours)>=scrnsvr_ena_hour)) ScrnsvrEnable();
        else if(IsScrnsvrEnabled()) ScrnsvrDisable(), screen = 7;
    }
    /************************************/
    /*      DISPLAY  DATE  &  TIME      */
    /************************************/ 
    if (IsScrnsvrActiv()&&IsScrnsvrEnabled()&&IsScrnsvrClkActiv()){
        if(!IsScrnsvrInitActiv()||(old_day!=rtcdt.WeekDay)){
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
    }else if(old_min != rtctm.Minutes){
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
    
    if (old_day != rtcdt.WeekDay){
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
void DISPResetScrnsvr(void){
    if(IsScrnsvrActiv() && IsScrnsvrEnabled()) screen = 7;
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
void DISPSetPoint(void){
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
void PID_Hook(GUI_PID_STATE * pTS){
    uint8_t click = 0;
    if (pTS->Pressed  == 1U){
        pTS->Layer = 1U;
        if ((pTS->y > 60) && (pTS->y < 200) && ((screen == 3) || (screen == 4) || (screen == 5))){
            click = 1;
            ctrl1 = 0;
            ctrl2 = 0;
            ctrl3 = 0;
            if      ((pTS->x >   0) && (pTS->x < 160)) ctrl1 = 1;
            else if ((pTS->x > 160) && (pTS->x < 320)) ctrl2 = 1;
            else if ((pTS->x > 320) && (pTS->x < 480)) ctrl3 = 1;
        }

        if ((pTS->x > 400) && (pTS->y < 60) && ((screen <= 2) || (screen == 6) || ((screen > 14) && (screen != 17)))){
            click = 1;
            if      (screen  < 2) screen = 2;
            else if ((screen == 2) || (screen == 6) || ((screen > 14) && (screen != 17))) screen = 7;
        }
        else if(screen == 2)
        {
            if(pTS->x < 400)
            {
                if(pTS->y < 136)
                {
                    if(pTS->x < 190)
                    {
                        screen = 15;
                        shouldDrawScreen = 1;
                    }
                    else
                    {
                        screen = 6;
                    }
                }
                else
                {
                    if(pTS->x < 190)
                    {
                        screen = 16;
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
                        
                        screen = 19;
                        qr_code_draw_id = QR_CODE_WIFI_ID;
                        shouldDrawScreen = 1;
                    }
                }
            }
            else if(pTS->y > 159)
            {
                shouldDrawScreen = 1;
                menu_lc = 0;
                screen = 18;
            }
            
            if(screen != 2) click = 1;
        }
        else if((pTS->x > 0)&&(pTS->x < 120)&&(pTS->y > 200)&&(pTS->y < 272)&&((screen == 4)||(screen == 5))){
            click = 1;
            screen = 3;
        }else if((pTS->x > 120)&&(pTS->x < 240)&&(pTS->y > 200)&&(pTS->y < 272)&&((screen == 3)||(screen == 5))){
            click = 1;
            screen = 4;
        }else if((pTS->x > 240)&&(pTS->y > 200)&&(pTS->x < 360)&&(pTS->y < 272)&&((screen == 3)||(screen == 4))){
            click = 1;
            screen = 5;
        }else if((pTS->x > 360)&&(pTS->y > 200)&&(pTS->x < 480)&&(pTS->y < 272)&&((screen == 3)||(screen == 4)||(screen == 5))){
            click = 1;
            screen = 7;
        }
        else if(screen == 6)
        {
            if((pTS->x > BTN_INC_X0)&&(pTS->y > BTN_INC_Y0)&&(pTS->x < BTN_INC_X1)&&(pTS->y < BTN_INC_Y1))
            {
                click = 1;
                btninc = 1;
            }
            else if((pTS->x > BTN_DEC_X0)&&(pTS->y > BTN_DEC_Y0)&&(pTS->x < BTN_DEC_X1)&&(pTS->y < BTN_DEC_Y1)){
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
        else if(screen == 15) //lights
        {
            light_selectedIndex = LIGHTS_MODBUS_SIZE; // clear selected light
            
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
                        light_settingsTimerStart = HAL_GetTick();
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
        else if(screen == 16) //curtains
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
                else if(pTS->y > 192)
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
        else if(screen == 18)   // select screen 2
        {
            if(pTS->x < 400)
            {
                if((pTS->y > 116) && (pTS->y < 216))
                {
                    if(pTS->x < 190)
                    {
                        screen = 19;
                        qr_code_draw_id = QR_CODE_APP_ID;
                        shouldDrawScreen = 1;
                    }
                    else
                    {
                        screen = 11;
                    }
                }
            }
            else if(pTS->y > 159)
            {
                screen = 2;
                menu_lc = 0;
                shouldDrawScreen = 1;
            }
        }
        else if(screen == 20)  // light settings
        {
            if(pTS->y < 180) Light_Modbus_SetColor(lights_modbus + light_selectedIndex, LCD_GetPixelColor(pTS->x, pTS->y));
            else Light_Modbus_SetBrightness(lights_modbus + light_selectedIndex, ((pTS->x - 20) / (float)bmblackWhiteGradient.XSize) * 100);
            shouldDrawScreen = 1;
        }
        else if((pTS->x > 100)&&(pTS->y > 100)&&(pTS->x < 400)&&(pTS->y < 272)&&(screen == 0))
        {
            if((!bOnlyLeaveScreenSaverAfterTouch) || (bOnlyLeaveScreenSaverAfterTouch && (!IsScrnsvrActiv())))
            {
                uint8_t isAnyLightActive = 0;
                
                click = 1;
                screen = 1;
                
                
                
                for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; ++i)
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
                    if(isAnyLightActive) Ventilator_OnDelayTimerDeactivate();
                    
                    for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; ++i)
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
        
        if ((pTS->x > 380)&&(pTS->y > 5)&&(pTS->x < 475)&&(pTS->y < 100)){
            btnset = 1;
        }
        
        if (click){
            BuzzerOn();
            HAL_Delay(1);
            BuzzerOff();
        }
    }
    else
    {
        if(screen == 1) screen = 0;
        else if(screen == 15)
        {
            if(light_settingsTimerStart)
            {
                light_settingsTimerStart = 0;
                Light_Modbus_Flip(lights_modbus + light_selectedIndex);
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
    DISPResetScrnsvr(); 
}
/**
  * @brief
  * @param
  * @retval
  */
static uint8_t DISPMenuSettings(uint8_t btn){
    static uint8_t last_state = 0U;
    static uint32_t menu_tmr = 0U;
    if      ((btn == 1U) && (last_state == 0U)){
        last_state = 1U;
        menu_tmr = HAL_GetTick(); 
    } else if ((btn == 1U) && (last_state == 1U)){
        if((HAL_GetTick() - menu_tmr) >= SETTINGS_MENU_ENABLE_TIME){
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
void DISP_UpdateLog(const char *pbuf){
    static char displog[6][128];	
    int i = 5;	
    GUI_ClearRect(120, 80, 480, 240);
    GUI_SetTextAlign(GUI_TA_LEFT|GUI_TA_TOP);
    GUI_SetBkColor(GUI_TRANSPARENT);
    GUI_SetFont(&GUI_Font16B_1);
	GUI_SetColor(GUI_WHITE);    
    do{
        ZEROFILL(displog[i], COUNTOF(displog[i]));
        strcpy(displog[i], displog[i-1]);
        GUI_DispStringAt(displog[i], 125, 200-(i*20));
    }while(--i);
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
static void SaveLightController(LIGHT_CtrlTypeDef* lc, uint16_t addr){
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
static void ReadLightController(LIGHT_CtrlTypeDef* lc, uint16_t addr){
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
/**
  * @brief
  * @param
  * @retval
  */
void SaveThermostatController(THERMOSTAT_TypeDef* tc, uint16_t addr){
    uint8_t buf[31];
    buf[0] = tc->th_ctrl;
    buf[1] = tc->th_state;
    buf[2] = tc->mv_temp>>8;
    buf[3] = tc->mv_temp&0xFF;
    buf[4] = tc->mv_offset;
    buf[5] = tc->mv_ntcref>>8;
    buf[6] = tc->mv_ntcref&0xFF;
    buf[7] = tc->mv_nctbeta>>8;
    buf[8] = tc->mv_nctbeta&0xFF;
    buf[9] = tc->sp_temp;
    buf[10] = tc->sp_diff;
    buf[11] = tc->sp_max;
    buf[12] = tc->sp_min;
    buf[13] = tc->fan_ctrl;
    buf[14] = tc->fan_speed;
    buf[15] = tc->fan_diff;
    buf[16] = tc->fan_loband;
    buf[17] = tc->fan_hiband;
    buf[18] = tc->fan_quiet_start;
    buf[19] = tc->fan_quiet_end;
    buf[20] = tc->fan_quiet_speed;
    buf[21] = (tc->relay1 >> 8) & 0xFF;
    buf[22] = tc->relay1 & 0xFF;
    buf[23] = (tc->relay2 >> 8) & 0xFF;
    buf[24] = tc->relay2 & 0xFF;
    buf[25] = (tc->relay3 >> 8) & 0xFF;
    buf[26] = tc->relay3 & 0xFF;
    buf[27] = (tc->relay4 >> 8) & 0xFF;
    buf[28] = tc->relay4 & 0xFF;
    buf[29] = tc->relay3Delay;
    buf[30] = tc->relay4Delay;
    EE_WriteBuffer(buf, addr, 31);
}
/**
  * @brief
  * @param
  * @retval
  */
void ReadThermostatController(THERMOSTAT_TypeDef* tc, uint16_t addr){
    uint8_t buf[31];
    EE_ReadBuffer(buf, addr, 31);
    tc->th_ctrl         = buf[0];
    tc->th_state        = buf[1];
    tc->mv_temp         =(buf[2]<<8)|buf[3];
    tc->mv_offset       = buf[4];
    tc->mv_ntcref       =(buf[5]<<8)|buf[6];
    tc->mv_nctbeta      =(buf[7]<<8)|buf[8];
    tc->sp_temp         = buf[9];
    tc->sp_diff         = buf[10];
    tc->sp_max          = 35; //buf[11];
    tc->sp_min          = 10; //buf[12];
    tc->fan_ctrl        = buf[13];
    tc->fan_speed       = buf[14];
    tc->fan_diff        = buf[15];
    tc->fan_loband      = buf[16];
    tc->fan_hiband      = buf[17];
    tc->fan_quiet_start = buf[18];
    tc->fan_quiet_end   = buf[19];
    tc->fan_quiet_speed = buf[20];
    tc->relay1          = (buf[21] << 8) | buf[22];
    tc->relay2          = (buf[23] << 8) | buf[24];
    tc->relay3          = (buf[25] << 8) | buf[26];
    tc->relay4          = (buf[27] << 8) | buf[28];
    tc->relay3Delay     = buf[29];
    tc->relay4Delay     = buf[30];
}






void Lights_Modbus_Count()
{
    lights_count = 0;
    
    for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; ++i)
    {
        if(Light_Modbus_GetRelay(lights_modbus + i)) ++lights_count;
        else break;
    }
}

uint8_t Lights_Modbus_getCount()
{
    return lights_count;
}




void Lights_Modbus_Rows_Count()
{
    lights_modbus_rows = (Lights_Modbus_getCount() / 4) + 1;
}

uint8_t Lights_Modbus_Rows_getCount()
{
    return lights_modbus_rows;
}



void Lights_Modbus_Calculate()
{
    Lights_Modbus_Count();
    Lights_Modbus_Rows_Count();
}


void Light_Modbus_Init(LIGHT_Modbus_CmdTypeDef* li, const uint16_t addr)
{
    li->value = 0;
    li->old_value = 0;
    EE_ReadBuffer((uint8_t*)&li->index,                addr,           2);
    EE_ReadBuffer(&li->tiedToMainLight,                addr + 2,       1);
    EE_ReadBuffer(&li->off_time,                       addr + 3,       1);
    EE_ReadBuffer(&li->iconID,                         addr + 4,       1);
    EE_ReadBuffer((uint8_t*)(&li->controllerID_on),    addr + 5,       2);
    EE_ReadBuffer(&li->controllerID_on_delay,          addr + 7,       1);
    EE_ReadBuffer(&li->on_hour,                        addr + 8,       1);
    EE_ReadBuffer(&li->on_minute,                      addr + 9,       1);
    EE_ReadBuffer(&li->communication_type,             addr + 10,       1);
    EE_ReadBuffer(&li->local_pin,                      addr + 11,       1);
    EE_ReadBuffer(&li->sleep_time,                     addr + 12,       1);
    EE_ReadBuffer(&li->button_external,                addr + 13,       1);
}


void Light_Modbus_Save(LIGHT_Modbus_CmdTypeDef* li, const uint16_t addr)
{
    EE_WriteBuffer((uint8_t*)&li->index,                addr,           2);
    EE_WriteBuffer(&li->tiedToMainLight,                addr + 2,       1);
    EE_WriteBuffer(&li->off_time,                       addr + 3,       1);
    EE_WriteBuffer(&li->iconID,                         addr + 4,       1);
    EE_WriteBuffer((uint8_t*)(&li->controllerID_on),    addr + 5,       2);
    EE_WriteBuffer(&li->controllerID_on_delay,          addr + 7,       1);
    EE_WriteBuffer(&li->on_hour,                        addr + 8,       1);
    EE_WriteBuffer(&li->on_minute,                      addr + 9,       1);
    EE_WriteBuffer(&li->communication_type,             addr + 10,       1);
    EE_WriteBuffer(&li->local_pin,                      addr + 11,       1);
    EE_WriteBuffer(&li->sleep_time,                     addr + 12,       1);
    EE_WriteBuffer(&li->button_external,                addr + 13,       1);
}

void Lights_Modbus_Init()
{
    for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; i++)
    {
        Light_Modbus_Init(lights_modbus + i, EE_LIGHTS_MODBUS + (i * 13));
    }
    
    Lights_Modbus_Calculate();
}

void Lights_Modbus_Save()
{
    for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; i++)
    {
        Light_Modbus_Save(lights_modbus + i, EE_LIGHTS_MODBUS + (i * 13));
    }
    
    Lights_Modbus_Calculate();
}



bool Light_Modbus_isIndexInRange(const uint8_t light_index)
{
    return (light_index >= 0) && (light_index < LIGHTS_MODBUS_SIZE);
}


uint8_t Light_Modbus_Set_byIndex(const uint8_t light_index, const uint8_t val)
{
    if(Light_Modbus_isIndexInRange(light_index))
    {
        if(val) Light_Modbus_On(lights_modbus + light_index);
        else Light_Modbus_Off(lights_modbus + light_index);
    }
    
    return Light_Modbus_isNewValueOn(lights_modbus + light_index);
}

uint8_t Light_Modbus_Get_byIndex(const uint8_t light_index)
{
    if(Light_Modbus_isIndexInRange(light_index))
    {
        return Light_Modbus_isNewValueOn(lights_modbus + light_index);
    }
    
    return LIGHT_MODBUS_QUERY_RESPONSE_INDEX_OUT_OF_RANGE;
}


void Light_Modbus_On(LIGHT_Modbus_CmdTypeDef* const li)
{
    li->value = 1;
    
    if(li->local_pin < 5) SetPin(li->local_pin, 1);
    else PCA9685_SetOutput(li->local_pin, 255);
    
    if(Light_Modbus_isOffTimeEnabled(li))
    {
        uint32_t time = HAL_GetTick();
        if(!time) time = 1;
        Light_Modbus_SetOffTimeTimer(li, time);
    }
}

void Light_Modbus_On_External(LIGHT_Modbus_CmdTypeDef* const li)
{
    if(li->controllerID_on_delay)
    {
        li->on_delay_timer_start = HAL_GetTick();
        if(!li->on_delay_timer_start) --li->on_delay_timer_start;
    }
    else
    {
        Light_Modbus_On(li);
    }
}

void Light_Modbus_Off(LIGHT_Modbus_CmdTypeDef* const li)
{
    li->value = 0;
    
    if(li->local_pin < 5) SetPin(li->local_pin, 0);
    else PCA9685_SetOutput(li->local_pin, 0);
    
    Light_Modbus_OffTimeTimerDeactivate(li);
}

void Light_Modbus_Off_External(LIGHT_Modbus_CmdTypeDef* const li)
{
    if(li->controllerID_on_delay)
    {
        li->on_delay_timer_start = 0;
    }
    else
    {
        Light_Modbus_Off(li);
    }
}

void Light_Modbus_Update_External(LIGHT_Modbus_CmdTypeDef* const li, const uint8_t val)
{
    li->old_value = val;
    li->value = val;
}

bool Light_Modbus_isActive(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->old_value;
}

bool Light_Modbus_isNewValueOn(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->value;
}

bool Light_Modbus_isOldValueOn(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->old_value;
}

void Light_Modbus_Flip(LIGHT_Modbus_CmdTypeDef* const li)
{
    if(Light_Modbus_isActive(li)) Light_Modbus_Off(li);
    else Light_Modbus_On(li);
}





uint16_t Light_Modbus_GetRelay(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->index;
}

void Light_Modbus_SetRelay(LIGHT_Modbus_CmdTypeDef* const li, const uint16_t val)
{
    li->index = val;
}



void Light_Modbus_TieToMainLight(LIGHT_Modbus_CmdTypeDef* const li)
{
    li->tiedToMainLight = 1;
}

void Light_Modbus_UntieFromMainLight(LIGHT_Modbus_CmdTypeDef* const li)
{
    li->tiedToMainLight = 0;
}

bool Light_Modbus_isTiedToMainLight(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->tiedToMainLight;
}





uint8_t Light_Modbus_GetOnDelayTime(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->controllerID_on_delay;
}

void Light_Modbus_SetOnDelayTime(LIGHT_Modbus_CmdTypeDef* const li, const uint8_t val)
{
    li->controllerID_on_delay = val;
}

bool Light_Modbus_isOnDelayTimeEnabled(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return Light_Modbus_GetOnDelayTime(li);
}



uint32_t Light_Modbus_GetOnDelayTimeTimer(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->on_delay_timer_start;
}

void Light_Modbus_SetOnDelayTimeTimer(LIGHT_Modbus_CmdTypeDef* const li, const uint32_t val)
{
    li->on_delay_timer_start = val;
}

bool Light_Modbus_isOnDelayTimeTimerActive(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return Light_Modbus_GetOnDelayTimeTimer(li);
}

bool Light_Modbus_hasOnDelayTimeTimerExpired(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return (HAL_GetTick() - li->on_delay_timer_start) >= (Light_Modbus_GetOnDelayTime(li) * 60 * 1000);
}

void Light_Modbus_OnDelayTimeTimerDeactivate(LIGHT_Modbus_CmdTypeDef* const li)
{
    li->on_delay_timer_start = 0;
}





uint8_t Light_Modbus_GetOffTime(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->off_time;
}

void Light_Modbus_SetOffTime(LIGHT_Modbus_CmdTypeDef* const li, const uint8_t val)
{
    li->off_time = val;
}

bool Light_Modbus_isOffTimeEnabled(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return Light_Modbus_GetOffTime(li);
}



uint32_t Light_Modbus_GetOffTimeTimer(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->off_timer_start;
}

void Light_Modbus_SetOffTimeTimer(LIGHT_Modbus_CmdTypeDef* const li, const uint32_t val)
{
    li->off_timer_start = val;
}

bool Light_Modbus_isOffTimeTimerActive(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return Light_Modbus_GetOffTimeTimer(li);
}

bool Light_Modbus_hasOffTimeTimerExpired(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return (HAL_GetTick() - li->off_timer_start) >= (Light_Modbus_GetOffTime(li) * 60 * 1000);
}

void Light_Modbus_OffTimeTimerDeactivate(LIGHT_Modbus_CmdTypeDef* const li)
{
    li->off_timer_start = 0;
}




bool Light_Modbus_isTimeOnEnabled(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return (li->on_hour < 24) && (li->on_minute < 60);
}

bool Light_Modbus_isTimeToTurnOn(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return (li->on_hour == Bcd2Dec(rtctm.Hours)) && (li->on_minute == Bcd2Dec(rtctm.Minutes));
}





void Light_Modbus_SetColor(LIGHT_Modbus_CmdTypeDef* const li, GUI_COLOR color)
{
    li->color = color;
}

GUI_COLOR Light_Modbus_GetColor(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->color;
}

bool Light_Modbus_hasColorChanged(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return Light_Modbus_GetColor(li);
}

void Light_Modbus_ResetColor(LIGHT_Modbus_CmdTypeDef* const li)
{
    li->color = 0;
}





void Light_Modbus_SetBrightness(LIGHT_Modbus_CmdTypeDef* const li, uint8_t brightness)
{
    if(brightness > 100) li->brightness = 100;
    else if(brightness < 0) li->brightness = 0;
    else li->brightness = brightness;
}

uint8_t Light_Modbus_GetBrightness(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->brightness;
}

bool Light_Modbus_hasBrightnessChanged(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return Light_Modbus_GetBrightness(li);
}

void Light_Modbus_ResetBrightness(LIGHT_Modbus_CmdTypeDef* const li)
{
    li->brightness = 0;
}





void Lights_Modbus_Button_External_Press(LIGHT_Modbus_CmdTypeDef* const li)
{
    if(!li->button_external)
    {
        if(li->button_external == 1)
        {
            Light_Modbus_On(li);
        }
        else if(li->button_external == 2)
        {
            Light_Modbus_Off(li);
        }
        else if(li->button_external == 3)
        {
            Light_Modbus_Flip(li);
        }
    }
}




bool Light_Modbus_hasStatusChanged(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return Light_Modbus_isOldValueOn(li) != Light_Modbus_isNewValueOn(li);
}

void Light_Modbus_ResetStatus(LIGHT_Modbus_CmdTypeDef* const li)
{
    li->old_value = li->value;
}





bool Light_Modbus_hasChanged(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return Light_Modbus_hasStatusChanged(li) || Light_Modbus_hasBrightnessChanged(li) || Light_Modbus_hasColorChanged(li);
}

void Light_Modbus_ResetChange(LIGHT_Modbus_CmdTypeDef* const li)
{
    li->old_value = li->value;
    Light_Modbus_ResetBrightness(li);
    Light_Modbus_ResetColor(li);
}



GUI_CONST_STORAGE GUI_BITMAP* Light_Modbus_GetIcon(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return light_modbus_images[(li->iconID * 2) + Light_Modbus_isNewValueOn(li)];
}






uint32_t Ventilator_GetOnDelayTimer()
{
    return ventilatorOnDelayTimer_Start;
}

void Ventilator_SetOnDelayTimer(const uint32_t val)
{
    ventilatorOnDelayTimer_Start = val;
}

bool Ventilator_isOnDelayTimerActive()
{
    return Ventilator_GetOnDelayTimer();
}

bool Ventilator_hasOnDelayTimerExpired()
{
    return (HAL_GetTick() - ventilatorOnDelayTimer_Start) >= (2 * 60 * 1000);
}

void Ventilator_OnDelayTimerDeactivate()
{
    ventilatorOnDelayTimer_Start = 0;
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

