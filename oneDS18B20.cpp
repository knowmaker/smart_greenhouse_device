#include "oneDS18B20.h"

// Конструктор класса OneDS18B20
OneDS18B20::OneDS18B20(uint8_t pin) : _pin(pin) {
    pinMode(_pin, INPUT);
    digitalWrite(_pin, LOW);
}

// Установить разрешение термометра 9-12 бит
void OneDS18B20::setResolution(uint8_t res) {
    if (!oneWire_reset(_pin)) return;              // Проверка присутствия
    oneWire_write(0x4E, _pin);                     // Запись RAM
    oneWire_write(0xFF, _pin);                     // Максимум в верхний регистр тревоги
    oneWire_write(0x00, _pin);                     // Минимум в верхний регистр тревоги
    oneWire_write(((constrain(res, 9, 12) - 9) << 5) | 0x1F, _pin); // Запись конфигурации разрешения
}

// Запросить температуру
void OneDS18B20::requestTemp() {
    state = 0;                                     // Запрошена новая температура
    if (!oneWire_reset(_pin)) return;              // Проверка присутствия
    oneWire_write(0xCC, _pin);                     // Пропускаем адресацию
    oneWire_write(0x44, _pin);                     // Запросить преобразование
}

// Получить температуру float
float OneDS18B20::getTemp() {
    if (!state) readTemp();
    return (_buf / 16.0);
}

// Получить температуру int
int16_t OneDS18B20::getTempInt() {
    if (!state) readTemp();
    return (_buf >> 4);
}

// Получить "сырое" значение температуры
int16_t OneDS18B20::getRaw() {
    if (!state) readTemp();
    return _buf;
}

// Прочитать температуру с датчика, true если успешно
bool OneDS18B20::readTemp() {
    state = 1;
    if (!oneWire_reset(_pin)) return false;        // датчик оффлайн
    oneWire_write(0xCC, _pin);                     // Пропускаем адресацию
    oneWire_write(0xBE, _pin);                     // Запросить температуру
    uint8_t crc = 0;                               // обнуляем crc
    int16_t temp;                                  // переменная для расчёта температуры
    uint16_t sum = 0;                              // контрольная сумма

    for (uint8_t i = 0; i < 9; i++) {              // Считать RAM
        uint8_t data = oneWire_read(_pin);         // Прочитать данные
        sum += data;

        #if (DS_CHECK_CRC == true)
        _ds_crc8_upd(crc, data);                   // Обновить значение CRC8
        #endif

        if (i == 0) temp = data;
        else if (i == 1) temp |= (data << 8);
    }

    if (sum == 0x8F7 || !sum || crc) return false; // датчик оффлайн или данные повреждены
    if (temp != 0x0550) _buf = temp;               // пропускаем первое чтение (85 градусов)
    return true;
}

// Обновление CRC8 (с использованием таблицы или расчёта)
void OneDS18B20::_ds_crc8_upd(uint8_t &crc, uint8_t data) {
#if (DS_CRC_USE_TABLE == true)
    crc = pgm_read_byte(&_ds_crc8_table[crc ^ data]); // Используем таблицу
#else
    uint8_t i = 8;
    while (i--) {
        crc = ((crc ^ data) & 1) ? (crc >> 1) ^ 0x8C : (crc >> 1); // расчет вручную
        data >>= 1;
    }
#endif
}
