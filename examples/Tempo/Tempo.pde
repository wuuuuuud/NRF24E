/*
这是class的实例程序。
本程序可以实现以下功能:
1.连接串口,向A(rduino)发送'P'可令A进入配对模式,
	A将试图与信号范围内另一个进入配对模式的A匹配。
	在一定时间之后A会自动从配对模式退出。
	该时间可以设置。
2.向A发送'T'可以测试配对是否正常,如果正常会返回"Success"
3.向A发送'S',A会返回NRF配置信息。
4.向A发送其他内容,A会试图将接收到的内容通过NRF转发出去。
5.如果A从无线接收到0/1,A会试图熄灭/点亮TestLed引脚上的Led。
注意:
请勿在中断中执行串口输出操作。
*/

#include <SPI.h>
#include "nRF24L01.h"
#include "NRF24E.h"
#include <EEPROM.h>
#include "printf.h"
#include "portConfig.h"
//以上include请保留

//#define UseSerial
#ifdef UseSerial
#define SerialBuffLength 32
#endif
#define NRF_CE_PIN 9	//定义spi的Enable引脚,请根据具体连线修改。
#define NRF_CS_PIN 10	//定义spi的Select引脚,请根据具体连线修改。
#define TestLed 0	//定义测试用的Led连接引脚,请根据具体连线修改。
#define NRF_Pairing_Pin 3 //定义nrf配对按钮中断管脚。
#define inputA (7)
#define inputB (6)
#define inputC (5)
#define inputD (4)
#define outputPort (6)
NRF24E radio(NRF_CE_PIN, NRF_CS_PIN); //初始化NRF对象。




//以下函数仅会在非配对模式下起作用
void NRF_Rx_interrupt(void *);		//定义收到接收中断时执行的函数
void NRF_Tx_interrupt(void *);		//定义收到发送中断时执行的函数
void NRF_Fail_interrupt(void *);	//定义收到错误中断时执行的函数(通常情况下不用管)
//the delay time counts in ms
void ledCycle(uint8_t repeatT, uint16_t delayT);

uint8_t startPair = false;
uint8_t coin = 0;
volatile uint8_t NRFTriggered = false;
//Here 0 means untriggered;
volatile uint8_t portStatus[4] = {0, 0, 0, 0};
//Trigger flag for all four ports.
uint8_t overAllTriggered = false;
void setup()
{


    /* add setup code here */
    pinMode(NRF_Pairing_Pin, INPUT);
    pinMode(NRF_IRQ_Pin, INPUT);

    pinMode(TestLed, OUTPUT);
    if (inputAEnable) pinMode(inputA, INPUT);
    if (inputBEnable) pinMode(inputB, INPUT);
    if (inputCEnable) pinMode(inputC, INPUT);
    if (inputDEnable) pinMode(inputD, INPUT);
    if (outputEnable) pinMode(outputPort, OUTPUT);
#ifdef UseSerial
    Serial.begin(9600);
    radio.useSerial = 1;
    printf_begin();
    Serial.println("Beginning ...");
    Serial.println("Initializing unique ID...");
#endif
    randomSeed(analogRead(0));		//初始化随机种子,请保留。




    radio.LoadEEPROM();		    //从EEPROM中初始化NRF,请保留。


    //启用NRF中断,请保留
    radio.begin();
    radio.setPayloadSize(NRF_Payload_Length);
    //radio.QuickConfig();
    radio.setPALevel(RF24_PA_HIGH);
    radio.setChannel('X');
    radio.setDataRate(RF24_1MBPS);
    radio.setRetries(10, 10);
    radio.StartListening();
    radio.TurnOffACKPayload();
    delay(1000);

    //以上NRF设置均请保留。
    radio.rxInterruptFunction = NRF_Rx_interrupt;	//设定收到接收中断时执行的函数
    //其他中断的设置同理,请参考NRF24E的头文件。

    attachInterrupt(NRF_Pairing_Pin, Pin3_it, FALLING);//启用配对按钮中断
    attachInterrupt(INT0, Pin2_it, FALLING);

    //interrupts();



}

void loop()
{

    /* add main program code here */
    //detachInterrupt(INT0);

    //attachInterrupt(INT0,Pin2_it,FALLING);
    //detachInterrupt(INT1);

    //button logic
    if(digitalRead(NRF_Pairing_Pin) == 0)
    {
        delay(10);
        if (digitalRead(NRF_Pairing_Pin) == 1) return;
        
        delay(1890);
        if(digitalRead(NRF_Pairing_Pin) == 0)
        {
            startPair = true;
            ledCycle(3, 100);
        }
        else
        {
            ledBlink(3, 25);
            uint8_t _a[2];
            if (coin == 0)
            {
                coin = 1;
                _a[0] = '0';
            }
            else
            {
                coin = 0;
                _a[0] = '1';
            }
            bool success = radio.Send((uint8_t*)_a, 1);
            delay(200);
            ledBlink(2, 25);
            if (success)
            {
                ledCycle(2,500);
#ifdef UseSerial
                Serial.println("Success!");
#endif
            }

            else
            {
               ledCycle(10,100);
#ifdef UseSerial
                Serial.println("Fail!");
#endif
            }
        }
    }

    //pairing logic
    if (startPair)
    {


        startPair = false;
        radio.StartPairing();
        if (radio.CheckState() == WORKING)
        {
            delay(1000);
            ledBlink(7, 30);

        }
        else
        {
            ledCycle(5, 500);
        }

    }

    //port-check logic
    {
#if (inputAEnable)
        portStatus[0] = ((digitalRead(inputA) == inputATrigger) ? 1 : 0);
#endif
#if (inputBEnable)
        portStatus[1] = ((digitalRead(inputB) == inputBTrigger) ? 1 : 0);
#endif
#if (inputCEnable)
        portStatus[2] = ((digitalRead(inputC) == inputCTrigger) ? 1 : 0);
#endif
#if (inputDEnable)
        portStatus[3] = ((digitalRead(inputD) == inputDTrigger) ? 1 : 0);
#endif
        uint8_t index = 0;
        for (int j = 0; j < 4; j++)
        {
            index |= (portStatus[j] << (3 - j));
        }

        overAllTriggered = truthTable[index];
#if (outputEnable)
#if(!NRFModuleReceiver )
        if (overAllTriggered) digitalWrite(outputPort, outputActive);
#else
        if (overAllTriggered && NRFTriggered) digitalWrite(outputPort, outputActive);
#endif
#endif
#if (NRFModuleTransmitter)
	if(radio.CheckState()==WORKING)
    {
        if (overAllTriggered)
        {
            digitalWrite(TestLed,LOW);
            radio.Send((uint8_t*)"1", 1);
#ifdef UseSerial
	    Serial.println("nrf signal sent!");
	    delay(500);
#endif
        }
        else 
        {
	    digitalWrite(TestLed,HIGH);
	    radio.Send((uint8_t*)"0",1);
#ifdef UseSerial
	    Serial.println("nrf false signal sent!");
	    delay(500);
#endif
        }
    }
#endif

#if (NRFModuleReceiver && outputEnable)
#if (UseSerial)
	if (NRFTriggered) Serial.println("NRF triggered!");
	if (overAllTriggered) Serial.println("overAllTriggeres!");
#endif
        if (NRFTriggered && overAllTriggered) 
            {
                digitalWrite(outputPort, outputActive);
#if (UseSerial)
		Serial.println("receive and triggered!");
#endif
        }
	        else//go default
        {
	    digitalWrite(outputPort, 1-outputActive);
        }
#endif

    }

    //serial control logic
#ifdef UseSerial
    if(Serial.available() > 0)//以下串口轮询
    {
        char InChar = Serial.read();
        Serial.print(InChar);
        //
        if (InChar == 'T')//NRF测试
        {
            bool success = radio.Send((uint8_t*)"test", 4);//NRF无线发送方法
            if (success)
            {
                Serial.println("Success!");
            }

            else
            {
                Serial.println("Fail!");
            }
        }
        else if(InChar == 'P')//进入配对模式
        {
            Serial.println("Entering pairing mode!");
            radio.StartPairing();
        }
        else if(InChar == 'S')//输出配置信息
        {
            radio.printDetails();
        }

        else//转发
        {
            radio.Send((uint8_t*)&InChar, 1, 0);
        }
    }
#endif
    delay(5+random()%10);

}


void Pin2_it()//无线中断,请勿修改

{
    radio.InterruptService();
}
void Pin3_it()
{
    delay(5);
    if (digitalRead(NRF_Pairing_Pin) == 0)
    {
        digitalWrite(TestLed, HIGH);
        delay(1000);
        digitalWrite(TestLed, LOW);
        startPair = true;
    }
}
void NRF_Rx_interrupt(void *Npointer)
{

    //uint8_t* Rx=(uint8_t*)Npointer;
    uint8_t Rx[NRF_Payload_Length + 1];
    radio.read(Rx, NRF_Payload_Length);//读取无线数据,此时eventState应当为NRF_Rx,调用的是RF24的方法。


    if (Rx[0] == NRFModuleTrigger)
    {
        NRFTriggered = true;
        ledBlink(3, 100);
    }

    else
    {
        NRFTriggered = false;
        ledBlink(1, 100);
    }



}
//led on and off, duty 50%
void ledCycle(uint8_t repeatT, uint16_t delayT)
{

    for (int i = 0; i < repeatT; i++)
    {
        digitalWrite(TestLed, LOW);
        delay(delayT);
        digitalWrite(TestLed, HIGH);
        delay(delayT);
    }

}
void ledBlink(uint8_t repeatT, uint16_t delayT)
{
    for (int i = 0; i < repeatT; i++)
    {
        digitalWrite(TestLed, LOW);
        delay(10);
        digitalWrite(TestLed, HIGH);
        delay(delayT);
    }

}