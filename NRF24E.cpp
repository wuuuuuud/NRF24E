#include "NRF24E.h"
NRF24E::NRF24E(uint8_t _cepin,uint8_t _cspin):RF24(_cepin,_cspin)
{
    pairingTimeStamp=0;
    randomSeed(analogRead(0));


}

generalStates NRF24E::LoadEEPROM(){
    if (EEPROM.read(ID_Existed_Address) == ID_Existed)
    {
        Serial.println("The MCU has");
        Serial.println(" got a supposed unique ID!");
        Serial.println("The ID is :");
        for (int i = 0; i <= ID_Length; i++)
        {
            Local_ID[i] = EEPROM.read(ID_Address + i);
            Serial.write(Local_ID[i]);

        }
    }
    else
    {
        Serial.println("The MCU doesn't have an unique ID!");
        Serial.println("It will be assigned one");
        char ID_String[ID_Length] = __TIMESTAMP__;


        for (int i = 0; i <= ID_Length; i++)
        {
            if (ID_String[i] == ' ' || ID_String[i] == ':' || ID_String[i] == '\0') ID_String[i] = random(33, 126);
            Local_ID[i] = ID_String[i];
            EEPROM.write(ID_Address + i, ID_String[i]);

        }
        EEPROM.write(ID_Existed_Address, ID_Existed);
        Serial.println("The ID is assigned.");

    }
    if (EEPROM.read(Local_Address_Existed_Address) == NRF_Address_Existed)
    {
        Serial.println("Found a local address, will use it!");
        for (int i = 0; i < NRF_Address_Length; i++)
        {
            LocalAddress <<= 8;
            LocalAddress += EEPROM.read(Local_Address_Address + i);
        }
        GeneralStates = WORKING;
    }
    else
    {
        Serial.println("Failed to find a local address!");
        GeneralStates = IDLING;
    }
    if (EEPROM.read(Remote_Address_Existed_Address) == NRF_Address_Existed)
    {
        Serial.println("Found a remote address, will use it!");
        for (int i = 0; i < NRF_Address_Length; i++)
        {
            RemoteAddress <<= 8;
            RemoteAddress += EEPROM.read(Remote_Address_Address + i);
        }

    }
    else
    {
        Serial.println("Failed to find a remote address!");
        GeneralStates = IDLING;
    }
    if (EEPROM.read(Remote_ID_Existed_Address) == ID_Existed)
    {
        Serial.println("Found a remote ID, will use it!");
        for (int i = 0; i < ID_Length; i++)
        {
            Remote_ID[i] = EEPROM.read(Remote_ID_Address + i);
        }
    }
    else
    {
        Serial.println("Failed to find a remote ID!");
        GeneralStates = IDLING;
    }
    return GeneralStates;
}

void NRF24E::EEPROMWrite(uint32_t addr, uint8_t* data, uint32_t len)
{
    for (int i = 0; i < len; i++)
    {
        EEPROM.write(addr + i, data[i]);
    }

}
void NRF24E::StartListening(){
    switch (GeneralStates){
    case IDLING:
    case PAIRING_I_RX:
    case PAIRING_I_TX:
    case PAIRING_II_RX:
    case PAIRING_II_TX:
    case WORKING:
        {
            setAutoAck(true);
            openReadingPipe(0,bytes2int64((uint8_t*)NRF_Public_Address,NRF_Address_Length));
            openReadingPipe(1,LocalAddress);
            startListening();
            break;
        }
    }
}

uint8_t NRF24E::Send(uint8_t* data ,uint8_t len, uint8_t noListen){
    stopListening();
    switch (GeneralStates)
    {
    case WORKING:
        {
            openWritingPipe(RemoteAddress);
            break;
        }
    default:
        {
            openWritingPipe(bytes2int64((uint8_t*)NRF_Public_Address,NRF_Address_Length));
            break;
        }
    }

    noInterrupts();
    uint8_t result=write((const void*)data,len);
    interrupts();
    if (!noListen)	StartListening();
    return result;
}

generalStates NRF24E::EnterState(generalStates newState){
    switch (newState)
    {
    case PAIRING_I_RX:
        {


            StartListening();
            writeAckPayload(0,NRF_ACK_Phase_I,NRF_ACK_Length);
            pairingTimeStamp = millis();
            return PAIRING_I_RX;
        }
    case PAIRING_I_TX:
        {
            GeneralStates = PAIRING_I_TX;
            Serial.println("Begin pairing process!");
            Serial.println("Work as a transmitter");
            int64Tobytes((uint8_t*)TX_buff, 2, NRF_CMD_Pair_I);


            for (int i = 0; i < NRF_Address_Length; i++)
            {
                LocalAddress = LocalAddress << 8;
                LocalAddress += random(34, 127); //in ascii looks better!
            }
            if (LocalAddress % 2 != 0) LocalAddress -= 1; //ensure the last bit is zero;
            int64Tobytes((uint8_t*)Local_Address, NRF_Address_Length, LocalAddress);
            Serial.println((char*)Local_Address);
            EEPROMWrite(Local_Address_Address, (uint8_t*)Local_Address, NRF_Address_Length);
            EEPROM.write(Local_Address_Existed_Address, NRF_Address_Existed);
            int64Tobytes((uint8_t*)(TX_buff + 2), 5, LocalAddress);
            Serial.println((const char*)TX_buff);
            memcpy((void*)(TX_buff + 7), (const void*)Local_ID, 25);
            //for (int i=0;i<=32;i++) Serial.println(TX_buff[i]);
            Serial.println((char*)TX_buff);

            bool success = Send((uint8_t*)TX_buff, NRF_Payload_Length,1);

            if (success)
            {
                read(ACK_buff,NRF_ACK_Length);
                Serial.println((char*)"Received Ack package:");
                Serial.println((char*)ACK_buff);
                Serial.println((char*)"Supposed Ack package:");
                Serial.println((char*)NRF_ACK_Phase_I);
                if (memcmp(ACK_buff,NRF_ACK_Phase_I,2)==0)
                {

                    flush_tx();

                    StartListening();
                    writeAckPayload(0,NRF_ACK_Phase_II,NRF_ACK_Length);
                    pairingTimeStamp=millis();
                    Serial.println("transmitted successfully!");
                    return PAIRING_II_RX;
                }
                else
                {

                    return EnterState(PAIRING_I_RX);
                }
            }
            else
            {

                Serial.println("?");

                return EnterState(PAIRING_I_RX);
            }
            break;
        }
    case PAIRING_II_TX:
        {
            uint16_t cmd = bytes2int64((uint8_t*)RX_buff, 2);
            Serial.println(cmd, 16);
            if (cmd == NRF_CMD_Pair_I)
            {
                /*
                for(int i = 0; i < NRF_Address_Length; i++) //write remote address into the EEPROM
                {
                EEPROM.write(Remote_Address_Address + i, RX_buff[2 + i]);
                }
                */
                EEPROMWrite(Remote_Address_Address,RX_buff+2,NRF_Address_Length);
                EEPROM.write(Remote_Address_Existed_Address, NRF_Address_Existed);
                RemoteAddress = bytes2int64((uint8_t*)(RX_buff + 2), NRF_Address_Length);

                LocalAddress = RemoteAddress + 1; //the last number of the address transmitted must be even.
                /*
                for(int i = 0; i < NRF_Address_Length; i++) //write the local address into the EEPROM
                {
                EEPROM.write(Local_Address_Address + i, RX_buff[2 + i]);
                }
                */
                EEPROMWrite(Local_Address_Address,RX_buff+2,NRF_Address_Length);
                EEPROM.write(Local_Address_Address + 4, RX_buff[6] + 1); //#dirty code#
                EEPROM.write(Local_Address_Existed_Address, NRF_Address_Existed);
                /*
                for(int i = 0; i < ID_Length; i++) //write remote ID into the EEPROM
                {
                EEPROM.write(Remote_ID_Address + i, RX_buff[7 + i]);
                Remote_ID[i] = RX_buff[7 + i];
                }
                */
                EEPROMWrite(Remote_ID_Address,RX_buff+7,ID_Length);
                EEPROM.write(Remote_ID_Existed_Address, ID_Existed);

                GeneralStates = PAIRING_II_TX;

                int64Tobytes((uint8_t*)TX_buff + 2, NRF_Address_Length, LocalAddress);
                int64Tobytes((uint8_t*)TX_buff, 2, NRF_CMD_Pair_II);
                memcpy((void*)(TX_buff + 7), (const void *)Local_ID, ID_Length);
                Serial.println((char*)TX_buff);
                delay(50);

                uint8_t success=Send(TX_buff, NRF_Payload_Length,1);

                if (success)
                {
                    read(ACK_buff,NRF_ACK_Length);
                    Serial.println((char*)ACK_buff);
                    if (memcmp(ACK_buff,NRF_ACK_Phase_II,2)==0)
                    {
                        StartListening();
                        printDetails();
                        return WORKING;
                    }
                    else{
                        return EnterState(PAIRING_I_RX);
                    }

                }
                else
                {

                    return EnterState(PAIRING_I_RX);

                }
            }
            else{
                return EnterState(PAIRING_I_RX);
            }
            break;
        }
    case PAIRING_II_RX:
        {
            uint16_t cmd = bytes2int64((uint8_t*)RX_buff, 2);
            if (cmd == NRF_CMD_Pair_II)
            {
                for(int i = 0; i < NRF_Address_Length; i++) //write remote address into the EEPROM
                {
                    EEPROM.write(Remote_Address_Address + i, RX_buff[2 + i]);
                }
                EEPROM.write(Remote_Address_Existed_Address, NRF_Address_Existed);
                RemoteAddress = bytes2int64((uint8_t*)(RX_buff + 2), NRF_Address_Length);


                for(int i = 0; i < ID_Length; i++) //write remote ID into the EEPROM
                {
                    EEPROM.write(Remote_ID_Address + i, RX_buff[7 + i]);
                    Remote_ID[i] = RX_buff[7 + i];
                }
                EEPROM.write(Remote_ID_Existed_Address, ID_Existed);
                StartListening();
                printDetails();
                return WORKING;

            }
            else

            {

                return EnterState(PAIRING_I_RX);
            }

            break;
        }
    }
}

generalStates NRF24E::CheckState(){
    return GeneralStates;
}

uint64_t NRF24E::bytes2int64(uint8_t* data, uint8_t len)
{
    uint64_t result = 0x0, i = 0;
    for (; i < len; i++)
    {
        result = result << 8;
        result |= data[i];
    }
    return result;
}
void NRF24E::int64Tobytes(uint8_t* to, uint8_t len, uint64_t data)
{
    //this function trims the first 4x zeros in data
    uint8_t* d = (uint8_t*)&data;
    int i;
    for (i = 0; i < 8; i++)
    {
        if (d[i] != 0x00) break;
    }
    //i=pos;
    //i--;
    int j = len - 1;

    for (; i >= 0 && j >= 0; i++, j--)
    {
        to[j] = d[i];
    }


}

uint8_t NRF24E::StartPairing(){
    EventState=Idle;
    QuickConfig();
    flush_tx();
    writeAckPayload(0,(void*)NRF_ACK_Phase_I,2);
    flush_rx();
    uint32_t TotalTimeStamp=millis();
    GeneralStates=PAIRING_I_RX;
    while (GeneralStates!=WORKING && (millis()-TotalTimeStamp)<20000){
        switch(EventState)
        {

        case NRF_Rx:
            {
                EventState = Idle;
                Serial.println("Receive something!");
                //preprocess,extract data
                read((uint8_t*)RX_buff, NRF_Payload_Length);
                Serial.println((char*)RX_buff);
                //radio.startListening();
                switch (GeneralStates)
                {
                case PAIRING_I_RX:
                    {
                        Serial.println("IDLING!");

                        GeneralStates = EnterState(PAIRING_II_TX);
                        break;

                    }

                case PAIRING_II_RX:
                    {

                        GeneralStates = EnterState(PAIRING_II_RX);
                        break;

                    }

                }
                break;
            }
        case NRF_Tx:
            {
                EventState = Idle;
                Serial.println("Sent something!");
                break;
            }
        case Idle:
            {
                switch (GeneralStates)
                {
                case PAIRING_I_RX:

                    {
                        if (millis() - pairingTimeStamp > 1000)
                        {
                            bool coin = random(2);
                            if (coin)
                            {
                                pairingTimeStamp = millis();
                                Serial.println("continue waiting!");
                            }
                            else
                            {
                                Serial.println("go actively!");
                                GeneralStates = EnterState(PAIRING_I_TX);
                            }

                        }
                        break;
                    }
                case PAIRING_II_RX:

                    {
                        if (millis() - pairingTimeStamp > 3000)
                        {
                            GeneralStates = EnterState(PAIRING_I_RX);
                            pairingTimeStamp = millis();
                        }
                        break;
                    }
                }
            }
        }
        delay(5);
    }
    toggle_features();
    StartListening();
}

void NRF24E::InterruptService()
{
    bool rx, tx, fail;
    this->whatHappened(tx, fail, rx);
    if (GeneralStates==WORKING)
    {
        if (rx)
        {
            this->EventState = NRF_Rx;
            (*rxInterruptFunction)((void*)this);
            this->EventState= Idle;
            return;
        }
        else if (tx)
        {
            this->EventState = NRF_Tx;
            (*txInterruptFunction)((void*)this);
            this->EventState=Idle;
            return;
        }
        else if (fail)
        {
            this->EventState = NRF_Fail;
            (*failInterruptFunction)((void*)this);
            this->EventState=Idle;
            return;
        }
    }
    else
    {
        if (rx)
        {
            this->EventState = NRF_Rx;
            return;
        }
        else if (tx)
        {
            this->EventState = NRF_Tx;
        }
        else if (fail)
        {
            this->EventState = NRF_Fail;
        }
    }
    return;
}

void NRF24E::QuickConfig(){
    enableAckPayload();
}
void NRF24E::TurnOffACKPayload(){
    write_register(FEATURE,0);
    if ( read_register(FEATURE) )
    {
        // So enable them and try again
        toggle_features();
        write_register(FEATURE,0);
    }
}
