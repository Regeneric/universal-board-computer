//  ​Universal Board Computer for cars with electronic MPI
//  Copyright © 2021 IT Crowd, Hubert "hkk" Batkiewicz
// 
//  This file is part of UBC.
//  UBC is free software: you can redistribute it and/or modify
//  ​it under the terms of the GNU Affero General Public License as
//  published by the Free Software Foundation, either version 3 of the
//  ​License, or (at your option) any later version.
// 
//  ​This program is distributed in the hope that it will be useful,
//  ​but WITHOUT ANY WARRANTY; without even the implied warranty of
//  ​MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//  See the ​GNU Affero General Public License for more details.
// 
//  ​You should have received a copy of the GNU Affero General Public License
//  ​along with this program.  If not, see <https://www.gnu.org/licenses/>

// <https://itcrowd.net.pl/>


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