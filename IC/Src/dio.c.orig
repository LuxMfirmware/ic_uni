/**
 ******************************************************************************
 * File Name          : dio_interface.c
 * Date               : 28/02/2016 23:16:19
 * Description        :  digital in/out and capacitive sensor  modul
 ******************************************************************************
 */
 
#if (__DIO_H__ != FW_BUILD)
    #error "doi header version mismatch"
#endif
/* Include  ------------------------------------------------------------------*/
#include "png.h"
#include "pwm.h"
#include "main.h"
#include "rs485.h"
#include "logger.h"
#include "display.h"
#include "onewire.h"
#include "thermostat.h"
#include "stm32746g.h"
#include "stm32746g_ts.h"
#include "stm32746g_qspi.h"
#include "stm32746g_sdram.h"
#include "stm32746g_eeprom.h"
/* Imported Types  -----------------------------------------------------------*/
#define BUZZER_SHORT_TIME                   100U    // 100 ms buzzer activ then repeat pause  
#define BUZZER_MIDDLE_TIME                  500U    // 500 ms buzzer activ then repeat pause
#define BUZZER_LONG_TIME                    1000U   // 1 s buzzer activ then repeat pause
/* Imported Variables --------------------------------------------------------*/
/* Imported Functions    -----------------------------------------------------*/
/* Private Variables  --------------------------------------------------------*/
__IO uint32_t diofl;
__IO uint32_t din_0_7;          // digital input after debouncing mask with config buffer for application digital inputs 
__IO uint32_t din_state;        // digital input real time update states input to debouncing
__IO uint32_t din_cap_sen;      // capacitive taster sensors state
__IO uint32_t dout_0_7;         // digital output buffer mask with config buffer to actuate real ouputs 
__IO uint32_t dout_0_7_rem;     // digital output state forcing from remote, not valid after new power cycle 
uint8_t din_cfg[8];             // digital inputs configuration saved and restored from eeprom
uint8_t dout_cfg[8];            // digital output configuration saved and restored from eeprom 
uint8_t buzzer_volume;
uint8_t doorlock_force;
uint8_t hc595_dout;
/* Private Define  -----------------------------------------------------------*/
#define INPUT_DEBOUNCE_CNT  						100U    // number of input state check 100
#define FAST_INPUT_DEBOUNCE_CNT  					50U     // number of input state check 50
#define SLOW_INPUT_DEBOUNCE_CNT  					10000U  // number of input state check 50
#define ENTRY_DOOR_MAX_CYCLES						60U     // check entry door 60 time, then write log
//#define WATER_FLOOD_MAX_CYCLES                    60U     // check water flood sensor 60 time, then write log
#define ENTRY_DOOR_CHECK_CYCLE_TIME                 1111U   // 1000ms cycle period
//#define WATER_FLOOD_CHECK_CYCLE_TIME              1111U   // ~1000ms cycle period
#define DIO_PROCESS_TIME							5432U   // 5s temp  regulator cycle time
#define CAP_SW_ACTIV_TIME						    1234U   // 1 s handmaid capacitive switch activ time
#define MENU_BUTTON_TIME				            56U     // when cap switch used as service menu button
#define DOOR_LOCK_COIL_PULSE_CYCLES					10U     // 10 times on - off cycles for door lock coil
#define DOOR_LOCK_COIL_PULSE_DURATION				250U    // 250 ms door lock coil duty period
#define DOOR_LOCK_MAX_CYCLE_TIME					5112U   // 5 sec. max door lock on time
#define DOOR_BELL_LED_ACTIV_TOGGLE_TIME             200U
#define HANDMAID_LED_ACTIV_TOGGLE_TIME              200U
/* Private Macro   -----------------------------------------------------------*/
/* Private Function Prototype  -----------------------------------------------*/
static void DIN_Service(void);
static void DOUT_Service(void);
/* Program Code  -------------------------------------------------------------*/
/**
  * @brief
  * @param
  * @retval
  */
void DIO_Service(void) 
{
    static uint8_t dout_0_7_mem = 0x0U;
    static uint32_t doorlock_timer = 0x0U;
    /** ============================================================================*/                                                                         
	/**		R E L O A D 	D I G I T A L		I N P U T 		R E G I S T E R    	*/                                                                           	
	/** ============================================================================*/
    /*  Indor Card Reader   */
    if (din_cfg[0] == 2)
    {
        if      (HAL_GPIO_ReadPin (GPIOG, GPIO_PIN_14) == GPIO_PIN_RESET)   DIN0StateSet();
        else if (HAL_GPIO_ReadPin (GPIOG, GPIO_PIN_14) == GPIO_PIN_SET)     DIN0StateReset();
    }    
    /*  Balcony Door Sensor */
    if (din_cfg[5] == 2)
    {
        if      (HAL_GPIO_ReadPin (GPIOG, GPIO_PIN_13) == GPIO_PIN_RESET)   DIN5StateSet();
        else if (HAL_GPIO_ReadPin (GPIOG, GPIO_PIN_13) == GPIO_PIN_SET)     DIN5StateReset();
    }
    /* Entry Door Sensor    */
    if (din_cfg[7] == 2)
    {
        if      (HAL_GPIO_ReadPin (GPIOC, GPIO_PIN_2) == GPIO_PIN_RESET)    DIN7StateSet();
        else if (HAL_GPIO_ReadPin (GPIOC, GPIO_PIN_2) == GPIO_PIN_SET)      DIN7StateReset();
    }
    DIN_Service(); // execute digita input changes
	/** ============================================================================*/
	/**		F O R C E   D I G I T A L   O U T P U T    F R O M  	C O M M A N D 	*/
	/** ============================================================================*/
    if      (dout_0_7_rem & 0x100U)
	{
		if(!(dout_0_7_rem & 0x200U))
		{
			dout_0_7_mem = dout_0_7;
			dout_0_7_rem |= 0x200U;
		}
		dout_0_7 = (dout_0_7_rem & 0xFFU);		
        if      ((dout_0_7_rem & 0x40U) && !(dout_0_7_rem & 0x1000U))
		{
			dout_0_7_rem |= 0x1000U;
            doorlock_timer = HAL_GetTick();
		}
		else if(!(dout_0_7_rem & 0x40U))
		{
			dout_0_7_rem &= 0xEFFFU;
		}
		else if ((dout_0_7_rem & 0x40U) && (dout_0_7_rem & 0x1000U))
		{
			if((HAL_GetTick() - doorlock_timer) >= DOOR_LOCK_MAX_CYCLE_TIME) dout_0_7 &= 0xBFU;
		}
	}
	else if (dout_0_7_rem & 0x200U)
	{
		dout_0_7 = dout_0_7_mem;
		dout_0_7_rem = 0x0U;
	}
    DOUT_Service(); // execute digital output changes
}
/**
  * @brief
  * @param
  * @retval
  */
static void DIN_Service(void)
{
	static uint32_t din_0_timer = 0U;
	static uint32_t din_5_timer = 0U;
	static uint32_t din_7_timer = 0U;
    static uint32_t entry_door_tmr = 0U;
    static uint32_t entry_door_pcnt = 0U;  
    //
	// CARD STACKER STATE
    //
    if      (din_cfg[0] == 5) 
    {
        DIN0StateReset();
        CardStackerReset();
    }
    else if (din_cfg[0] == 4)
    {       
        DIN0StateSet();
        CardStackerSet();
        CardStackerEnable();
    }
    else if (din_cfg[0] == 3) ;
	else if (IsDIN0StateActiv() && !IsDIN0Activ())	
	{   /*  debounce digital input 0 on state */
		if(++din_0_timer >= SLOW_INPUT_DEBOUNCE_CNT)
		{        
			din_0_timer = 0;    // reset bit 0 debounce counter
			DIN0Set();    // change input register bit 0 state
            if (!IsCardStackerActiv()) 
            {
                CardStackerEnable();
            }
		}   
	} 
	else if (!IsDIN0StateActiv()  && IsDIN0Activ())
	{   /*  debounce digital input 0 off state */
		if(++din_0_timer >= SLOW_INPUT_DEBOUNCE_CNT)
		{        
            din_0_timer = 0;           // reset bit 0 debounce counter
            DIN0Reset();    // change input register bit 0 state
            if (IsCardStackerActiv())
            {
                CardStackerDisable();
            }
		}
	} 
	else din_0_timer = 0; // reset bit 0 debounce counter 
    //
    // BALCONY DOOR SWITCH STATE
    //
	if      (din_cfg[5] == 5) 
    {
        DIN5StateReset();
        HVACCtrlReset();
    }
    else if (din_cfg[5] == 4) 
    {
        DIN5StateSet();
        HVACCtrlEnable();
        HVACCtrlSet();
    }
    else if (din_cfg[5] == 3) ;
	else if (IsDIN5StateActiv()  && !IsDIN5Activ())	
	{   /*  debounce digital input 5 on state */ 
		if(++din_5_timer >= SLOW_INPUT_DEBOUNCE_CNT)
		{
			din_5_timer = 0;
			DIN5Set();
            if (!IsHVACCtrlActiv()) 
            {
                HVACCtrlEnable();
            }
        }        
	} 
	else if (!IsDIN5StateActiv()  && IsDIN5Activ())
	{   /*  debounce digital input 5 off state */ 
		if(++din_5_timer >= SLOW_INPUT_DEBOUNCE_CNT)
		{        
			din_5_timer = 0;
			DIN5Reset();
            if (IsHVACCtrlActiv()) 
            {
                HVACCtrlDisable();
            }
		}        
	} 
	else din_5_timer = 0; // reset bit 5 debounce counter
    //
    // ENTRY DOOR SWITCH 
    // WATER FLOOD SENSOR
    //
	if      (din_cfg[7] == 5)
    {
        DIN7StateReset();
        EntryDoorSwReset();
    }
    else if (din_cfg[7] == 4)
    {
        DIN7StateSet();
        EntryDoorSwEnable();
        EntryDoorSwSet();
    }
	else if (din_cfg[7] == 3) ;
	else if (IsDIN7StateActiv() && !IsDIN7Activ())	
	{   /*  debounce digital input 7 on state */ 
		if(++din_7_timer >= FAST_INPUT_DEBOUNCE_CNT)
		{
			din_7_timer = 0;        // reset bit 7 debounce counter
			DIN7Set();    // change input register bit 7 state
            if (!IsEntryDoorSwActiv())
            {
                EntryDoorSwEnable();
            }
		}	
	} 
	else if (!IsDIN7StateActiv() && IsDIN7Activ())
	{   /*  debounce digital input 7 off state */ 
		if(++din_7_timer >= FAST_INPUT_DEBOUNCE_CNT)
		{
			din_7_timer = 0;    // reset bit 7 debounce counter
			DIN7Reset();        // change input register bit 7 state
            if (IsEntryDoorSwActiv())
            {
                EntryDoorSwDisable();
            }
		}
	} 
	else  din_7_timer = 0; // reset bit 7 debounce counter
	//
    // ENTRY DOOR SWITCH OR WATER FLOOD SENSOR
    // ALARM REPETITION
    //
    if (!IsEntryDoorSwActiv() && IsEntryDoorSwEnabled())
	{
		if((HAL_GetTick() - entry_door_tmr) >= ENTRY_DOOR_CHECK_CYCLE_TIME)
		{
			entry_door_tmr = HAL_GetTick();
			if(++entry_door_pcnt >= ENTRY_DOOR_MAX_CYCLES)
			{   //  @TODO : activate doorbell or thermostat buzzer
				entry_door_pcnt = 0;
                LogEvent.log_event = ENTRY_DOOR_NOT_CLOSED;
				LOGGER_Write();
                //  @TODO : activate doorbell or thermostat buzzer
			}
		}
	}
	else
	{
		entry_door_pcnt = 0;
		entry_door_tmr = HAL_GetTick();
	}
}
/**
  * @brief
  * @param
  * @retval
  */
static void DOUT_Service(void)
{	
    /** ============================================================================*/                                                                         
	/**			S E T 		D I G I T A L	  O U T P U T 		D R I V E R   		*/                                                                           	
	/** ============================================================================*/
    /* heating valve */
    if      ( dout_cfg[5] == 5U) dout_0_7   &= (~(1U<<5)),  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);    
    else if ( dout_cfg[5] == 4U) dout_0_7   |=   (1U<<5),   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);
    else if ((dout_cfg[5] == 2U) &&  (dout_0_7 & (1U<<5)))  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);
	else if ((dout_cfg[5] == 2U) &&(!(dout_0_7 & (1U<<5)))) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
    /* power contactor */
    if      ( dout_cfg[6] == 5U) dout_0_7   &= (~(1U<<6)),  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
    else if ( dout_cfg[6] == 4U) dout_0_7   |=   (1U<<6),   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
    else if ((dout_cfg[6] == 2U) &&  (dout_0_7 & (1U<<6)))  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
    else if ((dout_cfg[6] == 2U) &&(!(dout_0_7 & (1U<<6)))) HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
    /* buzzer */        
    if      ( dout_cfg[7] == 5U) dout_0_7   &= (~(1U<<7)),  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);
    else if ( dout_cfg[7] == 4U) dout_0_7   |=   (1U<<7),   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET);
    else if ((dout_cfg[7] == 2U) &&  (dout_0_7 & (1U<<7)))  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET);
    else if ((dout_cfg[7] == 2U) &&(!(dout_0_7 & (1U<<7)))) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);
}
/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
