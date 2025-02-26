#include "ventilator.h"
#include "stm32746g_eeprom.h"

#if (__VENTILATOR_CTRL_H__ != FW_BUILD)
#error "ventilator header version mismatch"
#endif





Ventilator ventilator;



void Ventilator_setRelay(Ventilator* const vent, const uint8_t val)
{
    vent->relay = val;
}

uint8_t Ventilator_getRelay(Ventilator* const vent)
{
    return vent->relay;
}



void Ventilator_setDelayOnTime(Ventilator* const vent, uint8_t val)
{
    vent->delayOnTime = val;
}

uint8_t Ventilator_getDelayOnTime(Ventilator* const vent)
{
    return vent->delayOnTime;
}



void Ventilator_setDelayOffTime(Ventilator* const vent, uint8_t val)
{
    vent->delayOffTime = val;
}

uint8_t Ventilator_getDelayOffTime(Ventilator* const vent)
{
    return vent->delayOffTime;
}



void Ventilator_setDelayOnUse(Ventilator* const vent, const uint8_t val)
{
    vent->delayOnUse = val;
}

uint8_t Ventilator_getDelayOnUse(Ventilator* const vent)
{
    return vent->delayOnUse;
}



void Ventilator_setDelayOffUse(Ventilator* const vent, const uint8_t val)
{
    vent->delayOffUse = val;
}

uint8_t Ventilator_getDelayOffUse(Ventilator* const vent)
{
    return vent->delayOffUse;
}



void Ventilator_Save(Ventilator* const vent, const uint16_t addr)
{
    EE_WriteBuffer((uint8_t*)(&(vent->relay)),    addr,       2);
    EE_WriteBuffer(&(vent->delayOnTime),          addr + 2,   1);
    EE_WriteBuffer(&(vent->delayOffTime),         addr + 3,   1);
    EE_WriteBuffer(&(vent->delayOnUse),           addr + 4,   1);
    EE_WriteBuffer(&(vent->delayOffUse),          addr + 5,   1);
}

void Ventilator_Init(Ventilator* const vent, const uint16_t addr)
{
    vent->flags = 0;
    vent->delayOnTimerStart = 0;
    vent->DelayOffTimerStart = 0;
    EE_ReadBuffer((uint8_t*)(&(vent->relay)),      addr,       2);
    EE_ReadBuffer(&(vent->delayOnTime),            addr + 3,   1);
    EE_ReadBuffer(&(vent->delayOffTime),           addr + 4,   1);
    EE_ReadBuffer(&(vent->delayOnUse),             addr + 5,   1);
    EE_ReadBuffer(&(vent->delayOffUse),            addr + 6,   1);
}





void Ventilator_On(const bool useDelay)
{
    if(!ventilator.relay) return;

    if(useDelay && ventilator.delayOnUse)
    {
        ventilator.delayOnTimerStart = HAL_GetTick();
        if(!ventilator.delayOnTimerStart) ventilator.delayOnTimerStart = 1;
    }
    else
    {
        VENTILATOR_ACTIVATE();
    }
}

void Ventilator_Off()
{
    if(!ventilator.relay) return;

    if(ventilator.delayOnTimerStart)
    {
        ventilator.delayOnTimerStart = 0;
    }
    else if(ventilator.delayOffUse)
    {
        ventilator.DelayOffTimerStart = HAL_GetTick();
        if(!ventilator.DelayOffTimerStart) ventilator.DelayOffTimerStart = 1;
    }
    else
    {
        VENTILATOR_DEACTIVATE();
    }
}




void Ventilator_Service()
{
    if(ventilator.DelayOffTimerStart && ((HAL_GetTick() - ventilator.DelayOffTimerStart) >= (ventilator.delayOffTime * 10 * 1000)))
    {
        VENTILATOR_DEACTIVATE();
    }
    else if(ventilator.delayOnTimerStart && ((HAL_GetTick() - ventilator.delayOnTimerStart) >= (ventilator.delayOnTime * 10 * 1000)))
    {
        VENTILATOR_ACTIVATE();
    }
}
