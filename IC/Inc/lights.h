#ifndef __LIGHTS_H__
#define __LIGHTS_H__                             FW_BUILD // version


#include "main.h"



#define LIGHT_COM_BIN                        1
#define LIGHT_COM_DIM                        2
#define LIGHT_COM_COLOR                      3

#define LIGHTS_MODBUS_SIZE                   6





typedef struct
{
    GUI_COLOR color;
    uint32_t off_timer_start, on_delay_timer_start;
    uint16_t index, old_index, controllerID_on;
    uint8_t value, old_value, iconID, tiedToMainLight, off_time, controllerID_on_delay, on_hour, on_minute, communication_type, local_pin, sleep_time, button_external, brightness;
} LIGHT_Modbus_CmdTypeDef;




extern uint8_t lights_count, lights_modbus_rows;
extern LIGHT_Modbus_CmdTypeDef lights_modbus[LIGHTS_MODBUS_SIZE];






uint8_t Lights_Modbus_getCount();
void Light_Modbus_On(LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_Off(LIGHT_Modbus_CmdTypeDef* const li);
uint8_t Light_Modbus_Set_byIndex(const uint8_t light_index, const uint8_t val);
uint8_t Light_Modbus_Get_byIndex(const uint8_t light_index);
uint16_t Light_Modbus_GetRelay(const LIGHT_Modbus_CmdTypeDef* const li);
bool Light_Modbus_isIndexInRange(const uint8_t light_index);
void Light_Modbus_Update_External(LIGHT_Modbus_CmdTypeDef* const li, const uint8_t val);
bool Light_Modbus_isNewValueOn(const LIGHT_Modbus_CmdTypeDef* const li);
bool Light_Modbus_isOldValueOn(const LIGHT_Modbus_CmdTypeDef* const li);
bool Light_Modbus_hasStatusChanged(const LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_ResetStatus(LIGHT_Modbus_CmdTypeDef* const li);
bool Light_Modbus_hasChanged(const LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_ResetChange(LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_On_External(LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_Off_External(LIGHT_Modbus_CmdTypeDef* const li);
uint8_t Light_Modbus_GetBrightness(const LIGHT_Modbus_CmdTypeDef* const li);
bool Light_Modbus_hasBrightnessChanged(const LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_ResetBrightness(LIGHT_Modbus_CmdTypeDef* const li);
GUI_COLOR Light_Modbus_GetColor(const LIGHT_Modbus_CmdTypeDef* const li);
bool Light_Modbus_hasColorChanged(const LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_ResetColor(LIGHT_Modbus_CmdTypeDef* const li);
bool Light_Modbus_isBinary(const LIGHT_Modbus_CmdTypeDef* const li);
bool Light_Modbus_isDimmer(const LIGHT_Modbus_CmdTypeDef* const li);
bool Light_Modbus_isRGB(const LIGHT_Modbus_CmdTypeDef* const li);
uint8_t Light_Modbus_GetOnDelayTime(const LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_SetOnDelayTime(LIGHT_Modbus_CmdTypeDef* const li, const uint8_t val);
bool Light_Modbus_isOnDelayTimeEnabled(const LIGHT_Modbus_CmdTypeDef* const li);
uint32_t Light_Modbus_GetOnDelayTimeTimer(const LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_SetOnDelayTimeTimer(LIGHT_Modbus_CmdTypeDef* const li, const uint32_t val);
bool Light_Modbus_isOnDelayTimeTimerActive(const LIGHT_Modbus_CmdTypeDef* const li);
bool Light_Modbus_hasOnDelayTimeTimerExpired(const LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_OnDelayTimeTimerDeactivate(LIGHT_Modbus_CmdTypeDef* const li);
uint8_t Light_Modbus_GetOffTime(const LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_SetOffTime(LIGHT_Modbus_CmdTypeDef* const li, const uint8_t val);
bool Light_Modbus_isOffTimeEnabled(const LIGHT_Modbus_CmdTypeDef* const li);
uint32_t Light_Modbus_GetOffTimeTimer(const LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_SetOffTimeTimer(LIGHT_Modbus_CmdTypeDef* const li, const uint32_t val);
bool Light_Modbus_isOffTimeTimerActive(const LIGHT_Modbus_CmdTypeDef* const li);
bool Light_Modbus_hasOffTimeTimerExpired(const LIGHT_Modbus_CmdTypeDef* const li);
void Light_Modbus_OffTimeTimerDeactivate(LIGHT_Modbus_CmdTypeDef* const li);
bool Light_Modbus_isTimeOnEnabled(const LIGHT_Modbus_CmdTypeDef* const li);
bool Light_Modbus_isTimeToTurnOn(const LIGHT_Modbus_CmdTypeDef* const li);



#endif