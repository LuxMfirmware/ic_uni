/**
 ******************************************************************************
 * File Name          : main.c
 * Date               : 10.3.2018.
 * Description        : Hotel Room Thermostat Program Header
 ******************************************************************************
 *
 *
 ******************************************************************************
 */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H__
#define __MAIN_H__                              FW_BUILD // version
/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "GUI.h"
#include "DIALOG.h"
#include "common.h"
#include "Resource.h"
#include "LuxNET.h"
#include "stm32746g_eeprom.h" 
#include "stm32746g.h"
#include "stm32746g_ts.h"

// << ISPRAVKA: Dodajemo headere za BSP drajvere ovdje >>
// Na ovaj nacin, funkcije iz ovih drajvera postaju vidljive u cijelom projektu.
#include "stm32746g_qspi.h"
#include "stm32746g_sdram.h"

/* Exported types ------------------------------------------------------------*/
#define BUZZER_CLICK_TIME                   20U     // single 50 ms tone when button pressed
typedef struct{
	uint8_t seconds;     	/*!< Seconds parameter, from 00 to 59 */
	uint16_t subseconds; 	/*!< Subsecond downcounter. When it reaches zero, it's reload value is the same as */
	uint8_t minutes;     	/*!< Minutes parameter, from 00 to 59 */
	uint8_t hours;       	/*!< Hours parameter, 24Hour mode, 00 to 23 */
	uint8_t day;         	/*!< Day in a week, from 1 to 7 */
	uint8_t date;        	/*!< Date in a month, 1 to 31 */
	uint8_t month;       	/*!< Month in a year, 1 to 12 */
	uint8_t year;        	/*!< Year parameter, 00 to 99, 00 is 2000 and 99 is 2099 */
	uint32_t unix;       	/*!< Seconds from 01.01.1970 00:00:00 */	
} RTC_t;
/* Exported constants --------------------------------------------------------*/

/** ==========================================================================*/
/**    	 P C A 9 6 8 5    	R E G I S T E R  		A D D R E S S E	  		  */
/** ==========================================================================*/
//
#define PWM_0_15_FREQUENCY_DEFAULT			1000U		// i2c pwm controller 1 default frequency in Hertz 
#define PWM_16_31_FREQUENCY_DEFAULT			1000U		// i2c pwm controller 2 default frequency in Hertz 
#define PCA9685_REGISTER_SIZE				256U        // nuber of pca9685 registers
#define PWM_BUFFER_SIZE						16U			// number of pwm output
#define PWM_UPDATE_TIMEOUT					12U			// 20 ms pwm data transfer timeout
#define PWM_REFRESH_TIME					23U			// update pwm output every 23 ms
#define PWM_NUMBER_OF_TRIAL					34U			// try max. 50 times to accomplish pca9685 operation
#define PWM_ZERO_TRESHOLD					8U			// +/- zero treshold to start pwm
#define PWM_DEFAULT_PRESCALE				0x1eU		// prescale value for 200Hz default pwm frequncy

	
#define PCA9685_GENERAL_CALL_ACK			0x00U		// pca9685 general call address with ACK response
#define PCA9685_GENERAL_CALL_NOACK			0x01U		// pca9685 general call address without ACK response
#define PCA9685_DEFAULT_ALLCALLADR			0xe0U		// pca9685 default all device call address
#define PCA9685_DEFAULT_SUBADR_1			0xe2U		// pca9685 default subaddress 1
#define PCA9685_DEFAULT_SUBADR_2			0xe4U		// pca9685 default subaddress 2
#define PCA9685_DEFAULT_SUBADR_3			0xe2U		// pca9685 default subaddress 3
#define PCA9685_SW_RESET_COMMAND			0x06U		// i2c pwm controller reset command

#define PCA9685_MODE_1_REG_ADDRESS				0x00U
#define PCA9685_MODE_1_RESTART_BIT				(1U << 7U)
#define PCA9685_MODE_1_EXTCLK_BIT				(1U << 6U)
#define PCA9685_MODE_1_AI_BIT					(1U << 5U)
#define PCA9685_MODE_1_SLEEP_BIT				(1U << 4U)
#define PCA9685_MODE_1_SUB_1_BIT				(1U << 3U)
#define PCA9685_MODE_1_SUB_2_BIT				(1U << 2U)
#define PCA9685_MODE_1_SUB_3_BIT				(1U << 1U)
#define PCA9685_MODE_1_ALLCALL_BIT				(1U << 0U)
#define PCA9685_MODE_2_REG_ADDRESS				0x01U
#define PCA9685_MODE_2_INVRT_BIT				(1U << 4U)
#define PCA9685_MODE_2_OCH_BIT					(1U << 3U)
#define PCA9685_MODE_2_OUTDRV_BIT				(1U << 2U)
#define PCA9685_MODE_2_OUTNE_1_BIT				(1U << 1U)
#define PCA9685_MODE_2_OUTNE_0_BIT				(1U << 0U)
#define PCA9685_SUBADR_1_REG_ADDRESS			0x02U
#define PCA9685_SUBADR_2_REG_ADDRESS			0x03U
#define PCA9685_SUBADR_3_REG_ADDRESS			0x04U
#define PCA9685_ALLCALLADR_REG_ADDRESS			0x05U
#define PCA9685_LED_0_ON_L_REG_ADDRESS			0x06U
#define PCA9685_LED_0_ON_H_REG_ADDRESS			0x07U
#define PCA9685_LED_0_OFF_L_REG_ADDRESS			0x08U
#define PCA9685_LED_0_OFF_H_REG_ADDRESS			0x09U
#define PCA9685_LED_1_ON_L_REG_ADDRESS			0x0aU
#define PCA9685_LED_1_ON_H_REG_ADDRESS			0x0bU
#define PCA9685_LED_1_OFF_L_REG_ADDRESS			0x0cU
#define PCA9685_LED_1_OFF_H_REG_ADDRESS			0x0dU
#define PCA9685_LED_2_ON_L_REG_ADDRESS			0x0eU
#define PCA9685_LED_2_ON_H_REG_ADDRESS			0x0fU
#define PCA9685_LED_2_OFF_L_REG_ADDRESS			0x10U
#define PCA9685_LED_2_OFF_H_REG_ADDRESS			0x11U
#define PCA9685_LED_3_ON_L_REG_ADDRESS			0x12U
#define PCA9685_LED_3_ON_H_REG_ADDRESS			0x13U
#define PCA9685_LED_3_OFF_L_REG_ADDRESS			0x14U
#define PCA9685_LED_3_OFF_H_REG_ADDRESS			0x15U
#define PCA9685_LED_4_ON_L_REG_ADDRESS			0x16U
#define PCA9685_LED_4_ON_H_REG_ADDRESS			0x17U
#define PCA9685_LED_4_OFF_L_REG_ADDRESS			0x18U
#define PCA9685_LED_4_OFF_H_REG_ADDRESS			0x19U
#define PCA9685_LED_5_ON_L_REG_ADDRESS			0x1aU
#define PCA9685_LED_5_ON_H_REG_ADDRESS			0x1bU
#define PCA9685_LED_5_OFF_L_REG_ADDRESS			0x1cU
#define PCA9685_LED_5_OFF_H_REG_ADDRESS			0x1dU
#define PCA9685_LED_6_ON_L_REG_ADDRESS			0x1eU
#define PCA9685_LED_6_ON_H_REG_ADDRESS			0x1fU
#define PCA9685_LED_6_OFF_L_REG_ADDRESS			0x20U
#define PCA9685_LED_6_OFF_H_REG_ADDRESS			0x21U
#define PCA9685_LED_7_ON_L_REG_ADDRESS			0x22U
#define PCA9685_LED_7_ON_H_REG_ADDRESS			0x23U
#define PCA9685_LED_7_OFF_L_REG_ADDRESS			0x24U
#define PCA9685_LED_7_OFF_H_REG_ADDRESS			0x25U
#define PCA9685_LED_8_ON_L_REG_ADDRESS			0x26U
#define PCA9685_LED_8_ON_H_REG_ADDRESS			0x27U
#define PCA9685_LED_8_OFF_L_REG_ADDRESS			0x28U
#define PCA9685_LED_8_OFF_H_REG_ADDRESS			0x29U
#define PCA9685_LED_9_ON_L_REG_ADDRESS			0x2aU
#define PCA9685_LED_9_ON_H_REG_ADDRESS			0x2bU
#define PCA9685_LED_9_OFF_L_REG_ADDRESS			0x2cU
#define PCA9685_LED_9_OFF_H_REG_ADDRESS			0x2dU
#define PCA9685_LED_10_ON_L_REG_ADDRESS			0x2eU
#define PCA9685_LED_10_ON_H_REG_ADDRESS			0x2eU
#define PCA9685_LED_10_OFF_L_REG_ADDRESS		0x30U
#define PCA9685_LED_10_OFF_H_REG_ADDRESS		0x31U
#define PCA9685_LED_11_ON_L_REG_ADDRESS			0x32U
#define PCA9685_LED_11_ON_H_REG_ADDRESS			0x33U
#define PCA9685_LED_11_OFF_L_REG_ADDRESS		0x34U
#define PCA9685_LED_11_OFF_H_REG_ADDRESS		0x35U
#define PCA9685_LED_12_ON_L_REG_ADDRESS			0x36U
#define PCA9685_LED_12_ON_H_REG_ADDRESS			0x37U
#define PCA9685_LED_12_OFF_L_REG_ADDRESS		0x38U
#define PCA9685_LED_12_OFF_H_REG_ADDRESS		0x39U
#define PCA9685_LED_13_ON_L_REG_ADDRESS			0x3aU
#define PCA9685_LED_13_ON_H_REG_ADDRESS			0x3bU
#define PCA9685_LED_13_OFF_L_REG_ADDRESS		0x3cU
#define PCA9685_LED_13_OFF_H_REG_ADDRESS		0x3dU
#define PCA9685_LED_14_ON_L_REG_ADDRESS			0x3eU
#define PCA9685_LED_14_ON_H_REG_ADDRESS			0x3fU
#define PCA9685_LED_14_OFF_L_REG_ADDRESS		0x40U
#define PCA9685_LED_14_OFF_H_REG_ADDRESS		0x41U
#define PCA9685_LED_15_ON_L_REG_ADDRESS			0x42U
#define PCA9685_LED_15_ON_H_REG_ADDRESS			0x43U
#define PCA9685_LED_15_OFF_L_REG_ADDRESS		0x44U
#define PCA9685_LED_15_OFF_H_REG_ADDRESS		0x45U
/**
*	Reserved addresse for future use
*/
#define PCA9685_ALL_LED_ON_L_REG_ADDRESS		0xfaU
#define PCA9685_ALL_LED_ON_H_REG_ADDRESS		0xfbU
#define PCA9685_ALL_LED_OFF_L_REG_ADDRESS		0xfcU
#define PCA9685_ALL_LED_OFF_H_REG_ADDRESS		0xfdU
#define PCA9685_PRE_SCALE_REG_ADDRESS			0xfeU
#define PCA9685_TEST_MODE_REG_ADDRESS			0xffU
//
/** ==========================================================================*/
/**    	 P C A 9 6 8 5    	R E G I S T E R S	M N E M O N I C				  */
/** ==========================================================================*/
//
#define PCA9685_MODE_1_REGISTER					(pca9685_register[0])
#define PCA9685_MODE_2_REGISTER					(pca9685_register[1])
#define PCA9685_SUBADR_1_REGISTER				(pca9685_register[2])
#define PCA9685_SUBADR_2_REGISTER				(pca9685_register[3])
#define PCA9685_SUBADR_3_REGISTER				(pca9685_register[4])
#define PCA9685_ALLCALLADR_REGISTER				(pca9685_register[5])
#define PCA9685_LED_0_ON_L_REGISTER				(pca9685_register[6])
#define PCA9685_LED_0_ON_H_REGISTER				(pca9685_register[7])
#define PCA9685_LED_0_OFF_L_REGISTER			(pca9685_register[8])
#define PCA9685_LED_0_OFF_H_REGISTER			(pca9685_register[9])
#define PCA9685_LED_1_ON_L_REGISTER				(pca9685_register[10])
#define PCA9685_LED_1_ON_H_REGISTER				(pca9685_register[11])
#define PCA9685_LED_1_OFF_L_REGISTER			(pca9685_register[12])
#define PCA9685_LED_1_OFF_H_REGISTER			(pca9685_register[13])
#define PCA9685_LED_2_ON_L_REGISTER				(pca9685_register[14])
#define PCA9685_LED_2_ON_H_REGISTER				(pca9685_register[15])
#define PCA9685_LED_2_OFF_L_REGISTER			(pca9685_register[16])
#define PCA9685_LED_2_OFF_H_REGISTER			(pca9685_register[17])
#define PCA9685_LED_3_ON_L_REGISTER				(pca9685_register[18])
#define PCA9685_LED_3_ON_H_REGISTER				(pca9685_register[19])
#define PCA9685_LED_3_OFF_L_REGISTER			(pca9685_register[20])
#define PCA9685_LED_3_OFF_H_REGISTER			(pca9685_register[21])
#define PCA9685_LED_4_ON_L_REGISTER				(pca9685_register[22])
#define PCA9685_LED_4_ON_H_REGISTER				(pca9685_register[23])
#define PCA9685_LED_4_OFF_L_REGISTER			(pca9685_register[24])
#define PCA9685_LED_4_OFF_H_REGISTER			(pca9685_register[25])
#define PCA9685_LED_5_ON_L_REGISTER				(pca9685_register[26])
#define PCA9685_LED_5_ON_H_REGISTER				(pca9685_register[27])
#define PCA9685_LED_5_OFF_L_REGISTER			(pca9685_register[28])
#define PCA9685_LED_5_OFF_H_REGISTER			(pca9685_register[29])
#define PCA9685_LED_6_ON_L_REGISTER				(pca9685_register[30])
#define PCA9685_LED_6_ON_H_REGISTER				(pca9685_register[31])
#define PCA9685_LED_6_OFF_L_REGISTER			(pca9685_register[32])
#define PCA9685_LED_6_OFF_H_REGISTER			(pca9685_register[33])
#define PCA9685_LED_7_ON_L_REGISTER				(pca9685_register[34])
#define PCA9685_LED_7_ON_H_REGISTER				(pca9685_register[35])
#define PCA9685_LED_7_OFF_L_REGISTER			(pca9685_register[36])
#define PCA9685_LED_7_OFF_H_REGISTER			(pca9685_register[37])
#define PCA9685_LED_8_ON_L_REGISTER				(pca9685_register[38])
#define PCA9685_LED_8_ON_H_REGISTER				(pca9685_register[39])
#define PCA9685_LED_8_OFF_L_REGISTER			(pca9685_register[40])
#define PCA9685_LED_8_OFF_H_REGISTER			(pca9685_register[41])
#define PCA9685_LED_9_ON_L_REGISTER				(pca9685_register[42])			
#define PCA9685_LED_9_ON_H_REGISTER				(pca9685_register[43])			
#define PCA9685_LED_9_OFF_L_REGISTER			(pca9685_register[44])		
#define PCA9685_LED_9_OFF_H_REGISTER			(pca9685_register[45])			
#define PCA9685_LED_10_ON_L_REGISTER			(pca9685_register[46])		
#define PCA9685_LED_10_ON_H_REGISTER			(pca9685_register[47])	
#define PCA9685_LED_10_OFF_L_REGISTER			(pca9685_register[48])	
#define PCA9685_LED_10_OFF_H_REGISTER			(pca9685_register[49])		
#define PCA9685_LED_11_ON_L_REGISTER			(pca9685_register[50])			
#define PCA9685_LED_11_ON_H_REGISTER			(pca9685_register[51])		
#define PCA9685_LED_11_OFF_L_REGISTER			(pca9685_register[52])	
#define PCA9685_LED_11_OFF_H_REGISTER			(pca9685_register[53])	
#define PCA9685_LED_12_ON_L_REGISTER			(pca9685_register[54])		
#define PCA9685_LED_12_ON_H_REGISTER			(pca9685_register[55])
#define PCA9685_LED_12_OFF_L_REGISTER			(pca9685_register[56])
#define PCA9685_LED_12_OFF_H_REGISTER			(pca9685_register[57])		
#define PCA9685_LED_13_ON_L_REGISTER			(pca9685_register[58])		
#define PCA9685_LED_13_ON_H_REGISTER			(pca9685_register[59])		
#define PCA9685_LED_13_OFF_L_REGISTER			(pca9685_register[60])	
#define PCA9685_LED_13_OFF_H_REGISTER			(pca9685_register[61])	
#define PCA9685_LED_14_ON_L_REGISTER			(pca9685_register[62])		
#define PCA9685_LED_14_ON_H_REGISTER			(pca9685_register[63])		
#define PCA9685_LED_14_OFF_L_REGISTER			(pca9685_register[64])		
#define PCA9685_LED_14_OFF_H_REGISTER			(pca9685_register[65])		
#define PCA9685_LED_15_ON_L_REGISTER			(pca9685_register[66])
#define PCA9685_LED_15_ON_H_REGISTER			(pca9685_register[67])
#define PCA9685_LED_15_OFF_L_REGISTER			(pca9685_register[68])
#define PCA9685_LED_15_OFF_H_REGISTER			(pca9685_register[69])
#define PCA9685_END_1_REGISTER					(pca9685_register[70])
#define PCA9685_ALL_LED_ON_L_REGISTER			(pca9685_register[250])
#define PCA9685_ALL_LED_ON_H_REGISTER			(pca9685_register[251])
#define PCA9685_ALL_LED_OFF_L_REGISTER			(pca9685_register[252])
#define PCA9685_ALL_LED_OFF_H_REGISTER			(pca9685_register[253])
#define PCA9685_PRE_SCALE_REGISTER				(pca9685_register[254])
#define PCA9685_END_2_REGISTER					(pca9685_register[255])

#define I2CEXP0_WRADD       0x40
#define I2CEXP0_RDADD       0x41
#define I2CEXP1_WRADD       0x42
#define I2CEXP1_RDADD       0x43
#define I2CEXP2_WRADD       0x44
#define I2CEXP2_RDADD       0x45
#define I2CEXP3_WRADD       0x46
#define I2CEXP3_RDADD       0x47
#define I2CEXP4_WRADD       0x48
#define I2CEXP4_RDADD       0x49
#define I2CEXP5_WRADD       0x4A
#define I2CEXP5_RDADD       0x4B
#define I2CPWM0_WRADD       0x90
#define I2CPWM0_RDADD       0x91
#define I2CPWM1_WRADD       0x92
#define I2CPWM1_RDADD       0x93
#define I2CPWM_TOUT         15
/* Exported variable  --------------------------------------------------------*/


// Globalni fleg koji aktivira mod visoke preciznosti
extern bool g_high_precision_mode;
extern uint8_t sysfl, initfl;
extern uint32_t thstfl_memo;
extern uint8_t dispfl_memo;
extern bool LSE_Failed; // flag oznacava LSE oscilator rtc modula false = 32.768 Hz kristal / true = interni oscilator
/* Exported macros  --------------------------------------------------------*/
#define SYS_NewLogSet()             (sysfl |=  (0x1U<<0))
#define SYS_NewLogReset()           (sysfl &=(~(0x1U<<0)))
#define IsSYS_NewLogSet()           (sysfl &   (0x1U<<0))
#define SYS_LogListFullSet()        (sysfl |=  (0x1U<<1))
#define SYS_LogListFullReset()      (sysfl &=(~(0x1U<<1)))
#define IsSYS_LogListFullSet()      (sysfl &   (0x1U<<1))
#define SYS_FileRxOkSet()           (sysfl |=  (0x1U<<2))
#define SYS_FileRxOkReset()         (sysfl &=(~(0x1U<<2)))
#define IsSYS_FileRxOkSet()         (sysfl &   (0x1U<<2))
#define SYS_FileRxFailSet()         (sysfl |=  (0x1U<<3))
#define SYS_FileRxFailReset()       (sysfl &=(~(0x1U<<3)))
#define IsSYS_FileRxFailSet()       (sysfl &   (0x1U<<3))
#define SYS_UpdOkSet()              (sysfl |=  (0x1U<<4))
#define SYS_UpdOkReset()            (sysfl &=(~(0x1U<<4)))
#define IsSYS_UpdOkSet()            (sysfl &   (0x1U<<4))
#define SYS_UpdFailSet()            (sysfl |=  (0x1U<<5))
#define SYS_UpdFailReset()          (sysfl &=(~(0x1U<<5)))
#define IsSYS_UpdFailSet()          (sysfl &   (0x1U<<5))
#define SYS_ImageRqSet()            (sysfl |=  (0x1U<<6))
#define SYS_ImageRqReset()          (sysfl &=(~(0x1U<<6)))
#define IsSYS_ImageRqSet()          (sysfl &   (0x1U<<6))
#define SYS_FwRqSet()               (sysfl |=  (0x1U<<7))
#define SYS_FwRqReset()             (sysfl &=(~(0x1U<<7)))
#define IsSYS_FwRqSet()             (sysfl &   (0x1U<<7))
//#define BuzzerOn()                  (0 /*DoNothing() HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET)*/)
//#define BuzzerOff()                 (0 /*DoNothing() HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET)*/)
//#define IsBuzzerActiv()             (false /*HAL_GPIO_ReadPin (GPIOD, GPIO_PIN_4) == GPIO_PIN_SET*/)
//#define Light1On()                  (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET))
//#define Light1Off()                 (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET))
//#define IsLight1Active()            (HAL_GPIO_ReadPin (GPIOC, GPIO_PIN_12) == GPIO_PIN_SET)
//#define Light2On()                  (HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET))
//#define Light2Off()                 (HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET))
//#define IsLight2Active()            (HAL_GPIO_ReadPin (GPIOD, GPIO_PIN_2) == GPIO_PIN_SET) 
//#define Light3On()                  (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET))
//#define Light3Off()                 (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET))
//#define IsLight3Active()            (HAL_GPIO_ReadPin (GPIOC, GPIO_PIN_8) == GPIO_PIN_SET)
//#define Light4On()                  (HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET))
//#define Light4Off()                 (HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET))
//#define IsLight4Active()            (HAL_GPIO_ReadPin (GPIOD, GPIO_PIN_4) == GPIO_PIN_SET)
//#define Light5On()                  (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET))
//#define Light5Off()                 (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET))
//#define IsLight5Active()            (HAL_GPIO_ReadPin (GPIOC, GPIO_PIN_11) == GPIO_PIN_SET)
//#define Light6On()                  (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET))
//#define Light6Off()                 (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET))
//#define IsLight6Active()            (HAL_GPIO_ReadPin (GPIOC, GPIO_PIN_10) == GPIO_PIN_SET)
#define IsButtonActive()             (HAL_GPIO_ReadPin (GPIOC, GPIO_PIN_3) == GPIO_PIN_RESET)
/*============================================================================*/
/* MAKROI ZA KONTROLU LOKALNIH GPIO IZLAZA                                    */
/*============================================================================*/

// --- Kontrola Svjetala ---
#define Light1On()                  (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET))
#define Light1Off()                 (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET))
#define IsLight1Active()            (HAL_GPIO_ReadPin (GPIOC, GPIO_PIN_12) == GPIO_PIN_SET)

#define Light2On()                  (HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET))
#define Light2Off()                 (HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET))
#define IsLight2Active()            (HAL_GPIO_ReadPin (GPIOD, GPIO_PIN_2) == GPIO_PIN_SET) 

#define Light3On()                  (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET))
#define Light3Off()                 (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET))
#define IsLight3Active()            (HAL_GPIO_ReadPin (GPIOC, GPIO_PIN_8) == GPIO_PIN_SET)

#define Light4On()                  (HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET))
#define Light4Off()                 (HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET))
#define IsLight4Active()            (HAL_GPIO_ReadPin (GPIOD, GPIO_PIN_4) == GPIO_PIN_SET)

#define Light5On()                  (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET))
#define Light5Off()                 (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET))
#define IsLight5Active()            (HAL_GPIO_ReadPin (GPIOC, GPIO_PIN_11) == GPIO_PIN_SET)

#define Light6On()                  (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET))
#define Light6Off()                 (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET))
#define IsLight6Active()            (HAL_GPIO_ReadPin (GPIOC, GPIO_PIN_10) == GPIO_PIN_SET)

// --- Kontrola Zujalice (Buzzer) ---
#define BuzzerOn()                  (HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET))
#define BuzzerOff()                 (HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET))
#define IsBuzzerActiv()             (HAL_GPIO_ReadPin (GPIOD, GPIO_PIN_4) == GPIO_PIN_SET)

// --- Provjera Eksternog Tastera ---
#define IsButtonActive()            (HAL_GPIO_ReadPin (GPIOC, GPIO_PIN_3) == GPIO_PIN_RESET)


/* Exported hal handler --------------------------------------------------------*/
extern RTC_t date_time; 
extern RTC_TimeTypeDef rtctm;
extern RTC_DateTypeDef rtcdt;
extern CRC_HandleTypeDef hcrc;
extern RTC_HandleTypeDef hrtc;
extern I2C_HandleTypeDef hi2c3;
extern I2C_HandleTypeDef hi2c4;
extern TIM_HandleTypeDef htim9;
extern QSPI_HandleTypeDef hqspi; 
extern IWDG_HandleTypeDef hiwdg;
extern LTDC_HandleTypeDef hltdc;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern DMA2D_HandleTypeDef hdma2d;
/* Exported function --------------------------------------------------------*/
void SYSRestart(void);
void SetPin(uint8_t pin, uint8_t pinVal);
void RTC_GetDateTime(RTC_t* data, uint32_t format);
void ErrorHandler(uint8_t function, uint8_t driver);
void PCA9685_Init(void);
void PCA9685_Reset(void);
void PCA9685_OutputUpdate(void);
void PCA9685_SetOutput(const uint8_t pin, const uint8_t value);
void PCA9685_SetOutputFrequency(uint16_t frequency);
void SetDefault(void);
#endif /* __MAIN_H */
/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
