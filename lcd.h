//  ​Universal Board Computer for cars with electronic MPI
//  Copyright © 2021 IT Crowd, Marcin Kraśnicki
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


#ifndef LCD_H
#define LCD_H

#include <avr/pgmspace.h>
#include <stdint.h>

// LCD's port
#define PORT_LCD PORTB
#define DDR_LCD DDRB

// LCD's pins
#define LCD_RST PB5  // RST
#define LCD_SCE PB4  // CS
#define LCD_DC  PB3  // D/C
#define LCD_DIN PB2  // DIN
#define LCD_CLK PB1  // CLK

#define LCD_CONTRAST 0x40

struct lcdInterface {
    void (*init)(void);
    void (*clear)(void);
    // void (*power)(uint8_t on);
    // void (*setPixel)(uint8_t xPos, uint8_t yPos, uint8_t value);
    void (*sendc)(char code, uint8_t scale);
    void (*sends)(const char* word, uint8_t scale);
    void (*cursor)(uint8_t xPos, uint8_t yPos);
    void (*render)(void);
}; extern const struct lcdInterface LCD;

#endif  // LCD_H
