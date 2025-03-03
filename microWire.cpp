#include "microWire.h"

void TwoWire::begin() {
  // Устанавливаем пины SDA и SCL в режим ввода
  pinMode(SDA, INPUT);
  pinMode(SCL, INPUT);
  // Устанавливаем стандартную скорость работы шины I2C (100 кГц)
  TWBR = 72;
  TWSR = 0;  // Устанавливаем делитель тактовой частоты в 1
}

void TwoWire::setClock(uint32_t clock) {
  // Расчёт значения регистра TWBR для заданной частоты I2C
  TWBR = (((long)F_CPU / clock) - 16) / 2;
}

void TwoWire::beginTransmission(uint8_t address) {
  // Начало передачи данных к slave-устройству с указанным адресом
  TwoWire::start();              // Генерация Start condition
  TwoWire::write(address << 1);  // Отправка адреса устройства с битом "write"
}

uint8_t TwoWire::endTransmission(void) {
  return TwoWire::endTransmission(true);  // По умолчанию завершаем передачу с отправкой Stop
}

uint8_t TwoWire::endTransmission(bool stop) {
  if (stop) TwoWire::stop();  // Завершаем передачу (Stop condition)
  else TwoWire::start();      // Повторный старт (Restart condition)

  // Проверяем наличие ошибок (NACK при передаче адреса или данных)
  if (_address_nack) {
    _address_nack = false;
    _data_nack = false;
    return 2;  // Код ошибки: адрес не принят
  }
  if (_data_nack) {
    _address_nack = false;
    _data_nack = false;
    return 3;  // Код ошибки: данные не приняты
  }
  return 0;  // Успешное завершение передачи
}

size_t TwoWire::write(uint8_t data) {
  TWDR = data;                    // Загружаем данные в регистр TWDR
  TWCR = _BV(TWEN) | _BV(TWINT);  // Запускаем передачу
  while (!(TWCR & _BV(TWINT)))
    ;  // Ждём окончания передачи

  uint8_t _bus_status = TWSR & 0xF8;              // Получаем статус шины
  if (_bus_status == 0x20) _address_nack = true;  // Адрес не принят (NACK)
  if (_bus_status == 0x30) _data_nack = true;     // Данные не приняты (NACK)
  return 1;                                       // Для совместимости с Wire API
}

uint8_t TwoWire::available() {
  return _requested_bytes;  // Возвращаем количество оставшихся для чтения байт
}

uint8_t TwoWire::read() {
  // Чтение байта данных с шины I2C
  if (--_requested_bytes) {                     // Если не последний байт
    TWCR = _BV(TWEN) | _BV(TWINT) | _BV(TWEA);  // Читаем с подтверждением (ACK)
    while (!(TWCR & _BV(TWINT)))
      ;           // Ждём завершения чтения
    return TWDR;  // Возвращаем данные
  }
  _requested_bytes = 0;           // Если последний байт
  TWCR = _BV(TWEN) | _BV(TWINT);  // Читаем без подтверждения (NACK)
  while (!(TWCR & _BV(TWINT)))
    ;                               // Ждём завершения чтения
  if (_stop_after_request) TwoWire::stop();  // Завершаем передачу (Stop condition)
  else TwoWire::start();                     // Повторный старт (Restart condition)
  return TWDR;                      // Возвращаем данные
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t length) {
  return TwoWire::requestFrom(address, length, true);  // По умолчанию завершаем Stop
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t length, bool stop) {
  _stop_after_request = stop;   // Запоминаем, нужно ли завершать передачу
  _requested_bytes = length;    // Количество байт, которые нужно прочитать
  TwoWire::start();                      // Генерируем Start condition
  TwoWire::write((address << 1) | 0x1);  // Отправляем адрес устройства с битом "read"
  return length;                // Возвращаем количество запрошенных байт
}

// Остальные методы сохраняют совместимость с оригинальной библиотекой Wire

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop) {
  return requestFrom(address, quantity, sendStop != 0);
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity, uint32_t iaddress, uint8_t isize, uint8_t sendStop) {
  if (isize > 0) {
    beginTransmission(address);
    if (isize > 4) isize = 4;  // Ограничение размера внутреннего адреса
    while (isize-- > 0) write((uint8_t)(iaddress >> (isize * 8)));
    endTransmission(false);  // Завершаем без Stop
  }
  return requestFrom(address, quantity, sendStop);
}

uint8_t TwoWire::requestFrom(int address, int quantity) {
  return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t) true);
}

uint8_t TwoWire::requestFrom(int address, int quantity, int sendStop) {
  return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)sendStop);
}

size_t TwoWire::write(const uint8_t *buffer, size_t size) {
  for (size_t i = 0; i < size; ++i) write(buffer[i]);
  return size;
}

void TwoWire::start() {
  TWCR = _BV(TWSTA) | _BV(TWEN) | _BV(TWINT);  // Генерация Start condition
  while (!(TWCR & _BV(TWINT)))
    ;  // Ждём завершения
}

void TwoWire::stop() {
  TWCR = _BV(TWSTO) | _BV(TWEN) | _BV(TWINT);  // Генерация Stop condition
}

TwoWire Wire = TwoWire();
