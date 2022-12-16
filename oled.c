#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include <stdlib.h>
#include <string.h>

#include "oled.h"


byte ssd1327_buf[1024] = {

};  


static void initSPI() {
    MOSI_DDR |= MOSI;

    SCK_DDR  |= SCK;
    SCK_PORT |= SCK;

    #if USE_RST == 1
        RST_DDR  |= RST;
        RST_PORT |= RST;
    #endif

    DC_DDR |= DC;

    #if USE_CS == 1
        CS_DDR  |= CS;
        CS_PORT |= CS;
    #endif
}

static void writeSPI(byte data) {
    for(byte i = 0x80; i; i >>= 1) {
        SCK_LO;
        if(data & i) MOSI_HI;
        else MOSI_LO;
        SCK_HI;
    } return;
}


void sendCMD(byte cmd) {
    #if USE_CS == 1 
        CS_HI;
    #endif

    DC_LO;
    #if USE_CS == 1 
        CS_LO;
    #endif

    writeSPI(cmd);
    #if USE_CS == 1
        CS_HI;
    #endif
}

void sendData(byte data) {
    #if USE_CS == 1 
        CS_HI;
    #endif

    DC_HI;
    #if USE_CS == 1 
        CS_LO;
    #endif

    writeSPI(data);
    #if USE_CS == 1 
        CS_HI;
    #endif
}


void oledInit(byte vcc, byte refresh) {
    initSPI();

    #if USE_RST == 1
        RST_HI;
        _delay_ms(25);
        RST_LO;
        _delay_ms(25);
        RST_HI;
    #else
        CS_HI;
        _delay_ms(25);
        CS_LO;
        _delay_ms(25);
        CS_HI;
    #endif
    
    
}