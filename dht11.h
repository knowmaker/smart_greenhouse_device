#ifndef DHT11_h
#define DHT11_h

#include "Arduino.h"

class DHT11 {
public:
  DHT11(int pin);                                                // Конструктор. Указывает пин.
  int readHumidity();                                            // Читает влажность.
  int readTemperature();                                         // Читает температуру.
  int readTemperatureHumidity(int &temperature, int &humidity);  // Читает температуру и влажность.

  // Константы для кодов ошибок.
  static const int ERROR_CHECKSUM = 254;    // Ошибка: контрольная сумма не совпала.
  static const int ERROR_TIMEOUT = 253;     // Ошибка: превышено время ожидания.
  static const int TIMEOUT_DURATION = 100;  // Время ожидания в мс.

  static String getErrorString(int errorCode);  // Возвращает описание ошибки.

private:
  int _pin;  // Пин для подключения DHT11.

  int readRawData(byte data[5]);  // Чтение сырых данных.
  byte readByte();                // Читает один байт данных.
  void startSignal();             // Отправляет сигнал запуска.
};

#endif
