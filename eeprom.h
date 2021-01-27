#ifndef EEPROM_H
#define EEPROM_H

struct eepromInterface {
    void (*save)(struct);
}; extern const struct eepromInterface EEPROM;

#endif  // EEPROM_H