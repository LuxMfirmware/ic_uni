#include "defroster.h"
#include "display.h"
#include "stm32746g_eeprom.h"







Defroster defroster;








void Defroster_Init()
{
    defroster.cycleTime_TimerStart = 0;
    defroster.activeTime_TimerStart = 0;
    
    EE_ReadBuffer(&defroster.cycleTime,       EE_DEFROSTER,            1);
    EE_ReadBuffer(&defroster.activeTime,      EE_DEFROSTER + 1,        1);
    EE_ReadBuffer(&defroster.pin,             EE_DEFROSTER + 2,        1);
}





void Defroster_Save()
{
    EE_WriteBuffer(&defroster.cycleTime,       EE_DEFROSTER,            1);
    EE_WriteBuffer(&defroster.activeTime,      EE_DEFROSTER + 1,        1);
    EE_WriteBuffer(&defroster.pin,             EE_DEFROSTER + 2,        1);
}










void Defroster_SetCycleTime(uint8_t time)
{
    defroster.cycleTime = time;
    
    if(defroster.activeTime > time)
    {
        defroster.activeTime = time;
    }
}











void Defroster_SetActiveTime(uint8_t time)
{
    if(time > defroster.cycleTime)
    {
        defroster.activeTime = defroster.cycleTime;
    }
    else
    {
        defroster.activeTime = time;
    }
}









void Defroster_ActiveTimeTimerStart()
{
    uint32_t tick = HAL_GetTick();
    defroster.activeTime_TimerStart = tick ? (tick - 1) : 1;
}

bool Defroster_isActiveTimeTimerOn()
{
    return defroster.activeTime_TimerStart;
}

bool Defroster_hasActiveTimeTimerExpired(const uint32_t tick)
{
    return (tick - defroster.activeTime_TimerStart) >= (defroster.activeTime * 1000 * 60);
}

void Defroster_ActiveTimeTimerStop()
{
    defroster.activeTime_TimerStart = 0;
}







void Defroster_CycleTimerStart()
{
    uint32_t tick = HAL_GetTick();
    defroster.cycleTime_TimerStart = tick ? (tick - 1) : 1;
}

bool Defroster_isCycleTimerOn()
{
    return defroster.cycleTime_TimerStart;
}

bool Defroster_hasCycleTimerExpired(const uint32_t tick)
{
    return (tick - defroster.cycleTime_TimerStart) >= (defroster.cycleTime * 1000 * 60);
}

void Defroster_CycleTimerStop()
{
    defroster.cycleTime_TimerStart = 0;
}






bool Defroster_isActive()
{
    return Defroster_isCycleTimerOn();
}

void Defroster_On()
{
    Defroster_CycleTimerStart();
    Defroster_ActiveTimeTimerStart();
    SetPin(defroster.pin, 1);
}

void Defroster_Off()
{
    Defroster_CycleTimerStop();
    Defroster_ActiveTimeTimerStop();
    SetPin(defroster.pin, 0);
}









void Defroster_SetDefault()
{
    defroster.cycleTime = 0;
    defroster.cycleTime_TimerStart = 0;
    defroster.activeTime = 0;
    defroster.activeTime_TimerStart = 0;
    defroster.pin = 0;
}









void Defroster_Service()
{
    if(Defroster_isActive())
    {
        const uint32_t tick = HAL_GetTick();
        
        if(Defroster_isCycleTimerOn() && Defroster_hasCycleTimerExpired(tick))
        {
            Defroster_CycleTimerStart();
            Defroster_ActiveTimeTimerStart();
            SetPin(defroster.pin, 1);
        }
        else if(Defroster_isActiveTimeTimerOn() && Defroster_hasActiveTimeTimerExpired(tick))
        {
            Defroster_ActiveTimeTimerStop();
            SetPin(defroster.pin, 0);
        }
    }
}







