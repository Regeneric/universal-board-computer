#define clockCyclesToMicroseconds(a) (((a) * 1000L)/(F_CPU / 1000L))
#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(64 * 256))
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)
#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000)>>3)
#define FRACT_MAX (1000>>3)

unsigned long int millis();