#include "lights.h"
#include "rs485.h"
#include "display.h"
#include "stm32746g_sdram.h"
#include "stm32746g_eeprom.h"






uint8_t isButtonActive_old = 0;
uint8_t lights_count = 0, lights_modbus_rows = 0;
uint8_t LightNightTimer_isEnabled = 0;
uint32_t LightNightTimer_StartTime = 0;
LIGHT_Modbus_CmdTypeDef lights_modbus[LIGHTS_MODBUS_SIZE];
GUI_CONST_STORAGE GUI_BITMAP* light_modbus_images[] = {&bmSijalicaOff, &bmSijalicaOn, &bmVENTILATOR_OFF, &bmVENTILATOR_ON};









void Lights_Modbus_Count(void)
{
    lights_count = 0;
    
    for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; ++i)
    {
        if(Light_Modbus_GetRelay(lights_modbus + i)) ++lights_count;
        else break;
    }
}

uint8_t Lights_Modbus_getCount(void)
{
    return lights_count;
}




void Lights_Modbus_Rows_Count(void)
{
    lights_modbus_rows = (Lights_Modbus_getCount() / 4) + 1;
}

uint8_t Lights_Modbus_Rows_getCount(void)
{
    return lights_modbus_rows;
}



void Lights_Modbus_Calculate(void)
{
    Lights_Modbus_Count();
    Lights_Modbus_Rows_Count();
}


void Light_Modbus_Init(LIGHT_Modbus_CmdTypeDef* const li, const uint16_t addr)
{
    li->value = 0;
    li->old_value = 0;
    li->color = 0;
    
    EE_ReadBuffer((uint8_t*)&li->index,                addr,           2);
    EE_ReadBuffer(&li->tiedToMainLight,                addr + 2,       1);
    EE_ReadBuffer(&li->off_time,                       addr + 3,       1);
    EE_ReadBuffer(&li->iconID,                         addr + 4,       1);
    EE_ReadBuffer((uint8_t*)(&li->controllerID_on),    addr + 5,       2);
    EE_ReadBuffer(&li->controllerID_on_delay,          addr + 7,       1);
    EE_ReadBuffer(&li->on_hour,                        addr + 8,       1);
    EE_ReadBuffer(&li->on_minute,                      addr + 9,       1);
    EE_ReadBuffer(&li->communication_type,             addr + 10,       1);
    EE_ReadBuffer(&li->local_pin,                      addr + 11,       1);
    EE_ReadBuffer(&li->sleep_time,                     addr + 12,       1);
    EE_ReadBuffer(&li->button_external,                addr + 13,       1);
    EE_ReadBuffer(&li->rememberBrightness,             addr + 14,       1);
    EE_ReadBuffer(&li->brightness,                     addr + 15,       1);
    
    li->brightness_old = li->brightness;
}


void Light_Modbus_Save(LIGHT_Modbus_CmdTypeDef* const li, const uint16_t addr)
{
    EE_WriteBuffer((uint8_t*)&li->index,                addr,           2);
    EE_WriteBuffer(&li->tiedToMainLight,                addr + 2,       1);
    EE_WriteBuffer(&li->off_time,                       addr + 3,       1);
    EE_WriteBuffer(&li->iconID,                         addr + 4,       1);
    EE_WriteBuffer((uint8_t*)(&li->controllerID_on),    addr + 5,       2);
    EE_WriteBuffer(&li->controllerID_on_delay,          addr + 7,       1);
    EE_WriteBuffer(&li->on_hour,                        addr + 8,       1);
    EE_WriteBuffer(&li->on_minute,                      addr + 9,       1);
    EE_WriteBuffer(&li->communication_type,             addr + 10,       1);
    EE_WriteBuffer(&li->local_pin,                      addr + 11,       1);
    EE_WriteBuffer(&li->sleep_time,                     addr + 12,       1);
    EE_WriteBuffer(&li->button_external,                addr + 13,       1);
    EE_WriteBuffer(&li->rememberBrightness,             addr + 14,       1);
    EE_WriteBuffer(&li->brightness,                     addr + 15,       1);
}

void Lights_Modbus_Init(void)
{
    for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; i++)
    {
        Light_Modbus_Init(lights_modbus + i, EE_LIGHTS_MODBUS + (i * 16));
    }
    
    EE_ReadBuffer(&LightNightTimer_isEnabled, EE_LIGHT_NIGHT_TIMER, 1);
    
    Lights_Modbus_Calculate();
}

void Lights_Modbus_Save(void)
{
    for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; i++)
    {
        Light_Modbus_Save(lights_modbus + i, EE_LIGHTS_MODBUS + (i * 16));
    }
    
    EE_WriteBuffer(&LightNightTimer_isEnabled, EE_LIGHT_NIGHT_TIMER, 1);
    
    Lights_Modbus_Calculate();
}



bool Light_Modbus_isIndexInRange(const uint8_t light_index)
{
    return (light_index >= 0) && (light_index < LIGHTS_MODBUS_SIZE);
}


uint8_t Light_Modbus_Set_byIndex(const uint8_t light_index, const uint8_t val)
{
    if(Light_Modbus_isIndexInRange(light_index))
    {
        if(val) Light_Modbus_On(lights_modbus + light_index);
        else Light_Modbus_Off(lights_modbus + light_index);
    }
    
    return Light_Modbus_isNewValueOn(lights_modbus + light_index);
}

uint8_t Light_Modbus_Get_byIndex(const uint8_t light_index)
{
    if(Light_Modbus_isIndexInRange(light_index))
    {
        return Light_Modbus_isNewValueOn(lights_modbus + light_index);
    }
    
    return LIGHT_MODBUS_QUERY_RESPONSE_INDEX_OUT_OF_RANGE;
}



void Light_Modbus_StatusSet(LIGHT_Modbus_CmdTypeDef* const li, const uint8_t value)
{
    if(value)
    {
        li->value = 1;
        
        if((!Light_Modbus_isBinary(li) && (!Light_Modbus_isBrightnessRemembered(li))))
        {
            Light_Modbus_SetBrightness(li, 100);
        }
        
        if(li->local_pin < 5) SetPin(li->local_pin, 1);
        else PCA9685_SetOutput(li->local_pin, 255);
        
        if(Light_Modbus_isOffTimeEnabled(li))
        {
            uint32_t time = HAL_GetTick();
            if(!time) time = 1;
            Light_Modbus_SetOffTimeTimer(li, time);
        }
    }
    else
    {
        li->value = 0;
        
        if(li->local_pin < 5) SetPin(li->local_pin, 0);
        else PCA9685_SetOutput(li->local_pin, 0);
        
        Light_Modbus_OffTimeTimerDeactivate(li);
    }
}


void Light_Modbus_On(LIGHT_Modbus_CmdTypeDef* const li)
{
    Light_Modbus_StatusSet(li, 1);
}

void Light_Modbus_On_External(LIGHT_Modbus_CmdTypeDef* const li)
{
    if(li->controllerID_on_delay)
    {
        li->on_delay_timer_start = HAL_GetTick();
        if(!li->on_delay_timer_start) --li->on_delay_timer_start;
    }
    else
    {
        Light_Modbus_On(li);
    }
}

void Light_Modbus_Off(LIGHT_Modbus_CmdTypeDef* const li)
{
    Light_Modbus_StatusSet(li, 0);
}

void Light_Modbus_Off_External(LIGHT_Modbus_CmdTypeDef* const li)
{
    if(li->controllerID_on_delay)
    {
        li->on_delay_timer_start = 0;
    }
    else
    {
        Light_Modbus_Off(li);
    }
}



void Light_Modbus_Flip(LIGHT_Modbus_CmdTypeDef* const li)
{
    if(Light_Modbus_isActive(li)) Light_Modbus_Off(li);
    else Light_Modbus_On(li);
}

void Light_Modbus_Update_External(LIGHT_Modbus_CmdTypeDef* const li, const uint8_t val)
{
    li->old_value = val;
    li->value = val;
}

bool Light_Modbus_isActive(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->old_value;
}

bool Light_Modbus_isNewValueOn(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->value;
}

bool Light_Modbus_isOldValueOn(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->old_value;
}





bool Light_Modbus_hasStatusChanged(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return Light_Modbus_isOldValueOn(li) != Light_Modbus_isNewValueOn(li);
}

void Light_Modbus_ResetStatus(LIGHT_Modbus_CmdTypeDef* const li)
{
    li->old_value = li->value;
}







uint16_t Light_Modbus_GetRelay(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->index;
}

void Light_Modbus_SetRelay(LIGHT_Modbus_CmdTypeDef* const li, const uint16_t val)
{
    li->index = val;
}



void Light_Modbus_TieToMainLight(LIGHT_Modbus_CmdTypeDef* const li)
{
    li->tiedToMainLight = 1;
}

void Light_Modbus_UntieFromMainLight(LIGHT_Modbus_CmdTypeDef* const li)
{
    li->tiedToMainLight = 0;
}

bool Light_Modbus_isTiedToMainLight(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->tiedToMainLight;
}





uint8_t Light_Modbus_GetOnDelayTime(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->controllerID_on_delay;
}

void Light_Modbus_SetOnDelayTime(LIGHT_Modbus_CmdTypeDef* const li, const uint8_t val)
{
    li->controllerID_on_delay = val;
}

bool Light_Modbus_isOnDelayTimeEnabled(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return Light_Modbus_GetOnDelayTime(li);
}



uint32_t Light_Modbus_GetOnDelayTimeTimer(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->on_delay_timer_start;
}

void Light_Modbus_SetOnDelayTimeTimer(LIGHT_Modbus_CmdTypeDef* const li, const uint32_t val)
{
    li->on_delay_timer_start = val;
}

bool Light_Modbus_isOnDelayTimeTimerActive(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return Light_Modbus_GetOnDelayTimeTimer(li);
}

bool Light_Modbus_hasOnDelayTimeTimerExpired(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return (HAL_GetTick() - li->on_delay_timer_start) >= (Light_Modbus_GetOnDelayTime(li) * 60 * 1000);
}

void Light_Modbus_OnDelayTimeTimerDeactivate(LIGHT_Modbus_CmdTypeDef* const li)
{
    li->on_delay_timer_start = 0;
}





uint8_t Light_Modbus_GetOffTime(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->off_time;
}

void Light_Modbus_SetOffTime(LIGHT_Modbus_CmdTypeDef* const li, const uint8_t val)
{
    li->off_time = val;
}

bool Light_Modbus_isOffTimeEnabled(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return Light_Modbus_GetOffTime(li);
}



uint32_t Light_Modbus_GetOffTimeTimer(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->off_timer_start;
}

void Light_Modbus_SetOffTimeTimer(LIGHT_Modbus_CmdTypeDef* const li, const uint32_t val)
{
    li->off_timer_start = val;
}

bool Light_Modbus_isOffTimeTimerActive(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return Light_Modbus_GetOffTimeTimer(li);
}

bool Light_Modbus_hasOffTimeTimerExpired(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return (HAL_GetTick() - li->off_timer_start) >= (Light_Modbus_GetOffTime(li) * 60 * 1000);
}

void Light_Modbus_OffTimeTimerDeactivate(LIGHT_Modbus_CmdTypeDef* const li)
{
    li->off_timer_start = 0;
}




bool Light_Modbus_isTimeOnEnabled(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return (li->on_hour < 24) && (li->on_minute < 60);
}

bool Light_Modbus_isTimeToTurnOn(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return (li->on_hour == Bcd2Dec(rtctm.Hours)) && (li->on_minute == Bcd2Dec(rtctm.Minutes));
}





void Light_Modbus_SetColor(LIGHT_Modbus_CmdTypeDef* const li, GUI_COLOR color)
{
    li->color = color;
}

GUI_COLOR Light_Modbus_GetColor(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->color;
}

bool Light_Modbus_hasColorChanged(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return Light_Modbus_GetColor(li);
}

void Light_Modbus_ResetColor(LIGHT_Modbus_CmdTypeDef* const li)
{
    li->color = 0;
}





void Light_Modbus_SetBrightness(LIGHT_Modbus_CmdTypeDef* const li, uint8_t brightness)
{
    if(brightness > 100) li->brightness = 100;
    else if(brightness < 0) li->brightness = 0;
    else li->brightness = brightness;
    
    if(Light_Modbus_isBrightnessRemembered(li))
    {
        Light_Modbus_Save(li, EE_LIGHTS_MODBUS + ((li - lights_modbus) * 16));
    }
}

uint8_t Light_Modbus_GetBrightness(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->brightness;
}

bool Light_Modbus_hasBrightnessChanged(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return Light_Modbus_GetBrightness(li) != li->brightness_old;
}




void Light_Modbus_RememberBrightnessSet(LIGHT_Modbus_CmdTypeDef* const li, const bool remember)
{
    li->rememberBrightness = remember;
}


bool Light_Modbus_isBrightnessRemembered(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->rememberBrightness;
}



void Light_Modbus_ResetBrightness(LIGHT_Modbus_CmdTypeDef* const li)
{
    li->brightness_old = li->brightness;
}



void Light_Modbus_Brightness_Update_External(LIGHT_Modbus_CmdTypeDef* const li, const uint8_t value)
{
    if(value > 100)
    {
        li->brightness = 100;
    }
    if(value < 0)
    {
        li->brightness = 0;
    }
    else
    {
        li->brightness = value;
    }
    
    Light_Modbus_ResetBrightness(li);
    
    
    
    if(Light_Modbus_isBrightnessRemembered(li))
    {
        Light_Modbus_Save(li, EE_LIGHTS_MODBUS + ((li - lights_modbus) * 16));
    }
}




bool Light_Modbus_isBinary(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->communication_type == LIGHT_COM_BIN;
}

bool Light_Modbus_isDimmer(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->communication_type == LIGHT_COM_DIM;
}

bool Light_Modbus_isRGB(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->communication_type == LIGHT_COM_COLOR;
}





bool Light_Modbus_hasChanged(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return Light_Modbus_hasStatusChanged(li) || Light_Modbus_hasBrightnessChanged(li) || Light_Modbus_hasColorChanged(li);
}

void Light_Modbus_ResetChange(LIGHT_Modbus_CmdTypeDef* const li)
{
    Light_Modbus_ResetStatus(li);
    Light_Modbus_ResetBrightness(li);
    Light_Modbus_ResetColor(li);
}





GUI_CONST_STORAGE GUI_BITMAP* Light_Modbus_GetIcon(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return light_modbus_images[(li->iconID * 2) + Light_Modbus_isNewValueOn(li)];
}


uint8_t Light_Modbus_GetIconID(const LIGHT_Modbus_CmdTypeDef* const li)
{
    return li->iconID;
}


void Light_Modbus_SetIcon(LIGHT_Modbus_CmdTypeDef* const li, const uint8_t id)
{
    if(id < 0)
    {
        li->iconID = LIGHT_ICON_ID_BULB;
    }
    else if(id >= LIGHT_ICON_COUNT)
    {
        li->iconID = LIGHT_ICON_ID_VENTILATOR;
    }
    else
    {
        li->iconID = id;
    }
}






void Lights_Modbus_StatusSet(const bool state)
{
    for(uint8_t i = 0; i < Lights_Modbus_getCount(); i++)
    {
        Light_Modbus_StatusSet(lights_modbus + i, state);
    }
}


void Lights_Modbus_On(void)
{
    Lights_Modbus_StatusSet(1);
}


void Lights_Modbus_Off(void)
{
    Lights_Modbus_StatusSet(0);
}






void Light_Modbus_SetDefault(LIGHT_Modbus_CmdTypeDef* const li)
{
    li->index = 0;
    li->old_index = 0;
    li->value = 0;
    li->old_value = 0;
    li->brightness = 0;
    li->brightness_old = 0;
    li->color = 0;
    li->iconID = 0;
    li->local_pin = 0;
    li->communication_type = LIGHT_COM_BIN;
    li->button_external = 0;
    li->controllerID_on = 0;
    li->controllerID_on_delay = 0;
    li->off_time = 0;
    li->off_timer_start = 0;
    li->on_delay_timer_start = 0;
    li->on_hour = 0;
    li->on_minute = 0;
    li->sleep_time = 0;
    li->tiedToMainLight = false;
}


void Lights_Modbus_SetDefault(void)
{
    for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; i++)
    {
        Light_Modbus_SetDefault(lights_modbus + i);
    }
}









void Light_Modbus_Service(void)
{
    if((isButtonActive_old != IsButtonActive()) && (!isButtonActive_old))
    {
        for(uint8_t i = 0; i < Lights_Modbus_getCount(); i++)
        {
            LIGHT_Modbus_CmdTypeDef* const light = lights_modbus + i;
            
            if(!light->button_external)
            {
                if(light->button_external == 1)
                {
                    Light_Modbus_On(light);
                }
                else if(light->button_external == 2)
                {
                    Light_Modbus_Off(light);
                }
                else if(light->button_external == 3)
                {
                    Light_Modbus_Flip(light);
                }
            }
        }
        
        isButtonActive_old = IsButtonActive();
    }
    
    
    
    
    
    for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; ++i)
    {
        if(Light_Modbus_isOnDelayTimeTimerActive(lights_modbus + i) && Light_Modbus_hasOnDelayTimeTimerExpired(lights_modbus + i))
        {
            Light_Modbus_OnDelayTimeTimerDeactivate(lights_modbus + i);
            Light_Modbus_On(lights_modbus + i);
        }
        
        if(screen == SCREEN_LIGHTS) shouldDrawScreen = 1;
    }
    
    
    
    
    
    for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; ++i)
    {
        if(Light_Modbus_isOffTimeTimerActive(lights_modbus + i) && Light_Modbus_hasOffTimeTimerExpired(lights_modbus + i))
        {
            Light_Modbus_OffTimeTimerDeactivate(lights_modbus + i);
            Light_Modbus_Off(lights_modbus + i);
        }
        
        if(screen == SCREEN_LIGHTS) shouldDrawScreen = 1;
    }
    
    
    
    
    if(LightNightTimer_StartTime && ((HAL_GetTick() - LightNightTimer_StartTime) >= (LIGHT_NIGHT_TIMER_DURATION * 1000)))
    {
        LightNightTimer_StartTime = 0;
        
        for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; ++i)
        {
            if(Light_Modbus_isTiedToMainLight(lights_modbus + i) && Light_Modbus_isActive(lights_modbus + i))
            {
                Light_Modbus_Off(lights_modbus + i);
            }
        }
        
        if(screen == SCREEN_RESET_MENU_SWITCHES) screen = 1;
        shouldDrawScreen = 1;
    }
    
    
    
    
    for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; i++)
    {
        if(Light_Modbus_hasStatusChanged(lights_modbus + i))
        {
            if(Light_Modbus_isBinary(lights_modbus + i) || Light_Modbus_isRGB(lights_modbus + i))
            {
                uint8_t sendDataBuffBin[3] = {0}, sendDataCountBin = 0;
                
                *(sendDataBuffBin + sendDataCountBin) = (Light_Modbus_GetRelay(lights_modbus + i) >> 8) & 0xFF;
                *(sendDataBuffBin + sendDataCountBin + 1) = Light_Modbus_GetRelay(lights_modbus + i) & 0xFF;
                sendDataCountBin += 2;
                sendDataBuffBin[sendDataCountBin++] = Light_Modbus_isNewValueOn(lights_modbus + i) ? 0x01 : 0x02;
                
                //DodajKomandu(, BINARY_SET, sendDataBuff, sendDataCount);
            }
            else
            {
                uint8_t sendDataBuffDimm[3] = {0}, sendDataCountDimm = 0;
                
                *(sendDataBuffDimm + sendDataCountDimm) = (Light_Modbus_GetRelay(lights_modbus + i) >> 8) & 0xFF;
                *(sendDataBuffDimm + sendDataCountDimm + 1) = Light_Modbus_GetRelay(lights_modbus + i) & 0xFF;
                sendDataCountDimm += 2;
                sendDataBuffDimm[sendDataCountDimm++] = Light_Modbus_isNewValueOn(lights_modbus + i) ? Light_Modbus_GetBrightness(lights_modbus + i) : 0;
                
                //DodajKomandu(, DIMMER_SET, sendDataBuff, sendDataCount);
            }
            
            Light_Modbus_ResetStatus(lights_modbus + i);
            
            if(screen == SCREEN_LIGHTS) shouldDrawScreen = 1;
            else if(!screen) screen = SCREEN_MAIN;
        }
        else if(Light_Modbus_hasBrightnessChanged(lights_modbus + i))
        {
            uint8_t sendDataBuffDimm[3] = {0}, sendDataCountDimm = 0;
            
            *(sendDataBuffDimm + sendDataCountDimm) = (Light_Modbus_GetRelay(lights_modbus + i) >> 8) & 0xFF;
            *(sendDataBuffDimm + sendDataCountDimm + 1) = Light_Modbus_GetRelay(lights_modbus + i) & 0xFF;
            sendDataCountDimm += 2;
            sendDataBuffDimm[sendDataCountDimm++] = Light_Modbus_GetBrightness(lights_modbus + i);
            
            //DodajKomandu(, DIMMER_SET, sendDataBuff, sendDataCount);
            
            Light_Modbus_ResetBrightness(lights_modbus + i);
        }
        else if(Light_Modbus_hasColorChanged(lights_modbus + i))
        {
            uint8_t sendDataBuffRGB[5] = {0}, sendDataCountRGB = 0;
            
            //if(isSendDataBufferEmpty()) sendDataBuff[sendDataCount++] = LIGHT_SEND_COLOR_SET;
            *(sendDataBuffRGB + sendDataCountRGB) = (Light_Modbus_GetRelay(lights_modbus + i) >> 8) & 0xFF;
            *(sendDataBuffRGB + sendDataCountRGB + 1) = Light_Modbus_GetRelay(lights_modbus + i) & 0xFF;
            sendDataCountRGB += 2;
            sendDataBuffRGB[sendDataCountRGB++] = Light_Modbus_GetColor(lights_modbus + i) & 0xFF;             // blue
            sendDataBuffRGB[sendDataCountRGB++] = (Light_Modbus_GetColor(lights_modbus + i) >> 8) & 0xFF;      // green
            sendDataBuffRGB[sendDataCountRGB++] = (Light_Modbus_GetColor(lights_modbus + i) >> 16) & 0xFF;     // red
            
            //DodajKomandu(, RGB_SET, sendDataBuff, sendDataCount);
            
            Light_Modbus_ResetColor(lights_modbus + i);
        }
    }
}





