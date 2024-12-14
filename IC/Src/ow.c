/**
 ******************************************************************************
 * File Name          : one_wire.c
 * Date               : 17/11/2016 00:59:00
 * Description        : one wire communication modul
 ******************************************************************************
 *
 *
 ******************************************************************************
 */

#if (__OW_H__ != FW_BUILD)
    #error "onewire header version mismatch"
#endif 
/* Includes ------------------------------------------------------------------*/
#include "png.h"
#include "pwm.h"
#include "main.h"
#include "rs485.h"
#include "logger.h"
#include "display.h"
#include "onewire.h"
#include "thermostat.h"
#include "stm32746g.h"
#include "stm32746g_ts.h"
#include "stm32746g_qspi.h"
#include "stm32746g_sdram.h"
#include "stm32746g_eeprom.h"
/* Imported Type  ------------------------------------------------------------*/
/* Imported Variable  --------------------------------------------------------*/
/* Imported Function  --------------------------------------------------------*/
/* Private Type --------------------------------------------------------------*/
static RX_TypeDef   OW_State;
static LinkTypeDef  OW_Link;
#ifdef  OW_DS18B20   // define structure used for dallas onewire temp. sensor
typedef struct
{
	uint8_t	sensor_id;
	uint8_t rom_code[8];
	int16_t temperature;
	
}TempSensorTypeDef;

TempSensorTypeDef ds18b20_1;
#endif
/* Private Define ------------------------------------------------------------*/
/* Private Variable ----------------------------------------------------------*/
uint32_t owtmr;
uint8_t owbps;
uint8_t owifa;
uint8_t owgra;
uint8_t owbra;
uint8_t rtimg;
uint8_t owdev;
uint8_t owrep;
uint8_t owcmd;
uint8_t owpld;
uint32_t owtout;
uint32_t owbcnt;
uint32_t owrxtmr;
uint32_t owtxtmr;
uint32_t owrxtout;
uint32_t owtxtout;
uint8_t owadd[9];
uint8_t owtx[32] = {0};
uint8_t owrx[32] = {0};
GUI_PID_STATE eTS;
static uint32_t owsync_tmr = 0;
#ifdef  OW_DS18B20 // define some variables for onewire dallas temp. sensor
static uint8_t ow_last_discrepancy;
static uint8_t ow_last_device_flag;
uint8_t ow_last_family_discrepancy;
uint8_t ow_sensor_number;
#endif
/* Constants ----------------------------------------------------------------*/              
/* Private Macro -------------------------------------------------------------*/
static uint8_t OW_RS485_Bridge(uint8_t *buff);
/* Private Function Prototype ------------------------------------------------*/
#ifdef  OW_DS18B20 // define functions to config and use dallas onewire temp. sensors
static uint8_t OW_Reset(void);
static uint8_t OW_ReadByte(void);
static void OW_SendByte(uint8_t data);
static void OW_SendBit(uint8_t send_bit);
static uint8_t OW_ReceiveBit(void);
static void OW_Send(uint8_t *command, uint8_t lenght);
static void OW_Receive(uint8_t *data, uint8_t lenght);
static uint8_t OW_ReadROM(uint8_t *ow_address);
static uint8_t OW_CrcCheck(uint8_t *ow_address, uint8_t lenght);
static void OW_Pack (uint8_t cmd, uint8_t buffer[8]);
static uint8_t OW_Unpack (uint8_t buffer[8]);
static uint16_t OW_ScratchpadToTemperature(uint16_t scratchpad);
static void OW_ResetSearch(void);
static uint8_t OW_Search(TempSensorTypeDef* ds18b20, uint8_t* sensor_cnt);
static void OW_Select(uint8_t* addr);
#endif
void OW_SetUsart(uint8_t brate, uint8_t bsize);
/* Program Code  -------------------------------------------------------------*/
/**
  * @brief
  * @param
  * @retval
  */
void OW_Init(void)
{
    owbps = 2;
    owbra = DEF_OWBRA;
    owifa = DEF_IC_OWIFA;
    OW_SetUsart(BR_9600, WL_8BIT);
    __HAL_UART_FLUSH_DRREGISTER(&huart2);
    HAL_UART_Receive_IT(&huart2, owrx, 6);
}
/**
  * @brief
  * @param
  * @retval
  */
void OW_Service(void)
{
    if(eTS.x||eTS.y||eTS.Pressed||eTS.Layer)
    {
        PID_Hook(&eTS);
        eTS.x = 0;
        eTS.y = 0;
        eTS.Layer = 0;
        eTS.Pressed = 0;
        ZEROFILL(owrx, COUNTOF(owrx));
        __HAL_UART_FLUSH_DRREGISTER(&huart2);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
        HAL_UART_Receive_IT(&huart2, owrx, 6);
    }
    else if(owtx[0]||owtx[1]||owtx[2]
    ||      owtx[3]||owtx[4]||owtx[5])
    {
        HAL_NVIC_DisableIRQ(USART2_IRQn);
        HAL_UART_Transmit(&huart2, owtx, 6, 50);
        ZEROFILL(owtx, COUNTOF(owtx));
        ZEROFILL(owtx, COUNTOF(owrx));
        __HAL_UART_FLUSH_DRREGISTER(&huart2);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
        HAL_UART_Receive_IT(&huart2, owrx, 6);
    }
//    else if ((HAL_GetTick() - owsync_tmr) >= OW_SYNC_TIME)  // ITS TIME FOR DATA EXCHANGE
//    {
//        owsync_tmr = HAL_GetTick();
//        HAL_NVIC_DisableIRQ(USART2_IRQn);
//        ZEROFILL(owtx, COUNTOF(owtx));
//        ZEROFILL(owrx, COUNTOF(owrx));
//        __HAL_UART_FLUSH_DRREGISTER(&huart2);
//        //
//        //  DATA PACKET HEADER 
//        //
//        owtx[0] = SOH;          // START OF HEADER
//        owtx[1] = owbra;        // RECEIVER ADDRESS
//        owtx[2] = owifa;        // THIS INTERFACE ADDRESS
//        owtx[3] = SYSNC_DATA;   // SYNC DATA PACKET 
//        owtx[4] = 16;           // PAYLOAD LENGHT
//        owtx[5] = EOT;          // END OF TRANSMISSION
//        HAL_UART_Transmit(&huart2, owtx, 6, PAK_TOUT(6, BR_9600));
//        //
//        //  DATA PACKET PAYLOAD AFTER 10ms 
//        //
//        owtx[0] = STX;          // START OF TEXT
//        owtx[1] = (FW_TYPE>>24);// THIS DEVICE TYPE
//        owtx[2] = 0;            // FLAGS 
//        owtx[3] = 0;            // FLAGS 
//        if (IsTempRegActiv())       owtx[4] |= 0x01U;
//        if (IsTempRegHeating())     owtx[4] |= 0x02U;
//        if (IsTempRegEnabled())     owtx[4] |= 0x04U;
//        if (IsTempRegOutputActiv()) owtx[4] |= 0x08U;
//        if (IsNtcConnected())       owtx[4] |= 0x10U;
//        if (IsPWMErrorActiv())      owtx[4] |= 0x20U;
//        if (IsDISPCleaningActiv())  owtx[4] |= 0x40U; 
//        if (IsSPUpdateActiv())      owtx[4] |= 0x80U;
//        if (IsPWM_OutEnabled())     owtx[5] |= 0x01U;
//        if (IsScrnsvrActiv())       owtx[5] |= 0x02U;
//        if (IsRtcTimeValid())       owtx[5] |= 0x04U;
//        if (IsLightSwActiv())       owtx[5] |= 0x08U;
//        if (IsLight1On())           owtx[5] |= 0x10U;
//        if (IsLight2On())           owtx[5] |= 0x20U;
//        if (IsLight3On())           owtx[5] |= 0x40U;
//        if (IsLight4On())           owtx[5] |= 0x80U;
//        owtx[6] = pwm[0];
//        owtx[7] = pwm[1];
//        owtx[8]=  pwm[2];
//        owtx[9]=  pwm[3];
//        owtx[10]= (room_temp>>8);
//        owtx[11]= (room_temp&0xFF);
//        owtx[12]= thst_sp;
//        owtx[13]= fan_speed;
//        owtx[14]= ETX;              // END OF TEXT
//        CRC_ResetDR();
//        owtx[15]= (uint8_t) CRC_Calculate8(owtx, 15);
//        HAL_UART_Transmit(&huart2, owtx, 16, PAK_TOUT(16, BR_9600));
//        ZEROFILL(owtx, COUNTOF(owtx));
//        ZEROFILL(owrx, COUNTOF(owrx));
//        __HAL_UART_FLUSH_DRREGISTER(&huart2);
//        HAL_NVIC_EnableIRQ(USART2_IRQn);
//        HAL_UART_Receive_IT(&huart2, owrx, 6);
//    }
}
/**
  * @brief
  * @param
  * @retval
  */
void OW_TxCpltCallback(void)
{
}
/**
  * @brief
  * @param
  * @retval
  */
void OW_RxCpltCallback(void)
{
//    HAL_NVIC_DisableIRQ(USART2_IRQn);
//    
//    if      ((owrx[0] == SOH)&&(owrx[1] == owbra)&&(owrx[5] == EOT))
//    {
//        owrep = owrx[2]; // SENDER ADDRESS
//        owcmd = owrx[3]; // PACKET COMMAND
//        owpld = owrx[4]; // PACKET PAYLOAD
//        ZEROFILL(owrx, COUNTOF(owrx));
//        __HAL_UART_FLUSH_DRREGISTER(&huart2);
//        HAL_NVIC_EnableIRQ(USART2_IRQn);
//        HAL_UART_Receive_IT(&huart2, owrx, owpld);
//    }
//    else if ((owrx[0] == STX)&&(owrx[14] == ETX))
//    {
//        CRC_ResetDR();
//        if (owrx[15] == (uint8_t)CRC_Calculate8(owtx,15))
//        {
//            if (owtx[5] & 0x10) Light1On();
//            else Light1Off();
//            if (owtx[5] & 0x20) Light2On();
//            else Light2Off();
//            if (owtx[5] & 0x40) Light3On();
//            else Light3Off();
//            if (owtx[5] & 0x80) Light4On();
//            else Light4Off();
//            pwm[0]      = owtx[6];
//            pwm[1]      = owtx[7];
//            pwm[2]      = owtx[8];
//            pwm[3]      = owtx[9];
//            room_temp = ((owtx[10]<<8)|owtx[11]);
//            thst_sp     = owtx[12];
//            fan_speed   = owtx[13];
//        }
//        ZEROFILL(owtx, COUNTOF(owrx));
//        __HAL_UART_FLUSH_DRREGISTER(&huart2);
//        HAL_NVIC_EnableIRQ(USART2_IRQn);
//        HAL_UART_Receive_IT(&huart2, owrx, 6);
//    }
//    else
//    {
        eTS.x = ((owrx[0]<<8)|owrx[1]);
        eTS.y = ((owrx[2]<<8)|owrx[3]);
        eTS.Pressed = owrx[4];
        eTS.Layer = owrx[5];
//    }
}
/**
  * @brief
  * @param
  * @retval
  */
void OW_ErrorCallback(void)
{
    __HAL_UART_CLEAR_PEFLAG     (&huart2);
    __HAL_UART_CLEAR_FEFLAG     (&huart2);
    __HAL_UART_CLEAR_NEFLAG     (&huart2);
    __HAL_UART_CLEAR_IDLEFLAG   (&huart2);
    __HAL_UART_CLEAR_OREFLAG    (&huart2);
    __HAL_UART_FLUSH_DRREGISTER (&huart2);
    huart2.ErrorCode = HAL_UART_ERROR_NONE;
    OW_Init();
}
/**
  * @brief
  * @param
  * @retval
  */
void OW_SetUsart(uint8_t brate, uint8_t bsize)
{
    HAL_NVIC_DisableIRQ(USART2_IRQn);
    HAL_UART_DeInit(&huart2);
    huart2.Instance        		= USART2;
    huart2.Init.BaudRate        = bps[brate];
    if      (bsize == WL_9BIT) huart2.Init.WordLength = UART_WORDLENGTH_9B;
	else if (bsize == WL_8BIT) huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits   		= UART_STOPBITS_1;
	huart2.Init.Parity     		= UART_PARITY_NONE;
	huart2.Init.HwFlowCtl  		= UART_HWCONTROL_NONE;
	huart2.Init.Mode       		= UART_MODE_TX_RX;
	huart2.Init.OverSampling	= UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling  = UART_ONE_BIT_SAMPLE_DISABLE;
	huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    HAL_NVIC_SetPriority(USART2_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    
    if      (bsize == WL_8BIT)
    {
        if (HAL_UART_Init(&huart2) != HAL_OK) ErrorHandler(MAIN_FUNC, USART_DRV);
    }
    else if (bsize == WL_9BIT)
    {
        if (HAL_MultiProcessor_Init(&huart2, STX, UART_WAKEUPMETHOD_ADDRESSMARK) != HAL_OK) ErrorHandler(OW_FUNC, USART_DRV);
        HAL_MultiProcessor_EnableMuteMode(&huart2);
        HAL_MultiProcessor_EnterMuteMode(&huart2);        
    }
}
/**
  * @brief
  * @param
  * @retval
  */
static uint8_t OW_RS485_Bridge(uint8_t *buff)
{
    uint8_t txhdr[2];
    uint8_t rxhdr[8];
    uint32_t repcnt = 0;
    HAL_StatusTypeDef stat;
    
    CRC_ResetDR();
    repcnt = 5U;
    txhdr[0] = STX;     // start of text to wakeup all device receivers 
    txhdr[1] = 1;    // this is address mark 9 bit set high
    buff[0] = STX;      // set for crc calculation
    buff[2] = owifa;   // insert sender interface address 
    buff[buff[3]+4] = (uint8_t) CRC_Calculate8(buff, buff[3]+4); // recalculate packet crc8 
    
    while(repcnt)
    {
        OW_SetUsart(BR_9600, WL_9BIT);
        stat  = HAL_UART_Transmit (&huart2, txhdr, 1, PAK_TOUT(1, BR_9600)); 
        OW_SetUsart(BR_9600, WL_8BIT);
        stat += HAL_UART_Transmit (&huart2, &buff[1], (buff[3]+4), PAK_TOUT((buff[3]+4), BR_9600));
        if ((buff[1] == DEF_OWBRA) 
        ||  (buff[1] == DEF_HC_OWGRA) 
        ||  (buff[1] == DEF_CR_OWGRA) 
        ||  (buff[1] == DEF_RC_OWGRA) 
        ||  (buff[1] == DEF_RT_OWGRA)) return (ESC); // brake here for request without response
        __HAL_UART_FLUSH_DRREGISTER (&huart2);
        ZEROFILL(rxhdr, COUNTOF(rxhdr));
        stat += HAL_UART_Receive (&huart2, rxhdr, 7, OW_PKTIME);
        if ((stat     == HAL_OK)
        &&  (rxhdr[0] == STX) 
        &&  (rxhdr[1] == buff[2]) 
        &&  (rxhdr[2] == buff[1])
        &&  (rxhdr[3] > 1))
        {
            CRC_ResetDR();
            if (rxhdr[3] == 2) 
            {   /* simple response to acknowledge received packet */
                if (rxhdr[6] == (uint8_t) CRC_Calculate8(rxhdr, 6))
                {   // copy cmd echo or response data byte to first buffer byte,
                    buff[5] = 1; // response packet data payload size
                    buff[0] = rxhdr[5]; // it will be included in response from bridge
                    return   (rxhdr[4]); // and send request status response from addressed device
                }   // if response packet check fail, try again till max. trials
                else stat = HAL_ERROR; 
            }
            else
            {   /* receive extende data direct to call buffer */
                buff[0] = rxhdr[5]; // copy response byte
                buff[5] = rxhdr[3]; // response packet data payload size
                buff[7] = rxhdr[6]; // copy first received payload byte
                stat = HAL_UART_Receive (&huart2, &buff[8], buff[5]-2, OW_PKTIME); // get all packet data
                CRC_Calculate8(rxhdr, 7); // calculate first part response crc and try send again if transfer not success
                if (buff[buff[5]+5] == (uint8_t) CRC_Calculate8(&buff[8], buff[5]-3)) 
                {
                    buff[buff[5]+5] = '\0';
                    return (rxhdr[4]); // return response
                }
                else stat = HAL_ERROR;
            }
        }
        --repcnt;   // valid response should allready return to caller 
        HAL_Delay(RX2TX_DEL);
    }
    return (NAK);
}
/**
  * @brief
  * @param
  * @retval
  */
#ifdef  OW_DS18B20 
static uint8_t OW_Reset(void)
{
	OW_SetUsart(BR_9600, WL_8BIT);
	ow_rxbuf[0] = 0xf0U;
	HAL_UART_Transmit(&huart2, ow_rxbuf, 1, OW_TSENS_TOUT);
	HAL_UART_Receive(&huart2, ow_rxbuf, 1, OW_TSENS_TOUT);
	OW_SetUsart(BR_115200, WL_8BIT);
	if((ow_rxbuf[0] != 0xf0U) && (ow_rxbuf[0] != 0x00U) && (ow_rxbuf[0] != 0xffU)) return (1U);
	else return(0U);	
}
/**
  * @brief
  * @param
  * @retval
  */
static void OW_SendByte(uint8_t data)
{
	uint32_t i;
	
	for(i = 0U; i < 8U; i++, (data = data >> 1U))
	{
		OW_SendBit(data & (1U<<0));
	}
}
/**
  * @brief
  * @param
  * @retval
  */
static uint8_t OW_ReadByte(void)
{
	uint8_t rd_byte, i;
	
	for(i = 0U; i < 8U; i++)
	{
		rd_byte = (rd_byte >> 1U) + 0x80U * OW_ReceiveBit();
	}
	
	return rd_byte;
}
/**
  * @brief
  * @param
  * @retval
  */
static uint8_t OW_ReceiveBit(void)
{
	uint8_t txd, rxd;
	txd = 0xffU;	
	HAL_UART_Transmit(&huart2, &txd, 1U, OW_TSENS_TOUT);	
	HAL_UART_Receive(&huart2, &rxd, 1U, OW_TSENS_TOUT);	
	if(rxd == txd) return(1U);
	else return(0U);
}
/**
  * @brief
  * @param
  * @retval
  */
static void OW_SendBit(uint8_t send_bit)
{
	uint8_t txb, rxb;	
	if(send_bit == 0U)  txb = 0x00U;
	else txb = 0xffU;	
	HAL_UART_Transmit(&huart2, &txb, 1U, OW_TSENS_TOUT);
	HAL_UART_Receive(&huart2, &rxb, 1U, OW_TSENS_TOUT);
}
/**
  * @brief
  * @param
  * @retval
  */
static void OW_Send(uint8_t *command, uint8_t lenght)
{
	uint32_t i;
	
	uint32_t one_wire_lenght = lenght * 8U;
	
	for (i = 0U;  i < lenght; i++) 
	{
		OW_Pack(command[i], &(ow_rxbuf[i * 8U]));
	}
	
	HAL_UART_Transmit(&huart2, ow_rxbuf, one_wire_lenght, OW_TSENS_TOUT);
}
/**
  * @brief
  * @param
  * @retval
  */
static void OW_Receive(uint8_t *data, uint8_t lenght)
{
	uint32_t i;
	uint32_t ow_lenght = lenght * 8U;
	uint8_t tx_byte = 0xffU;
	
	for(i = 0U; i < ow_lenght; i++)
	{
		HAL_UART_Transmit(&huart2, &tx_byte, 1U, OW_TSENS_TOUT);
		HAL_UART_Receive(&huart2, &ow_rxbuf[i], 1U, OW_TSENS_TOUT);
	}
	
	for(i = 0U; i < lenght; i++)
	{
		data[i] = OW_Unpack(&(ow_rxbuf[i * 8U]));
	}
}
/**
  * @brief
  * @param
  * @retval
  */
static uint8_t OW_ReadROM(uint8_t *ow_address)
{
	uint8_t crc;
	
	if(OW_Reset() != 0U)
	{
		OW_SendByte(OW_TSENS_RDROM);
		OW_Receive(ow_address, 8U);
		crc = OW_CrcCheck(ow_address, 7U);
		if((crc != ow_address[7U]) || (crc == 0U))return (1U);
		else return(0U);
	}
	else return (2U);
}
/**
  * @brief
  * @param
  * @retval
  */
static uint8_t OW_CrcCheck(uint8_t *ow_address, uint8_t lenght)
{
	uint8_t crc = 0U;
	uint8_t i, j;

	for (i = 0U; i < lenght; i++) 
	{
		uint8_t inbyte = ow_address[i];
		
		for (j = 0U; j < 8U; j++) 
		{
			uint8_t mix = (crc ^ inbyte) & 0x01U;
			crc >>= 1U;
			if (mix) 
			crc ^= 0x8CU;
			inbyte >>= 1U;
		}
	}
	
	return crc;
}
/**
  * @brief
  * @param
  * @retval
  */
static void OW_Pack(uint8_t command, uint8_t buffer[8])
{
	uint32_t i;
	
	for (i = 0U;  i < 8U; i++)
	{
		buffer[i] = (command & (1U<<i)) ? 0xffU : 0x00U;
	}
}
/**
  * @brief
  * @param
  * @retval
  */
static uint8_t OW_Unpack (uint8_t buffer[8])
{
	uint32_t i;
	uint8_t res = 0U;

	for (i = 0U; i < 8U; i++) 
	{
		if (buffer[i] == 0xffU)
		{
			res |=  (1U<<i);
		}
	}

	return res;
}
/**
  * @brief
  * @param
  * @retval
  */
static uint16_t OW_ScratchpadToTemperature(uint16_t scratchpad) 
{
    uint16_t result;
	
	if(scratchpad & (1U<<15))
	{
		scratchpad = ~scratchpad + 1U;
		result = scratchpad >> 4U; 							// cijelobrojni dio temperature
		result *= 10U; 										// 22 -> 220
		result += (((scratchpad & 0x000fU) *625U) / 1000U);
		result |= 0x8000U; 									// add minus sign
	}
	else
	{
		result = scratchpad >> 4U; 							// cijelobrojni dio temperature
		result *= 10U; 										// 22 -> 220
		result += (((scratchpad & 0x000fU) *625U) / 1000U);	// add decimal part
	}
    
    return result;
}
/**
  * @brief
  * @param
  * @retval
  */
static void OW_ResetSearch(void) 
{
	ow_last_discrepancy = 0U;
	ow_last_family_discrepancy = 0U;
	ow_last_device_flag = 0U;
	ow_sensor_number = 0U;
}
/**
  * @brief
  * @param
  * @retval
  */
static uint8_t OW_Search(TempSensorTypeDef* ds18b20, uint8_t* sensor_cnt) 
{
	static uint8_t init_cnt = 0U;
	uint8_t last_zero, rom_byte_number, search_result;
	uint8_t id_bit, cmp_id_bit, id_bit_number;
	uint8_t rom_byte_mask, search_direction;

	id_bit_number = 1U;
	last_zero = 0U;
	rom_byte_number = 0U;
	rom_byte_mask = 1U;

	if (ow_last_device_flag == 0U)
	{
		if (OW_Reset() == 0U)
		{
			ow_last_discrepancy = 0U;
			ow_last_device_flag = 0U;
			ow_last_family_discrepancy = 0U;
			return (0U);
		}

		OW_SendByte(OW_TSENS_SRCHROM); 

		do{
			id_bit = OW_ReceiveBit();
			cmp_id_bit = OW_ReceiveBit();
			
			if ((id_bit == 1U) && (cmp_id_bit == 1U)) break;
			else
			{
				if (id_bit != cmp_id_bit) search_direction = id_bit;  // bit write value for search
				else
				{
					if (id_bit_number < ow_last_discrepancy)
					{
						search_direction = ((ds18b20->rom_code[rom_byte_number] & rom_byte_mask) > 0U);
					}
					else search_direction = (id_bit_number == ow_last_discrepancy);
					
					if (search_direction == 0U)
					{
						last_zero = id_bit_number;
						if (last_zero < 9U)  ow_last_family_discrepancy = last_zero;
					}
				}
				
				if (search_direction == 1) ds18b20->rom_code[rom_byte_number] |= rom_byte_mask;
				else ds18b20->rom_code[rom_byte_number] &= ~rom_byte_mask;
				
				OW_SendBit(search_direction);
				id_bit_number++;
				rom_byte_mask <<= 1U;
				
				if (rom_byte_mask == 0U)
				{
					rom_byte_number++;
					rom_byte_mask = 1U;
				}
			}
		} while(rom_byte_number < 8U);
		
		if (!(id_bit_number < 65U))
		{
			search_result = 1U;
			ow_last_discrepancy = last_zero;
			if (ow_last_discrepancy == 0U) ow_last_device_flag = 1U;
		}
	}
	
	if ((search_result == 0U) || (ds18b20->rom_code[0] == 0U))
	{
		ow_last_discrepancy = 0U;
		ow_last_device_flag = 0U;
		ow_last_family_discrepancy = 0U;
		return (0U);
	} 
	else 
	{
		init_cnt++;
		*sensor_cnt = init_cnt;
		ds18b20->sensor_id = init_cnt;
		return (init_cnt);
	}	
}
/**
  * @brief
  * @param
  * @retval
  */
static void OW_Select(uint8_t* addr) 
{
	uint8_t i;
	
	OW_SendByte(OW_TSENS_MCHROM);
	
	for (i = 0U; i < 8U; i++) 
	{
		OW_SendByte(*(addr + i));
	}
}
#endif
/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
