#include "curtain.h"
#include "rs485.h"
#include "display.h"
#include "stm32746g_eeprom.h"

#if (__CURTAIN_CTRL_H__ != FW_BUILD)
    #error "curtain header version mismatch"
#endif





#define CURTAIN_SWITCH_DIRECTION_WAIT_TIME         500
#define CURTAIN_MOVE_TIME                          (20 * 1000)




uint8_t upDownDurationSeconds = 0;
uint8_t curtains_send = 0;
uint8_t curtains_count = 0;
Curtain curtains[CURTAINS_SIZE];





void Curtains_Count()
{
    curtains_count = 0;
    
    for(uint8_t i = 0; i < CURTAINS_SIZE; i++)
    {
        if(Curtain_hasRelays(curtains + i)) ++curtains_count;
        else break;
    }
}


uint8_t Curtains_getCount()
{
    return curtains_count;
}



void Curtain_Init(Curtain* const cur, const uint16_t addr)
{
    cur->upDownTimer = 0;
    cur->upDown = 0;
    cur->upDown_old = 0;
    cur->resend = 0;
    EE_ReadBuffer((uint8_t*)&cur->relayUp,        addr,       2);
    EE_ReadBuffer((uint8_t*)&cur->relayDown,      addr + 2,   2);
}

void Curtain_Save(Curtain* const cur, const uint16_t addr)
{
    EE_WriteBuffer((uint8_t*)&cur->relayUp,        addr,       2);
    EE_WriteBuffer((uint8_t*)&cur->relayDown,      addr + 2,   2);
}

void Curtains_Init()
{
    EE_ReadBuffer(&upDownDurationSeconds,    EE_CURTAINS_MOVE_TIME,   1);
    
    for(uint8_t i = 0; i < CURTAINS_SIZE; i++)
    {
        Curtain_Init(curtains + i, EE_CURTAINS + (i * 4));
    }
    
    Curtains_Count();
}

void Curtains_Save()
{
    EE_WriteBuffer(&upDownDurationSeconds,    EE_CURTAINS_MOVE_TIME,   1);
    
    for(uint8_t i = 0; i < CURTAINS_SIZE; i++)
    {
        Curtain_Save(curtains + i, EE_CURTAINS + (i * 4));
    }
    
    Curtains_Count();
}



bool Curtain_hasRelays(const Curtain* const cur)
{
    return cur->relayUp && cur->relayDown;
}



uint16_t Curtain_GetRelayUp(const Curtain* const cur)
{
    return cur->relayUp;
}

void Curtain_SetRelayUp(Curtain* const cur, const uint16_t val)
{
    cur->relayUp = val;
}




uint16_t Curtain_GetRelayDown(const Curtain* const cur)
{
    return cur->relayDown;
}

void Curtain_SetRelayDown(Curtain* const cur, const uint16_t val)
{
    cur->relayDown = val;
}



uint8_t Curtain_getDirection(const Curtain* const cur)
{
    return cur->upDown_old;
}

uint8_t Curtain_getNewDirection(const Curtain* const cur)
{
    return cur->upDown;
}




void Curtain_SetMoveTime(const uint8_t seconds)
{
    upDownDurationSeconds = seconds;
}

uint8_t Curtain_GetMoveTime()
{
    return upDownDurationSeconds;
}




bool Curtain_hasDirectionChanged(const Curtain* const cur)
{
    return cur->upDown != cur->upDown_old;
}

void Curtain_DirectionEqualize(Curtain* const cur)
{
    cur->upDown_old = cur->upDown;
}





bool Curtain_isMoving(const Curtain* const cur)
{
    return (Curtain_hasRelays(cur)) ? (cur->upDown_old != CURTAIN_STOP) : false;
}

bool Curtains_isAnyCurtainMoving()
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        if(Curtain_isMoving(curtains + i)) return true;
    }
    
    return false;
}

bool Curtains_areAllMoving()
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        if(Curtain_hasRelays(curtains + i) && (!Curtain_isMoving(curtains + i))) return false;
    }
    
    return true;
}

bool Curtains_areAllMovinginSameDirection(const uint8_t direction)
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        if(Curtain_hasRelays(curtains + i) && (curtains[i].upDown_old != direction)) return false;
    }
    
    return true;
}

bool Curtain_isMovingUp(const Curtain* const cur)
{
    return cur->upDown_old == CURTAIN_UP;
}

bool Curtains_isAnyCurtainMovingUp()
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        if(Curtain_isMovingUp(curtains + i)) return true;
    }
    
    return false;
}

bool Curtain_isMovingDown(const Curtain* const cur)
{
    return cur->upDown_old == CURTAIN_DOWN;
}

bool Curtains_isAnyCurtainMovingDown()
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        if(Curtain_isMovingDown(curtains + i)) return true;
    }
    
    return false;
}




bool Curtain_isNewDirectionStop(const Curtain* const cur)
{
    return cur->upDown == CURTAIN_STOP;
}

bool Curtains_isNewDirectionStop(const Curtain* const cur)
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        if(!Curtain_isNewDirectionStop(cur)) return false;
    }
    
    return true;
}

bool Curtain_isNewDirectionUp(const Curtain* const cur)
{
    return cur->upDown == CURTAIN_UP;
}

bool Curtains_isNewDirectionUp()
{
    uint8_t anyRelay = 0;
    
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        if(Curtain_hasRelays(curtains + i) && (!Curtain_isNewDirectionUp(curtains + i))) return false;
        if(Curtain_hasRelays(curtains + i)) anyRelay = 1;
    }
    
    return anyRelay ? true : false;
}

bool Curtain_isNewDirectionDown(const Curtain* const cur)
{
    return cur->upDown == CURTAIN_DOWN;
}

bool Curtains_isNewDirectionDown()
{
    uint8_t anyRelay = 0;
    
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        if(Curtain_hasRelays(curtains + i) && (!Curtain_isNewDirectionDown(curtains + i))) return false;
        if(Curtain_hasRelays(curtains + i)) anyRelay = 1;
    }
    
    return anyRelay ? true : false;
}






void Curtain_Stop(Curtain* const cur)
{
    cur->upDown = CURTAIN_STOP;
}

void Curtains_Stop()
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        Curtain_Stop(curtains + i);
    }
}


void Curtain_Move(Curtain* const cur, const uint8_t direction)
{
    if(Curtain_hasRelays(cur))
    {
        if(/*(!curtains_send) &&*/ Curtain_shouldResend(cur))
        {
            cur->upDown_old = CURTAIN_STOP;
        }
        cur->upDown = direction;
        cur->upDownTimer = HAL_GetTick();
        if(!cur->upDownTimer) cur->upDownTimer = 1;
    }
}

void Curtain_MoveSignal(Curtain* const cur, const uint8_t direction)
{
        if(Curtain_isMoving(cur) && (cur->upDown == direction) && (cur->upDown_old == direction)) Curtain_Stop(cur);
        else Curtain_Move(cur, direction);
}

void Curtains_Move(const uint8_t direction)
{
    curtains_send = 1;
    
    if(Curtains_isAnyCurtainMoving())
    {
        for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
        {
            if(Curtain_hasRelays(curtains + i))
                (curtains + i)->upDown_old = (direction == CURTAIN_UP) ? CURTAIN_DOWN : CURTAIN_UP;
        }
    }
    
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        Curtain_Move(curtains + i, direction);
    }
}

void Curtains_MoveSignal(const uint8_t direction)
{
    /*const uint8_t newdir = (((direction == CURTAIN_UP) && (Curtains_isAnyCurtainMovingUp())) || ((direction == CURTAIN_DOWN) && (Curtains_isAnyCurtainMovingDown()))) ? CURTAIN_STOP : direction;
    
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        if(Curtain_getDirection(curtains + i) == direction) Curtain_Move(curtains + i, newdir);
    }*/
    
    uint8_t stop = 1;
    
    if(direction != CURTAIN_STOP)
    {
        for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
        {
            if((Curtain_hasRelays(curtains + i)) && (!(Curtain_isMoving(curtains + i) && ((curtains + i)->upDown == direction) && ((curtains + i)->upDown_old == direction)))) stop = 0;
        }
    }
    
    if(stop) Curtains_Stop();
    else Curtains_Move(/*Curtains_areAllMovinginSameDirection(direction) ? CURTAIN_STOP : */direction);
}





bool Curtain_Modbus_isIndexInRange(const uint8_t index)
{
    return (index >= 0) && (index < CURTAINS_SIZE);
}

uint8_t Curtain_Modbus_Get_byIndex(const uint8_t index)
{
    if(Curtain_isNewDirectionStop(curtains + index)) return CURTAIN_STOP;
    else if(Curtain_isNewDirectionUp(curtains + index)) return CURTAIN_UP;
    else return CURTAIN_DOWN;
}


void Curtain_MoveSignal_byIndex(uint8_t const index, const uint8_t direction)
{
    Curtain_MoveSignal(curtains + index, direction);
}








void Curtain_RestartTimer(Curtain* const cur)
{
    cur->upDownTimer = HAL_GetTick();
    if(!cur->upDownTimer) cur->upDownTimer = 1;
}




bool Curtain_HasSwitchDirectionTimeExpired(const Curtain* const cur)
{
    return (HAL_GetTick() - cur->upDownTimer) >= CURTAIN_SWITCH_DIRECTION_WAIT_TIME;
}

bool Curtain_hasMoveTimeExpired(const Curtain* const cur)
{
    return (HAL_GetTick() - cur->upDownTimer) >= (Curtain_GetMoveTime() * 1000);
}



void Curtain_Resend(Curtain* const cur)
{
    cur->resend = 1;
}

void Curtain_ResendReset(Curtain* const cur)
{
    cur->resend = 0;
}

bool Curtain_shouldResend(const Curtain* const cur)
{
    return cur->resend;
}




void Curtain_Reset(Curtain* const cur)
{
    cur->upDown = CURTAIN_STOP;
    cur->upDown_old = CURTAIN_STOP;
    cur->upDownTimer = 0;
    Curtain_ResendReset(cur);
}







void Curtain_Service()
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; i++)
    {
        Curtain* const cur = curtains + i;
        
        if(!Curtain_hasRelays(cur)) continue;
        
        if(Curtain_hasMoveTimeExpired(cur))
        {
            Curtain_Stop(cur);
        }
        
        if(Curtain_hasDirectionChanged(cur))
        {
            uint16_t relay = 0;
            
            relay = (Curtain_isNewDirectionUp(cur) || (Curtain_isNewDirectionStop(cur) && Curtain_isMovingUp(cur))) ? Curtain_GetRelayUp(cur) : Curtain_GetRelayDown(cur);
            *(sendDataBuff + sendDataCount) = (relay >> 8) & 0xFF;
            *(sendDataBuff + sendDataCount + 1) = relay & 0xFF;
            sendDataCount += 2;
            sendDataBuff[sendDataCount++] = Curtain_isNewDirectionStop(cur) ? 0 : (Curtain_isNewDirectionUp(cur) ? 1 : 2);
            
            //DodajKomandu(, JALOUSIE_SET, sendDataBuff, sendDataCount);
            
            if(screen == SCREEN_CURTAINS) shouldDrawScreen = 1;
            
            if(Curtain_isNewDirectionStop(cur)) Curtain_Reset(cur);
            else Curtain_DirectionEqualize(cur);
            
            break; // blinds have to be sent one by one
        }
    }
}







