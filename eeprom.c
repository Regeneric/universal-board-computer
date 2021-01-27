#include <avr/eeprom.h>
#include <avr/interrupt.h>

#include "eeprom.h"

void saveData(struct data) {
    cli();

    eeprom_update_float(&data.eeTraveledDistance, traveledDistance);

    eeprom_update_float(&data.eeAverageFuelConsumption, averageFuelConsumption);
    eeprom_update_float(&data.eeUsedFuel, usedFuel);

    eeprom_update_byte(&data.eeMaxSpeedCount, maxSpeedCount);
    eeprom_update_byte(&data.eeAverageSpeedCount, avgSpeedCount);
    eeprom_update_dword(&data.eeAvgSpeedDivider, avgSpeedDivider);

    eeprom_update_word(&data.eeSaveNumber, saveNumber);

    saveCounter = 0;
    toBeSaved = 0;

    sei();
}

// void loadData() {
//     traveledDistance = isnan(eeprom_read_float(&eeSavedData.eeTraveledDistance)) ? 0 : eeprom_read_float(&eeSavedData.eeTraveledDistance);

//     averageFuelConsumption = isnan(eeprom_read_float(&eeSavedData.eeAverageFuelConsumption)) ? 0 : eeprom_read_float(&eeSavedData.eeAverageFuelConsumption);
//     usedFuel = isnan(eeprom_read_float(&eeSavedData.eeUsedFuel)) ? 0 : eeprom_read_float(&eeSavedData.eeUsedFuel);

//     maxSpeedCount = isnan(eeprom_read_byte(&eeSavedData.eeMaxSpeedCount)) ? 0 : eeprom_read_byte(&eeSavedData.eeMaxSpeedCount);
//     avgSpeedCount = isnan(eeprom_read_byte(&eeSavedData.eeAverageSpeedCount)) ? 0 : eeprom_read_byte(&eeSavedData.eeAverageSpeedCount);
//     avgSpeedDivider = isnan(eeprom_read_dword(&eeSavedData.eeAvgSpeedDivider)) ? 0 : eeprom_read_dword(&eeSavedData.eeAvgSpeedDivider);

//     saveNumber = isnan(eeprom_read_word(&eeSavedData.eeSaveNumber)) ? 0 : eeprom_read_word(&eeSavedData.eeSaveNumber);
// }


const struct eepromInterface EEPROM = {
    .save = saveData
};