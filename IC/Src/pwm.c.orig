/**
 ******************************************************************************
 * File Name          : pwm.c
 * Date               : 04/01/2018 5:26:19
 * Description        : pwm ouput processing
 ******************************************************************************
 *
 *
 ******************************************************************************
 */
 
 
/* Includes ------------------------------------------------------------------*/
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
/* Variable ------------------------------------------------------------------*/
uint32_t pwmfl;
uint16_t pwm1_freq;
uint16_t pwm2_freq;
uint8_t pwm[PWM_BUFSIZE];
uint8_t pca[PCA_REGSIZE];
/* Typedef -------------------------------------------------------------------*/
/* Define --------------------------------------------------------------------*/
/* Macro ---------------------------------------------------------------------*/
/* Program code   ------------------------------------------------------------*/
void PWM_Service(void)
{
	uint32_t i, s;
    uint8_t buf[2];
    static uint32_t oldpwm;
    
    static enum
    {
        PWM_INIT = 0,
        PWM_STOP,
        PWM_UPDATE,
        PWM_ERROR
	
    } PWM_State = PWM_INIT;
    
    
	switch(PWM_State)
	{
		case PWM_INIT:
		{
			ZEROFILL(pwm, PWM_BUFSIZE);
			ZEROFILL(pca, PCA_REGSIZE);
            buf[0] = PCA9685_SW_RESET_COMMAND;
            if(HAL_I2C_Master_Transmit(&hi2c4, PCA9685_GENERAL_CALL_ACK, buf, 1U, PWM_TOUT) != HAL_OK)
            {
                PWM_State = PWM_ERROR;
                break;
            }
			
            buf[0] = 0U;
            buf[1] = 0U;
            
            if(HAL_I2C_Master_Transmit(&hi2c4, PWM1_WRADD, buf, 2U, PWM_TOUT) != HAL_OK)
            {
                PWM_State = PWM_ERROR;
                break;
            }
            
            HAL_Delay(5);
            buf[0] = 1U;
            buf[1] = 4U;
            
            if(HAL_I2C_Master_Transmit(&hi2c4, PWM1_WRADD, buf, 2U, PWM_TOUT) != HAL_OK)
            {
                PWM_State = PWM_ERROR;
                break;
            }
            
            HAL_Delay(2);
			PCA9685_SetOutFreq(PWM1_FREQDEF);		
			PWM_State = PWM_STOP;
            PWM_OutDisable();
            PWMErrorReset();
			break;
		}		
	
		case PWM_STOP:
		{
            if (IsPWM_OutEnabled()) PWM_State = PWM_UPDATE;
            else
            {
                i = 0;
                s = 0;
                while (i < 16) s += pwm[i++];
                if (s) 
                {
                    ZEROFILL(pwm, PWM_BUFSIZE);
                    PCA9685_WriteOut();  
                }
            }
            PWMErrorReset();
			break;
		}
		
		case PWM_UPDATE:
		{
            if (!IsPWM_OutEnabled()) PWM_State = PWM_STOP;
            else
            {
                i = 0;
                s = 0;
                while (i < 16) s += pwm[i++];
                if (oldpwm != s)
                {
                    oldpwm = s;				
                    PCA9685_WriteOut();
                }
            }
            PWMErrorReset();
			break;
		}	
            
        case PWM_ERROR:
        default:
            PWMErrorSet();
            PWM_OutDisable();
            break;
	}	
}


void PCA9685_SetOutFreq(uint16_t val)
{
	uint8_t buf[2];
	
	buf[0] = 0x00U;
	buf[1] = 0x10U;
	
	if(HAL_I2C_Master_Transmit(&hi2c4, PWM1_WRADD, buf, 2U, PWM_TOUT) != HAL_OK)
	{
		SYSRestart();	
	}
	
	PWM_CalcPresc(val);	
	buf[0] = PCA9685_PRE_SCALE_REG_ADD;
	buf[1] = PCA9685_PRE_SCALE_REG;
	
	if(HAL_I2C_Master_Transmit(&hi2c4, PWM1_WRADD, buf, 2U, PWM_TOUT) != HAL_OK)
	{
		SYSRestart();	
	}
	
	buf[0] = 0x00U;
	buf[1] = 0xa0U;
	
	if(HAL_I2C_Master_Transmit(&hi2c4, PWM1_WRADD, buf, 2U, PWM_TOUT) != HAL_OK)
	{
		SYSRestart();	
	}
	
	HAL_Delay(5);
	buf[0] = 0x01U;
	buf[1] = 0x04U;
	
	if(HAL_I2C_Master_Transmit(&hi2c4, PWM1_WRADD, buf, 2U, PWM_TOUT) != HAL_OK)
	{
		SYSRestart();	
	}
}


void PCA9685_WriteInc(uint8_t add, uint8_t len)
{
	uint8_t i, buf[70];
	
	buf[0] = add;
	
	for(i = 1; i < (len+1); i++)
	{
		buf[i] = pca[add++];
	}
	
	if(HAL_I2C_Master_Transmit(&hi2c4, PWM1_WRADD, buf, len+1, PWM_TOUT) != HAL_OK)
	{
        SYSRestart();	
	}
}


void PCA9685_WriteOut(void)
{
	uint16_t pwm_out;
	ZEROFILL(&pca[6], 32);
	
		
    pwm_out = pwm[0] * 16U;
    if (pwm[0] == 0xFF) pwm_out = 0xFFF;
	pca[8] = (pwm_out & 0xFF);
	pca[9] = (pwm_out >> 8);
	
	pwm_out = pwm[1] * 16U;
    if (pwm[1] == 0xFF) pwm_out = 0xFFF;	
	pca[12] = (pwm_out & 0xffU);
	pca[13] = (pwm_out >> 8U);
	
	pwm_out = pwm[2] * 16U;
    if (pwm[2] == 0xFF) pwm_out = 0xFFF;	
	pca[16] = (pwm_out & 0xffU);
	pca[17] = (pwm_out >> 8U);
	
	pwm_out = pwm[3] * 16U;
    if (pwm[3] == 0xFF) pwm_out = 0xFFF;
	pca[20] = (pwm_out & 0xffU);
	pca[21] = (pwm_out >> 8U);
	
	pwm_out = pwm[4] * 16U;
    if (pwm[4] == 0xFF) pwm_out = 0xFFF;	
	pca[24] = (pwm_out & 0xffU);
	pca[25] = (pwm_out >> 8U);
	
	pwm_out = pwm[5] * 16U;
    if (pwm[5] == 0xFF) pwm_out = 0xFFF;	
	pca[28] = (pwm_out & 0xffU);
	pca[29] = (pwm_out >> 8U);
	
	pwm_out = pwm[6] * 16U;
    if (pwm[6] == 0xFF) pwm_out = 0xFFF;	
	pca[32] = (pwm_out & 0xffU);
	pca[33] = (pwm_out >> 8U);
	
	pwm_out = pwm[7] * 16U;
    if (pwm[7] == 0xFF) pwm_out = 0xFFF;	
	pca[36] = (pwm_out & 0xffU);
	pca[37] = (pwm_out >> 8U);
	
	pwm_out = pwm[8] * 16U;
    if (pwm[8] == 0xFF) pwm_out = 0xFFF;	
	pca[40] = (pwm_out & 0xffU);
	pca[41] = (pwm_out >> 8U);
	
	pwm_out = pwm[9] * 16U;
    if (pwm[9] == 0xFF) pwm_out = 0xFFF;	
	pca[44] = (pwm_out & 0xffU);
	pca[45] = (pwm_out >> 8U);
	
	pwm_out = pwm[10] * 16U;
    if (pwm[10] == 0xFF) pwm_out = 0xFFF;	
	pca[48] = (pwm_out & 0xffU);
	pca[49] = (pwm_out >> 8U);
	
	pwm_out = pwm[11] * 16U;
    if (pwm[11] == 0xFF) pwm_out = 0xFFF;	
	pca[52] = (pwm_out & 0xffU);
	pca[53] = (pwm_out >> 8U);
	
	pwm_out = pwm[12] * 16U;
    if (pwm[12] == 0xFF) pwm_out = 0xFFF;	
	pca[56] = (pwm_out & 0xffU);
	pca[57] = (pwm_out >> 8U);
	
	pwm_out = pwm[13] * 16U;
    if (pwm[13] == 0xFF) pwm_out = 0xFFF;	
	pca[60] = (pwm_out & 0xffU);
	pca[61] = (pwm_out >> 8U);
	
	pwm_out = pwm[14] * 16U;
    if (pwm[14] == 0xFF) pwm_out = 0xFFF;
	pca[64] = (pwm_out & 0xffU);
	pca[65] = (pwm_out >> 8U);
		
	pwm_out = pwm[15] * 16U;
    if (pwm[15] == 0xFF) pwm_out = 0xFFF;	
	pca[68] = (pwm_out & 0xffU);
	pca[69] = (pwm_out >> 8U);
	
 	PCA9685_WriteInc(6, 64); // WRITE FROM  LED0_ON_L TILL LED15_OFF_H
}
/******************************   END OF FILE  ********************************/ 
