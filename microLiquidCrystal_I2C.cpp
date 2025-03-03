#include "microLiquidCrystal_I2C.h"
#include <inttypes.h>
#if defined(ARDUINO) && ARDUINO >= 100

#include "Arduino.h"

#define printIIC(args) Wire.write(args)
inline size_t LiquidCrystal_I2C::write(uint8_t value) {
  send(value, Rs);
  return 1;
}

#else
#include "WProgram.h"

#define printIIC(args) Wire.send(args)
inline void LiquidCrystal_I2C::write(uint8_t value) {
  send(value, Rs);
}

#endif
#include "microWire.h"


// При включении дисплей настроен следующим образом:
//
// 1. Дисплей очищен
// 2. Установлена функция:
//    DL = 1; 8-битный интерфейс
//    N = 0; однострочный дисплей
//    F = 0; шрифт 5x8 точек
// 3. Управление дисплеем:
//    D = 0; дисплей выключен
//    C = 0; курсор выключен
//    B = 0; мигание выключено
// 4. Режим ввода:
//    I/D = 1; инкремент позиции
//    S = 0; без сдвига
//
// Обратите внимание, что сброс Arduino не сбрасывает настройки дисплея,
// поэтому нельзя предполагать, что он находится в исходном состоянии,
// когда конструктор LiquidCrystal вызывается.

LiquidCrystal_I2C::LiquidCrystal_I2C(uint8_t lcd_Addr, uint8_t lcd_cols, uint8_t lcd_rows) {
  _Addr = lcd_Addr;                 // Адрес дисплея на шине I2C
  _cols = lcd_cols;                 // Количество колонок
  _rows = lcd_rows;                 // Количество строк
  _backlightval = LCD_NOBACKLIGHT;  // Подсветка по умолчанию выключена
}

void LiquidCrystal_I2C::init() {
  init_priv();  // Вызов внутренней функции инициализации
}

void LiquidCrystal_I2C::init_priv() {
  Wire.begin();                                               // Инициализация шины I2C
  _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;  // Установка начальных параметров дисплея
  begin(_cols, _rows);
}

void LiquidCrystal_I2C::begin(uint8_t cols, uint8_t lines, uint8_t dotsize) {
  if (lines > 1) {
    _displayfunction |= LCD_2LINE;  // Включение режима 2 строк
  }
  _numlines = lines;  // Сохранение количества строк

  // Для некоторых дисплеев с одной строкой можно выбрать шрифт высотой 10 пикселей
  if ((dotsize != 0) && (lines == 1)) {
    _displayfunction |= LCD_5x10DOTS;
  }

  // СМОТРИТЕ СТРАНИЦЫ 45/46 ДАННЫХ ДЛЯ СПЕЦИФИКАЦИИ ИНИЦИАЛИЗАЦИИ!
  // Согласно документации, нужно минимум 40 мс после подачи питания
  // до отправки команд. Arduino может включиться быстрее, поэтому ждем 50 мс
  delay(50);

  // Теперь сбрасываем RS и R/W для начала отправки команд
  expanderWrite(_backlightval);  // Сбрасываем расширитель и выключаем подсветку
  delay(1000);

  // Переводим дисплей в режим 4-битного интерфейса
  write4bits(0x03 << 4);
  delayMicroseconds(4500);  // ждем минимум 4.1 мс

  // Вторая попытка
  write4bits(0x03 << 4);
  delayMicroseconds(4500);  // ждем минимум 4.1 мс

  // Третья попытка
  write4bits(0x03 << 4);
  delayMicroseconds(150);

  // Наконец, устанавливаем режим 4-битного интерфейса
  write4bits(0x02 << 4);

  // Устанавливаем количество строк, размер шрифта и т.д.
  command(LCD_FUNCTIONSET | _displayfunction);

  // Включаем дисплей, выключаем курсор и мигание (по умолчанию)
  _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
  display();

  // Очищаем дисплей
  clear();

  // Устанавливаем направление текста по умолчанию (для языков с латиницей)
  _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

  // Устанавливаем режим ввода
  command(LCD_ENTRYMODESET | _displaymode);

  home();  // Устанавливаем курсор в начальную позицию
}

// *** Команды верхнего уровня для пользователя ***
void LiquidCrystal_I2C::clear() {
  command(LCD_CLEARDISPLAY);  // Очистка дисплея и сброс позиции курсора
  delayMicroseconds(2000);    // Эта команда выполняется долго!
}

void LiquidCrystal_I2C::home() {
  command(LCD_RETURNHOME);  // Сброс позиции курсора
  delayMicroseconds(2000);  // Эта команда выполняется долго!
}

void LiquidCrystal_I2C::setCursor(uint8_t col, uint8_t row) {
  int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
  if (row > _numlines) {
    row = _numlines - 1;  // Проверяем предел строк (начинается с 0)
  }
  command(LCD_SETDDRAMADDR | (col + row_offsets[row]));  // Устанавливаем позицию курсора
}

// Включение/выключение дисплея
void LiquidCrystal_I2C::noDisplay() {
  _displaycontrol &= ~LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal_I2C::display() {
  _displaycontrol |= LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Включение/выключение подчеркивания курсора
void LiquidCrystal_I2C::noCursor() {
  _displaycontrol &= ~LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal_I2C::cursor() {
  _displaycontrol |= LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Включение/выключение мигающего курсора
void LiquidCrystal_I2C::noBlink() {
  _displaycontrol &= ~LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal_I2C::blink() {
  _displaycontrol |= LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Прокрутка текста на дисплее
void LiquidCrystal_I2C::scrollDisplayLeft(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void LiquidCrystal_I2C::scrollDisplayRight(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// Установка направления текста слева направо
void LiquidCrystal_I2C::leftToRight(void) {
  _displaymode |= LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// Установка направления текста справа налево
void LiquidCrystal_I2C::rightToLeft(void) {
  _displaymode &= ~LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// Автопрокрутка текста
void LiquidCrystal_I2C::autoscroll(void) {
  _displaymode |= LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// Отключение автопрокрутки текста
void LiquidCrystal_I2C::noAutoscroll(void) {
  _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// Создание пользовательского символа
void LiquidCrystal_I2C::createChar(uint8_t location, uint8_t charmap[]) {
  location &= 0x7;  // Доступно только 8 позиций (0-7)
  command(LCD_SETCGRAMADDR | (location << 3));
  for (int i = 0; i < 8; i++) {
    write(charmap[i]);
  }
}

// Создание пользовательского символа из PROGMEM
void LiquidCrystal_I2C::createChar(uint8_t location, const char *charmap) {
  location &= 0x7;  // Доступно только 8 позиций (0-7)
  command(LCD_SETCGRAMADDR | (location << 3));
  for (int i = 0; i < 8; i++) {
    write(pgm_read_byte_near(charmap++));
  }
}

// Включение/выключение подсветки дисплея
void LiquidCrystal_I2C::noBacklight(void) {
  _backlightval = LCD_NOBACKLIGHT;
  expanderWrite(0);
}

void LiquidCrystal_I2C::backlight(void) {
  _backlightval = LCD_BACKLIGHT;
  expanderWrite(0);
}

// Команды среднего уровня для отправки данных/команд
inline void LiquidCrystal_I2C::command(uint8_t value) {
  send(value, 0);
}

// Низкоуровневые команды для отправки данных
void LiquidCrystal_I2C::send(uint8_t value, uint8_t mode) {
  uint8_t highnib = value & 0xf0;
  uint8_t lownib = (value << 4) & 0xf0;
  write4bits((highnib) | mode);
  write4bits((lownib) | mode);
}

void LiquidCrystal_I2C::write4bits(uint8_t value) {
  expanderWrite(value);
  pulseEnable(value);
}

void LiquidCrystal_I2C::expanderWrite(uint8_t _data) {
  Wire.beginTransmission(_Addr);
  printIIC((int)(_data) | _backlightval);
  Wire.endTransmission();
}

void LiquidCrystal_I2C::pulseEnable(uint8_t _data) {
  expanderWrite(_data | En);  // Устанавливаем En в высокий уровень
  delayMicroseconds(1);       // Импульс En должен быть >450 нс

  expanderWrite(_data & ~En);  // Устанавливаем En в низкий уровень
  delayMicroseconds(50);       // Командам нужно >37 мкс для выполнения
}

// Псевдонимы функций
void LiquidCrystal_I2C::cursor_on() {
  cursor();
}

void LiquidCrystal_I2C::cursor_off() {
  noCursor();
}

void LiquidCrystal_I2C::blink_on() {
  blink();
}

void LiquidCrystal_I2C::blink_off() {
  noBlink();
}

void LiquidCrystal_I2C::load_custom_character(uint8_t char_num, uint8_t *rows) {
  createChar(char_num, rows);
}

void LiquidCrystal_I2C::setBacklight(uint8_t new_val) {
  if (new_val) {
    backlight();  // Включаем подсветку
  } else {
    noBacklight();  // Выключаем подсветку
  }
}

void LiquidCrystal_I2C::printstr(const char c[]) {
  // Эта функция не идентична той, что используется для "настоящих" I2C дисплеев
  // Она здесь для совместимости с пользовательским кодом
  print(c);
}
