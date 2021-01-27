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
    void (*power)(uint8_t on);
    void (*setPixel)(uint8_t xPos, uint8_t yPos, uint8_t value);
    void (*sendc)(char code, uint8_t scale);
    void (*sends)(const char* word, uint8_t scale);
    void (*cursor)(uint8_t xPos, uint8_t yPos);
    void (*render)(void);
}; extern const struct lcdInterface LCD;

#endif  // LCD_H
