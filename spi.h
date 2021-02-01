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


#ifndef SPI_H
#define SPI_H

#define MOSI (1<<PB5)
#define LT (1<<PB4)
#define SCK (1<<PB3)

#define LT_ON PORTB |= LT
#define LT_OFF PORTB &= ~LT
#define LT_TOG PORTB ^= LT

struct spiInterface {
    void (*init)(void);
    void (*send)(uint8_t data);
}; extern const struct spiInterface SPI;

#endif  // SPI_H