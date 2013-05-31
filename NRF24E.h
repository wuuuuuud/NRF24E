/**************************************************
这是NRF24E(nhanced) 类的头文件
应该没什么好改的
内有尽可能详细的使用说明。
**************************************************/

#ifndef __NRF24E_H__
#define __NRF24E_H__ 
#include "..\RF24\RF24.h"
#include "..\RF24\nRF24L01.h"
#include <NRF24E_config.h>
#include "..\EEPROM\EEPROM.h"

typedef enum//定义事件状态枚举值
{
	Idle, Button_Pressed, NRF_Rx, NRF_Tx, NRF_Fail
} eventStates;

typedef enum//定义NRF状态枚举值
{
	IDLING, WORKING, PAIRING_I_TX, \
	PAIRING_I_RX, PAIRING_II_TX, PAIRING_II_RX
} generalStates;



class NRF24E:public RF24 //NRF24E类继承RF24类,对于RF24的说明请参考相应文档。
{
private://内部变量
	/*buffs ,including a string terminator '\0' */
	uint8_t TX_buff[NRF_Payload_Length + 1];
	uint8_t RX_buff[NRF_Payload_Length + 1];
	uint8_t ACK_buff[NRF_ACK_Length+1];
	/* the length of the id is the same as __timestamp__ including one terminator */
	uint8_t Local_ID[ID_Length];
	uint8_t Remote_ID[ID_Length];

	uint8_t Local_Address[NRF_Address_Length];
	uint8_t Remote_Address[NRF_Address_Length];

	uint64_t pairingTimeStamp;

	generalStates GeneralStates;
public:
	/*
	 *The construct function calls the RF24's constructor
	 *_cepin:the pin number of the CE pin
	 *_cspin:the pin number of the CSN pin
	 */
	NRF24E(uint8_t _cepin,uint8_t _cspin);
	volatile eventStates EventState ;
	volatile uint8_t PairButtonState ;
	volatile uint64_t LocalAddress;
	volatile uint64_t RemoteAddress;
	void (*rxInterruptFunction)(void*);//接收中断函数指针
	void (*txInterruptFunction)(void *);//发送中断
	void (*failInterruptFunction)(void *);//错误中断
	uint64_t bytes2int64(uint8_t*, uint8_t);//字符串向int64转换,最好别用

	void int64Tobytes(uint8_t*, uint8_t, uint64_t);//反向,最好别用+1
	void EEPROMWrite(uint32_t, uint8_t*, uint32_t);
	/*
	 *Used when pairing
	 */
	generalStates EnterState(generalStates);
	generalStates CheckState(void);//返回generalState的值。
	/*
	 *Try to load necessary data from the EEPROM,
	 *if no local ID is found, one will be assign to the MCU,
	 *generating from the compiling timestamp and random function.
	 *All id characters are printable ASCII.
	 *If any other parameters not found,the general state will be idling. 
	 */
	generalStates LoadEEPROM(void);
	/*
	 *Set the reading pipe and begin listening
	 *pipe0:NRF_Public_Address
	 *pipe1:Local_Address
	 */
	void StartListening(void);
	/*
	 *Stop listening and interrupts and then send the data in a *blocking* way
	 *Don't think it is necessary to use an interrupt
	 *After sending ,return to the origin state(listening and interrupt)
	 *1 for success or 0 for failed is returned as a result.
	 *calls RF24::write internally.
	 *para1:字符串指针
	 *para2:字符串长度
	 *para3:历史遗留问题,不用管。
	 */
	uint8_t Send(uint8_t*,uint8_t,uint8_t=0);//无线发送数据

	uint8_t StartPairing();//进入配对模式

	void InterruptService(void);
	
	/*
	 *config with a set of default value
	 */
	void QuickConfig(void);//这是历史遗留问题

	void TurnOffACKPayload(void);


};


#endif