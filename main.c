#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "lcd.h"
#include "ftoa.h"
#include "millis.h"


volatile static const float PULSE_DISTANCE = 0.00006823;
volatile static const float INJECTION_VALUE = 0.0031667;

volatile float traveledDistance = .0, displayedTraveledDistance = .0, injectorOpenTime = .0, usedFuel = .0, tankCapacity = 68, instantFuelConsumption = .0, displayInstantFuelConsumption = .0;
volatile float averageFuelConsumption = .0, fuelSumInv = .0;
volatile uint8_t toBeSaved = 0, mode = 1;
volatile uint8_t speed = 0, displaySpeed = 0, avgSpeedCount = 0, maxSpeedCount = 0, fcCounter = 0, saveCounter = 0, i = 0;
volatile uint16_t counter = 0, distPulseCount = 0, tempDistPulseCount = 0, injectorPulseTime = 0, injTimeHigh = 0, injTimeLow = 0, tempInjectorPulseTime = 0, saveNumber = 0;
volatile float avgSpeedDivider = .0, sumInv = .0;



// Structure stored in EEPROM
typedef struct {
    float eeAverageFuelConsumption;
    float eeTraveledDistance;
    float eeUsedFuel;
    uint8_t eeAverageSpeedCount;
    uint8_t eeMaxSpeedCount;
    uint16_t eeSaveNumber;  
    uint32_t eeAvgSpeedDivider;
} eeStruct;
eeStruct EEMEM eeSavedData;



void maxSpeed();
void avgSpeed();
void currentSpeed();

void fuelConsumption();

void saveData();
void loadData();


int main() {
    // VSS signal, injector signal and navigation buttons - Atmega 8
    DDRD &= ~0x8C;  // PD2/INT0, PD3/INT1 and PD7 as input
    PORTD = 0x8C;  // PD2/INT0, PD3/INT1 and PD7 internal pull-up resistor

    MCUCR = 0x6;  // ANY change of state of INT1 generates interrupt  (1<<ISC10)
    GICR = 0xC0;  // Turns on INT0 and INT1  ((1<<INT1) | (1<<INT0))

    // 16 bit timer for VSS
    TCCR1A = 0;  // Timer1 normal mode
    TCCR1B = 0x3;  // Prescaler 64, causes overflow every 0.262144s (8 MHz crystal)  ((1<<CS10) | (1<<CS11))
    TCNT1 = 34286;  // Counts from 34286 to 65535 - causes overflow every 0.25s (8 MHz crystal, 3036 for 16 MHz)

    // Counter for millis() function
    TCCR0 = 0x3;  // Prescaler 64, causes overflow every 0.262144s (8 MHz crystal)  ((1<<CS01) | (1<<CS00))
    TCNT0 = 0;  // Counts from 0 to 255;

    // Enable timer overflow interrupt  ((1<<TOIE1) | (1<<TOIE0))
    TIMSK = 0x5;


    // Button debouncing 
	uint8_t i = 0;              // counter variable
	uint8_t buttonPressed = 0;  // keeps track if button is pressed, 0 false, 1 true

    loadData();
    char buffer[8];
    LCD.init();

    sei();
    while(1) {
        if(!(PIND & (1<<PD7)) && !buttonPressed) {
            if(i < 15) i++;
            else i = 0;

            PORTD = i | (1<<PD7);
            buttonPressed = 1;

            mode++;
            if(mode > 6) mode = 1;
        } else if((PIND & (1<<PD7)) && buttonPressed) {
            buttonPressed = 0;
            PORTD ^= (1<<PD7);
        }
        

        char res[24];
        switch(mode) {
            case 1: 
            // Instant fuel consumption
                FTOA.convert(displayInstantFuelConsumption, res, 1);

                LCD.cursor(1, 7);
                if(displayInstantFuelConsumption > 99 || displayInstantFuelConsumption <= 0) LCD.sends("--.-", 2);
                else LCD.sends(res, 2);

                LCD.cursor(54, 11);
                if(speed > 3) LCD.sends("L/100", 1);
                else LCD.sends("L/H", 1);

                LCD.cursor(6, 30);
                LCD.sends("INSTANT FUEL", 1);
                LCD.cursor(3, 39);
                LCD.sends(" CONSUMPTION", 1);
            break;

            case 2: 
            // Average fuel consumption
                FTOA.convert(averageFuelConsumption, res, 1);

                LCD.cursor(1, 7); 
                if(averageFuelConsumption > 99 || averageFuelConsumption <= 0) LCD.sends("--.-", 2);
                else LCD.sends(res, 2);
                LCD.cursor(54, 11); LCD.sends("L/100", 1);

                LCD.cursor(6, 30);
                LCD.sends("AVERAGE FUEL", 1);
                LCD.cursor(3, 39);
                LCD.sends(" CONSUMPTION", 1);
            break;

            case 3: 
            // Current speed
                FTOA.convert(displaySpeed, res, 2);

                LCD.cursor(6, 7); 
                if(displaySpeed <= 0) LCD.sends("--", 2);
                else if(displaySpeed < 1 && displaySpeed > 0) {LCD.sends("0", 2); LCD.sends(itoa(displaySpeed, buffer, 10), 2);}
                else LCD.sends(itoa(displaySpeed, buffer, 10), 2);
                LCD.cursor(54, 11); LCD.sends("KM/H", 1);

                LCD.cursor(4, 30);
                LCD.sends("CURRENT SPEED", 1);
            break;

            case 4:
            // Average speed
                LCD.cursor(6, 7); 
                if(avgSpeedCount <= 0) LCD.sends("--", 2);
                else LCD.sends(itoa(avgSpeedCount, buffer, 10), 2);
                LCD.cursor(54, 11); LCD.sends("KM/H", 1);

                LCD.cursor(4, 30);
                LCD.sends("AVERAGE SPEED", 1);
            break;

            case 5:
            // Used fuel
                FTOA.convert(usedFuel, res, 2);

                LCD.cursor(1, 7); 
                if(usedFuel <= 0) LCD.sends("--.-", 2);
                else if(usedFuel < 1 && usedFuel > 0) {LCD.sends("0", 2); LCD.sends(res, 2);}
                else LCD.sends(res, 2);
                LCD.cursor(72, 11); LCD.sends("L", 1);

                LCD.cursor(15, 30);
                LCD.sends("USED FUEL", 1);
            break;

            case 6:
            // Traveled distance
                FTOA.convert(displayedTraveledDistance, res, 2);

                LCD.cursor(1, 7); 
                if(displayedTraveledDistance <= 0) LCD.sends("--", 2);
                else LCD.sends(res, 2);
                LCD.cursor(72, 11); LCD.sends("KM", 1);

                LCD.cursor(18, 30);
                LCD.sends("TRAVELED", 1);
                LCD.cursor(18, 39);
                LCD.sends("DISTANCE", 1);
            break;

            default: 
                LCD.cursor(1, 1); LCD.sends("MODE NOT SET", 1);
                LCD.cursor(1, 9); LCD.sends("MODE: ", 1); LCD.sends(itoa(mode, buffer, 10), 1);
            break;
        }

        LCD.render();
        LCD.clear();
    } return 0;
}



ISR(TIMER1_OVF_vect) {
    counter++;

    if(tempInjectorPulseTime < 800 && tempDistPulseCount == 0 && toBeSaved == 1) saveData();
    if(saveCounter > 45 && speed == 0 && toBeSaved == 1) saveData();

    tempInjectorPulseTime = 0;
    tempDistPulseCount = 0;

    if(counter > 3) {
        currentSpeed();
        fuelConsumption();

        if(instantFuelConsumption > 1.0) toBeSaved = 1;
        if(speed > 5) {
            avgSpeedDivider++;
            avgSpeed();
            maxSpeed();
        } 
        
        saveCounter++;
        distPulseCount = 0;
        injectorPulseTime = 0;
        counter = 0;

        displayInstantFuelConsumption = instantFuelConsumption;
        displayedTraveledDistance = traveledDistance;
        displaySpeed = speed;
    } TCNT1 = 34286;
}


// VSS signal interrupt
ISR(INT0_vect) {
    distPulseCount++;
    traveledDistance += PULSE_DISTANCE;
    tempDistPulseCount++;
}

// Injector signal interrupt 
ISR(INT1_vect) {
    if(!(PIND & (1<<PD3))) injTimeLow = millis();  // Low state on PD3
    else {
        // High state on PD3
        injTimeHigh = millis();
        injectorPulseTime = injectorPulseTime + (injTimeHigh - injTimeLow);
        tempInjectorPulseTime = tempInjectorPulseTime + (injTimeHigh - injTimeLow);
        PORTD |= (1<<PD3);
    }
}



void maxSpeed() {
    if(speed > maxSpeedCount) maxSpeedCount = speed;
}

void avgSpeed() {
    // Harmonic 
    sumInv += 1.0f/speed;
    avgSpeedCount = avgSpeedDivider/sumInv;
}

void currentSpeed() {
    speed = PULSE_DISTANCE * distPulseCount * 3600;
}


void fuelConsumption() {
    injectorOpenTime = ((float)injectorPulseTime/1000);  // Converting to seconds
    if(speed > 5) {
        instantFuelConsumption = (100 * ((injectorOpenTime * INJECTION_VALUE) * 3600 * 4))/speed;

        // Harmonic 
        if(instantFuelConsumption > 0) {
            fuelSumInv += 1.0f/instantFuelConsumption;
            averageFuelConsumption = avgSpeedDivider/fuelSumInv;
        }

    } else instantFuelConsumption = ((injectorOpenTime * INJECTION_VALUE) * 3600 * 4);
    
    usedFuel += ((injectorOpenTime * INJECTION_VALUE) * 4);
    if(tankCapacity > 0) tankCapacity = 68 - usedFuel;
}



void saveData() {
    cli();

    eeprom_update_float(&eeSavedData.eeTraveledDistance, traveledDistance);

    eeprom_update_float(&eeSavedData.eeAverageFuelConsumption, averageFuelConsumption);
    eeprom_update_float(&eeSavedData.eeUsedFuel, usedFuel);

    eeprom_update_byte(&eeSavedData.eeMaxSpeedCount, maxSpeedCount);
    eeprom_update_byte(&eeSavedData.eeAverageSpeedCount, avgSpeedCount);
    eeprom_update_float(&eeSavedData.eeAvgSpeedDivider, avgSpeedDivider);

    eeprom_update_word(&eeSavedData.eeSaveNumber, saveNumber);

    saveCounter = 0;
    toBeSaved = 0;

    sei();
}

void loadData() {
    traveledDistance = isnan(eeprom_read_float(&eeSavedData.eeTraveledDistance)) ? 0 : eeprom_read_float(&eeSavedData.eeTraveledDistance);

    averageFuelConsumption = isnan(eeprom_read_float(&eeSavedData.eeAverageFuelConsumption)) ? 0 : eeprom_read_float(&eeSavedData.eeAverageFuelConsumption);
    usedFuel = isnan(eeprom_read_float(&eeSavedData.eeUsedFuel)) ? 0 : eeprom_read_float(&eeSavedData.eeUsedFuel);

    maxSpeedCount = isnan(eeprom_read_byte(&eeSavedData.eeMaxSpeedCount)) ? 0 : eeprom_read_byte(&eeSavedData.eeMaxSpeedCount);
    avgSpeedCount = isnan(eeprom_read_byte(&eeSavedData.eeAverageSpeedCount)) ? 0 : eeprom_read_byte(&eeSavedData.eeAverageSpeedCount);
    avgSpeedDivider = isnan(eeprom_read_float(&eeSavedData.eeAvgSpeedDivider)) ? 0 : eeprom_read_float(&eeSavedData.eeAvgSpeedDivider);

    saveNumber = isnan(eeprom_read_word(&eeSavedData.eeSaveNumber)) ? 0 : eeprom_read_word(&eeSavedData.eeSaveNumber);
}