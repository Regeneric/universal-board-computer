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



volatile static const float PULSE_DISTANCE = 0.00006823;  // distance in kilometers/all pulses
volatile static const float INJECTION_VALUE = 0.0031667;  // Based on value that injector can inject 191.3 cc/min of fuel


volatile float avgSpeedDivider = .0, traveledDistance = .0, sumInv = .0;
volatile float instantFuelConsumption = .0, averageFuelConsumption = .0, usedFuel = .0, fuelSumInv = .0;
volatile float injectorOpenTime = .0;

volatile uint8_t toBeSaved = 0, mode = 1, saveCounter = 0;
volatile uint8_t speed = 0, avgSpeedCount = 0, fuelLeft = 0;

volatile uint16_t counter = 0, distPulseCount = 0, injectorPulseTime = 0, injTimeHigh = 0, injTimeLow = 0, saveNumber = 0, rangeDistance = 0;



// Structure stored in EEPROM
typedef struct {
    float eeAverageFuelConsumption;
    float eeTraveledDistance;
    float eeUsedFuel;
    float eeAvgSpeedDivider;
    uint8_t eeAverageSpeedCount;  
} eeStruct; eeStruct EEMEM eeSavedData;



void avgSpeed();
void currentSpeed();

void fuelConsumption();

void saveData();
void loadData();


int main() {
    // I use hex values 'cause they take less space in the output file. 
    // For now, (07.02.2021, commit on GH no. f8415bd, as I write this) there are 224 bytes of free space in flash memory.
    // Yes, bytes. And still there is no built-in calibration method. 


    // ADC for checking fuel level
    DDRC &= ~0x32;  // PC5/ADC as input

    ADCSRA = 0x86;  // Enables ADC and sets prescaller to 64 (8 MHz, 128 for 16 MHz) (1<<ADEN)  ((1<<ADPS2) | (1<<ADPS1))
    ADMUX = 0xC5;  // Internal 2.56V ref voltage with external capacitor, input channel select ((1<<REFS1) | (1<<REFS0))  ((1<<MUX2) | (1<<MUX0))


    // VSS signal, injector signal and navigation buttons - Atmega 8
    DDRD &= ~0x8C;  // PD2/INT0, PD3/INT1 and PD7 as input
    PORTD = 0x8C;  // PD2/INT0, PD3/INT1 and PD7 internal pull-up resistor to avoid high impedance state of the pin

    MCUCR = 0x6;  // FALLING edge of INT0 and ANY change of state of INT1 generates interrupt  (1<<ISC10)
    GICR = 0xC0;  // Turns on INT0 and INT1  ((1<<INT1) | (1<<INT0))

    // 16 bit timer for VSS
    TCCR1A = 0;  // Timer1 normal mode
    TCCR1B = 0x3;  // Prescaler 64, causes overflow every 0.524288s (8 MHz crystal, 0.262144s for 16 MHz)  ((1<<CS10) | (1<<CS11))
    TCNT1 = 34286;  // Counts from 34286 to 65535 - causes overflow every 0.25s (8 MHz crystal, 3036 for 16 MHz)

    // Counter for millis() function
    TCCR0 = 0x3;  // Prescaler 64 ((1<<CS01) | (1<<CS00))
    TCNT0 = 0;  // Counts from 0 to 255;

    TIMSK = 0x5;  // Enable timer overflow interrupt  ((1<<TOIE1) | (1<<TOIE0))


    // Button debouncing 
	uint8_t i = 0;  // Counter variable
	uint8_t buttonPressed = 0;  // Keeps track if button is pressed; 0 false, 1 true

    loadData();  // Loads data from EEPROM
    char buffer[8];  // Buffer for itoa() function
    LCD.init();

    sei();  // Global interrupts enabled
    while(1) {
        if(!(PIND & (1<<PD7)) && !buttonPressed) {
            if(i < 15) i++;
            else i = 0;

            PORTD = i | (1<<PD7);
            buttonPressed = 1;

            mode++;
            if(mode > 3) mode = 1;  // "Modes" of what should be displayed on the LCD
        } else if((PIND & (1<<PD7)) && buttonPressed) {
            buttonPressed = 0;
            PORTD ^= (1<<PD7);
        }
        

        char res[24];
        switch(mode) {
            case 1: 
            // Main Screen
                
                // Range
                LCD.cursor(1, 1); LCD.sends("$%&  ", 1);  // Range symbol
                LCD.sends(itoa(rangeDistance, buffer, 10), 1);
                LCD.cursor(65, 1); LCD.sends(" KM", 1);
                LCD.cursor(1, 9); LCD.sends("--------------", 1);
                
                // Instant fuel consumption
                FTOA.convert(instantFuelConsumption, res, 1);   
                LCD.cursor(1, 17);
                if(instantFuelConsumption > 99 || instantFuelConsumption <= 0) LCD.sends("--.-", 2);
                else LCD.sends(res, 2);

                LCD.cursor(54, 22);
                if(speed > 5) LCD.sends("L/100", 1);  // Car is moving
                else LCD.sends("L/H", 1);  // Car is not moving

                // Average fuel consumption
                LCD.cursor(1, 32); LCD.sends("--------------", 1);
                LCD.cursor(5, 40); LCD.sends("#  ", 1);  // Average fuel consumption symbol
                FTOA.convert(averageFuelConsumption, res, 1);
                LCD.sends(res, 1); 
                LCD.cursor(54, 40); LCD.sends("L/100", 1);
            break;

            case 2: 
            // Speed infoscreen
                FTOA.convert(speed, res, 2);

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


            case 3:
            // Fuel infoscreen

                // Used fuel
                FTOA.convert(usedFuel, res, 2);
                LCD.cursor(1, 1); LCD.sends("%&$  ", 1);  // Range symbol
                if(usedFuel <= 0) LCD.sends("--.-", 1);
                else if(usedFuel < 1 && usedFuel > 0) {LCD.sends("0", 1); LCD.sends(res, 1);}
                else LCD.sends(res, 1);
                LCD.cursor(65, 1); LCD.sends(" L", 1);
                LCD.cursor(1, 9); LCD.sends("--------------", 1);
                
                // Traveled distance
                FTOA.convert(traveledDistance, res, 1);   
                LCD.cursor(1, 17);
                if(traveledDistance <= 0) LCD.sends("--", 2);
                else if(traveledDistance > 0 && traveledDistance < 1) {LCD.sendc('0', 2); LCD.sends(res, 2);}
                else LCD.sends(res, 2);
                LCD.cursor(70, 22); LCD.sends("KM", 1);

                // // Left fuel
                LCD.cursor(1, 32); LCD.sends("--------------", 1);
                LCD.cursor(5, 40); LCD.sends("!\"   ", 1);  // Fuel distributor symbol
                LCD.sends(itoa(fuelLeft, buffer, 10), 1); 
                LCD.cursor(70, 40); LCD.sends("L", 1);

            break;

            // `switch()` without `default` case is taking more space in output file. Huh, interesting.
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

    if(injectorPulseTime < 800 && distPulseCount == 0 && toBeSaved == 1) saveData();
    if(saveCounter > 45 && speed == 0 && toBeSaved == 1) saveData();

    if(counter > 3) {
        currentSpeed();
        fuelConsumption();

        if(instantFuelConsumption > 1.0) toBeSaved = 1;
        if(speed > 5) {
            rangeDistance = (fuelLeft/averageFuelConsumption)*100;
            avgSpeedDivider++;
            avgSpeed();
        }
        
        saveCounter++;
        distPulseCount = 0;
        injectorPulseTime = 0;
        counter = 0;
    } TCNT1 = 34286;
}


// VSS signal interrupt
ISR(INT0_vect) {
    distPulseCount++;
    traveledDistance += PULSE_DISTANCE;
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
    injectorOpenTime = ((float)injectorPulseTime/1000);  // Converting to seconds
    if(speed > 5) {
        instantFuelConsumption = (100*(injectorOpenTime*INJECTION_VALUE)*3600*4)/speed;

        // Harmonic mean 
        if(instantFuelConsumption > 0 && instantFuelConsumption < 100) {
            fuelSumInv += 1.0f/instantFuelConsumption;
            averageFuelConsumption = avgSpeedDivider/fuelSumInv;
        }

    } else instantFuelConsumption = (injectorOpenTime*INJECTION_VALUE)*3600*4;
    usedFuel += ((injectorOpenTime * INJECTION_VALUE) * 4);

    // ADC checks for level fuel in the tank
    ADCSRA |= (1<<ADSC);
    while(ADCSRA & (1<<ADSC));

    fuelLeft = ADC/15;  // For the 68 liters tank
}



void saveData() {
    cli();

    eeprom_update_float(&eeSavedData.eeTraveledDistance, traveledDistance);

    eeprom_update_float(&eeSavedData.eeAverageFuelConsumption, averageFuelConsumption);
    eeprom_update_float(&eeSavedData.eeUsedFuel, usedFuel);

    eeprom_update_byte(&eeSavedData.eeAverageSpeedCount, avgSpeedCount);
    eeprom_update_float(&eeSavedData.eeAvgSpeedDivider, avgSpeedDivider);

    saveCounter = 0;
    toBeSaved = 0;

    sei();
}

void loadData() {
    traveledDistance = isnan(eeprom_read_float(&eeSavedData.eeTraveledDistance)) ? 0 : eeprom_read_float(&eeSavedData.eeTraveledDistance);

    averageFuelConsumption = isnan(eeprom_read_float(&eeSavedData.eeAverageFuelConsumption)) ? 0 : eeprom_read_float(&eeSavedData.eeAverageFuelConsumption);
    usedFuel = isnan(eeprom_read_float(&eeSavedData.eeUsedFuel)) ? 0 : eeprom_read_float(&eeSavedData.eeUsedFuel);

    avgSpeedCount = isnan(eeprom_read_byte(&eeSavedData.eeAverageSpeedCount)) 
                        ? 0 : (eeprom_read_byte(&eeSavedData.eeAverageSpeedCount) >= 255 
                            ? 0 : eeprom_read_byte(&eeSavedData.eeAverageSpeedCount));
    avgSpeedDivider = isnan(eeprom_read_float(&eeSavedData.eeAvgSpeedDivider)) ? 0 : eeprom_read_float(&eeSavedData.eeAvgSpeedDivider);
}