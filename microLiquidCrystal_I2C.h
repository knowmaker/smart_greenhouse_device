#ifndef microLiquidCrystal_I2C_h
#define microLiquidCrystal_I2C_h

#include <inttypes.h>
#include "Print.h"
#include "microWire.h"

// Команды LCD
#define LCD_CLEARDISPLAY 0x01    // Очистка дисплея
#define LCD_RETURNHOME 0x02      // Возврат курсора на начальную позицию
#define LCD_ENTRYMODESET 0x04    // Установка режима ввода
#define LCD_DISPLAYCONTROL 0x08  // Управление дисплеем (включение/выключение, курсор, мигание)
#define LCD_CURSORSHIFT 0x10     // Сдвиг курсора/дисплея
#define LCD_FUNCTIONSET 0x20     // Установка функциональных параметров дисплея
#define LCD_SETCGRAMADDR 0x40    // Установка адреса CGRAM (пользовательские символы)
#define LCD_SETDDRAMADDR 0x80    // Установка адреса DDRAM (положение текста)

// Флаги режима ввода
#define LCD_ENTRYRIGHT 0x00           // Текст вводится справа налево
#define LCD_ENTRYLEFT 0x02            // Текст вводится слева направо
#define LCD_ENTRYSHIFTINCREMENT 0x01  // Сдвиг текста при вводе вправо
#define LCD_ENTRYSHIFTDECREMENT 0x00  // Без сдвига текста при вводе

// Флаги управления дисплеем
#define LCD_DISPLAYON 0x04   // Включить дисплей
#define LCD_DISPLAYOFF 0x00  // Выключить дисплей
#define LCD_CURSORON 0x02    // Показать курсор
#define LCD_CURSOROFF 0x00   // Скрыть курсор
#define LCD_BLINKON 0x01     // Включить мигание курсора
#define LCD_BLINKOFF 0x00    // Выключить мигание курсора

// Флаги сдвига
#define LCD_DISPLAYMOVE 0x08  // Сдвиг всего дисплея
#define LCD_CURSORMOVE 0x00   // Сдвиг только курсора
#define LCD_MOVERIGHT 0x04    // Сдвиг вправо
#define LCD_MOVELEFT 0x00     // Сдвиг влево

// Флаги настройки дисплея
#define LCD_8BITMODE 0x10  // 8-битный режим
#define LCD_4BITMODE 0x00  // 4-битный режим
#define LCD_2LINE 0x08     // Дисплей с 2 строками
#define LCD_1LINE 0x00     // Дисплей с 1 строкой
#define LCD_5x10DOTS 0x04  // Символы размером 5x10 точек
#define LCD_5x8DOTS 0x00   // Символы размером 5x8 точек

// Флаги управления подсветкой
#define LCD_BACKLIGHT 0x08    // Подсветка включена
#define LCD_NOBACKLIGHT 0x00  // Подсветка выключена

// Биты управления I2C
#define En B00000100  // Enable бит
#define Rw B00000010  // Read/Write бит
#define Rs B00000001  // Register Select бит

class LiquidCrystal_I2C : public Print {
public:
  // Конструктор: принимает адрес дисплея, количество колонок и строк
  LiquidCrystal_I2C(uint8_t lcd_Addr, uint8_t lcd_cols, uint8_t lcd_rows);

  // Инициализация дисплея
  void begin(uint8_t cols, uint8_t rows, uint8_t charsize = LCD_5x8DOTS);

  // Базовые функции
  void clear();                                            // Очистить дисплей
  void home();                                             // Установить курсор в начало
  void noDisplay();                                        // Выключить дисплей
  void display();                                          // Включить дисплей
  void noBlink();                                          // Выключить мигание курсора
  void blink();                                            // Включить мигание курсора
  void noCursor();                                         // Скрыть курсор
  void cursor();                                           // Показать курсор
  void scrollDisplayLeft();                                // Прокрутка текста влево
  void scrollDisplayRight();                               // Прокрутка текста вправо
  void leftToRight();                                      // Режим ввода текста слева направо
  void rightToLeft();                                      // Режим ввода текста справа налево
  void shiftIncrement();                                   // Сдвиг текста вправо при вводе
  void shiftDecrement();                                   // Без сдвига текста при вводе
  void noBacklight();                                      // Выключить подсветку
  void backlight();                                        // Включить подсветку
  void autoscroll();                                       // Автопрокрутка текста
  void noAutoscroll();                                     // Отключить автопрокрутку
  void createChar(uint8_t, uint8_t[]);                     // Создание пользовательских символов
  void createChar(uint8_t location, const char *charmap);  // С PROGMEM

  // Установка позиции курсора
  void setCursor(uint8_t, uint8_t);

  // Вывод данных
#if defined(ARDUINO) && ARDUINO >= 100
  virtual size_t write(uint8_t);  // Для версии Arduino >= 1.0
#else
  virtual void write(uint8_t);  // Для старых версий Arduino
#endif

  void command(uint8_t);  // Отправка команды
  void init();            // Внутренняя инициализация

  // Синонимы функций
  void blink_on();                                              // Включить мигание
  void blink_off();                                             // Выключить мигание
  void cursor_on();                                             // Включить курсор
  void cursor_off();                                            // Выключить курсор
  void setBacklight(uint8_t new_val);                           // Управление подсветкой
  void load_custom_character(uint8_t char_num, uint8_t *rows);  // Загрузка пользовательских символов
  void printstr(const char[]);                                  // Печать строки

private:
  void init_priv();             // Приватная инициализация
  void send(uint8_t, uint8_t);  // Отправка данных или команды
  void write4bits(uint8_t);     // Запись 4 бит данных
  void expanderWrite(uint8_t);  // Запись через расширитель
  void pulseEnable(uint8_t);    // Пульс сигнала Enable
  uint8_t _Addr;                // Адрес I2C устройства
  uint8_t _displayfunction;     // Конфигурация дисплея
  uint8_t _displaycontrol;      // Управление дисплеем
  uint8_t _displaymode;         // Режим ввода текста
  uint8_t _numlines;            // Количество строк
  uint8_t _cols;                // Количество колонок
  uint8_t _rows;                // Количество строк
  uint8_t _backlightval;        // Значение подсветки
};

#endif
