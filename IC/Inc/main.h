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
/* Exported variable  --------------------------------------------------------*/
extern uint8_t sysfl, initfl;
extern uint32_t thstfl_memo;
extern uint8_t dispfl_memo;
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
#define BuzzerOn()                  (HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET))
#define BuzzerOff()                 (HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET))
#define IsBuzzerActiv()             (HAL_GPIO_ReadPin (GPIOD, GPIO_PIN_4) == GPIO_PIN_SET)
#define Light1On()                  (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET))
#define Light1Off()                 (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET))
#define IsLight1Active()            (HAL_GPIO_ReadPin (GPIOC, GPIO_PIN_12) == GPIO_PIN_SET)
#define Light2On()                  (HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET))
#define Light2Off()                 (HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET))
#define IsLight2Active()            (HAL_GPIO_ReadPin (GPIOD, GPIO_PIN_2) == GPIO_PIN_SET)
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
void PresentSystem(void);
void RTC_GetDateTime(RTC_t* data, uint32_t format);
void ErrorHandler(uint8_t function, uint8_t driver);
#endif /* __MAIN_H */
/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
