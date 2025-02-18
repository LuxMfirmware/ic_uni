#ifndef __VENTILATOR_CTRL_H__
#define __VENTILATOR_CTRL_H__                             FW_BUILD // version

#include "main.h"
#include "stm32f7xx.h"




#define VENTILATOR_ACTIVATE()       (ventilator.flags |= 1U)
#define VENTILATOR_DEACTIVATE()     (ventilator.flags &= (~1U))
#define VENTILATOR_IS_ACTIVE()      (ventilator.flags & 1U)
#define VENTILATOR_HAS_CHANGED()    ((ventilator.flags & (1U << 1U)) != ((ventilator.flags & 1U) << 1U))
#define VENTILATOR_CHANGE_RESET()   (VENTILATOR_IS_ACTIVE() ? (ventilator.flags |= (1U << 1U)) : (ventilator.flags &= (~(1U << 1U))))




typedef struct
{
    uint32_t delayOnTimerStart, DelayOffTimerStart;
    uint16_t relay;
    uint8_t delayOnTime, delayOffTime, delayOnUse, delayOffUse, flags;
    
} Ventilator;



extern Ventilator ventilator;



void Ventilator_setRelay(Ventilator* const vent, const uint8_t val);
uint8_t Ventilator_getRelay(Ventilator* const vent);

void Ventilator_setDelayOnTime(Ventilator* const vent, const uint8_t val);
uint8_t Ventilator_getDelayOnTime(Ventilator* const vent);

void Ventilator_setDelayOffTime(Ventilator* const vent, const uint8_t val);
uint8_t Ventilator_getDelayOffTime(Ventilator* const vent);

void Ventilator_setDelayOnUse(Ventilator* const vent, const uint8_t val);
uint8_t Ventilator_getDelayOnUse(Ventilator* const vent);

void Ventilator_setDelayOffUse(Ventilator* const vent, const uint8_t val);
uint8_t Ventilator_getDelayOffUse(Ventilator* const vent);

void Ventilator_On(const bool useDelay);
void Ventilator_Off(void);

void Ventilator_Save(Ventilator* const vent, const uint16_t addr);
void Ventilator_Init(Ventilator* const vent, const uint16_t addr);

void Ventilator_Service(void);



#endif
