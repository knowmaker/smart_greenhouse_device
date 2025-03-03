#ifndef myDS3231_h
#define myDS3231_h
#include "microWire.h"
#include <Arduino.h>

// Структура для хранения даты и времени
struct DateTime {
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  uint8_t day;
  uint8_t date;
  uint8_t month;
  uint16_t year;
};

// Класс для работы с DS3231
class MyDS3231 {
public:
  MyDS3231(uint8_t addr = 0x68);                                                                          // Конструктор с адресом
  bool begin();                                                                                           // Инициализация (true - если RTC найден)
  void setTime(DateTime time);                                                                            // Установка времени из структуры
  void setTime(int8_t seconds, int8_t minutes, int8_t hours, int8_t date, int8_t month, int16_t year);    // Установка времени вручную

  DateTime getTime();    // Получение времени в структуру
  uint8_t getSeconds();  // Получить секунды
  uint8_t getMinutes();  // Получить минуты
  uint8_t getHours();    // Получить часы
  uint8_t getDay();      // Получить день недели
  uint8_t getDate();     // Получить дату
  uint16_t getYear();    // Получить год
  uint8_t getMonth();    // Получить месяц

  String getTimeString();         // Время в формате HH:MM:SS
  String getDateString();         // Дата в формате DD.MM.YYYY
  void getTimeChar(char* array);  // Время в массив [8]: HH:MM:SS
  void getDateChar(char* array);  // Дата в массив [10]: DD.MM.YYYY
  void getDateTimeChar(char* array);

  bool lostPower();             // Проверить сброс питания
  int getTemperature();         // Температура (целое число)
  float getTemperatureFloat();  // Температура (дробное число)

private:
  uint8_t encodeRegister(int8_t data);   // Преобразовать данные для записи
  int getTemperatureRaw();               // Сырой код температуры
  uint8_t readRegister(uint8_t addr);    // Прочитать регистр
  uint8_t unpackRegister(uint8_t data);  // Распаковать регистр
  uint8_t unpackHours(uint8_t data);     // Распаковать часы
  const uint8_t _addr;                   // Адрес устройства
};

#endif
