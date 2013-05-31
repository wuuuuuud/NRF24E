#ifndef __NRF24E_CONFIG_H__
#define __NRF24E_CONFIG_H__

#define ID_Address (0x2)//Local ID address,2-26
#define ID_Length (25)//same as __timestamp__ including '\0'
#define ID_Existed_Address (0x1)//1
#define ID_Existed ('W')
#define Remote_ID_Existed_Address (39)//39
#define Remote_ID_Address (40)//40-44

#define Local_Address_Existed_Address (27)//27
#define NRF_Address_Existed ('M')
#define Local_Address_Address (28)//28-32
#define NRF_Address_Length (5)
#define Remote_Address_Existed_Address (33)//33
#define Remote_Address_Address (34)//34-38

#define NRF_Public_Address ((uint8_t*)"AOIWT")

#define NRF_Payload_Length (32)

#define NRF_CMD_Pair_I (0x4243)
#define NRF_CMD_Pair_II (0x4244)

#define Pairing_Button (2)
#define NRF_IRQ_Pin (2)
//in ms,if the mcu has waited longer than this value, 
//it will toss a coin to decide what to do next
#define NRF_Pairing_Phase_I_Timeout (2000)


#define NRF_ACK_Phase_I ("EF")  //check code for pairing phase I
#define NRF_ACK_Phase_II ("B2") //check code for pairing phase II
#define NRF_ACK_Length (2)
#endif