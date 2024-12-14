/**
 ******************************************************************************
 * Project          : KuceDzevadova
 ******************************************************************************
 *
 *
 ******************************************************************************
 */

 
#if (__TEMP_CTRL_H__ != FW_BUILD)
    #error "thermostat header version mismatch"
#endif 
/* Include  ------------------------------------------------------------------*/
#include "png.h"
#include "main.h"
#include "rs485.h"
#include "logger.h"
#include "display.h"
#include "thermostat.h"
#include "stm32746g.h"
#include "stm32746g_ts.h"
#include "stm32746g_qspi.h"
#include "stm32746g_sdram.h"
#include "stm32746g_eeprom.h"
/* Imported Type  ------------------------------------------------------------*/
THERMOSTAT_TypeDef thst;
/* Imported Variable  --------------------------------------------------------*/
/* Imported Function  --------------------------------------------------------*/
/* Private Type --------------------------------------------------------------*/
/* Private Define ------------------------------------------------------------*/
/* Private Variable ----------------------------------------------------------*/
uint8_t termfl;
/* Private Macro -------------------------------------------------------------*/
/** ============================================================================*/
/**   F A N C O I L   C O N T O L   W I T H   4   D I G I T A L   O U T P U T   */
/** ============================================================================*/
#define FanLowSpeedOn()             (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET))
#define FanLowSpeedOff()            (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET))
#define FanMiddleSpeedOn()          (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET))
#define FanMiddleSpeedOff()         (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET))
#define FanHighSpeedOn()            (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8,  GPIO_PIN_SET))
#define FanHighSpeedOff()           (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8,  GPIO_PIN_RESET))
#define FanOff()                    (FanLowSpeedOff(),FanMiddleSpeedOff(),FanHighSpeedOff())
/* Private Function Prototype ------------------------------------------------*/
/* Program Code  -------------------------------------------------------------*/
/**
  * @brief
  * @param
  * @retval
  */
void THSTAT_Init(void){
	ReadThermostatController(&thst,  EE_THST1);
    thst.sendChangeSignalFlags = 0;
    TempRegHeating();
}


/**
  * @brief
  * @param
  * @retval
  */
void THSTAT_Service(void)
{
    static int16_t temp_sp = 0U;
    static uint32_t fan_pcnt = 0U;
    static uint32_t old_fan_speed = 0U;
    static uint32_t fancoil_fan_timer = 0U;
    /** ============================================================================*/
	/**				T E M P E R A T U R E		C O N T R O L L E R					*/
	/** ============================================================================*/
	if(!IsTempRegActiv()){
        fan_pcnt = 0U;
        thst.fan_speed = 0U;
//		FanOff(); // seet to off state all 3 triac/relay outputs controlling 3 winding/speed electric fan
	}
	else if(IsTempRegActiv())
    {
        temp_sp =(int16_t) ((thst.sp_temp & 0x3FU) * 10);        
        /*if(IsTempRegCooling()){
            if      ((thst.fan_speed == 0U) && (thst.mv_temp > (temp_sp + thst.fan_loband)))                                    thst.fan_speed = 1U;
            else if ((thst.fan_speed == 1U) && (thst.mv_temp > (temp_sp + thst.fan_hiband)))                                    thst.fan_speed = 2U;
            else if ((thst.fan_speed == 1U) && (thst.mv_temp <= temp_sp))                                                       thst.fan_speed = 0U;         
            else if ((thst.fan_speed == 2U) && (thst.mv_temp > (temp_sp + thst.fan_hiband + thst.fan_loband)))                  thst.fan_speed = 3U;
            else if ((thst.fan_speed == 2U) && (thst.mv_temp <=(temp_sp + thst.fan_hiband - thst.fan_diff)))                    thst.fan_speed = 1U; 
            else if ((thst.fan_speed == 3U) && (thst.mv_temp <=(temp_sp + thst.fan_hiband + thst.fan_loband - thst.fan_diff)))  thst.fan_speed = 2U;                 
        }*/
            if      ((thst.fan_speed == 0U) && (thst.mv_temp < (temp_sp - thst.fan_diff)))                                      thst.fan_speed = 1U;
            else if ((thst.fan_speed == 1U) && (thst.mv_temp >= temp_sp))                                                       thst.fan_speed = 0U;
    }
    /** ============================================================================*/
	/**		S W I T C H		F A N		S P E E D		W I T H		D E L A Y		*/
	/** ============================================================================*/
	if (thst.fan_speed != old_fan_speed)
    {
        if((HAL_GetTick() - fancoil_fan_timer) >= FANC_FAN_MIN_ON_TIME)
        {
//            if(fan_pcnt > 1U)  fan_pcnt = 0U;                
//            if(fan_pcnt == 0U)
//            {
////                FanOff();
//                if(thst.relay1) THST_SendRelay1();
//                if(thst.relay2) THST_SendRelay2();
//                if(thst.relay3)
//                {
//                    THST_SendRelay3();
//                    thst.relay3DelayTimerStart = 0;
//                }
//                if(thst.relay4)
//                {
//                    THST_SendRelay4();
//                    thst.relay4DelayTimerStart = 0;
//                }
//                if (old_fan_speed) fancoil_fan_timer = HAL_GetTick();
//                ++fan_pcnt;
//            }
//            else if(fan_pcnt == 1U)
//            {                
                /*if      (thst.fan_speed == 1) FanLowSpeedOn();
                else if (thst.fan_speed == 2) FanMiddleSpeedOn();
                else if (thst.fan_speed == 3) FanHighSpeedOn();*/
//                if (thst.fan_speed)
//                {
                    if(thst.relay1) THST_SendRelay1();
                    if(thst.relay2) THST_SendRelay2();
                    if(thst.relay3)
                    {
//                        THST_SendRelay3();
                        thst.relay3DelayTimerStart = HAL_GetTick();
                        if(!(thst.relay3DelayTimerStart)) thst.relay3DelayTimerStart = 1;
                    }
                    if(thst.relay4)
                    {
//                        THST_SendRelay4();
                        thst.relay4DelayTimerStart = HAL_GetTick();
                        if(!(thst.relay4DelayTimerStart)) thst.relay4DelayTimerStart = 1;
                    }
                    
                    fancoil_fan_timer = HAL_GetTick();
//                }
                old_fan_speed = thst.fan_speed;
//                ++fan_pcnt;
//            }            
        }
	}
    
    if((thst.relay3DelayTimerStart) && ((HAL_GetTick() - thst.relay3DelayTimerStart) >= (thst.relay3Delay * 10 * 1000 * (thst.fan_speed ? 1 : 0))))
    {
        thst.relay3DelayTimerStart = 0;
        THST_SendRelay3();
    }
    
    if((thst.relay4DelayTimerStart) && ((HAL_GetTick() - thst.relay4DelayTimerStart) >= (thst.relay4Delay * 10 * 1000 * (thst.fan_speed ? 1 : 0))))
    {
        thst.relay4DelayTimerStart = 0;
        THST_SendRelay4();
    }
}
/******************************   RAZLAZ SIJELA  ********************************/
