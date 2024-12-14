/**
 *******************************************************************************
 * File Name          : one_wire.h
 * Date               : 17/11/2016 00:59:00
 * Description        : one wire communication modul header
 *******************************************************************************
 */                                                                             
 
#ifndef __OW_H__
#define __OW_H__                                FW_BUILD // version
/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx.h"
/* Exporeted Types   ---------------------------------------------------------*/
/* Exporeted Define   --------------------------------------------------------*/
/* Log event defines    ------------------------------------------------------*/
/* Exporeted Variable   ------------------------------------------------------*/
extern uint32_t owflg;
extern uint8_t owbps;
extern uint8_t owifa;
extern uint8_t owgra;
extern uint8_t owbra;
extern uint8_t owdev;
extern uint8_t owrx[32];
extern uint8_t owtx[32];
/* Exporeted  Macro  ---------------------------------------------------------*/
#define OWFileUpdSet()                  (owflg |=  (1U<<0))
#define OWFileUpdReset()                (owflg &=(~(1U<<0)))
#define IsOWFileUpdActiv()              (owflg &   (1U<<0))
#define OWStatUpdSet()                  (owflg |=  (1U<<1)) 
#define OWStatUpdReset()                (owflg &=(~(1U<<1)))
#define IsOWStatUpdActiv()              (owflg &   (1U<<1))
#define OWCardStackerSet()              (owflg |=  (1U<<2))
#define OWCardStackerReset()            (owflg &=(~(1U<<2)))
#define IsOWCardStackerActiv()          (owflg &   (1U<<2))
#define OWCardStackerEnable()           (owflg |=  (1U<<3))
#define OWCardStackerDisable()          (owflg &=(~(1U<<3)))
#define IsOWCardStackerEnabled()        (owflg &   (1U<<3))
/* Exported Function   -------------------------------------------------------*/
void OW_Init(void);
void OW_Service(void);
void OW_ErrorCallback (void);
void OW_RxCpltCallback(void);
void OW_TxCpltCallback(void);
void OW_Send(uint8_t add);
#endif
/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/

