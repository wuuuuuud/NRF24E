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
//以上include请保留

#define SerialBuffLength 32
#define NRF_CE_PIN 9	//定义spi的Enable引脚,请根据具体连线修改。
#define NRF_CS_PIN 10	//定义spi的Select引脚,请根据具体连线修改。
#define TestLed 4	//定义测试用的Led连接引脚,请根据具体连线修改。
#define NRF_Pairing_Pin 3 //定义nrf配对按钮中断管脚。
NRF24E radio(NRF_CE_PIN, NRF_CS_PIN); //初始化NRF对象。



//以下函数仅会在非配对模式下起作用
void NRF_Rx_interrupt(void *);		//定义收到接收中断时执行的函数
void NRF_Tx_interrupt(void *);		//定义收到发送中断时执行的函数
void NRF_Fail_interrupt(void *);	//定义收到错误中断时执行的函数(通常情况下不用管)


uint8_t startPair=false;
uint8_t coin=0;
uint8_t value5=0,value6=0,value7=0;
void setup()
{

    /* add setup code here */
    pinMode(NRF_Pairing_Pin, INPUT);
    pinMode(NRF_IRQ_Pin, INPUT);
    pinMode(TestLed, OUTPUT);
    pinMode(5,INPUT);
    pinMode(6,INPUT);
    pinMode(7,INPUT);
    Serial.begin(9600);
    randomSeed(analogRead(0));		//初始化随机种子,请保留。
    printf_begin();

    printf("Beginning ... \n");
    
	delay(1000);
printf("Initializing unique ID...\n");

    radio.LoadEEPROM();		    //从EEPROM中初始化NRF,请保留。

    attachInterrupt(NRF_Pairing_Pin, Pin3_it, FALLING);//启用配对按钮中断
    attachInterrupt(INT0, Pin2_it, FALLING);	//启用NRF中断,请保留
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
    radio.printDetails();	    //输出NRF配置状态,使用串口。

	interrupts();



}

void loop()
{

    /* add main program code here */
    //detachInterrupt(INT0);
    //if (GeneralStates==WORKING) Serial.println("working");

    //if (GeneralStates==IDLING) Serial.println("Idle");
    //attachInterrupt(INT0,Pin2_it,FALLING);
    //detachInterrupt(INT1);
    value5=digitalRead(5);
    value6=digitalRead(6);
    value7=digitalRead(7);
    if ((value5+value6+value7)==1)
    {
	digitalWrite(TestLed,0);
	delay(20);
    digitalWrite(TestLed,1);
	delay(20);
    digitalWrite(TestLed,0);
	delay(20);
    digitalWrite(TestLed,1);
        radio.Send((uint8_t*)"1",1);
    }
    else
    {
	radio.Send((uint8_t*)"0",1);
    }
if(digitalRead(NRF_Pairing_Pin)==0){
delay(2000);
if(digitalRead(NRF_Pairing_Pin)==0)
{
startPair=true;
}
else
{
printf("send a number!");
uint8_t _a[2];
if (coin==0){coin=1;_a[0]='0';}else{coin=0;_a[0]='1';}
bool success=radio.Send((uint8_t*)_a,1);

            if (success)
            {
                Serial.println("Success!");
            }

            else
            {
                Serial.println("Fail!");
            }
}
}
	if (startPair){

printf("start pairing, button.");
startPair=false;
radio.StartPairing();

}
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



}


void Pin2_it()//无线中断,请勿修改

{
    radio.InterruptService();
}
void Pin3_it()
{
	delay(5);
	if (digitalRead(NRF_Pairing_Pin)==0)
{
	digitalWrite(TestLed, HIGH);
	delay(1000);
	digitalWrite(TestLed,LOW);
	startPair=true;
}
}
void NRF_Rx_interrupt(void *Npointer)
{
     
        //uint8_t* Rx=(uint8_t*)Npointer;
        uint8_t Rx[NRF_Payload_Length+1];
        radio.read(Rx, NRF_Payload_Length);//读取无线数据,此时eventState应当为NRF_Rx,调用的是RF24的方法。
        //Serial.println((char* )Rx);
       
        if (Rx[0] == '1')
        {
            digitalWrite(TestLed, LOW);
        }

        else if(Rx[0] == '0')
        {
            digitalWrite(TestLed, HIGH);
        }


   
}