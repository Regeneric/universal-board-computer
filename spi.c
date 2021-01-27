#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <util/delay.h>

#include "spi.h"

void initSPI() {
    // Set output pins for data send
    DDRB |= MOSI|LT|SCK;

    // Set up SPI interface and clock
    SPCR |= (1<<SPE)|(1<<MSTR);
    SPSR |= (1<<SPI2X);

    // Clear SPI before sending proper data
    SPI.send(0x00);    
    return;
}

void sendSPI(uint8_t data) {
    SPDR = data;
    while(!(SPSR & (1<<SPIF)));

    LT_ON;
    LT_OFF;
    return;
} 

const struct spiInterface SPI = {
    .init = initSPI,
    .send = sendSPI
};