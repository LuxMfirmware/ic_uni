/**
 ******************************************************************************
 * File Name          : rs485.h
 * Date               : 28/02/2016 23:16:19
 * Description        : rs485 communication modul header
 ******************************************************************************
 *
 ******************************************************************************
 */
 
#ifndef __RS485_H__
#define __RS485_H__                         FW_BUILD // version
/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx.h"
#include "TinyFrame.h"
/* Exported Type  ------------------------------------------------------------*/
typedef enum{
    DALI_DAPC  = 0,
    DALI_UP,    
    DALI_DOWN,
    DALI_STEP_UP,
    DALI_STEP_DOWN,
    DALI_RECALL_MAX_LEVEL,
    DALI_RECALL_MIN_LEVEL,
    DALI_STEP_DOWN_AND_OFF,
    DALI_ON_AND_STEP_UP,
    DALI_RESET = 32,
    DALI_SET_SCENE_DTR0,
    DALI_STORE_ACTUAL_LEVEL_IN_DTR0,
    DALI_SET_MAX_LEVEL_DTR0 = 42,
    DALI_SET_MIN_LEVEL_DTR0,
    DALI_SET_SYSTEM_FAILURE_LEVEL_DTR0,
    DALI_SET_POWER_ON_LEVEL_DTR0,
    DALI_SET_FADE_TIME_DTR0,
    DALI_SET_FADE_RATE_DTR0, 
    DALI_ADD_TO_SCENE_DTR0,
    DALI_DELETE_FROM_SCENE,
    DALI_GO_TO_LAST_ACTIVE_LEVEL,
    DALI_GO_TO_SCENE,
    DALI_ADD_TO_GROUP,
    DALI_DELETE_FROM_GROUP,
    DALI_SET_SHORT_ADDR_DTR0,
    DALI_QUERY_STATUS,
    DALI_QUERY_LAMP_FAILURE,
    DALI_QUERY_ACTUAL_LEVEL,
    DALI_SET_EXTENDED_FADE_TIME_DTR0,
    DALI_IDENTIFY_DEVICE,
    DALI_OFF,
}DALI_CmdTypeDef;
/* Exported variables  -------------------------------------------------------*/
extern uint8_t  rec;
extern uint32_t tfbps;
extern uint8_t  tfifa;
extern uint8_t  tfgra;
extern uint8_t  tfbra;
extern uint8_t  tfgwa;
extern uint16_t sysid;
extern uint32_t rsflg;
/* Exported Define  ----------------------------------------------------------*/
/* Exported macros     -------------------------------------------------------*/
#define StartFwUpdate()             (rsflg |=  ((uint32_t)0x00000001U))
#define StopFwUpdate()              (rsflg &= ~((uint32_t)0x00000001U))
#define IsFwUpdateActiv()           (rsflg &   ((uint32_t)0x00000001U))
/* Exported functions ------------------------------------------------------- */
void RS485_Init(void);
void RS485_Tick(void);
void RS485_Service(void);
void RS485_RxCpltCallback(void);
void RS485_TxCpltCallback(void);
void RS485_ErrorCallback(void);
#endif
/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
