/**
 ******************************************************************************
 * File Name          : dio.h
 * Date               : 28/02/2016 23:16:19
 * Description        : digital in/out modul header
 ******************************************************************************
 *
 *
 ******************************************************************************
 */
 
 
 /* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DIO_H__
#define __DIO_H__					    FW_BUILD	// version
/* Include  ------------------------------------------------------------------*/
#include "stm32f7xx.h"
/* Exporeted Types   ---------------------------------------------------------*/
/* Exporeted Define   --------------------------------------------------------*/
/* Exporeted Variable   ------------------------------------------------------*/
extern __IO uint32_t dout_0_7_rem;
extern __IO uint32_t din_cap_sen;
extern __IO uint32_t din_state;
extern __IO uint32_t dout_0_7;
extern __IO uint32_t din_0_7;
extern __IO uint32_t diofl;
extern uint8_t din_cfg[8];
extern uint8_t dout_cfg[8];
extern uint8_t hc595_dout;
extern uint8_t buzzer_volume;
extern uint8_t doorlock_force;
/* Exporeted  Macro  ---------------------------------------------------------*/
/** ==========================================================================*/
/**     M E M O R I S E D       A N D     R E S T O R E D       F L A G S     */
/** ==========================================================================*/
#define BTNDndSet()                     (sysfl |=  (1U<<16))
#define BTNDndReset()                   (sysfl &=(~(1U<<16)))
#define IsBTNDndActiv()                 (sysfl &   (1U<<16))

#define BTNDndEnable()                  (sysfl |=  (1U<<17))
#define BTNDndDisable()                 (sysfl &=(~(1U<<17)))
#define IsBTNDndEnabled()               (sysfl &   (1U<<17))

#define BTNMaidSet()                    (sysfl |=  (1U<<18))
#define BTNMaidReset()                  (sysfl &=(~(1U<<18)))
#define IsBTNMaidActiv()                (sysfl &   (1U<<18))

#define BTNMaidEnable()                 (sysfl |=  (1U<<19))
#define BTNMaidDisable()                (sysfl &=(~(1U<<19)))
#define IsBTNMaidEnabled()              (sysfl &   (1U<<19))

#define BTNSosSet()                     (sysfl |=  (1U<<20))
#define BTNSosReset()                   (sysfl &=(~(1U<<20)))
#define IsBTNSosActiv()                 (sysfl &   (1U<<20))

#define BTNSosEnable()                  (sysfl |=  (1U<<21))
#define BTNSosDisable()                 (sysfl &=(~(1U<<21)))
#define IsBTNSosEnabled()               (sysfl &   (1U<<21))

#define PwrExpTimeSet()                 (sysfl |=  (1U<<22))
#define PwrExpTimeReset()               (sysfl &=(~(1U<<22)))
#define IsPwrExpTimeActiv()             (sysfl &   (1U<<22))

#define ShowUserInfoEnable()            (sysfl |=  (1U<<23))
#define ShowUserInfoDisable()           (sysfl &=(~(1U<<23)))
#define IsShowUserInfoEnabled()         (sysfl &   (1U<<23))
/** ==========================================================================*/
/**         D I G I T A L       I N / O U T         F L A G S                 */
/** ==========================================================================*/
#define LEDGreenOn()        	        (diofl |=  (1U<<0))
#define LEDGreenOff()       	        (diofl &=(~(1U<<0)))
#define IsLEDGreenActiv()               (diofl &   (1U<<0))

#define LEDRedOn()                      (diofl |=  (1U<<1))
#define LEDRedOff()     	            (diofl &=(~(1U<<1)))
#define IsLEDRedActiv()                 (diofl &   (1U<<1))

#define LEDBlueOn()     		        (diofl |=  (1U<<2))
#define LEDBlueOff()    		        (diofl &=(~(1U<<2)))
#define IsLEDBlueActiv()                (diofl &   (1U<<2))

#define ExtDoorLockSet()                (diofl |=  (1U<<3))
#define ExtDoorLockReset()              (diofl &=(~(1U<<3)))
#define IsExtDoorLockActiv()            (diofl &   (1U<<3))

#define ExtDoorLockEnable()             (diofl |=  (1U<<4))
#define ExtDoorLockDisable()            (diofl &=(~(1U<<4)))
#define IsExtDoorLockEnabled()          (diofl &   (1U<<4))

#define CardStackerSet()                (diofl |=  (1U<<5))
#define CardStackerReset()              (diofl &=(~(1U<<5)))
#define IsCardStackerActiv()            (diofl &   (1U<<5))

#define CardStackerEnable()             (diofl |=  (1U<<6))
#define CardStackerDisable()            (diofl &=(~(1U<<6)))
#define IsCardStackerEnabled()          (diofl &   (1U<<6))

#define BTNUpdSet()                     (diofl |=  (1U<<7)) 
#define BTNUpdReset()                   (diofl &=(~(1U<<7)))
#define IsBTNUpdActiv()                 (diofl &   (1U<<7))

#define BTNBellSet()                    (diofl |=  (1U<<8))
#define BTNBellReset()                  (diofl &=(~(1U<<8)))
#define IsBTNBellActiv()                (diofl &   (1U<<8))

#define BTNBellEnable()                 (diofl |=  (1U<<9))
#define BTNBellDisable()                (diofl &=(~(1U<<9)))
#define IsBTNBellEnabled()              (diofl &   (1U<<9))

#define ExtBTNBellSet()                 (diofl |=  (1U<<10))
#define ExtBTNBellReset()               (diofl &=(~(1U<<10)))
#define IsExtBTNBellActiv()             (diofl &   (1U<<10))

#define ExtBTNBellEnable()              (diofl |=  (1U<<11))
#define ExtBTNBellDisable()             (diofl &=(~(1U<<11)))
#define IsExtBTNBellEnabled()           (diofl &   (1U<<11))

#define BTNWfcSet()                     (diofl |=  (1U<<12))
#define BTNWfcReset()                   (diofl &=(~(1U<<12)))
#define IsBTNWfcActiv()                 (diofl &   (1U<<12))

#define BTNWfcEnable()                  (diofl |=  (1U<<13))
#define BTNWfcDisable()                 (diofl &=(~(1U<<13)))
#define IsBTNWfcEnabled()               (diofl &   (1U<<13))

#define BTNOpenSet()                    (diofl |=  (1U<<14)) 
#define BTNOpenReset()                  (diofl &=(~(1U<<14)))
#define IsBTNOpenActiv()                (diofl &   (1U<<14))

#define BTNOpenEnable()                 (diofl |=  (1U<<15)) 
#define BTNOpenDisable()                (diofl &=(~(1U<<15)))
#define IsBTNOpenEnabled()              (diofl &   (1U<<15))

#define BTNOkSet()                      (diofl |=  (1U<<16)) 
#define BTNOkReset()                    (diofl &=(~(1U<<16)))
#define IsBTNOkActiv()                  (diofl &   (1U<<16))

#define BTNOkEnable()                   (diofl |=  (1U<<17)) 
#define BTNOkDisable()                  (diofl &=(~(1U<<17)))
#define IsBTNOkEnabled()                (diofl &   (1U<<17))

#define EntryDoorSwSet()                (diofl |=  (1U<<18))
#define EntryDoorSwReset()              (diofl &=(~(1U<<18)))
#define IsEntryDoorSwActiv()            (diofl &   (1U<<18))

#define EntryDoorSwEnable()             (diofl |=  (1U<<19))
#define EntryDoorSwDisable()            (diofl &=(~(1U<<19)))
#define IsEntryDoorSwEnabled()          (diofl &   (1U<<19))

#define HVACCtrlSet()                   (diofl |=  (1U<<20))
#define HVACCtrlReset()                 (diofl &=(~(1U<<20)))
#define IsHVACCtrlActiv()               (diofl &   (1U<<20))

#define HVACCtrlEnable()                (diofl |=  (1U<<21))
#define HVACCtrlDisable()               (diofl &=(~(1U<<21)))
#define IsHVACCtrlEnabled()             (diofl &   (1U<<21))

#define ExtHVACCtrlSet()                (diofl |=  (1U<<22))
#define ExtHVACCtrlReset()              (diofl &=(~(1U<<22)))
#define IsExtHVACCtrlActiv()            (diofl &   (1U<<22))

#define ExtHVACCtrlEnable()             (diofl |=  (1U<<23))
#define ExtHVACCtrlDisable()            (diofl &=(~(1U<<23)))
#define IsExtHVACCtrlEnabled()          (diofl &   (1U<<23))

#define ExtHVACPowerSet()               (diofl |=  (1U<<24))    
#define ExtHVACPowerReset()             (diofl &=(~(1U<<24)))
#define IsExtHVACPowerActiv()           (diofl &   (1U<<24))

#define ExtHVACPowerEnable()            (diofl |=  (1U<<25))
#define ExtHVACPowerDisable()           (diofl &=(~(1U<<25)))
#define IsExtHVACPowerEnabled()         (diofl &   (1U<<25))

#define ExtRoomPowerSet()               (diofl |=  (1U<<26))
#define ExtRoomPowerReset()             (diofl &=(~(1U<<26)))
#define IsExtRoomPowerActiv()           (diofl &   (1U<<26))

#define ExtRoomPowerEnable()            (diofl |=  (1U<<27))
#define ExtRoomPowerDisable()           (diofl &=(~(1U<<27)))
#define IsExtRoomPowerEnabled()         (diofl &   (1U<<27))

#define ExtCardStackerSet()             (diofl |=  (1U<<28))
#define ExtCardStackerReset()           (diofl &=(~(1U<<28)))
#define IsExtCardStackerActiv()         (diofl &   (1U<<28))

#define ExtCardStackerEnable()          (diofl |=  (1U<<29))
#define ExtCardStackerDisable()         (diofl &=(~(1U<<29)))
#define IsExtCardStackerEnabled()       (diofl &   (1U<<29))
/***********************************************************************
**	 D I G I T A L		 I N P U T		0 ~	7		S T A T E S	
***********************************************************************/
#define DIN0Set()                       (din_0_7 |=   (1U<<0)) 
#define DIN1Set()                       (din_0_7 |=   (1U<<1))
#define DIN2Set()                       (din_0_7 |=   (1U<<2))
#define DIN3Set()                       (din_0_7 |=   (1U<<3))
#define DIN4Set()                       (din_0_7 |=   (1U<<4))
#define DIN5Set()                       (din_0_7 |=   (1U<<5))
#define DIN6Set()                       (din_0_7 |=   (1U<<6))
#define DIN7Set()                       (din_0_7 |=   (1U<<7))

#define DIN0Reset()                     (din_0_7 &= (~(1U<<0)))
#define DIN1Reset()                     (din_0_7 &= (~(1U<<1)))
#define DIN2Reset()                     (din_0_7 &= (~(1U<<2)))
#define DIN3Reset()                     (din_0_7 &= (~(1U<<3)))
#define DIN4Reset()                     (din_0_7 &= (~(1U<<4)))
#define DIN5Reset()                     (din_0_7 &= (~(1U<<5)))
#define DIN6Reset()                     (din_0_7 &= (~(1U<<6)))
#define DIN7Reset()                     (din_0_7 &= (~(1U<<7)))

#define IsDIN0Activ()                   (din_0_7 &   (1U<<0)) // Indor Card Reader - Not Debounced HW Input Register Shadow
#define IsDIN1Activ()                   (din_0_7 &   (1U<<1)) // SOS Alarm Switch
#define IsDIN2Activ()                   (din_0_7 &   (1U<<2)) // SOS Reset Switch
#define IsDIN3Activ()                   (din_0_7 &   (1U<<3)) // Call Handmaid Switch
#define IsDIN4Activ()                   (din_0_7 &   (1U<<4)) // Minibar Sensor
#define IsDIN5Activ()                   (din_0_7 &   (1U<<5)) // Balcony Door Sensor
#define IsDIN6Activ()                   (din_0_7 &   (1U<<6)) // DND Switch
#define IsDIN7Activ()                   (din_0_7 &   (1U<<7)) // Entry Door Sensor

#define DIN0StateSet()                  (din_state |=  (1U<<0))
#define DIN1StateSet()                  (din_state |=  (1U<<1))
#define DIN2StateSet()                  (din_state |=  (1U<<2))
#define DIN3StateSet()                  (din_state |=  (1U<<3))
#define DIN4StateSet()                  (din_state |=  (1U<<4))
#define DIN5StateSet()                  (din_state |=  (1U<<5))
#define DIN6StateSet()                  (din_state |=  (1U<<6))
#define DIN7StateSet()                  (din_state |=  (1U<<7))

#define DIN0StateReset()                (din_state &= (~(1U<<0)))
#define DIN1StateReset()                (din_state &= (~(1U<<1)))
#define DIN2StateReset()                (din_state &= (~(1U<<2)))
#define DIN3StateReset()                (din_state &= (~(1U<<3)))
#define DIN4StateReset()                (din_state &= (~(1U<<4)))
#define DIN5StateReset()                (din_state &= (~(1U<<5)))
#define DIN6StateReset()                (din_state &= (~(1U<<6)))
#define DIN7StateReset()                (din_state &= (~(1U<<7)))

#define IsDIN0StateActiv()              (din_state &   (1U<<0))   // Indor Card Reader - Debounced And Actual Digital Input State 
#define IsDIN1StateActiv()              (din_state &   (1U<<1)) 
#define IsDIN2StateActiv()              (din_state &   (1U<<2)) 
#define IsDIN3StateActiv()              (din_state &   (1U<<3)) 
#define IsDIN4StateActiv()              (din_state &   (1U<<4)) 
#define IsDIN5StateActiv()              (din_state &   (1U<<5)) 
#define IsDIN6StateActiv()              (din_state &   (1U<<6)) 
#define IsDIN7StateActiv()              (din_state &   (1U<<7))
/***********************************************************************
**	 C A P A C I T I V E		S E N S O R 		S T A T E S		
***********************************************************************/
//#define IsBTNMaidActiv()			(din_cap_sen &   (1U<<0))
//#define IsBTNBellActiv()			(din_cap_sen &   (1U<<2))
/***********************************************************************
**	 D I G I T A L		O U T P U T		0 ~	7		C O N T R O L	
***********************************************************************/
#define RoomPowerOn()                   (dout_0_7 |=  (1U<<0))    // Room Main Power Contactor
#define RoomPowerOff()                  (dout_0_7 &=(~(1U<<0)))
#define IsRoomPowerActiv()			    (dout_0_7 &   (1U<<0))
#define DNDPanelPwrOn()                 (dout_0_7 |=  (1U<<1))    // Room DND Panel Power
#define DNDPanelPwrOff()   		        (dout_0_7 &=(~(1U<<1)))
#define IsDNDPanelPwrActiv()		    (dout_0_7 &   (1U<<1))
#define DoorLightOn()                   (dout_0_7 |=  (1U<<2))    // Room Welcome Light
#define DoorLightOff()                  (dout_0_7 &=(~(1U<<2)))
#define IsDoorLightActiv()              (dout_0_7 &   (1U<<2))
#define DoorBellOn()              		(dout_0_7 |=  (1U<<3))    // Room Door Bell 
#define DoorBellOff()             		(dout_0_7 &=(~(1U<<3)))
#define IsDoorBellActiv()              	(dout_0_7 &   (1U<<3))
#define HVACPowerOn()                   (dout_0_7 |=  (1U<<4))    // Room HVAC Power Contactor  
#define HVACPowerOff()                  (dout_0_7 &=(~(1U<<4)))
#define IsHVACPowerActiv()              (dout_0_7 &   (1U<<4))    
#define HVACRequestOn()                 (dout_0_7 |=  (1U<<5))    // Room HVAC Request  
#define HVACRequestOff()                (dout_0_7 &=(~(1U<<5)))
#define IsHVACRequestActiv()            (dout_0_7 &   (1U<<5))
#define DoorLockOn()                    (dout_0_7 |=  (1U<<6))    // Entry Door Lock Coil
#define DoorLockOff()                   (dout_0_7 &=(~(1U<<6)))
#define IsDoorLockActiv()			    (dout_0_7 &   (1U<<6))
#define BuzzerOn()                      (dout_0_7 |=  (1U<<7))    // PCB Buzzer
#define BuzzerOff()                     (dout_0_7 &=(~(1U<<7)))
#define IsBuzzerActiv()					(dout_0_7 &   (1U<<7))

#define DOUT_LEDStatusToggle()          (HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_7))
#define DOUT_LEDStatusOn()              (HAL_GPIO_WritePin (GPIOD, GPIO_PIN_7, GPIO_PIN_SET))
#define DOUT_LEDStatusOff()             (HAL_GPIO_WritePin (GPIOD, GPIO_PIN_7, GPIO_PIN_RESET))
#define DOUT_LEDStatus()                (HAL_GPIO_ReadPin  (GPIOD, GPIO_PIN_7))
#define DOUT_DoorLock()                 (HAL_GPIO_ReadPin  (GPIOC, GPIO_PIN_8))
#define DOUT_Buzzer()                   (HAL_GPIO_ReadPin  (GPIOD, GPIO_PIN_4))
#define DOUT_HVACRequest()              (HAL_GPIO_ReadPin  (GPIOD, GPIO_PIN_2))
/* Exported Function   -------------------------------------------------------*/
void DIO_Service(void);
#endif
/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
