#include <avr/interrupt.h>
#include <stdlib.h>

#include "millis.h"

volatile unsigned long int timer0_overflowCount = 0;
volatile unsigned long int timer0_millis = 0;
static unsigned char timer0_fract = 0;
unsigned long int starttime = 0, endtime = 0;

unsigned long int millis() {
    unsigned long int m = 0;
    uint8_t oldSREG = SREG;

    // Disable interrupts while we read timer0_millis or we might get an
    // Inconsistent value (e.g. in the middle of a write to timer0_millis)
    cli();
    m = timer0_millis;
    SREG = oldSREG;
    sei();
    return m;
}

ISR(TIMER0_OVF_vect) {
    unsigned long int m = timer0_millis;
    unsigned char f = timer0_fract;

    m += MILLIS_INC;
    f += FRACT_INC;
    if(f >= FRACT_MAX) {
        f -= FRACT_MAX;
        m += 1;
    }

    timer0_fract = f;
    timer0_millis = m;
    timer0_overflowCount++;
}