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




typedef struct
{
    SPINBOX_Handle cycleTime, activeTime, pin;
}
Defroster_settingsWidgets;






extern Defroster defroster;






void Defroster_Init(void);
void Defroster_Save(void);
bool Defroster_isActive(void);
void Defroster_On(void);
void Defroster_Off(void);
void Defroster_Service(void);
void Defroster_SetCycleTime(uint8_t time);
void Defroster_SetActiveTime(uint8_t time);
void Defroster_SetDefault(void);




#endif
