/**
 ******************************************************************************
 * File Name          : thermostat.h
 * Date               : 22/02/2018 07:03:00
 * Description        : temperature controller header
 ******************************************************************************
 */
 
 /* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TEMP_CTRL_H__
#define __TEMP_CTRL_H__                             FW_BUILD // version
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f7xx.h"
/* Exported Type  ------------------------------------------------------------*/



typedef struct{
    uint8_t  th_ctrl;           // thermostat control: off,auto,manual,heating,cooling,enabled,disabled,network
    uint8_t  th_state;          // thermostat status
    int16_t  mv_temp;           // temperature measured value  
    int8_t   mv_offset;         // temperature measured value  offset +/- 10.0�C
    uint8_t  sp_temp;           // temperature setpoint  x 1�C
    uint8_t  sp_diff;           // temperature setpoint difference x 0.1�C
    uint8_t  sp_max;            // temperature setpoint maximum for user x 1�C
    uint8_t  sp_min;            // temperature setpoint minimum for user x 1�C
    uint8_t  fan_ctrl;          // fan control: off,auto,manual 1-2-3,quiet mode   
    uint8_t  fan_speed;         // fan speed actual/set value depend on fan control type
    uint8_t  fan_diff;          // fan speed minimum difference to switch output
    uint8_t  fan_loband;        // fan speed controller low band 
    uint8_t  fan_hiband;        // fan speed controller high band
    uint8_t group;
    bool master;
    bool hasInfoChanged;
}THERMOSTAT_TypeDef;

extern THERMOSTAT_TypeDef thst;
/* Exported Define  ----------------------------------------------------------*/
#define USE_THERMOSTAT                              1       // switch kompajlera z akod termostata
#define FANC_FAN_MIN_ON_TIME                        560U    // 0,5s between two or on/off fan speed switching
#define THST_SP_MIN                                 5       // granicna minimalna vrijednost setpointa temperature
#define THST_SP_MAX                                 40      // granicna maksimalna vrijednost setpointa temperature
/* Exported Variable   -------------------------------------------------------*/
extern uint8_t termfl;
/* Exported Macro ------------------------------------------------------------*/
/** ==========================================================================*/
/**                 T H E R M O S T A T         F L A G S                     */
/** ==========================================================================*/
#define IsTempRegSta(x)             (x & (1U<<0)) // remote state config flag
#define IsTempRegMod(x)             (x & (1U<<1)) // remote mode config flag
#define IsTempRegCtr(x)             (x & (1U<<2)) // remote control flag
#define IsTempRegOut(x)             (x & (1U<<3)) // remote output flag
#define IsTempRegNewSta(x)          (x & (1U<<4)) // use remote state config flag 
#define IsTempRegNewMod(x)          (x & (1U<<5)) // use remote mode config flag
#define IsTempRegNewCtr(x)          (x & (1U<<6)) // use remote control flag
#define IsTempRegNewOut(x)          (x & (1U<<7)) // use remote output flag
#define IsTempRegNewCfg(x)          (x & 0xF0) // use at least one remote flag

#define THST_SendRelay1()                (thst.sendChangeSignalFlags |= 1U)
#define THST_ShouldSendRelay1()          (thst.sendChangeSignalFlags & 1U)
#define THST_ResetRelay1Flag()           (thst.sendChangeSignalFlags &= (~1U))
#define THST_SendRelay2()                (thst.sendChangeSignalFlags |= (1U << 1U))
#define THST_ShouldSendRelay2()          (thst.sendChangeSignalFlags & (1U << 1U))
#define THST_ResetRelay2Flag()           (thst.sendChangeSignalFlags &= (~(1U << 1U)))
#define THST_SendRelay3()                (thst.sendChangeSignalFlags |= (1U << 2U))
#define THST_ShouldSendRelay3()          (thst.sendChangeSignalFlags & (1U << 2U))
#define THST_ResetRelay3Flag()           (thst.sendChangeSignalFlags &= (~(1U << 2U)))
#define THST_SendRelay4()                (thst.sendChangeSignalFlags |= (1U << 3U))
#define THST_ShouldSendRelay4()          (thst.sendChangeSignalFlags & (1U << 3U))
#define THST_ResetRelay4Flag()           (thst.sendChangeSignalFlags &= (~(1U << 3U)))
/** ==========================================================================*/
/**       M E M O R I S E D     A N D     R E S T O R E D     F L A G S       */
/** ==========================================================================*/
#define TempRegOff()                (thst.th_ctrl = 0)
#define TempRegCooling()            (thst.th_ctrl = 1)
#define TempRegHeating()            (thst.th_ctrl = 2)
#define IsTempRegActiv()            (thst.th_ctrl != 0)
#define IsTempRegCooling()          (thst.th_ctrl == 1)
#define IsTempRegHeating()          (thst.th_ctrl == 2)

#define NtcConnected()              (termfl |=  (1U<<0))
#define NtcDisconnected()           (termfl &= ~(1U<<0))
#define IsNtcConnected()            (termfl &   (1U<<0))
#define NtcErrorSet()               (termfl |=  (1U<<1))
#define NtcErrorReset()             (termfl &= ~(1U<<1))
#define IsNtcErrorActiv()           (termfl &   (1U<<1))
/* Exported Function  ------------------------------------------------------- */
void THSTAT_Init(void);
void THSTAT_Service(void);
void THSTAT_SaveSettings(void);
void Thermostat_SP_Temp_Set(const uint8_t setpoint);
void Thermostat_SP_Temp_Increment(void);
void Thermostat_SP_Temp_Decrement(void);
void Thermostat_Set_SP_Min(const uint8_t value);
void Thermostat_Set_SP_Max(const uint8_t value);
void Thermostat_SetDefault(void);

#endif
/******************************   END OF FILE  ********************************/
