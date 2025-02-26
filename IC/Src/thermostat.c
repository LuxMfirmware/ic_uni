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
void THSTAT_Init(void) {
    thst.hasInfoChanged = false;
    ReadThermostatController(&thst,  EE_THST1);
    TempRegHeating();
}


/**
  * @brief
  * @param
  * @retval
  */
void THSTAT_Service(void)
{
    uint8_t buf[sizeof(thst)];

    // trebalo bi if(thst.master) termostat svejedno da li je master ili slave treba id za individualno adresiranje
    // dakle promjene preko busa se odnose na id termostata (thst.group) svi termostati jedne grupe su jedan isti termostat
    // samo su im graficki interfejsi razbacani po razlicitim kontrolerima po kuci ali samo što je jedan od njih master termostat
    // koji ili ima priljucene releje na izlaznim pinovima, a ne mora ni to ili ima ntc senzor za mjerenje tempeature, a ne mora ni to
    // njegova uloga je bitna što samo on diktira stanje aktuatorima bilo da su izlazni pinovi na PCB ili da su aktuatori na bus- u
    if(!thst.group)
    {
        static int16_t temp_sp = 0U;
        static uint32_t fan_pcnt = 0U;
        static uint32_t old_fan_speed = 0U;
        static uint32_t fancoil_fan_timer = 0U;
        /** ============================================================================*/
        /**				T E M P E R A T U R E		C O N T R O L L E R					*/
        /** ============================================================================*/
        if(!IsTempRegActiv()) {
            fan_pcnt = 0U;
            thst.fan_speed = 0U;
            //		FanOff(); // seet to off state all 3 triac/relay outputs controlling 3 winding/speed electric fan
        }
        else if(IsTempRegActiv())
        {
            temp_sp =(int16_t) ((thst.sp_temp & 0x3FU) * 10);
            if(IsTempRegCooling()) {
                if      ((thst.fan_speed == 0U) && (thst.mv_temp > (temp_sp + thst.fan_loband)))                                    thst.fan_speed = 1U;
                else if ((thst.fan_speed == 1U) && (thst.mv_temp > (temp_sp + thst.fan_hiband)))                                    thst.fan_speed = 2U;
                else if ((thst.fan_speed == 1U) && (thst.mv_temp <= temp_sp))                                                       thst.fan_speed = 0U;
                else if ((thst.fan_speed == 2U) && (thst.mv_temp > (temp_sp + thst.fan_hiband + thst.fan_loband)))                  thst.fan_speed = 3U;
                else if ((thst.fan_speed == 2U) && (thst.mv_temp <=(temp_sp + thst.fan_hiband - thst.fan_diff)))                    thst.fan_speed = 1U;
                else if ((thst.fan_speed == 3U) && (thst.mv_temp <=(temp_sp + thst.fan_hiband + thst.fan_loband - thst.fan_diff)))  thst.fan_speed = 2U;
            }
            else if(IsTempRegHeating())
            {
                if      ((thst.fan_speed == 0U) && (thst.mv_temp < (temp_sp - thst.fan_loband)))                                    thst.fan_speed = 1U;
                else if ((thst.fan_speed == 1U) && (thst.mv_temp < (temp_sp - thst.fan_hiband)))                                    thst.fan_speed = 2U;
                else if ((thst.fan_speed == 1U) && (thst.mv_temp >= temp_sp))                                                       thst.fan_speed = 0U;
                else if ((thst.fan_speed == 2U) && (thst.mv_temp < (temp_sp - thst.fan_hiband - thst.fan_loband)))                  thst.fan_speed = 3U;
                else if ((thst.fan_speed == 2U) && (thst.mv_temp >=(temp_sp - thst.fan_hiband + thst.fan_diff)))                    thst.fan_speed = 1U;
                else if ((thst.fan_speed == 3U) && (thst.mv_temp >=(temp_sp - thst.fan_hiband - thst.fan_loband + thst.fan_diff)))  thst.fan_speed = 2U;
            }
        }
        /** ============================================================================*/
        /**		S W I T C H		F A N		S P E E D		W I T H		D E L A Y		*/
        /** ============================================================================*/
        if (thst.fan_speed != old_fan_speed)
        {
            if((HAL_GetTick() - fancoil_fan_timer) >= FANC_FAN_MIN_ON_TIME) {
                if(fan_pcnt > 1U)  fan_pcnt = 0U;
                if(fan_pcnt == 0U) {
                    FanOff();
                    if (old_fan_speed) fancoil_fan_timer = HAL_GetTick();
                    ++fan_pcnt;
                }
                else if(fan_pcnt == 1U) {
                    if      (thst.fan_speed == 1) FanLowSpeedOn();
                    else if (thst.fan_speed == 2) FanMiddleSpeedOn();
                    else if (thst.fan_speed == 3) FanHighSpeedOn();
                    if (thst.fan_speed) fancoil_fan_timer = HAL_GetTick();
                    old_fan_speed = thst.fan_speed;
                    ++fan_pcnt;
                }
            }
        }
    }
    // metni ovdje jer ova funkcija ima vrijeme procesora kad dode na red stalno
    // ako je ovaj termostat master pošalji cijelu strukturu kod promjene settings-a
    if(thst.master && thst.hasInfoChanged)
    {
        // treba probrat šta se šalje neki od elemenata se i ne koriste nikako u kodu više
        // najbolje je ovo sve redefinisat šta treba za smart home
        buf[0] = thst.group;
        buf[1] = thst.master;
        buf[2] = thst.th_ctrl;
        buf[3] = thst.th_state;
        buf[4] = (thst.mv_temp >> 8) & 0xFF;
        buf[5] = thst.mv_temp & 0xFF;
        buf[6] = thst.sp_temp;
        buf[7] = thst.sp_min;
        buf[8] = thst.sp_max;
        buf[9] = thst.sp_diff;
        buf[10] = thst.fan_speed;
        buf[11] = thst.fan_loband;
        buf[12] = thst.fan_hiband;
        buf[13] = thst.fan_diff;
        buf[14] = thst.fan_ctrl;
        DodajKomandu(&thermoQueue, THERMOSTAT_INFO, buf, 15);
        thst.hasInfoChanged = false;
    }
    // treba dodat i za svaku najmanju promjenu izmjerene temperature koju šalje
    // master - cijelu ovakvu strukturu kao gore, a slave - samo manju kao dole ispod
    // i obadva šalju na THERMOSTAT_INFO
    // postoji flag IsMVUpdateActiv() koju setuje ADC3_Read() koji služi za ovo grafici ali
    // ga resetuje samo thermostat screeen kad je aktivan tako da treba novi flag samo za ovo
    // ili jednostavno setovat ovaj clan/flag strukture thst.hasInfoChanged tamo u ADC3_Read()
    else if(!thst.master && thst.hasInfoChanged)
    {
        buf[0] = thst.group;
        buf[1] = thst.master;
        buf[2] = thst.th_ctrl;
        buf[3] = thst.th_state;
        buf[4] = (thst.mv_temp >> 8) & 0xFF;
        buf[5] = thst.mv_temp & 0xFF;
        buf[6] = thst.sp_temp;
        DodajKomandu(&thermoQueue, THERMOSTAT_INFO, buf, 7);
        thst.hasInfoChanged = false;
    }
}


void Thermostat_SP_Temp_Set(const uint8_t setpoint)
{
    if(thst.sp_temp != setpoint)
    {
        if      (setpoint < thst.sp_min)    thst.sp_temp = thst.sp_min;
        else if (setpoint > thst.sp_max)    thst.sp_temp = thst.sp_max;
        else                                thst.sp_temp = setpoint;
        thst.hasInfoChanged = true;
    }
}

void Thermostat_SP_Temp_Increment(void)
{
    Thermostat_SP_Temp_Set(thst.sp_temp + 1);

}

void Thermostat_SP_Temp_Decrement(void)
{
    Thermostat_SP_Temp_Set(thst.sp_temp - 1);
}

void Thermostat_Set_SP_Min(const uint8_t value)
{
    if(value >= thst.sp_max)
    {
        thst.sp_min = thst.sp_max - 1;
    }
    else if(value < THST_SP_MIN)
    {
        thst.sp_min = THST_SP_MIN;
    }
    else
    {
        thst.sp_min = value;
    }
}

void Thermostat_Set_SP_Max(const uint8_t value)
{
    if(value <= thst.sp_min)
    {
        thst.sp_max = thst.sp_min + 1;
    }
    else if(value > THST_SP_MAX)
    {
        thst.sp_max = THST_SP_MAX;
    }
    else
    {
        thst.sp_max = value;
    }
}
/**
  * @brief
  * @param
  * @retval
  */
void SaveThermostatController(THERMOSTAT_TypeDef* tc, uint16_t addr) {
    uint8_t buf[14];
    buf[0] = tc->th_ctrl;
    buf[1] = tc->th_state;
    buf[2] = tc->mv_offset;
    buf[3] = tc->sp_temp;
    buf[4] = tc->sp_diff;
    buf[5] = tc->sp_max;
    buf[6] = tc->sp_min;
    buf[7] = tc->fan_ctrl;
    buf[8] = tc->fan_speed;
    buf[9] = tc->fan_diff;
    buf[10] = tc->fan_loband;
    buf[11] = tc->fan_hiband;
    buf[12] = tc->group;
    buf[13] = tc->master;
    EE_WriteBuffer(buf, addr, 14);
}
/**
  * @brief
  * @param
  * @retval
  */
void ReadThermostatController(THERMOSTAT_TypeDef* tc, uint16_t addr) {
    uint8_t buf[14];
    EE_ReadBuffer(buf, addr, 14);
    tc->th_ctrl         = buf[0];
    tc->th_state        = buf[1];
    tc->mv_offset       = buf[2];
    tc->sp_temp         = buf[3];
    tc->sp_diff         = buf[4];
    tc->sp_max          = buf[5];
    tc->sp_min          = buf[6];
    tc->fan_ctrl        = buf[7];
    tc->fan_speed       = buf[8];
    tc->fan_diff        = buf[9];
    tc->fan_loband      = buf[10];
    tc->fan_hiband      = buf[11];
    tc->group           = buf[12];
    tc->master          = buf[13];
}



void Thermostat_SetDefault(void)
{
    thst.group = 0;
    thst.master = false;
    thst.th_ctrl = 0;
    thst.th_state = 0;
    thst.mv_temp = 0;
    thst.sp_temp = 15;
    thst.sp_min = 15;
    thst.sp_max = 35;
    thst.sp_diff = 0;
    thst.fan_speed = 0;
    thst.fan_loband = 1;
    thst.fan_hiband = 2;
    thst.fan_diff = 1;
    thst.fan_ctrl = 0;
    thst.hasInfoChanged = false;
}


/******************************   RAZLAZ SIJELA  ********************************/
