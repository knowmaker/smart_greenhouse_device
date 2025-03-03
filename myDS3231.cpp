#include "myDS3231.h"

// Определение количества дней в месяцах
static uint8_t DS_dim(uint8_t i) {
  return (i < 7) ? ((i == 1) ? 28 : ((i & 1) ? 30 : 31)) : ((i & 1) ? 31 : 30);
}

// Вычисление дня недели
static uint16_t getWeekDay(uint16_t y, uint8_t m, uint8_t d) {
  if (y >= 2000) y -= 2000;
  uint16_t days = d;
  for (uint8_t i = 1; i < m; ++i) days += DS_dim(i - 1);
  if (m > 2 && y % 4 == 0) days++;
  return (days + 365 * y + (y + 3) / 4 + 4) % 7 + 1;
}

// Конструктор
MyDS3231::MyDS3231(uint8_t addr)
  : _addr(addr) {
  Wire.begin();
}

// Инициализация устройства
bool MyDS3231::begin(void) {
  Wire.begin();
  Wire.beginTransmission(_addr);
  return (!Wire.endTransmission());
}

// Установка времени вручную
void MyDS3231::setTime(int8_t seconds, int8_t minutes, int8_t hours, int8_t date, int8_t month, int16_t year) {
  uint8_t day = getWeekDay(year, month, date);
  year -= 2000;
  Wire.beginTransmission(_addr);
  Wire.write(0x00);
  Wire.write(encodeRegister(seconds));
  Wire.write(encodeRegister(minutes));
  if (hours > 19) Wire.write((0x2 << 4) | (hours % 20));
  else if (hours > 9) Wire.write((0x1 << 4) | (hours % 10));
  else Wire.write(hours);
  Wire.write(day);
  Wire.write(encodeRegister(date));
  Wire.write(encodeRegister(month));
  Wire.write(encodeRegister(year));
  Wire.endTransmission();
}

// Установка времени из структуры
void MyDS3231::setTime(DateTime time) {
  setTime(time.second, time.minute, time.hour, time.date, time.month, time.year);
}

// Получение времени в структуру
DateTime MyDS3231::getTime() {
  DateTime now;
  Wire.beginTransmission(_addr);
  Wire.write(0x0);
  if (Wire.endTransmission() != 0) return now;
  Wire.requestFrom(_addr, (uint8_t)7);
  now.second = unpackRegister(Wire.read());
  now.minute = unpackRegister(Wire.read());
  now.hour = unpackHours(Wire.read());
  now.day = Wire.read();
  now.date = unpackRegister(Wire.read());
  now.month = unpackRegister(Wire.read());
  now.year = unpackRegister(Wire.read()) + 2000;
  return now;
}

// Получение строки времени
String MyDS3231::getTimeString() {
  DateTime now = getTime();
  String str;
  str.reserve(8);
  if (now.hour < 10) str += '0';
  str += now.hour;
  str += ':';
  if (now.minute < 10) str += '0';
  str += now.minute;
  str += ':';
  if (now.second < 10) str += '0';
  str += now.second;
  return str;
}

// Получение строки даты
String MyDS3231::getDateString() {
  DateTime now = getTime();
  String str;
  str.reserve(10);
  if (now.date < 10) str += '0';
  str += now.date;
  str += '.';
  if (now.month < 10) str += '0';
  str += now.month;
  str += '.';
  str += now.year;
  return str;
}

// Получение времени в массив char
void MyDS3231::getTimeChar(char* array) {
  DateTime now = getTime();
  array[0] = now.hour / 10 + '0';
  array[1] = now.hour % 10 + '0';
  array[2] = ':';
  array[3] = now.minute / 10 + '0';
  array[4] = now.minute % 10 + '0';
  array[5] = ':';
  array[6] = now.second / 10 + '0';
  array[7] = now.second % 10 + '0';
  array[8] = '\0';
}

// Получение даты в массив char
void MyDS3231::getDateChar(char* array) {
  DateTime now = getTime();
  array[0] = now.date / 10 + '0';
  array[1] = now.date % 10 + '0';
  array[2] = '.';
  array[3] = now.month / 10 + '0';
  array[4] = now.month % 10 + '0';
  array[5] = '.';
  itoa(now.year, array + 6, DEC);
  array[10] = '\0';
}

void MyDS3231::getDateTimeChar(char* array) {
  DateTime now = getTime();
  array[0] = now.date / 10 + '0';
  array[1] = now.date % 10 + '0';
  array[2] = '.';
  array[3] = now.month / 10 + '0';
  array[4] = now.month % 10 + '0';
  array[5] = '.';
  itoa(now.year, array + 6, DEC);
  array[10] = ' ';
  array[11] = now.hour / 10 + '0';
  array[12] = now.hour % 10 + '0';
  array[13] = ':';
  array[14] = now.minute / 10 + '0';
  array[15] = now.minute % 10 + '0';
  array[16] = ':';
  array[17] = now.second / 10 + '0';
  array[18] = now.second % 10 + '0';
  array[19] = ' ';
  array[20] = '\0';
}

// Проверить сброс питания
bool MyDS3231::lostPower(void) {  // возвращает true, если 1 января 2000
  return (getYear() == 2000 && getMonth() == 1 && getDate() == 1);
}

uint8_t MyDS3231::getSeconds(void) {
  return (unpackRegister(readRegister(0x00)));
}

uint8_t MyDS3231::getMinutes(void) {
  return (unpackRegister(readRegister(0x01)));
}

uint8_t MyDS3231::getHours(void) {
  return (unpackHours(readRegister(0x02)));
}

uint8_t MyDS3231::getDay(void) {
  return readRegister(0x03);
}

uint8_t MyDS3231::getDate(void) {
  return (unpackRegister(readRegister(0x04)));
}

uint8_t MyDS3231::getMonth(void) {
  return (unpackRegister(readRegister(0x05)));
}

uint16_t MyDS3231::getYear(void) {
  return (unpackRegister(readRegister(0x06)) + 2000);
}

// сервис
uint8_t MyDS3231::readRegister(uint8_t addr) {
  Wire.beginTransmission(_addr);
  Wire.write(addr);
  if (Wire.endTransmission() != 0) return 0;
  Wire.requestFrom(_addr, (uint8_t)1);
  return Wire.read();
}

uint8_t MyDS3231::unpackRegister(uint8_t data) {
  return ((data >> 4) * 10 + (data & 0xF));
}

uint8_t MyDS3231::encodeRegister(int8_t data) {
  return (((data / 10) << 4) | (data % 10));
}

uint8_t MyDS3231::unpackHours(uint8_t data) {
  if (data & 0x20) return ((data & 0xF) + 20);
  else if (data & 0x10) return ((data & 0xF) + 10);
  else return (data & 0xF);
}

float MyDS3231::getTemperatureFloat(void) {
  return (getTemperatureRaw() * 0.25f);
}

int MyDS3231::getTemperature(void) {
  return (getTemperatureRaw() >> 2);
}

int MyDS3231::getTemperatureRaw(void) {
  Wire.beginTransmission(_addr);
  Wire.write(0x11);
  Wire.endTransmission();
  Wire.requestFrom(_addr, (uint8_t)2);
  return ((int8_t)Wire.read() << 2) + (Wire.read() >> 6);
}