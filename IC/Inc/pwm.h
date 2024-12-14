/**
 ******************************************************************************
 * File Name          : pwm.c
 * Date               : 04/01/2018 5:24:19
 * Description        : pwm ouput processing header
 ******************************************************************************
 *
 *
 ******************************************************************************
 */
 
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PWM_H
#define __PWM_H
 
 
/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx.h"
/* Typedef -------------------------------------------------------------------*/

/* Define --------------------------------------------------------------------*/
#define PWM1_FREQDEF			            240U		// i2c pwm controller 1 default frequency in Hertz 
#define PWM2_FREQDEF			            1000U		// i2c pwm controller 2 default frequency in Hertz 
#define PCA_REGSIZE				            256U        // nuber of pca9685 registers
#define PWM_BUFSIZE                         16U			// number of pwm output
#define PWM_TOUT					        12U			// 20 ms pwm data transfer timeout
#define PWM_REFRESH_TIME					50U			// update pwm output every 23 ms
#define PWM_NUMBER_OF_TRIAL					34U			// try max. 50 times to accomplish pca9685 operation
#define PWM_ZERO_TRESHOLD					8U			// +/- zero treshold to start pwm
#define PWM_DEFAULT_PRESCALE				0x1eU		// prescale value for 200Hz default pwm frequncy
#define PWM1_RDADD			                0x91U		// pca9685 i2c bus address
#define PWM1_WRADD			                0x90U	
#define PWM2_RDADD			                0x93U	
#define PWM2_WRADD			                0x92U	
#define PCA9685_GENERAL_CALL_ACK			0x00U		// pca9685 general call address with ACK response
#define PCA9685_GENERAL_CALL_NOACK			0x01U		// pca9685 general call address without ACK response
#define PCA9685_DEFAULT_ALLCALLADR			0xe0U		// pca9685 default all device call address
#define PCA9685_DEFAULT_SUBADR_1			0xe2U		// pca9685 default subaddress 1
#define PCA9685_DEFAULT_SUBADR_2			0xe4U		// pca9685 default subaddress 2
#define PCA9685_DEFAULT_SUBADR_3			0xe2U		// pca9685 default subaddress 3
#define PCA9685_SW_RESET_COMMAND			0x06U		// i2c pwm controller reset command
/* Variable ------------------------------------------------------------------*/
extern uint16_t pwm1_freq;
extern uint16_t pwm2_freq;
extern uint32_t pwmfl;
extern uint8_t pwm[PWM_BUFSIZE];
extern uint8_t pca[PCA_REGSIZE];
/** ==========================================================================*/
/**    	 P C A 9 6 8 5    	R E G I S T E R  		A D D R E S S E	  		  */
/** ==========================================================================*/
#define PCA9685_MODE_1_REG_ADD				0x00U
#define PCA9685_MODE_1_RESTART_BIT          (1U<<7)
#define PCA9685_MODE_1_EXTCLK_BIT           (1U<<6)
#define PCA9685_MODE_1_AI_BIT               (1U<<5)
#define PCA9685_MODE_1_SLEEP_BIT            (1U<<4)
#define PCA9685_MODE_1_SUB_1_BIT            (1U<<3)
#define PCA9685_MODE_1_SUB_2_BIT            (1U<<2)
#define PCA9685_MODE_1_SUB_3_BIT            (1U<<1)
#define PCA9685_MODE_1_ALLCALL_BIT          (1U<<0)
#define PCA9685_MODE_2_REG_ADD				0x01U
#define PCA9685_MODE_2_INVRT_BIT            (1U<<4)
#define PCA9685_MODE_2_OCH_BIT              (1U<<3)
#define PCA9685_MODE_2_OUTDRV_BIT           (1U<<2)
#define PCA9685_MODE_2_OUTNE_1_BIT          (1U<<1)
#define PCA9685_MODE_2_OUTNE_0_BIT          (1U<<0)
#define PCA9685_SUBADR_1_REG_ADD			0x02U
#define PCA9685_SUBADR_2_REG_ADD			0x03U
#define PCA9685_SUBADR_3_REG_ADD			0x04U
#define PCA9685_ALLCALLADR_REG_ADD			0x05U
/**
*	Reserved addresse for future use
*/
#define PCA9685_ALL_LED_ON_L_REG_ADD		0xfaU
#define PCA9685_ALL_LED_ON_H_REG_ADD		0xfbU
#define PCA9685_ALL_LED_OFF_L_REG_ADD		0xfcU
#define PCA9685_ALL_LED_OFF_H_REG_ADD		0xfdU
#define PCA9685_PRE_SCALE_REG_ADD			0xfeU
#define PCA9685_TEST_MODE_REG_ADD			0xffU
/** ==========================================================================*/
/**    	 P C A 9 6 8 5    	R E G I S T E R S	M N E M O N I C				  */
/** ==========================================================================*/
#define PCA9685_MODE_1_REG					(pca[0])
#define PCA9685_MODE_2_REG					(pca[1])
#define PCA9685_SUBADR_1_REG				(pca[2])
#define PCA9685_SUBADR_2_REG				(pca[3])
#define PCA9685_SUBADR_3_REG				(pca[4])
#define PCA9685_ALLCALLADR_REG				(pca[5])
#define PCA9685_END_1_REG					(pca[70])
#define PCA9685_ALL_LED_ON_L_REG			(pca[250])
#define PCA9685_ALL_LED_ON_H_REG			(pca[251])
#define PCA9685_ALL_LED_OFF_L_REG			(pca[252])
#define PCA9685_ALL_LED_OFF_H_REG			(pca[253])
#define PCA9685_PRE_SCALE_REG				(pca[254])
#define PCA9685_END_2_REG					(pca[255])
/* Macro ---------------------------------------------------------------------*/
#define PWM_CalcPresc(FREQUNCY)             (PCA9685_PRE_SCALE_REG = ((25000000U / (4096U * FREQUNCY)) - 1U))
#define PWM_OutEnable()                     (HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, GPIO_PIN_RESET))
#define PWM_OutDisable()                    (HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, GPIO_PIN_SET))
#define IsPWM_OutEnabled()                  (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_11) == GPIO_PIN_RESET)
#define Dimmer1Set(val)                     (pwm[0] = val)
#define Dimmer1Reset()                      (pwm[0] = 0)
#define IsDimmer1Set()                      (pwm[0] != 0)
#define Dimmer2Set(val)                     (pwm[1] = val)
#define Dimmer2Reset()                      (pwm[1] = 0)
#define IsDimmer2Set()                      (pwm[1] != 0)
#define Dimmer3Set(val)                     (pwm[2] = val)
#define Dimmer3Reset()                      (pwm[2] = 0)
#define IsDimmer3Set()                      (pwm[2] != 0)
#define Dimmer4Set(val)                     (pwm[3] = val)
#define Dimmer4Reset()                      (pwm[3] = 0)
#define IsDimmer4Set()                      (pwm[3] != 0)
#define Light1On()                          (pwm[4] = 0xFF)
#define Light1Off()                         (pwm[4] = 0)
#define IsLight1On()                        (pwm[4] == 0xFF)
#define Light2On()                          (pwm[8] = 0xFF)
#define Light2Off()                         (pwm[8] = 0)
#define IsLight2On()                        (pwm[8] == 0xFF)
#define Light3On()                          (pwm[11] = 0xFF)
#define Light3Off()                         (pwm[11] = 0)
#define IsLight3On()                        (pwm[11] == 0xFF)
#define Light4On()                          (pwm[15] = 0xFF)
#define Light4Off()                         (pwm[15] = 0)
#define IsLight4On()                        (pwm[15] == 0xFF)
/* Function prototypes   -----------------------------------------------------*/
void PWM_Service(void);
void PCA9685_SetOutFreq(uint16_t val);
void PCA9685_WriteInc(uint8_t add, uint8_t len);
void PCA9685_WriteOut(void);

#endif  /* __PWM_H */
/******************************   END OF FILE  ********************************/
