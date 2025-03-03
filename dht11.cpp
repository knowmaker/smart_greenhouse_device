#include "DHT11.h"

// Конструктор: задаёт пин и устанавливает HIGH.
DHT11::DHT11(int pin)
  : _pin(pin) {
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, HIGH);
}

// Чтение сырых данных.
int DHT11::readRawData(byte data[5]) {
  startSignal();  // Посылаем стартовый сигнал.
  unsigned long timeout_start = millis();

  // Ждём отклика LOW от датчика.
  while (digitalRead(_pin) == HIGH) {
    if (millis() - timeout_start > TIMEOUT_DURATION) {
      return DHT11::ERROR_TIMEOUT;  // Ошибка: таймаут.
    }
  }

  // Читаем данные после подтверждения LOW -> HIGH.
  if (digitalRead(_pin) == LOW) {
    delayMicroseconds(80);
    if (digitalRead(_pin) == HIGH) {
      delayMicroseconds(80);
      for (int i = 0; i < 5; i++) {
        data[i] = readByte();  // Считываем байты данных.
        if (data[i] == DHT11::ERROR_TIMEOUT) {
          return DHT11::ERROR_TIMEOUT;
        }
      }

      // Проверяем контрольную сумму.
      if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        return 0;  // Успех.
      } else {
        return DHT11::ERROR_CHECKSUM;  // Ошибка контрольной суммы.
      }
    }
  }
  return DHT11::ERROR_TIMEOUT;  // Ошибка: датчик не ответил.
}

// Считывает байт данных.
byte DHT11::readByte() {
  byte value = 0;

  for (int i = 0; i < 8; i++) {
    while (digitalRead(_pin) == LOW)
      ;  // Ждём LOW.
    delayMicroseconds(30);
    if (digitalRead(_pin) == HIGH) {
      value |= (1 << (7 - i));  // Записываем бит.
    }
    while (digitalRead(_pin) == HIGH)
      ;  // Ждём HIGH.
  }
  return value;
}

// Отправляет стартовый сигнал датчику.
void DHT11::startSignal() {
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);  // Низкий уровень 18 мс.
  delay(18);
  digitalWrite(_pin, HIGH);  // Высокий уровень 40 мкс.
  delayMicroseconds(40);
  pinMode(_pin, INPUT);  // Переход в режим чтения.
}

// Чтение температуры.
int DHT11::readTemperature() {
  byte data[5];
  int error = readRawData(data);
  if (error != 0) {
    return error;  // Возвращаем ошибку.
  }
  return data[2];  // Температура в третьем байте.
}

// Чтение влажности.
int DHT11::readHumidity() {
  byte data[5];
  int error = readRawData(data);
  if (error != 0) {
    return error;  // Возвращаем ошибку.
  }
  return data[0];  // Влажность в первом байте.
}

// Чтение температуры и влажности.
int DHT11::readTemperatureHumidity(int &temperature, int &humidity) {
  byte data[5];
  int error = readRawData(data);
  if (error != 0) {
    return error;  // Возвращаем ошибку.
  }
  humidity = data[0] - 10;  // Влажность.
  temperature = data[2];    // Температура.
  return 0;
}

// Возвращает строку с описанием ошибки.
String DHT11::getErrorString(int errorCode) {
  switch (errorCode) {
    case DHT11::ERROR_TIMEOUT:
      return "Ошибка 253: Таймаут чтения.";
    case DHT11::ERROR_CHECKSUM:
      return "Ошибка 254: Несовпадение контрольной суммы.";
    default:
      return "Ошибка: Неизвестная.";
  }
}
