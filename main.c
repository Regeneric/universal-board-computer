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
#include <avr/eeprom.h>

#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "lcd.h"
#include "ftoa.h"
#include "millis.h"



static float PULSE_DISTANCE = .0f;  // 0.00006823
static float INJECTION_VALUE = .0f;  // 0.0025f - based on value that injector can inject 149.8 cc/min of fuel


volatile static float avgSpeedDivider = .0, traveledDistance = .0, sailingDistance = .0, sumInv = .0, accTime = .0;
volatile static float instantFuelConsumption = .0, averageFuelConsumption = .0, usedFuel = .0, fuelLeft = .0, fuelSumInv = .0, divideFuelFactor = .0, savedFuel = .0;
volatile static float injectorOpenTime = .0, ccMin = 100.0;

volatile static uint8_t saveCounter = 60, btnCnt = 24, calibrationFlag = 0, pulseOverflows = 0, mode = 3, accBuffer = 0;
volatile static uint8_t speed = 0, avgSpeedCount = 0;

volatile static unsigned int counter = 4, distPulseCount = 0, injectorPulseTime = 0, injTimeHigh = 0, injTimeLow = 0, rangeDistance = 0;  // uint16_t


// Structure stored in EEPROM
typedef struct {
    float eeAverageFuelConsumption;
    float eeTraveledDistance;
    float eeUsedFuel;
    float eeSavedFuel;
    float eeDivideFuelFactor;
    float eeAvgSpeedDivider;
    float eePulseDistance;
    float eeSumInv;
    float eeFuelSumInv;
    float eeInjectionValue;
    uint8_t eeAverageSpeedCount;  
} eeStruct; eeStruct EEMEM eeSavedData;


static void avgSpeed();
static void currentSpeed();

static void fuelConsumption();

static void saveData();
static void loadData();

int main() {
    // I use hex values 'cause they take less space in the output file. 
    // For now, (07.02.2021, commit on GH no. 6adcdef, as I write this) there are 224 bytes of free space in flash memory.
    // Yes, bytes. And still there is no built-in calibration method. 
    // I did it! It's not the way I'd preffer it to be but, honestly, I don't know how to fit it all better in this space (15.02.2021)
    // Today is the day when I'm ready to do some coding for 328 (25.02.2021)


    // ADC for checking fuel level
    DDRC &= ~(1<<PC5);  // PC5/ADC as input

    ADCSRA |= ((1<<ADEN) | (1<<ADPS2) | (1<<ADPS1));  // Enables ADC and sets prescaller to 64 (8 MHz, 128 for 16 MHz)
    ADMUX |= ((1<<REFS1) | (1<<REFS0));  // Internal 1.1V ref voltage with external capacitor
    ADMUX |= ((1<<MUX2) | (1<<MUX0));    // Input channel select


    // VSS signal and injector signal - Atmega 328
    DDRD &= ~((1<<PD3) | (1<<PD2));  // PD3/INT1 and PD2/INT0 as input
    PORTD |= ((1<<PD3) | (1<<PD2));  // PD2/INT0 and PD3/INT1internal pull-up resistor to avoid high impedance state of the pin

    EICRA |= (1<<ISC01);  // The FALLING edge on INT0 generates interrupt
    EICRA |= (1<<ISC10);  // ANY change of state of INT1 generates interrupt
    EIMSK |= ((1<<INT0) | (1<<INT1));  // Turns on INT0 and INT1
    

    // Navigation buttons = Atmega 328
    DDRD &= ~((1<<PD7) | (1<<PD6));  // PD7/PCINT23 and PD6 as input    
    PORTD |= ((1<<PD7) | (1<<PD6));  // PD7/PCINT23 and PD6 internal pull-up resistor

    DDRB &= ~(1<<PB0);  // PB0/PCINT0 as input
    PORTB |= (1<<PB0);  // PB0/PCINT0 internal pull-up resistor

    PCICR |= ((1<<PCIE0) | (1<<PCIE2));  // Set PCIE0 and PCIE2 to enable PCMSK0 and PCMSK2 scan
    PCIFR |= ((1<<PCIF0) | (1<<PCIF2));  // Set PCIF0 and PCIF2 to enable interrupt flasg mask
    PCMSK0 |= (1<<PCINT0);   // Set PCINT0 to trigger an interrupt on state change 
    PCMSK2 |= (1<<PCINT23);  // Set PCINT23 to trigger an interrupt on state change


    // 16 bit timer for VSS
    TCCR1A = 0;                         // Timer1 normal mode
    TCCR1B |= ((1<<CS10) | (1<<CS11));  // Prescaler 64, causes overflow every 0.524288s (8 MHz crystal, 0.262144s for 16 MHz)
    TIMSK1 |= (1<<TOIE1);               // Enable timer overflow interrupt
    TCNT1 = 34286;                      // Counts from 34286 to 65535 - causes overflow every 0.25s (8 MHz crystal, 3036 for 16 MHz)

    // Counter for millis() function
    TCCR0B |= ((1<<CS01) | (1<<CS00));  // Prescaler 64
    TIMSK0 = (1<<TOIE0);                // Enable timer overflow interrupt
    TCNT0 = 0;                          // Counts from 0 to 255;


    // Button debouncing, mode change 
	uint8_t i = 15;             // Counter and mode iterators
	uint8_t buttonPressed = 0;  // Keeps track if button is pressed; 0 false, 1 true

    loadData();              // Loads data from EEPROM
    char buffer[8], res[8];  // Buffer for itoa() function
    LCD.init();

    sei();  // Global interrupts enabled
    while(1) {
        if(fuelLeft <= 0) {
            // ADC checks for level fuel in the tank
            ADCSRA |= (1<<ADSC);
            while(ADCSRA & (1<<ADSC));
            fuelLeft = (ADC > 0 
                            ? (divideFuelFactor > 0 ? ADC/divideFuelFactor : 0)
                            : savedFuel-usedFuel);
        }
        
        if(!(PIND & (1<<PD6)) && !buttonPressed) {
            while(i != 0) --i;
            i = 15;

            PORTD = i | (1<<PD6);
            buttonPressed = 1;

            if(calibrationFlag == 1 && mode == 2) {
                divideFuelFactor += 0.5f;
                eeprom_update_float(&eeSavedData.eeDivideFuelFactor, divideFuelFactor);
            }
        } else if((PIND & (1<<PD6)) && buttonPressed) {
            buttonPressed = 0;
            PORTD ^= (1<<PD6);
        } 
        
        if(!calibrationFlag) {
            switch(mode) {
                case 5:
                // Fuel infoscreen

                    // Used fuel
                    ftoa(usedFuel, res, 2);
                    LCD.cursor(1, 1); LCD.sends("&$  ", 1);  // Range symbol
                    if(usedFuel <= 0) LCD.sends("--.-", 1);
                    else if(usedFuel < 1 && usedFuel > 0) {LCD.sends("0", 1); LCD.sends(res, 1);}
                    else LCD.sends(res, 1);
                    LCD.cursor(65, 1); LCD.sends(" L", 1);
                    LCD.cursor(1, 9); LCD.sends("--------------", 1);
                    
                    // Traveled distance without burning fuel
                    ftoa(sailingDistance, res, 1);   
                    LCD.cursor(1, 17);
                    if(sailingDistance <= 0) LCD.sends("--", 2);
                    else if(sailingDistance > 0 && sailingDistance < 1) {LCD.sendc('0', 2); LCD.sends(res, 2);}
                    else LCD.sends(res, 2);
                    LCD.cursor(55, 22); LCD.sends("KM(S)", 1);

                    // Left fuel
                    ftoa(fuelLeft, res, 0); 
                    LCD.cursor(1, 32); LCD.sends("--------------", 1);
                    LCD.cursor(5, 40); LCD.sends("!\"   ", 1);  // Fuel distributor symbol
                    LCD.sends(res, 1); 
                    LCD.cursor(70, 40); LCD.sends("L", 1);
                break;

                case 4:
                // Speed infoscreen
                    ftoa(speed, res, 2);

                    // Current speed  
                    LCD.cursor(6, 7); 
                    if(speed <= 0) LCD.sends("--", 2);
                    else if(speed < 1 && speed > 0) {LCD.sends("0", 2); LCD.sends(itoa(speed, buffer, 10), 2);}
                    else LCD.sends(itoa(speed, buffer, 10), 2);
                    LCD.cursor(56, 11); LCD.sends("KM/H", 1);

                    // Acceleration time
                    LCD.cursor(1, 32); LCD.sends("--------------", 1);
                    LCD.cursor(5, 40); LCD.sends("   ", 1);
                    ftoa(accTime, res, 1);
                    if(accTime <= 0) LCD.sends("--.-", 1);
                    else LCD.sends(res, 1);
                    LCD.cursor(56, 40); LCD.sends("  S", 1);
                break;

                case 3: 
                // Main Screen                    
                    // Range
                    LCD.cursor(1, 1); LCD.sends("$%& ", 1);  // Range symbol
                    if(rangeDistance > 100) {LCD.sendc(' ', 1); LCD.sends(itoa(rangeDistance, buffer, 10), 1);}
                    else {
                        LCD.sends("-(", 1);
                        LCD.sends(itoa(rangeDistance, buffer, 10), 1);
                        LCD.sends(")-", 1);
                    }
                    LCD.cursor(65, 1); LCD.sends(" KM", 1);
                    LCD.cursor(1, 9); LCD.sends("--------------", 1);
                    
                    // Instant fuel consumption
                    ftoa(instantFuelConsumption, res, 1);   
                    LCD.cursor(1, 17);
                    if(instantFuelConsumption > 99 || instantFuelConsumption <= 0) LCD.sends("--.-", 2);
                    else LCD.sends(res, 2);

                    LCD.cursor(54, 22);
                    if(speed > 5) LCD.sends("L/100", 1);  // Car is moving
                    else LCD.sends("L/H", 1);  // Car is not moving

                    // Average fuel consumption
                    LCD.cursor(1, 32); LCD.sends("--------------", 1);
                    LCD.cursor(5, 40); LCD.sends("#  ", 1);  // Average fuel consumption symbol
                    
                    ftoa(averageFuelConsumption, res, 1);
                    if(averageFuelConsumption <= 0) LCD.sends("--.-", 1);
                    else LCD.sends(res, 1); 
                    LCD.cursor(54, 40); LCD.sends("L/100", 1);
                break;

                case 2: 
                // Speed infoscreen
                    ftoa(speed, res, 2);

                    // Current speed  
                    LCD.cursor(6, 7); 
                    if(speed <= 0) LCD.sends("--", 2);
                    else if(speed < 1 && speed > 0) {LCD.sends("0", 2); LCD.sends(itoa(speed, buffer, 10), 2);}
                    else LCD.sends(itoa(speed, buffer, 10), 2);
                    LCD.cursor(56, 11); LCD.sends("KM/H", 1);

                    // Average speed
                    LCD.cursor(1, 32); LCD.sends("--------------", 1);
                    LCD.cursor(5, 40); LCD.sends("#  ", 1);  // Average speed symbol
                    if(avgSpeedCount <= 0) LCD.sends("--", 1);
                    else LCD.sends(itoa(avgSpeedCount, buffer, 10), 1);
                    LCD.cursor(56, 40); LCD.sends("KM/H", 1);
                break;


                case 1:
                // Fuel infoscreen

                    // Used fuel
                    ftoa(usedFuel, res, 2);
                    LCD.cursor(1, 1); LCD.sends("&$  ", 1);  // Range symbol
                    if(usedFuel <= 0) LCD.sends("--.-", 1);
                    else if(usedFuel < 1 && usedFuel > 0) {LCD.sends("0", 1); LCD.sends(res, 1);}
                    else LCD.sends(res, 1);
                    LCD.cursor(65, 1); LCD.sends(" L", 1);
                    LCD.cursor(1, 9); LCD.sends("--------------", 1);
                    
                    // Traveled distance
                    ftoa(traveledDistance, res, 1);   
                    LCD.cursor(1, 17);
                    if(traveledDistance <= 0) LCD.sends("--", 2);
                    else if(traveledDistance > 0 && traveledDistance < 1) {LCD.sendc('0', 2); LCD.sends(res, 2);}
                    else LCD.sends(res, 2);
                    LCD.cursor(70, 22); LCD.sends("KM", 1);

                    // Left fuel
                    ftoa(fuelLeft, res, 0); 
                    LCD.cursor(1, 32); LCD.sends("--------------", 1);
                    LCD.cursor(5, 40); LCD.sends("!\"   ", 1);  // Fuel distributor symbol
                    LCD.sends(res, 1); 
                    LCD.cursor(70, 40); LCD.sends("L", 1);
                break;

                // `switch()` without `default` case is taking more space in output file. Huh, interesting.
                default: 
                    LCD.cursor(1, 1); LCD.sends("L//M - ", 1); 
                    LCD.sends(itoa(mode, buffer, 10), 1);
                break;
            }
        } else {
            switch(mode) {
                case 1: 
                    cli();  // Disable global interrupts to not corrupt the data

                    // Simple formula - distance/pulses
                    float ff = 65535*pulseOverflows;
                          ff += distPulseCount;
                          ff = 10/ff; 

                    float inv = ccMin/1000;
                          inv = inv/60;

                    // Calibration data is stored in EEPROM
                    eeprom_update_float(&eeSavedData.eePulseDistance, ff);
                    eeprom_update_float(&eeSavedData.eeInjectionValue, inv);

                    LCD.sends("L//K", 1);
                    LCD.cursor(0, 9);
                    ftoa(ff, res, 8);
                    LCD.sends(res, 1);

                    LCD.cursor(0, 18);
                    ftoa(inv, res, 8);
                    LCD.sends(res, 1);
                    
                    sei();
                break;

                case 2:
                    ftoa(ccMin, res, 1);
                    LCD.cursor(0, 1); LCD.sends("50: ", 1);
                    LCD.sends(res, 1);
                    
                    ftoa(divideFuelFactor, res, 1);
                    LCD.cursor(0, 9); LCD.sends("55: ", 1);
                    LCD.sends(res, 1);
                break;

                case 3: 
                    sei();
                    LCD.cursor(0, 1); LCD.sends("40: ", 1);
                    LCD.sends(ltoa(distPulseCount, buffer, 10), 1);
                    
                    LCD.cursor(0, 9); LCD.sends("41: ", 1);
                    LCD.sends(itoa(pulseOverflows, buffer, 10), 1);
                break;
            }
        }

        LCD.render();
        LCD.clear();
    } return 0;
}



ISR(TIMER1_OVF_vect) {
    --counter;

    // Acceleration from 0 to 100 km/h measure time
    if(speed > 0 && speed < 100) ++accBuffer;
    if(speed >= 100) accTime = (float)accBuffer/4.0f; 

    // Data saving based on speed and time
    if(injectorPulseTime < 800 && distPulseCount == 0) saveData();
    if(saveCounter <= 0 && speed == 0) saveData();

    if(!(PIND & (1<<PD6))) {
        --btnCnt;

        if(calibrationFlag == 0 && (btnCnt <= 20 && btnCnt > 0)) {
            // Check if button is pressed for ~1 second
            switch(mode) {
                case 2: mode = 4; break;
                case 1: mode = 5; break;
            }

        } else if(calibrationFlag == 0 && (btnCnt <= 12 && btnCnt > 0)) {
            // Check if button is pressed for ~3 seconds
            switch(mode) {
                case 3: 
                    averageFuelConsumption = .0f;
                    // fuelSumInv = .1f;

                    saveData();
                    loadData();
                break;

                case 2: 
                    avgSpeedCount = 0;
                    // sumInv = .1f;

                    saveData();
                    loadData();
                break;

                case 1: 
                    usedFuel = .0f;
                    traveledDistance = .0f;
                    
                    saveData();
                    loadData();
                break;
            }
        } 
        
        // Check if button is pressed for ~6 seconds on the first screen
        if(btnCnt <= 0 && calibrationFlag == 0 && mode == 3) calibrationFlag = 1;
    } else btnCnt = 24;

    if(counter <= 0) {
        currentSpeed();
        fuelConsumption();

        if(speed <= 0) accBuffer = 0;
        if(speed > 5) {
            rangeDistance = (fuelLeft/averageFuelConsumption)*100;
            ++avgSpeedDivider;
            avgSpeed();
        }
        
        --saveCounter;
        if(!calibrationFlag) distPulseCount = 0;
        injectorPulseTime = 0;
        counter = 4;
    } TCNT1 = 34286;
}


// VSS signal interrupt
ISR(INT0_vect) {
    ++distPulseCount;
    if(calibrationFlag && distPulseCount == 65535) ++pulseOverflows;

    traveledDistance += PULSE_DISTANCE;
    if(instantFuelConsumption <= 0) sailingDistance += PULSE_DISTANCE;
}

// Injector signal interrupt 
ISR(INT1_vect) {
    if(!(PIND & (1<<PD3))) injTimeLow = millis();  // Low state on PD3
    else {
        // High state on PD3
        injTimeHigh = millis();
        injectorPulseTime = injectorPulseTime + (injTimeHigh - injTimeLow);
        PORTD |= (1<<PD3);
    }
}


// Navigation buttons - Atmega 328
ISR(PCINT0_vect) {
    if(!(PINB & (1<<PB0))) {
        // Low state on PB0
        if(calibrationFlag == 1 &&  mode == 2 && !(PIND & (1<<PD6))) ccMin -= 0.5f;
        else if(calibrationFlag == 1 && mode == 3 && !(PIND & (1<<PD6))) distPulseCount -= 500;
        else if(calibrationFlag == 0 && mode == 1 && !(PIND & (1<<PD6))) {
            fuelLeft -= 0.5f;
            savedFuel = fuelLeft;
            eeprom_update_float(&eeSavedData.eeSavedFuel, savedFuel);
        } else {
            ++mode;
            if(mode > 3) mode = 1;
        }
    } 
}

ISR(PCINT2_vect) {
    if(!(PIND & (1<<PD7))) {
        // Low state on PD7
        if(calibrationFlag == 1 && mode == 2 && !(PIND & (1<<PD6))) ccMin += 0.5f;
        else if(calibrationFlag == 1 && mode == 3 && !(PIND & (1<<PD6))) distPulseCount += 500;
        else if(calibrationFlag == 0 && mode == 1 && !(PIND & (1<<PD6))) {
            fuelLeft += 0.5f;
            savedFuel = fuelLeft;
            eeprom_update_float(&eeSavedData.eeSavedFuel, savedFuel);
        } else {
            --mode;
            if(mode < 1) mode = 3;
        }
    } 
}



void avgSpeed() {
    // Harmonic mean
    // Thanks to Gabryś "Dragroth" Król we've got now really good solution for average speed and fuel calculations.
    // His disappointment, when he saw my miserable arithmetic mean, was immeasurable and his day was ruined.
    // He took matters into his own hands and after few tests he came up with the solution you can see in here.
    sumInv += 1.0f/speed;
    avgSpeedCount = avgSpeedDivider/sumInv;
}
void currentSpeed() {speed = PULSE_DISTANCE * distPulseCount * 3600;}


void fuelConsumption() {
    float iotv = (injectorOpenTime*INJECTION_VALUE)*21600;  // 21600 because 3600 (seconds in hour) * 6 (no. of injectors)
    float inv = (injectorOpenTime*INJECTION_VALUE)*6;

    injectorOpenTime = ((float)injectorPulseTime/1000);     // Converting to seconds
    if(speed > 5) {
        instantFuelConsumption = (100*iotv)/speed;

        // Harmonic mean 
        if(instantFuelConsumption > 0 && instantFuelConsumption < 100) {
            fuelSumInv += 1.0f/instantFuelConsumption;
            averageFuelConsumption = avgSpeedDivider/fuelSumInv;
        }
    } else instantFuelConsumption = iotv;

    usedFuel += inv;
    fuelLeft -= inv;
}



void saveData() {
    cli();

    eeprom_update_float(&eeSavedData.eeTraveledDistance, traveledDistance);

    eeprom_update_float(&eeSavedData.eeAverageFuelConsumption, averageFuelConsumption);
    eeprom_update_float(&eeSavedData.eeUsedFuel, usedFuel);
    eeprom_update_float(&eeSavedData.eeSavedFuel, savedFuel);

    eeprom_update_byte(&eeSavedData.eeAverageSpeedCount, avgSpeedCount);
    eeprom_update_float(&eeSavedData.eeAvgSpeedDivider, avgSpeedDivider);
    
    eeprom_update_float(&eeSavedData.eeSumInv, sumInv);
    eeprom_update_float(&eeSavedData.eeFuelSumInv, fuelSumInv);

    saveCounter = 60;
    sei();
}

void loadData() {
    cli();

    PULSE_DISTANCE = eeprom_read_float(&eeSavedData.eePulseDistance);
    INJECTION_VALUE = eeprom_read_float(&eeSavedData.eeInjectionValue);

    traveledDistance = isnan(eeprom_read_float(&eeSavedData.eeTraveledDistance)) ? 0 : eeprom_read_float(&eeSavedData.eeTraveledDistance);

    averageFuelConsumption = isnan(eeprom_read_float(&eeSavedData.eeAverageFuelConsumption)) ? 0 : eeprom_read_float(&eeSavedData.eeAverageFuelConsumption);
    usedFuel = isnan(eeprom_read_float(&eeSavedData.eeUsedFuel)) ? 0 : eeprom_read_float(&eeSavedData.eeUsedFuel);
    savedFuel = isnan(eeprom_read_float(&eeSavedData.eeSavedFuel)) ? 0 : eeprom_read_float(&eeSavedData.eeSavedFuel);
    divideFuelFactor = isnan(eeprom_read_float(&eeSavedData.eeDivideFuelFactor)) ? 0 : eeprom_read_float(&eeSavedData.eeDivideFuelFactor);

    avgSpeedCount = isnan(eeprom_read_byte(&eeSavedData.eeAverageSpeedCount)) ? 0 
                        : (eeprom_read_byte(&eeSavedData.eeAverageSpeedCount) >= 255 ? 0 
                            : eeprom_read_byte(&eeSavedData.eeAverageSpeedCount));
    avgSpeedDivider = isnan(eeprom_read_float(&eeSavedData.eeAvgSpeedDivider)) ? 0 : eeprom_read_float(&eeSavedData.eeAvgSpeedDivider);

    sumInv = isnan(eeprom_read_float(&eeSavedData.eeSumInv)) ? 0 : eeprom_read_float(&eeSavedData.eeSumInv);
    fuelSumInv = isnan(eeprom_read_float(&eeSavedData.eeFuelSumInv)) ? 0 : eeprom_read_float(&eeSavedData.eeFuelSumInv);

    sei();
}