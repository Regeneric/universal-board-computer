#ifndef OLED_H
#define OLED_H

#include "commons.h"

#define USE_CS  1
#define USE_RST 0


#define SCK  (1<<6)     // SCL
#define MOSI (1<<5)     // SDA
#define RST  (1<<4)     // RST
#define DC   (1<<3)     // D/C

#define CS   (1<<2)     // CS


#define SCK_PORT  PORTC
#define SCK_DDR   DDRC

#define MOSI_PORT PORTC
#define MOSI_DDR  DDRC

#define RST_PORT  PORTC
#define RST_DDR   DDRC

#define DC_PORT   PORTC 
#define DC_DDR    DDRC

#define CS_PORT   PORTC
#define CS_DDR    DDRC


#define RST_LO  RST_PORT &= ~RST
#define RST_HI  RST_PORT |= RST

#define CS_LO   CS_PORT &= ~CS
#define CS_HI   CS_PORT |= CS

#define DC_LO   DC_PORT &= ~DC
#define DC_HI   DC_PORT |= DC

#define SCK_LO  SCK_PORT &= ~SCK
#define SCK_HI  SCK_PORT |= SCK

#define MOSI_LO MOSI_PORT &= ~MOSI
#define MOSI_HI MOSI_PORT |= MOSI


#define SSD1327_128_128
#define SSD1351_128_128

#define SSD1327_WIDTH   128
#define SSD1327_HEIGTH  128

#define SSD1351_WIDTH   128
#define SSD1351_HEIGHT  128


#define REFRESH_MIN 0x80
#define REFRESH_MID 0xB0
#define REFRESH_MAX 0xF0


// SSD1327
#define SSD1327_BLACK 0x00
#define SSD1327_WHITE 0xF0

#define SSD1327_SETLOWCOLUMN                            0x00
#define SSD1327_EXTERNALVCC                             0x01
#define SSD1327_SWITCHCAPVCC                            0x02
#define SSD1327_SETHIGHCOLUMN                           0x10
#define SSD1327_SETCOLUMN                               0x15
#define SSD1327_MEMORYMODE                              0x20
#define SSD1327_RIGHT_HORIZONTAL_SCROLL                 0x26
#define SSD1327_LEFT_HORIZONTAL_SCROLL                  0x27
#define SSD1327_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL    0x29
#define SSD1327_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL     0x2A
#define SSD1327_DEACTIVATE_SCROLL                       0x2E
#define SSD1327_ACTIVATE_SCROLL                         0x2F
#define SSD1327_SETSTARTLINE                            0x40
#define SSD1327_SETROW                                  0x75
#define SSD1327_SETCONTRAST                             0x81
#define SSD1327_SETBRIGHTNESS                           0x82
#define SSD1327_CHARGEPUMP                              0x8D
#define SSD1327_SETLUT                                  0x91
#define SSD1327_SEGREMAP                                0xA0
#define SSD1327_SETSTARTLINE                            0xA1
#define SSD1327_SETDISPLAYOFFSET                        0xA2
#define SSD1327_SET_VERTICAL_SCROLL                     0xA3
#define SSD1327_NORMALDISPLAY                           0xA4
#define SSD1327_DISPLAYALLON                            0xA5
#define SSD1327_DISPLAYALLOFF                           0xA6
#define SSD1327_INVERTDISPLAY                           0xA7
#define SSD1327_SETMULTIPLEX                            0xA8
#define SSD1327_REGULATOR                               0xAB
#define SSD1327_DISPLAYOFF                              0xAE
#define SSD1327_DISPLAYON                               0xAF
#define SSD1327_PHASELEN                                0xB1
#define SSD1327_DCLK                                    0xB3
#define SSD1327_PRECHARGE2                              0xB6
#define SSD1327_GRAYTABLE                               0xB8
#define SSD1327_PRECHARGE                               0xBC
#define SSD1327_SETVCOM                                 0xBE
#define SSD1327_COMSCANINC                              0xC0
#define SSD1327_COMSCANDEC                              0xC8
#define SSD1327_SETDISPLAYOFFSET                        0xD3
#define SSD1327_FUNCSELB                                0xD5
#define SSD1327_SETCOMPINS                              0xDA
#define SSD1327_SETVCOMDETECT                           0xDB
#define SSD1327_SERPRECHARGE                            0xD9
#define SSD1327_CMDLOCK                                 0xFD


void oledInit(byte vcc, byte refresh);
void sendData(byte data);
void sendCMD(byte cmd);

#endif