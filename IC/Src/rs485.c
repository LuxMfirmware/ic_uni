/**
 ******************************************************************************
 * Project          : KuceDzevadova
 ******************************************************************************
 *
 *
 ******************************************************************************
 */

 
#if (__RS485_H__ != FW_BUILD)
    #error "rs485 header version mismatch"
#endif 
/* Includes ------------------------------------------------------------------*/
#include "png.h"
#include "main.h"
#include "rs485.h"
#include "logger.h"
#include "display.h"
#include "thermostat.h"
#include "ventilator.h"
#include "curtain.h"
#include "stm32746g.h"
#include "stm32746g_ts.h"
#include "stm32746g_qspi.h"
#include "stm32746g_sdram.h"
#include "stm32746g_eeprom.h"
/* Imported Types  -----------------------------------------------------------*/
/* Imported Variables --------------------------------------------------------*/
/* Imported Functions    -----------------------------------------------------*/
/* Private Typedef -----------------------------------------------------------*/
static TinyFrame tfapp;
/* Private Define  -----------------------------------------------------------*/
/* Private Variables  --------------------------------------------------------*/
TF_Msg sendData;
bool init_tf = false;
static uint8_t ud1, ud2;
static uint32_t rstmr = 0;
static uint32_t wradd = 0;
static uint32_t bcnt = 0;
uint16_t sysid;
uint32_t rsflg, tfbps, dlen, etmr;
uint8_t lbuf[32], dbuf[32], tbuf[32], sendDataBuff[50] = {0}, sendDataCount = 0, lcnt = 0, dcnt = 0, tcnt = 0, cmd = 0;
uint8_t responseData[6] = {0}, responseDataLength = 0;
uint8_t  ethst, efan,  etsp,  rec, tfifa, tfgra, tfbra, tfgwa;
uint8_t *lctrl1 =(uint8_t*) &LIGHT_Ctrl1.Main1;
uint8_t *lctrl2 =(uint8_t*) &LIGHT_Ctrl2.Main1;
/* Private macros   ----------------------------------------------------------*/
/* Private Function Prototypes -----------------------------------------------*/
bool isSendDataBufferEmpty();
/* Program Code  -------------------------------------------------------------*/




void AttachData(TF_Msg* mess)
{
    mess->data = responseData;
    mess->len = (TF_LEN) responseDataLength;
}

void SetRoomTemp(TF_Msg* message)
{
    if(message->data[2] > thst.sp_max) thst.sp_temp = thst.sp_max;
    else if(message->data[2] < thst.sp_min) thst.sp_temp = thst.sp_min;
    else thst.sp_temp = message->data[2];
    
    menu_thst = 0;
}

void SetThstHeating()
{
    TempRegHeating();
    menu_thst = 0;
    SaveThermostatController(&thst, EE_THST1);
}

void SetThstCooling()
{
    TempRegCooling();
    menu_thst = 0;
    SaveThermostatController(&thst, EE_THST1);
}

void SetThstOn()
{
    TempRegHeating();
    menu_thst = 0;
    SaveThermostatController(&thst, EE_THST1);
}

void SetThstOff()
{
    TempRegOff();
    menu_thst = 0;
    SaveThermostatController(&thst, EE_THST1);
}

void SendFanDiff(TF_Msg* message)
{
    responseData[responseDataLength++] = GET_FAN_DIFFERENCE;
    responseData[responseDataLength++] = thst.fan_diff;
                    
    AttachData(message);
}

void SendFanBand(TF_Msg* message)
{
    responseData[responseDataLength++] = GET_FAN_BAND;
    responseData[responseDataLength++] = thst.fan_loband;
    responseData[responseDataLength++] = thst.fan_hiband;
    
    message->data=responseData;
    message->len=(TF_LEN) responseDataLength;
}

void SendRoomTemp(TinyFrame* tinyframe, TF_Msg* message)
{
    ud1 = thst.mv_temp >> 8;
    ud2 = thst.mv_temp & 0xFF;
    tinyframe->userdata = &ud1;
    tfapp.data[0] = '1';
    tfapp.data[1] = '2';
    tfapp.data[2] = '3';
    
    responseData[responseDataLength++] = GET_ROOM_TEMP;
    responseData[responseDataLength++] =  (thst.mv_temp / 10) + thst.mv_offset;
    responseData[responseDataLength++] = thst.sp_temp;
    
    message->data=responseData;
    message->len=(TF_LEN) responseDataLength;
}

void Send_SP_Max(TF_Msg* message)
{
    responseData[responseDataLength++] = GET_SP_MAX;
    responseData[responseDataLength++] = thst.sp_max;
    
    AttachData(message);
}

void Set_SP_Max(TF_Msg* message)
{
    thst.sp_max = message->data[2];
    SaveThermostatController(&thst, EE_THST1);
}

void Send_SP_Min(TF_Msg* message)
{
    responseData[responseDataLength++] = GET_SP_MIN;
    responseData[responseDataLength++] = thst.sp_min;
    
    AttachData(message);
}

void Set_SP_Min(TF_Msg* message)
{
    thst.sp_min = message->data[2];
    SaveThermostatController(&thst, EE_THST1);
}



void Send_Light_Modbus(TF_Msg* message)
{
    responseData[responseDataLength++] = LIGHT_MODBUS_GET;
    
    if(Light_Modbus_isIndexInRange(message->data[2] - 1))
    {
        responseData[responseDataLength++] = Light_Modbus_Get_byIndex(message->data[2] - 1);
    }
    else
    {
        responseData[responseDataLength++] = LIGHT_MODBUS_QUERY_RESPONSE_INDEX_OUT_OF_RANGE;
    }
    
    AttachData(message);
}

void Set_Light_Modbus(TF_Msg* message)
{
    if(Light_Modbus_isIndexInRange(message->data[2] - 1))
    {
        Light_Modbus_Set_byIndex(message->data[2] - 1, message->data[3]);
    }
    else
    {
        responseData[responseDataLength++] = LIGHT_MODBUS_SET;
        responseData[responseDataLength++] = LIGHT_MODBUS_QUERY_RESPONSE_INDEX_OUT_OF_RANGE;
        AttachData(message);
    }
}



void Send_Curtain_Modbus(TF_Msg* message)
{
    responseData[responseDataLength++] = CURTAIN_MODBUS_GET;
    
    if(Curtain_Modbus_isIndexInRange(message->data[2] - 1))
    {
        responseData[responseDataLength++] = Curtain_Modbus_Get_byIndex(message->data[2] - 1);
    }
    else
    {
        responseData[responseDataLength++] = CURTAIN_MODBUS_QUERY_RESPONSE_INDEX_OUT_OF_RANGE;
    }
    
    AttachData(message);
}

void Set_Curtain_Modbus(TF_Msg* message)
{
    if(!message->data[2])
    {
        Curtains_MoveSignal(message->data[3]);
    }
    else if(Curtain_Modbus_isIndexInRange(message->data[2] - 1))
    {
        if(message->data[2]) Curtain_MoveSignal_byIndex(message->data[2] - 1, message->data[3]);
    }
    else
    {
        responseData[responseDataLength++] = CURTAIN_MODBUS_SET;
        responseData[responseDataLength++] = CURTAIN_MODBUS_QUERY_RESPONSE_INDEX_OUT_OF_RANGE;
        AttachData(message);
    }
}




void Set_QR_Code_RS485(TF_Msg* message)
{
    if(QR_Code_isDataLengthShortEnough(message->len - 3))
    {
        QR_Code_Set(message->data[2], message->data + 3);
        if(screen == 19) shouldDrawScreen = true;
    }
    else
    {
        responseData[responseDataLength++] = QR_CODE_SET;
        responseData[responseDataLength++] = QR_CODE_QUERY_RESPONSE_DATA_TOO_LONG;
        AttachData(message);
    }
}





/**
  * @brief
  * @param
  * @retval
  */
TF_Result ID_Listener(TinyFrame *tf, TF_Msg *msg){
    return TF_CLOSE;
}
TF_Result FWREQ_Listener(TinyFrame *tf, TF_Msg *msg){          
    if (IsFwUpdateActiv()){
        MX_QSPI_Init();
        if (QSPI_Write ((uint8_t*)msg->data, wradd, msg->len) == QSPI_OK){
            wradd += msg->len; 
        }else{
            wradd = 0;
            bcnt = 0;
        }            
        MX_QSPI_Init();
        QSPI_MemMapMode();        
    }
    TF_Respond(tf, msg);
    rstmr = HAL_GetTick();
    return TF_STAY;
}
TF_Result GEN_Listener(TinyFrame *tf, TF_Msg *msg){
    if (!IsFwUpdateActiv()){
        if ((msg->data[9] == ST_FIRMWARE_REQUEST)&& (msg->data[8] == tfifa)){
            wradd = ((msg->data[0]<<24)|(msg->data[1]<<16)|(msg->data[2] <<8)|msg->data[3]);
            bcnt  = ((msg->data[4]<<24)|(msg->data[5]<<16)|(msg->data[6] <<8)|msg->data[7]);
            MX_QSPI_Init();
            if (QSPI_Erase(wradd, wradd + bcnt) == QSPI_OK){
                StartFwUpdate();
                TF_AddTypeListener(&tfapp, ST_FIRMWARE_REQUEST, FWREQ_Listener);
                TF_Respond(tf, msg);
                rstmr = HAL_GetTick();
            }else{
                wradd = 0;
                bcnt = 0;
            }
            MX_QSPI_Init();
            QSPI_MemMapMode();
        }
        else if((msg->data[1] == tfifa)
            &&  ((msg->data[0] == RESTART_CTRL)
            ||  (msg->data[0] == LOAD_DEFAULT)
            ||  (msg->data[0] == FORMAT_EXTFLASH)
            ||  (msg->data[0] == GET_ROOM_TEMP)
            ||  (msg->data[0] == SET_ROOM_TEMP)
            ||  (msg->data[0] == SET_THST_HEATING)
            ||  (msg->data[0] == SET_THST_COOLING)
            ||  (msg->data[0] == SET_THST_ON)
            ||  (msg->data[0] == SET_THST_OFF)
            ||  (msg->data[0] == GET_APPL_STAT)
            ||  (msg->data[0] == GET_FAN_DIFFERENCE)
            ||  (msg->data[0] == GET_FAN_BAND)
            ||  (msg->data[0] == GET_SP_MAX)
            ||  (msg->data[0] == SET_SP_MAX)
            ||  (msg->data[0] == GET_SP_MIN)
            ||  (msg->data[0] == SET_SP_MIN)
            ||  (msg->data[0] == LIGHT_MODBUS_SET)
            ||  (msg->data[0] == LIGHT_MODBUS_GET)
            ||  (msg->data[0] == CURTAIN_MODBUS_SET)
            ||  (msg->data[0] == CURTAIN_MODBUS_GET)
            ||  (msg->data[0] == QR_CODE_SET)
            ||  (msg->data[0] == GET_APPL_STAT)))
            {
                //TF_Respond(tf, msg);
                cmd = msg->data[0];
                if (cmd == SET_ROOM_TEMP){
                    SetRoomTemp(msg);
                    
//                    if(msg->data[3] == 10)
//                    {
//                        GUI_DispDec(msg->data[3], 3); 
//                        
//                        responseData[0] = 1;
//                        responseData[1] = 10;
//                        responseData[2] = 5; 
//                        responseData[3] = tfifa; 
//                        
//                        TF_QuerySimple(&tfapp, S_TEMP, responseData, 4, ID_Listener, TF_PARSER_TIMEOUT_TICKS*4);
//                    }
                }
                else if (cmd == SET_THST_HEATING){
                    SetThstHeating();
                }
                else if (cmd == SET_THST_COOLING)
                {
                    SetThstCooling();
                }
                else if (cmd == SET_THST_ON){
                    SetThstOn();
                }
                else if (cmd == SET_THST_OFF){
                    SetThstOff();
                }
                else if (cmd == GET_FAN_DIFFERENCE){
                    SendFanDiff(msg);
                }
                else if (cmd == GET_FAN_BAND){
                    SendFanBand(msg);
                }
                else if (cmd == GET_ROOM_TEMP){
                    SendRoomTemp(tf, msg);
                }
                else if(cmd == GET_SP_MAX)
                {
                    Send_SP_Max(msg);
                }
                else if(cmd == SET_SP_MAX)
                {
                    Set_SP_Max(msg);
                }
                else if(cmd == GET_SP_MIN)
                {
                    Send_SP_Min(msg);
                }
                else if(cmd == SET_SP_MIN)
                {
                    Set_SP_Min(msg);
                }
                else if(cmd == LIGHT_MODBUS_GET)
                {
                    Send_Light_Modbus(msg);
                }
                else if(cmd == LIGHT_MODBUS_SET)
                {
                    Set_Light_Modbus(msg);
                }
                else if(cmd == CURTAIN_MODBUS_GET)
                {
                    Send_Curtain_Modbus(msg);
                }
                else if(cmd == CURTAIN_MODBUS_SET)
                {
                    Set_Curtain_Modbus(msg);
                }
                else if(cmd == QR_CODE_SET)
                {
                    Set_QR_Code_RS485(msg);
                }
                
                TF_Respond(tf, msg);
                
                ZEROFILL(responseData, sizeof(responseData));
                responseDataLength = 0;
        }
        else if(msg->data[0] == MODBUS_SEND_WRITE_SINGLE_REGISTER)
        {
            /*if(isSendDataBufferEmpty()) sendDataBuff[sendDataCount++] = MODBUS_SEND_WRITE_SINGLE_REGISTER;
            *(sendDataBuff + sendDataCount) = (Light_Modbus_GetRelay(lights_modbus + i) >> 8) & 0xFF;
            *(sendDataBuff + sendDataCount + 1) = Light_Modbus_GetRelay(lights_modbus + i) & 0xFF;
            sendDataCount += 2;
            sendDataBuff[sendDataCount++] = Light_Modbus_isNewValueOn(lights_modbus + i) ? 0x01 : 0x02;*/
            
            
            
            for(uint8_t i = 0; i < Lights_Modbus_getCount(); i++)
            {
                if(Light_Modbus_GetRelay(lights_modbus + i) == (((((uint16_t)msg->data[1]) << 8) & 0xFF00) | msg->data[2]))
                {
                    if((!msg->data[3]) && Light_Modbus_isNewValueOn(lights_modbus + i))
                    {
                        Light_Modbus_Off_External(lights_modbus + i);
                    }
                    else if(msg->data[3] && (!Light_Modbus_isNewValueOn(lights_modbus + i)))
                    {
                        Light_Modbus_On_External(lights_modbus + i);
                    }
                }
            }
            
            if(screen == 15) shouldDrawScreen = 1;
        }
        else if(msg->data[1] == tfbra)
        {
            if  (msg->data[0] == SET_RTC_DATE_TIME)
            {
                rtcdt.WeekDay = msg->data[2];
                rtcdt.Date    = msg->data[3];
                rtcdt.Month   = msg->data[4];
                rtcdt.Year    = msg->data[5];
                rtctm.Hours   = msg->data[6];
                rtctm.Minutes = msg->data[7];
                rtctm.Seconds = msg->data[8];
                HAL_RTC_SetTime(&hrtc, &rtctm, RTC_FORMAT_BCD);
                HAL_RTC_SetDate(&hrtc, &rtcdt, RTC_FORMAT_BCD);
                RtcTimeValidSet();
            }
            else if(msg->data[0] == MODBUS_SEND_READ_REGISTERS)
            {
                for(uint8_t i = 2;    i < msg->len;    i += 3)
                {
                    const uint16_t relay = (*(uint16_t*)(msg->data + i));
                    const uint8_t val = ((*(msg->data + i + 2)) == 0x01) ? 1 : 0 ;
                    
                    for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; ++i)
                    {
                        if(relay == Light_Modbus_GetRelay(lights_modbus + i))
                        {
                            Light_Modbus_Update_External(lights_modbus + i, val);
                            
                            if(screen == 15) shouldDrawScreen = 1;
                            else if(!screen) screen = 1;
                            
                            continue;
                        }
                    }
                    
                    /*if((!val) && ((relay == thst.relay1) || (relay == thst.relay2) || (relay == thst.relay3) || (relay == thst.relay4)) && IsTempRegActiv())
                    {
                        TempRegOff();
                        
                        continue;
                    }*/
                    
                    /*for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
                    {
                        if(relay == Curtain_GetRelayUp(curtains + i))
                        {
                            if((val != Curtain_isMoving(curtains + i)) && (!Curtain_hasDirectionChanged(curtains + i)))
                            {
                                Curtain_Move(curtains + i, CURTAIN_UP);
                                Curtain_DirectionEqualize(curtains + i);
                                
                                if(screen == 16) shouldDrawScreen = 1;
                            }
                            
                            continue;
                        }
                        else if((relay == Curtain_GetRelayDown(curtains + i)) && (!Curtain_hasDirectionChanged(curtains + i)))
                        {
                            if(val != Curtain_isMoving(curtains + i))
                            {
                                Curtain_Move(curtains + i, CURTAIN_DOWN);
                                Curtain_DirectionEqualize(curtains + i);
                                
                                if(screen == 16) shouldDrawScreen = 1;
                            }
                            
                            continue;
                        }
                    }*/
                    
                    
//                    updateModbusRegisterEnd:;
                }
            }
        }
    }
    return TF_STAY;
}
/**
* @brief :  init usart interface to rs485 9 bit receiving 
* @param :  and init state to receive packet control block 
* @retval:  wait to receive:
*           packet start address marker SOH or STX  2 byte  (1 x 9 bit)
*           packet receiver address 4 bytes msb + lsb       (2 x 9 bit)
*           packet sender address msb + lsb 4 bytes         (2 x 9 bit)
*           packet lenght msb + lsb 4 bytes                 (2 x 9 bit)
*/
void RS485_Init(void)
{
    sendData.data = NULL;
    sendData.len = 0;
    
    if(!init_tf){
        init_tf = TF_InitStatic(&tfapp, TF_SLAVE); // 1 = master, 0 = slave
        TF_AddGenericListener(&tfapp, GEN_Listener);
    }
	HAL_UART_Receive_IT(&huart1, &rec, 1);
}
/**
* @brief  : rs485 service function is  called from every
* @param  : main loop cycle to service rs485 communication
* @retval : receive and send on regulary base 
*/
void RS485_Service(void){
    uint8_t i; 
    if (IsFwUpdateActiv()){
        if(HAL_GetTick() > rstmr + 5000){
            TF_RemoveTypeListener(&tfapp, ST_FIRMWARE_REQUEST);
            StopFwUpdate();
            wradd = 0;
            bcnt = 0;
        }
    } else if ((HAL_GetTick() - etmr) >= TF_PARSER_TIMEOUT_TICKS){
        if (cmd){
            switch (cmd){
                case LOAD_DEFAULT:
                    i = 1;
                    EE_WriteBuffer(&i, EE_INIT_ADDR, 1);
                case RESTART_CTRL:
                    SYSRestart();
                    break;
                case FORMAT_EXTFLASH:
                    MX_QSPI_Init();
                    QSPI_Erase(0x90000000, 0x90FFFFFF);
                    MX_QSPI_Init();
                    QSPI_MemMapMode();
                    break;
                case GET_APPL_STAT:
                    PresentSystem();
                    HAL_UART_Receive_IT(&huart1, &rec, 1);
                    break;
                case GET_ROOM_TEMP:                   
                    tbuf[0] = thst.mv_temp >> 8;
                    tbuf[1] = thst.mv_temp & 0xFF;
                    tcnt = 2;
                    break;
                case SET_ROOM_TEMP:
                    break;
            }
            cmd = 0;
        }
        else if((!isSendDataBufferEmpty()) || sendData.data)
        {
            if(sendData.data)
            {
                TF_Query(&tfapp, &sendData, ID_Listener, TF_PARSER_TIMEOUT_TICKS);
            }
            else
            {
                TF_QuerySimple(&tfapp, S_CUSTOM, sendDataBuff, sendDataCount, ID_Listener, TF_PARSER_TIMEOUT_TICKS);
            }
            sendData.data = NULL;
            sendData.len = 0;
            sendDataCount = 0;
            ZEROFILL(sendDataBuff, COUNTOF(sendDataBuff));
        }
        else if (tcnt) {
            TF_QuerySimple(&tfapp, S_TEMP, tbuf, tcnt, ID_Listener, TF_PARSER_TIMEOUT_TICKS);
            etmr = HAL_GetTick();
            tcnt = 0;
        }
        else if (lcnt) {
            TF_QuerySimple(&tfapp, S_BINARY, lbuf, lcnt, ID_Listener, TF_PARSER_TIMEOUT_TICKS);
            etmr = HAL_GetTick();
            lcnt = 0;
        } else if (dcnt){
            TF_QuerySimple(&tfapp, S_DIMMER, dbuf, dcnt, ID_Listener, TF_PARSER_TIMEOUT_TICKS);
            etmr = HAL_GetTick();
            dcnt = 0;
        }
        /*else if(LightInitRequestShouldSend())
        {
            LightInitDisable();
            
            sendDataBuff[0] = ;
            
            TF_QuerySimple(&tfapp, S_CUSTOM, sendDataBuff, 4, ID_Listener, TF_PARSER_TIMEOUT_TICKS);
            ZEROFILL(sendDataBuff, COUNTOF(sendDataBuff));
        }*/
        else if(thst.sendChangeSignalFlags)
        {
            sendDataBuff[sendDataCount++] = MODBUS_SEND_WRITE_SINGLE_REGISTER;
            
            if(THST_ShouldSendRelay1())
            {
                THST_ResetRelay1Flag();
                *(sendDataBuff + sendDataCount) = (thst.relay1 >> 8) & 0xFF;
                *(sendDataBuff + sendDataCount + 1) = thst.relay1 & 0xFF;
                sendDataCount += 2;
                /*modbusSendData[modbusSendDataCount++] = (thst.relay1 >> 8) & 0xFF;
                modbusSendData[modbusSendDataCount++] = thst.relay1 & 0xFF;*/
                sendDataBuff[sendDataCount++] = thst.fan_speed ? 0x01 : 0x02;
            }
            
            if(THST_ShouldSendRelay2())
            {
                THST_ResetRelay2Flag();
                *(sendDataBuff + sendDataCount) = (thst.relay2 >> 8) & 0xFF;
                *(sendDataBuff + sendDataCount + 1) = thst.relay2 & 0xFF;
                sendDataCount += 2;
                /*modbusSendData[modbusSendDataCount++] = (thst.relay2 >> 8) & 0xFF;
                modbusSendData[modbusSendDataCount++] = thst.relay2 & 0xFF;*/
                sendDataBuff[sendDataCount++] = thst.fan_speed ? 0x01 : 0x02;
            }
            
            if(THST_ShouldSendRelay3())
            {
                THST_ResetRelay3Flag();
                *(sendDataBuff + sendDataCount) = (thst.relay3 >> 8) & 0xFF;
                *(sendDataBuff + sendDataCount + 1) = thst.relay3 & 0xFF;
                sendDataCount += 2;
                /*modbusSendData[modbusSendDataCount++] = (thst.relay3 >> 8) & 0xFF;
                modbusSendData[modbusSendDataCount++] = thst.relay3 & 0xFF;*/
                sendDataBuff[sendDataCount++] = thst.fan_speed ? 0x01 : 0x02;
            }
            
            if(THST_ShouldSendRelay4())
            {
                THST_ResetRelay4Flag();
                *(sendDataBuff + sendDataCount) = (thst.relay4 >> 8) & 0xFF;
                *(sendDataBuff + sendDataCount + 1) = thst.relay4 & 0xFF;
                sendDataCount += 2;
                /*modbusSendData[modbusSendDataCount++] = (thst.relay4 >> 8) & 0xFF;
                modbusSendData[modbusSendDataCount++] = thst.relay4 & 0xFF;*/
                sendDataBuff[sendDataCount++] = thst.fan_speed ? 0x01 : 0x02;
            }
        }
        /*else if(VENTILATOR_HAS_CHANGED())
        {
            VENTILATOR_CHANGE_RESET();
            modbusSendData[modbusSendDataCount++] = MODBUS_SEND_WRITE_SINGLE_REGISTER;
            modbusSendData[modbusSendDataCount++] = (ventilator.relay >> 8) & 0xFF;
            modbusSendData[modbusSendDataCount++] = ventilator.relay & 0xFF;
            modbusSendData[modbusSendDataCount++] = VENTILATOR_IS_ACTIVE();
        }*/
        else
        {
            if(!isSendDataBufferEmpty())
            {
                goto check_changes_loops_end;   // IMPORTANT!
            }
            
            
            
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
                    
                    /*if(Curtain_isMoving(cur) && (!Curtain_isNewDirectionStop(cur)) && (!Curtain_shouldResend(cur)))
                    {
                        Curtain_Resend(cur);
                        
                        if(sendDataBuff[0] != MODBUS_SEND_WRITE_SINGLE_REGISTER) sendDataBuff[sendDataCount++] = MODBUS_SEND_WRITE_SINGLE_REGISTER;
                        relay = Curtain_isMovingUp(cur) ? Curtain_GetRelayUp(cur) : Curtain_GetRelayDown(cur);
                        *(sendDataBuff + sendDataCount) = (relay >> 8) & 0xFF;
                        *(sendDataBuff + sendDataCount + 1) = relay & 0xFF;
                        sendDataCount += 2;
                        sendDataBuff[sendDataCount++] = 0x2;
                        
                        if(screen == 16) shouldDrawScreen = 1;
                        
                        Curtain_RestartTimer(cur);
                    }
                    else if((!Curtain_shouldResend(cur)) || (Curtain_shouldResend(cur) && Curtain_HasSwitchDirectionTimeExpired(cur)))
                    {*/
                        /*if(Curtain_shouldResend(cur))
                        {
                            Curtain_ResendReset(cur);
                            Curtain_RestartTimer(cur);
                        }*/
                        
//                        if(sendDataBuff[0] != MODBUS_SEND_WRITE_SINGLE_REGISTER) sendDataBuff[sendDataCount++] = MODBUS_SEND_WRITE_SINGLE_REGISTER;
                        relay = (Curtain_isNewDirectionUp(cur) || (Curtain_isNewDirectionStop(cur) && Curtain_isMovingUp(cur))) ? Curtain_GetRelayUp(cur) : Curtain_GetRelayDown(cur);
                        *(sendDataBuff + sendDataCount) = (relay >> 8) & 0xFF;
                        *(sendDataBuff + sendDataCount + 1) = relay & 0xFF;
                        sendDataCount += 2;
                        sendDataBuff[sendDataCount++] = Curtain_isNewDirectionStop(cur) ? 0 : (Curtain_isNewDirectionUp(cur) ? 1 : 2);
                        
                        if(screen == 16) shouldDrawScreen = 1;
                        
                        if(Curtain_isNewDirectionStop(cur)) Curtain_Reset(cur);
                        else Curtain_DirectionEqualize(cur);
                        
                        break; // blinds have to be sent one by one
//                    }
                }
            }
            
            
            if(!isSendDataBufferEmpty())
            {
                sendData.data = sendDataBuff;
                sendData.len = sendDataCount;
                sendData.type = S_JALOUSIE;
                goto check_changes_loops_end;   // IMPORTANT!
            }
            
            
            for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; i++)
            {
                if(Light_Modbus_hasStatusChanged(lights_modbus + i))
                {
//                    if(isSendDataBufferEmpty()) sendDataBuff[sendDataCount++] = MODBUS_SEND_WRITE_SINGLE_REGISTER;
                    *(sendDataBuff + sendDataCount) = (Light_Modbus_GetRelay(lights_modbus + i) >> 8) & 0xFF;
                    *(sendDataBuff + sendDataCount + 1) = Light_Modbus_GetRelay(lights_modbus + i) & 0xFF;
                    sendDataCount += 2;
                    sendDataBuff[sendDataCount++] = Light_Modbus_isNewValueOn(lights_modbus + i) ? 0x01 : 0x02;
                    
                    Light_Modbus_ResetChange(lights_modbus + i);
                    
                    if(screen == 15) shouldDrawScreen = 1;
                    else if(!screen) screen = 1;
                }
            }
            
            
            if(!isSendDataBufferEmpty())
            {
                sendData.data = sendDataBuff;
                sendData.len = sendDataCount;
                sendData.type = S_BINARY;
                goto check_changes_loops_end;   // IMPORTANT!
            }
            
            
            for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; i++)
            {
                if(Light_Modbus_hasBrightnessChanged(lights_modbus + i))
                {
//                    if(isSendDataBufferEmpty()) sendDataBuff[sendDataCount++] = LIGHT_SEND_BRIGHTNESS_SET;
                    *(sendDataBuff + sendDataCount) = (Light_Modbus_GetRelay(lights_modbus + i) >> 8) & 0xFF;
                    *(sendDataBuff + sendDataCount + 1) = Light_Modbus_GetRelay(lights_modbus + i) & 0xFF;
                    sendDataCount += 2;
                    sendDataBuff[sendDataCount++] = Light_Modbus_GetBrightness(lights_modbus + i);
                    
                    Light_Modbus_ResetBrightness(lights_modbus + i);
                }
            }
            
            
            if(!isSendDataBufferEmpty())
            {
                sendData.data = sendDataBuff;
                sendData.len = sendDataCount;
                sendData.type = S_DIMMER;
                goto check_changes_loops_end;   // IMPORTANT!
            }
            
            
            for(uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; i++)
            {
                if(Light_Modbus_hasColorChanged(lights_modbus + i))
                {
                    if(isSendDataBufferEmpty()) sendDataBuff[sendDataCount++] = LIGHT_SEND_COLOR_SET;
                    *(sendDataBuff + sendDataCount) = (Light_Modbus_GetRelay(lights_modbus + i) >> 8) & 0xFF;
                    *(sendDataBuff + sendDataCount + 1) = Light_Modbus_GetRelay(lights_modbus + i) & 0xFF;
                    sendDataCount += 2;
                    sendDataBuff[sendDataCount++] = Light_Modbus_GetColor(lights_modbus + i) & 0xFF;             // blue
                    sendDataBuff[sendDataCount++] = (Light_Modbus_GetColor(lights_modbus + i) >> 8) & 0xFF;      // green
                    sendDataBuff[sendDataCount++] = (Light_Modbus_GetColor(lights_modbus + i) >> 16) & 0xFF;     // red
                    
                    Light_Modbus_ResetColor(lights_modbus + i);
                }
            }
            
            
            if(!isSendDataBufferEmpty()) goto check_changes_loops_end;   // IMPORTANT!
            
            
            /*lcnt = 0;
            dcnt = 0;
            tcnt = 0;
            ZEROFILL(tbuf,COUNTOF(tbuf));
            ZEROFILL(lbuf,COUNTOF(lbuf));
            ZEROFILL(dbuf,COUNTOF(dbuf));            
            lctrl1 =(uint8_t*)&LIGHT_Ctrl1.Main1;
            lctrl2 =(uint8_t*)&LIGHT_Ctrl2.Main1;
            for(i = 0; i < 8; i++){
                if (*(lctrl1+3) != *(lctrl1+2)){
                    *(lctrl1+3)  = *(lctrl1+2);
                    if (*lctrl1){
                        if (i < 5){ 
                            lbuf[lcnt++] = *lctrl1;
                            lbuf[lcnt++] = *(lctrl1+2);
                            if (*lctrl2){
                                lbuf[lcnt++] = *lctrl2;
                                lbuf[lcnt++] = *(lctrl1+2);
                            }
                        } else {
                            dbuf[dcnt++] = *lctrl1;
                            dbuf[dcnt++] = *(lctrl1+2);
                        }
                    }
                }
                lctrl1 += 4;
                lctrl2 += 4;
            }*/
            
            check_changes_loops_end:;
        }
    }
}
/**
  * @brief
  * @param
  * @retval
  */
void RS485_Tick(void){
    if (init_tf == true) {
        TF_Tick(&tfapp);
    }
}
/**
  * @brief
  * @param
  * @retval
  */
void TF_WriteImpl(TinyFrame *tf, const uint8_t *buff, uint32_t len){
    HAL_UART_Transmit(&huart1,(uint8_t*)buff, len, RESP_TOUT);
    HAL_UART_Receive_IT(&huart1, &rec, 1);
}
/**
  * @brief  
  * @param  
  * @retval 
  */
void RS485_RxCpltCallback(void){
    TF_AcceptChar(&tfapp, rec);
	HAL_UART_Receive_IT(&huart1, &rec, 1);
}
/**
* @brief : all data send from buffer ?
* @param : what  should one to say   ? well done,   
* @retval: well done, and there will be more..
*/
void RS485_TxCpltCallback(void){
}
/**
* @brief : usart error occured during transfer
* @param : clear error flags and reinit usaart
* @retval: and wait for address mark from master 
*/
void RS485_ErrorCallback(void){
    __HAL_UART_CLEAR_PEFLAG(&huart1);
    __HAL_UART_CLEAR_FEFLAG(&huart1);
    __HAL_UART_CLEAR_NEFLAG(&huart1);
    __HAL_UART_CLEAR_IDLEFLAG(&huart1);
    __HAL_UART_CLEAR_OREFLAG(&huart1);
	__HAL_UART_FLUSH_DRREGISTER(&huart1);
	huart1.ErrorCode = HAL_UART_ERROR_NONE;
	HAL_UART_Receive_IT(&huart1, &rec, 1);
}



bool isSendDataBufferEmpty()
{
    return !sendDataCount;
}
/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
