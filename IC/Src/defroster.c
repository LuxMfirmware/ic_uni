#include "defroster.h"
#include "display.h"





Defroster defroster;





void Defroster_Init()
{
    defroster.cycleTime_TimerStart = 0;
    defroster.activeTime_TimerStart = 0;
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

bool Defroster_hasActiveTimeTimerExpired()
{
    return (defroster.activeTime_TimerStart - HAL_GetTick()) >= (defroster.activeTime * 1000);
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

bool Defroster_hasCycleTimerExpired()
{
    return (defroster.cycleTime_TimerStart - HAL_GetTick()) >= (defroster.cycleTime * 1000);
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







void Defroster_Service()
{
    if(Defroster_isActive())
    {
        if(Defroster_isCycleTimerOn() && Defroster_hasCycleTimerExpired())
        {
            Defroster_CycleTimerStart();
            Defroster_ActiveTimeTimerStart();
            SetPin(defroster.pin, 1);
        }
        else if(Defroster_isActiveTimeTimerOn() && Defroster_hasActiveTimeTimerExpired())
        {
            Defroster_ActiveTimeTimerStop();
            SetPin(defroster.pin, 0);
        }
    }
}







