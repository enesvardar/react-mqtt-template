#include "EEPROM.h"

#define EEPROM_SIZE 64

void initEprom(void)
{
    if (!EEPROM.begin(EEPROM_SIZE))
    {
        Serial.println("failed to init EEPROM");
        delay(1000);
    }
}

void writeEpromString(const char* str, int addr, int lenght)
{
    
    for (int i = 0; i < lenght; i++) {
        EEPROM.write(addr, str[i]);
        addr += 1;
    }

    EEPROM.commit();
}

String readEpromString(int addr, int lenght)
{
    String str = "";

    for (int i = 0; i < lenght; i++) {

        byte readValue = EEPROM.read(addr + i);

        if (readValue == 0 || readValue == 1) {
            break;
        }
        else{
            char readValueChar = char(readValue);
            str[i] = (readValueChar);
            str += readValueChar;
        }
    }

    return str;
}