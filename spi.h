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