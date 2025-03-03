#ifndef microWire_h
#define microWire_h
#include <Arduino.h>
#include "pins_arduino.h"

// Максимальная длина буфера для совместимости с Wire.h (максимальное значение uint8_t)
#define BUFFER_LENGTH 255

class TwoWire {
public:
  void begin(void);                                                 // Инициализация I2C-шины как master
  void setClock(uint32_t clock);                                    // Установка частоты шины в диапазоне 31–900 кГц (в герцах)
  void beginTransmission(uint8_t address);                          // Начало передачи данных на устройство с указанным адресом
  uint8_t endTransmission(bool stop);                               // Завершение передачи с опциональным stop или restart (по умолчанию stop)
  uint8_t endTransmission(void);                                    // Завершение передачи (с stop по умолчанию)
  size_t write(uint8_t data);                                       // Передача байта данных по шине (без буферизации)
  uint8_t requestFrom(uint8_t address, uint8_t length, bool stop);  // Запрос данных с устройства
  uint8_t requestFrom(uint8_t address, uint8_t length);             // Запрос данных с автоматическим отпусканием шины (stop)
  uint8_t read(void);                                               // Прямое чтение одного байта данных из шины
  uint8_t available(void);                                          // Получение количества оставшихся непрочитанных байт

  // Функции для совместимости с API Wire
  inline void beginTransmission(int address) {
    beginTransmission((uint8_t)address);
  }
  uint8_t requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop);
  uint8_t requestFrom(uint8_t address, uint8_t quantity, uint32_t iaddress, uint8_t isize, uint8_t sendStop);
  uint8_t requestFrom(int address, int quantity);
  uint8_t requestFrom(int address, int quantity, int sendStop);
  size_t write(const uint8_t *buffer, size_t size);
  inline size_t write(unsigned long n) {
    return write((uint8_t)n);
  }
  inline size_t write(long n) {
    return write((uint8_t)n);
  }
  inline size_t write(unsigned int n) {
    return write((uint8_t)n);
  }
  inline size_t write(int n) {
    return write((uint8_t)n);
  }

private:
  uint8_t _requested_bytes = 0;     // Количество запрошенных и ещё непрочитанных байт
  bool _address_nack = false;       // Флаг ошибки при передаче адреса
  bool _data_nack = false;          // Флаг ошибки при передаче данных
  bool _stop_after_request = true;  // Нужно ли отпускать шину после последнего байта
  void start(void);                 // Начало передачи на шине (генерация Start condition)
  void stop(void);                  // Завершение передачи на шине (генерация Stop condition)
};

extern TwoWire Wire;

#endif
