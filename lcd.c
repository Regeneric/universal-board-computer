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


#include "lcd.h"

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "chars.h"


static struct {
    uint8_t screen[504];

    uint8_t cursorX;
    uint8_t cursorY;

} screenLCD = {
    .cursorX = 0,
    .cursorY = 0
};


static void write(uint8_t bytes, uint8_t isData) {
	register uint8_t i;
	PORT_LCD &= ~(1<<LCD_SCE);  // Enable controller

	if(isData) PORT_LCD |= (1<<LCD_DC);  // Sending data
	else PORT_LCD &= ~(1<<LCD_DC);  // Sending commands

	for(i = 0; i != 8; ++i) {
		if((bytes >> (7-i)) & 0x01) PORT_LCD |= (1<<LCD_DIN);
		else PORT_LCD &= ~(1<<LCD_DIN);

		PORT_LCD |= (1<<LCD_CLK);
		PORT_LCD &= ~(1<<LCD_CLK);
	} PORT_LCD |= (1 << LCD_SCE);
}
static void writeCmd(uint8_t cmd) {write(cmd, 0);}
static void writeData(uint8_t data) {write(data, 1);}


void screenLCDInit(void) {
	register unsigned i;
	
    // Set pins as output
	DDR_LCD |= (1<<LCD_SCE);
	DDR_LCD |= (1<<LCD_RST);
	DDR_LCD |= (1<<LCD_DC);
	DDR_LCD |= (1<<LCD_DIN);
	DDR_LCD |= (1<<LCD_CLK);

	// Reset display
	PORT_LCD |= (1<<LCD_RST);
	PORT_LCD |= (1<<LCD_SCE);
	_delay_ms(10);
	PORT_LCD &= ~(1<<LCD_RST);
	_delay_ms(70);
	PORT_LCD |= (1<<LCD_RST);


	// Initialize display and enable controller
	PORT_LCD &= ~(1<<LCD_SCE);
	writeCmd(0x21);  // LCD Extended Commands mode
	writeCmd(0x13);  // LCD bias mode 1:48
	writeCmd(0x06);  // Set temperature coefficient
	writeCmd(0xC2);  // Default VOP (3.06 + 66 * 0.06 = 7V)
	writeCmd(0x20);  // Standard Commands mode, powered down
	writeCmd(0x09);  // LCD in normal mode

    // Clear LCD RAM
	writeCmd(0x80);
	writeCmd(LCD_CONTRAST);
	for(i = 0; i != 504; ++i) writeData(0x00);

	// Activate LCD
	writeCmd(0x08);
	writeCmd(0x0C);
}

void screenLCDClear(void) {
	register unsigned i;
    
	// Set column and row to 0
	writeCmd(0x80);
	writeCmd(0x40);

	// Cursor too
	screenLCD.cursorX = 0;
	screenLCD.cursorY = 0;
	
    // Clear everything (504 bytes = 84cols * 48rows / 8bits)
	for(i = 0; i != 504; ++i) screenLCD.screen[i] = 0x00;
}

void screenLCDPower(uint8_t on) {writeCmd(on ? 0x20 : 0x24);}

void screenLCDSetPixel(uint8_t x, uint8_t y, uint8_t value) {
	uint8_t *byte = &screenLCD.screen[y/8*84+x];
	if(value) *byte |= (1<<(y%8));
	else *byte &= ~(1<<(y%8));
}

void screenLCDWriteChar(char code, uint8_t scale) {
	register uint8_t x, y;

	for(x = 0; x != 5*scale; ++x)
		for (y = 0; y != 7*scale; ++y)
			if(pgm_read_byte(&CHARSET[code-32][x/scale]) & (1 << y/scale))
				screenLCDSetPixel(screenLCD.cursorX + x, screenLCD.cursorY + y, 1);
			else screenLCDSetPixel(screenLCD.cursorX + x, screenLCD.cursorY + y, 0);

	screenLCD.cursorX += 5*scale + 1;
	if(screenLCD.cursorX >= 84) {
		screenLCD.cursorX = 0;
		screenLCD.cursorY += 7*scale + 1;
	} 
    
    if(screenLCD.cursorY >= 48) {
		screenLCD.cursorX = 0;
		screenLCD.cursorY = 0;
	}
} void screenLCDWriteString(const char *str, uint8_t scale) {while(*str) screenLCDWriteChar(*str++, scale);}

void screenLCDSetCursor(uint8_t x, uint8_t y) {
	screenLCD.cursorX = x;
	screenLCD.cursorY = y;
}

void screenLCDRender(void) {
	register unsigned i;

	// Set column and row to 0
	writeCmd(0x80);
	writeCmd(0x40);

	// Write screen to display
	for(i = 0; i != 504; ++i) writeData(screenLCD.screen[i]);
}


const struct lcdInterface LCD = {
	.init = screenLCDInit,
	.clear = screenLCDClear,
	.power = screenLCDPower,
	.setPixel = screenLCDSetPixel,
	.sendc = screenLCDWriteChar,
	.sends = screenLCDWriteString,
	.cursor = screenLCDSetCursor,
	.render = screenLCDRender
};