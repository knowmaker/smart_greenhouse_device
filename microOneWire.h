#ifndef _microOneWire_h
#define _microOneWire_h
#include <Arduino.h>

// Объявление функций для работы с шиной OneWire
bool oneWire_reset(uint8_t pin);  // Функция сброса устройства на шине
void oneWire_write(uint8_t data, uint8_t pin);  // Функция записи байта данных в устройство
uint8_t oneWire_read(uint8_t pin);  // Функция чтения байта данных с устройства
#endif
