#ifndef __DEFROSTER_CTRL_H__
#define __DEFROSTER_CTRL_H__                             FW_BUILD // version


#include "main.h"
#include "stm32f7xx.h"


typedef struct
{
    uint8_t cycleTime, activeTime, pin;
    uint32_t cycleTime_TimerStart, activeTime_TimerStart;
}
Defroster;





extern Defroster defroster;



void Defroster_Init();
bool Defroster_isActive();
void Defroster_On();
void Defroster_Off();
void Defroster_Service();




#endif