#ifndef __CURTAIN_CTRL_H__
#define __CURTAIN_CTRL_H__                             FW_BUILD // version

#include "main.h"
#include "stm32f7xx.h"



#define CURTAINS_SIZE    15

#define CURTAIN_STOP                               0
#define CURTAIN_UP                                 1
#define CURTAIN_DOWN                               2



typedef struct
{
    uint32_t upDownTimer;
    uint16_t relayUp, relayDown;
    uint8_t upDown, upDown_old, resend;
}
Curtain;



extern uint8_t upDownDurationSeconds;
extern uint8_t curtains_send;
extern uint8_t curtainSendDataBuff[4 * CURTAINS_SIZE], curtainSendDataBuffCount;
extern Curtain curtains[CURTAINS_SIZE];



uint8_t Curtains_getCount();
void Curtains_Init();
void Curtains_Save();
bool Curtain_hasRelays(const Curtain* const cur);
uint16_t Curtain_GetRelayUp(const Curtain* const cur);
void Curtain_SetRelayUp(Curtain* const cur, const uint16_t val);
uint16_t Curtain_GetRelayDown(const Curtain* const cur);
void Curtain_SetRelayDown(Curtain* const cur, const uint16_t val);
void Curtain_SetMoveTime(const uint8_t seconds);
uint8_t Curtain_GetMoveTime();
bool Curtain_hasDirectionChanged(const Curtain* const cur);
void Curtain_DirectionEqualize(Curtain* const cur);
bool Curtain_isMoving(const Curtain* const cur);
bool Curtains_isAnyCurtainMoving();
bool Curtain_isMovingUp(const Curtain* const cur);
bool Curtains_isAnyCurtainMovingUp();
bool Curtain_isMovingDown(const Curtain* const cur);
bool Curtains_isAnyCurtainMovingDown();
bool Curtain_isNewDirectionStop(const Curtain* const cur);
bool Curtain_isNewDirectionUp(const Curtain* const cur);
bool Curtains_isNewDirectionUp();
bool Curtain_isNewDirectionDown(const Curtain* const cur);
bool Curtains_isNewDirectionDown();
void Curtain_MoveSignal(Curtain* const cur, const uint8_t direction);
void Curtains_MoveSignal(const uint8_t direction);
bool Curtain_Modbus_isIndexInRange(const uint8_t index);
uint8_t Curtain_Modbus_Get_byIndex(const uint8_t index);
void Curtain_MoveSignal_byIndex(uint8_t const index, const uint8_t direction);
void Curtain_Stop(Curtain* const cur);
void Curtain_RestartTimer(Curtain* const cur);
bool Curtain_HasSwitchDirectionTimeExpired(const Curtain* const cur);
bool Curtain_hasMoveTimeExpired(const Curtain* const cur);
void Curtain_Resend(Curtain* const cur);
void Curtain_ResendReset(Curtain* const cur);
bool Curtain_shouldResend(const Curtain* const cur);
void Curtain_Reset();
void Curtain_Service();




#endif