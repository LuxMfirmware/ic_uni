/**
 ******************************************************************************
 * File Name          : display.h
 * Date               : 10.3.2018
 * Description        : GUI Display Module Header
 ******************************************************************************
 *
 *
 ******************************************************************************
 */

#ifndef __DISP_H__
#define __DISP_H__                           FW_BUILD // version
/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx.h"
#include "main.h"
#include "thermostat.h"
/* Exported Define -----------------------------------------------------------*/
#define SCRNSVR_TOUT    30  // 10 sec timeout increment to set display in low brigntnes after last touch event
#define TS_LAYER        1   // touch screen layer event

#define LIGHTS_MODBUS_SIZE              6
/* Exported types ------------------------------------------------------------*/
typedef enum{
	RELEASED    = 0,
	PRESSED     = 1,
    BUTTON_SHIT = 2
}BUTTON_StateTypeDef;

typedef struct
{
    GUI_COLOR color;
    uint32_t off_timer_start;
    uint16_t index, old_index, controllerID_on;
    uint8_t value, old_value, iconID, tiedToMainLight, off_time, controllerID_on_delay, on_hour, on_minute, communication_type, local_pin, sleep_time, button_external;
} LIGHT_Modbus_CmdTypeDef;

typedef struct{
    uint8_t index;
    uint8_t old_index;
    uint8_t value;
    uint8_t old_value;
}LIGHT_CmdTypeDef;

typedef struct{
    LIGHT_CmdTypeDef Main1;
    LIGHT_CmdTypeDef Led1;
    LIGHT_CmdTypeDef Led2;
    LIGHT_CmdTypeDef Led3;
    LIGHT_CmdTypeDef Out1;
    LIGHT_CmdTypeDef Light1;
    LIGHT_CmdTypeDef Light2;
    LIGHT_CmdTypeDef Light3;
    LIGHT_Modbus_CmdTypeDef modbusLight;
}LIGHT_CtrlTypeDef;

extern LIGHT_CtrlTypeDef LIGHT_Ctrl1;
extern LIGHT_CtrlTypeDef LIGHT_Ctrl2;
/* Exported variables  -------------------------------------------------------*/
extern uint32_t dispfl, lightInitRequestTime;
extern char logbuf[128];
void DISP_UpdateLog(const char *pbuf);
extern uint8_t scrnsvr_ena_hour, scrnsvr_dis_hour, scrnsvr_tout;
extern uint8_t high_bcklght, low_bcklght, light_ldr;
extern uint8_t scrnsvr_clk_clr, scrnsvr_semiclk_clr;
extern uint8_t lightInitRequestSend;
extern uint8_t menu_thst;
extern uint8_t t;
extern uint8_t screen, shouldDrawScreen;
extern LIGHT_Modbus_CmdTypeDef lights_modbus[LIGHTS_MODBUS_SIZE];
/* Exported Macro   --------------------------------------------------------- */
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
void DISPResetScrnsvr(void);
void DISPSetBrightnes(uint8_t val);
void PID_Hook(GUI_PID_STATE* pState);
void SaveThermostatController(THERMOSTAT_TypeDef* tc, uint16_t addr);
void ReadThermostatController(THERMOSTAT_TypeDef* tc, uint16_t addr);
void Light_Modbus_On(LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_Off(LIGHT_Modbus_CmdTypeDef* const li);
uint8_t Light_Modbus_Set_byIndex(const uint8_t light_index, const uint8_t val);
uint8_t Light_Modbus_Get_byIndex(const uint8_t light_index);
uint16_t Light_Modbus_GetRelay(const LIGHT_Modbus_CmdTypeDef* const li);
bool Light_Modbus_isIndexInRange(const uint8_t light_index);
void Light_Modbus_Update_External(LIGHT_Modbus_CmdTypeDef* const li, const uint8_t val);
bool Light_Modbus_isNewValueOn(const LIGHT_Modbus_CmdTypeDef* const li);
bool Light_Modbus_hasChanged(const LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_ResetChange(LIGHT_Modbus_CmdTypeDef* const li);
bool QR_Code_isDataLengthShortEnough(uint8_t dataLength);
bool QR_Code_willDataFit(const uint8_t *data);
uint8_t* QR_Code_Get(const uint8_t qrCodeID);
void QR_Code_Set(const uint8_t qrCodeID, const uint8_t *data);
#endif
/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
