#define TINY_GSM_MODEM_SIM800
// #define SerialMon Serial

#include <AltSoftSerial.h>
AltSoftSerial SerialAT;

#define TINY_GSM_YIELD() \
  { delay(2); }

const char apn[] = "internet.mts.ru";
const char gprsUser[] = "mts";
const char gprsPass[] = "mts";
const char* broker = "broker.emqx.io";

#include <EEPROM.h>
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include "microWire.h"
#include "microLiquidCrystal_I2C.h"
#include "dht11.h"
#include "oneDS18B20.h"
#include "myDS3231.h"

#define PUB_DELAY (5 * 1000)

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

uint32_t lastReconnectAttempt = 0;

#define MOTOR1_DIR 2
#define MOTOR1_PWM 3
#define MOTOR2_DIR 4
#define MOTOR2_PWM 5
#define WATER_TEMP_SENSOR_PIN 6
#define VALVE1_PIN 7
#define VALVE2_PIN 10
#define FILL_VALVE_PIN 11
#define PUMP_PIN 12
#define LED_PIN 13
#define WATER_LEVEL_HIGH A1
#define WATER_LEVEL_LOW A0
#define LIGHT_SENSOR A3
#define AIR_TEMP_SENSOR_PIN A2
#define SOIL_MOISTURE_SENSOR2 A6
#define SOIL_MOISTURE_SENSOR1 A7
#define MOTION_SENSOR A10

#define ADDR_AIR_TEMP_THRESHOLD 0
#define ADDR_AIR_HUMIDITY_THRESHOLD 2
#define ADDR_SOIL_MOISTURE_THRESHOLD1 4
#define ADDR_SOIL_MOISTURE_THRESHOLD2 6
#define ADDR_WATER_TEMP_THRESHOLD1 8
#define ADDR_WATER_TEMP_THRESHOLD2 10
#define ADDR_WATER_LEVEL_THRESHOLD 12
#define ADDR_LIGHT_THRESHOLD 14
#define ADDR_MOTION_THRESHOLD 16

LiquidCrystal_I2C lcd(0x27, 20, 4);

OneDS18B20 waterTempSensor(WATER_TEMP_SENSOR_PIN);
DHT11 airTempSensor(AIR_TEMP_SENSOR_PIN);

MyDS3231 rtc;

char rtcDateTime[21];

int waterTemp = 0;
int airTemp = 0;
int airHumidity = 0;
bool waterLevelHigh = false;
bool waterLevelLow = false;
bool isDark = false;
int soilMoisture1 = 0;
int soilMoisture2 = 0;
bool isMotionDetected = false;

int airTempThreshold = 28;
int airHumidityThreshold = 40;
int soilMoistureThreshold1 = 50;
int soilMoistureThreshold2 = 50;
int waterTempThreshold1 = 23;
int waterTempThreshold2 = 23;
int waterLevelThreshold = 2;
bool lightThreshold = true;
bool motionThreshold = true;

unsigned long wateringStartTime = 0;

bool separateBedControl = false;
bool separateWindowControl = false;  // false - одно окно

bool manualWatering = false;
bool manualLighting = false;
bool manualVentilation = false;

bool curLightingState = false;
bool curVentilationState = false;
bool curWateringState1 = false;
bool curWateringState2 = false;

unsigned long lastPublishTime = 0;
#define PUBLISH_INTERVAL 60000

char password[5];
uint32_t guid = 0;
char guidStr[9];
bool registrationSent = false;

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  payload[len] = '\0';

  char topicPrefix[11];
  snprintf(topicPrefix, sizeof(topicPrefix), "m/%08X", guid);

  if (strncmp(topic, topicPrefix, 10) == 0) {
    if (strncmp(topic + 10, "/c/ventilation", 15) == 0) {
      int value = atoi((char*)payload);
      if (value != curVentilationState) {
        manualVentilation = !manualVentilation;
        controlVentilation(value);
      }
    } else if (strncmp(topic + 10, "/c/watering1", 12) == 0) {
      int value = atoi((char*)payload);
      manualWatering = true;
      controlWatering(1, value);
      wateringStartTime = millis();
    } else if (strncmp(topic + 10, "/c/watering2", 12) == 0) {
      int value = atoi((char*)payload);
      manualWatering = true;
      controlWatering(2, value);
      wateringStartTime = millis();
    } else if (strncmp(topic + 10, "/c/lighting", 11) == 0) {
      int value = atoi((char*)payload);
      if (value != curLightingState) {
        manualLighting = !manualLighting;
        controlLighting(value);
      }
    } else if (strncmp(topic + 10, "/s/update", 9) == 0) {
      updateParameters((char*)payload);
    } else if (strncmp(topic + 10, "/c/demo", 7) == 0) {
      demoMode();
    }
  }
}

boolean mqttConnect() {
  boolean status = mqtt.connect(guidStr);

  char topic[25];

  snprintf(topic, sizeof(topic), "m/%08X/c/ventilation", guid);
  mqtt.subscribe(topic);

  snprintf(topic, sizeof(topic), "m/%08X/c/watering1", guid);
  mqtt.subscribe(topic);

  snprintf(topic, sizeof(topic), "m/%08X/c/watering2", guid);
  mqtt.subscribe(topic);

  snprintf(topic, sizeof(topic), "m/%08X/c/lighting", guid);
  mqtt.subscribe(topic);

  snprintf(topic, sizeof(topic), "m/%08X/s/update", guid);
  mqtt.subscribe(topic);

  snprintf(topic, sizeof(topic), "m/%08X/c/demo", guid);
  mqtt.subscribe(topic);

  return mqtt.connected();
}

void setup() {
  //  SerialMon.begin(115200);
  //  delay(10);

  SerialAT.begin(9600);
  delay(3000);

  // modem.restart();
  modem.init();

  if (!modem.waitForNetwork()) {
    // SerialMon.println(" fail");
    delay(10000);
    return;
  }
  // SerialMon.println(" success");

  // if (modem.isNetworkConnected()) {
  //   SerialMon.println("Network connected");
  // }

  // GPRS connection parameters are usually set after network registration
  // SerialMon.print(F("Connecting to "));
  // SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    // SerialMon.println(" fail");
    delay(10000);
    return;
  }
  // SerialMon.println(" success");

  // if (modem.isGprsConnected()) {
  //   SerialMon.println("GPRS connected");
  // }

  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);

  guid = *(uint32_t*)&GUID0;
  snprintf(guidStr, sizeof(guidStr), "%08X", guid);

  pinMode(MOTOR1_DIR, OUTPUT);
  pinMode(MOTOR1_PWM, OUTPUT);
  pinMode(MOTOR2_DIR, OUTPUT);
  pinMode(MOTOR2_PWM, OUTPUT);
  pinMode(VALVE1_PIN, OUTPUT);
  pinMode(VALVE2_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(FILL_VALVE_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(WATER_LEVEL_HIGH, INPUT);
  pinMode(WATER_LEVEL_LOW, INPUT);
  pinMode(LIGHT_SENSOR, INPUT);
  pinMode(MOTION_SENSOR, INPUT);

  randomSeed(millis());
  int randomPassword = random(10000);
  snprintf(password, sizeof(password), "%04d", randomPassword);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(F("Initializing..."));
  lcd.setCursor(0, 1);
  lcd.print(F("GUID:"));
  lcd.setCursor(5, 1);
  lcd.print(guidStr);
  lcd.setCursor(0, 2);
  lcd.print(F("PIN:"));
  lcd.setCursor(4, 2);
  lcd.print(password);

  delay(3000);  // Пауза для отображения информации на экране

  rtc.begin();
  updateTimeFromGSM();

  airTempThreshold = readThresholdFromEEPROM(ADDR_AIR_TEMP_THRESHOLD);
  airHumidityThreshold = readThresholdFromEEPROM(ADDR_AIR_HUMIDITY_THRESHOLD);
  soilMoistureThreshold1 = readThresholdFromEEPROM(ADDR_SOIL_MOISTURE_THRESHOLD1);
  soilMoistureThreshold2 = readThresholdFromEEPROM(ADDR_SOIL_MOISTURE_THRESHOLD2);
  waterTempThreshold1 = readThresholdFromEEPROM(ADDR_WATER_TEMP_THRESHOLD1);
  waterTempThreshold2 = readThresholdFromEEPROM(ADDR_WATER_TEMP_THRESHOLD2);
  waterLevelThreshold = readThresholdFromEEPROM(ADDR_WATER_LEVEL_THRESHOLD);
  lightThreshold = readThresholdFromEEPROM(ADDR_LIGHT_THRESHOLD);
  motionThreshold = readThresholdFromEEPROM(ADDR_MOTION_THRESHOLD);
}

void loop() {
  if (!modem.isNetworkConnected() || !mqtt.connected()) {
    uint32_t t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
    }
    delay(100);
    return;
  }

  if (!registrationSent) {
    char topic[15];
    snprintf(topic, sizeof(topic), "m/%08X/reg", guid);

    mqtt.publish(topic, password);
    registrationSent = true;

    delay(100);
    publishStates();
    delay(100);
    publishParameters();
  }

  mqtt.loop();

  checkSensors();
  controlDevices();

  rtc.getDateTimeChar(rtcDateTime);

  displayData();

  unsigned long currentMillis = millis();
  if (currentMillis - lastPublishTime >= PUBLISH_INTERVAL) {
    publishData();
    lastPublishTime = currentMillis;
  }

  delay(100);
}

void checkSensors() {
  waterTempSensor.requestTemp();
  waterTemp = waterTempSensor.readTemp() ? waterTempSensor.getTempInt() : 0;

  int twoValues = airTempSensor.readTemperatureHumidity(airTemp, airHumidity);
  // if (twoValues != 0) {
  //   airTemp = 0;
  //   airHumidity = 0;
  // }

  waterLevelHigh = digitalRead(WATER_LEVEL_HIGH);
  waterLevelLow = digitalRead(WATER_LEVEL_LOW);
  isDark = digitalRead(LIGHT_SENSOR);

  soilMoisture1 = map(analogRead(SOIL_MOISTURE_SENSOR1), 670, 270, 0, 100);
  soilMoisture2 = map(analogRead(SOIL_MOISTURE_SENSOR2), 670, 270, 0, 100);

  isMotionDetected = digitalRead(MOTION_SENSOR);
}

void controlDevices() {
  if (!manualWatering) {
    if (separateBedControl) {
      if (soilMoisture1 < soilMoistureThreshold1 && waterLevelLow && waterTemp >= waterTempThreshold1) {
        controlWatering(1, 1);
      } else {
        controlWatering(1, 0);
      }

      if (soilMoisture2 < soilMoistureThreshold2 && waterLevelLow && waterTemp >= waterTempThreshold2) {
        controlWatering(2, 1);
      } else {
        controlWatering(2, 0);
      }
    } else {
      int averageMoisture = (soilMoisture1 + soilMoisture2) / 2;
      if (averageMoisture < (soilMoistureThreshold1 + soilMoistureThreshold2) / 2 && waterLevelLow && waterTemp >= (waterTempThreshold1 + waterTempThreshold2) / 2) {
        controlWatering(2, 1);
      } else {
        controlWatering(2, 0);
      }
    }
  }

  if (!manualVentilation) {
    if (airTemp > airTempThreshold && airHumidity > airHumidityThreshold) {
      controlVentilation(1);
    } else {
      controlVentilation(0);
    }
  }

  if (!manualLighting) {
    if ((lightThreshold && isDark)  && (!motionThreshold || (motionThreshold && !isMotionDetected))) {
      controlLighting(1);
    } else {
      controlLighting(0);
    }
  }

  if ((!waterLevelLow || waterLevelThreshold == 2) && !curWateringState1 && !curWateringState2) {
    controlWaterLevel(1);
  } else {
    controlWaterLevel(0);
  }

  if (manualWatering && millis() - wateringStartTime >= 5000) {
    controlWatering(1, 0);
    controlWatering(2, 0);
    manualWatering = false;
  }
}

void controlLighting(int state) {
  digitalWrite(LED_PIN, state);

  if (curLightingState != state) {
    curLightingState = state;
    publishStates();
  }
}

void controlWatering(int bed, int state) {
  if (bed == 1) {
    digitalWrite(PUMP_PIN, state);
    delay(10);
    digitalWrite(VALVE1_PIN, state);
    if (curWateringState1 != state) {
      curWateringState1 = state;
      publishStates();
    }
  } else if (bed == 2) {
    digitalWrite(PUMP_PIN, state);
    delay(10);
    digitalWrite(VALVE2_PIN, state);
    if (curWateringState2 != state) {
      curWateringState2 = state;
      publishStates();
    }
  }
}

void controlVentilation(int state) {
  if (state) {
    if (separateWindowControl) {
      digitalWrite(MOTOR1_DIR, LOW);
      digitalWrite(MOTOR1_PWM, HIGH);
      digitalWrite(MOTOR2_DIR, LOW);
      digitalWrite(MOTOR2_PWM, HIGH);
    } else {
      digitalWrite(MOTOR2_DIR, LOW);
      digitalWrite(MOTOR2_PWM, HIGH);
    }
  } else {
    if (separateWindowControl) {
      digitalWrite(MOTOR1_DIR, HIGH);
      digitalWrite(MOTOR1_PWM, LOW);
      digitalWrite(MOTOR2_DIR, HIGH);
      digitalWrite(MOTOR2_PWM, LOW);
    } else {
      digitalWrite(MOTOR2_DIR, HIGH);
      digitalWrite(MOTOR2_PWM, LOW);
    }
  }

  if (curVentilationState != state) {
    curVentilationState = state;
    publishStates();
  }
}

void controlWaterLevel(int state) {
  digitalWrite(FILL_VALVE_PIN, state);

  // publishStates();
}

void displayData() {
  lcd.setCursor(0, 0);
  lcd.print(rtcDateTime);

  lcd.setCursor(0, 1);
  lcd.print(F("Air T:"));
  lcd.print(airTemp);
  lcd.write(223);  // Символ "градуса"
  lcd.print(F("C H:"));
  lcd.print(airHumidity);
  lcd.print(F("%"));
  lcd.setCursor(17, 1);
  lcd.print(isDark ? F("Eve") : F("Day"));

  lcd.setCursor(0, 2);
  lcd.print(F("Water T:"));
  lcd.print(waterTemp);
  lcd.write(223);
  lcd.print(F("C Vol:"));
  if (waterLevelLow == 0 && waterLevelHigh == 0) {
    lcd.write(255);  // один заполненный квадрат
    lcd.write(219);  // два пустых квадрата
    lcd.write(219);
  } else if (waterLevelLow == 1 && waterLevelHigh == 0) {
    lcd.write(255);  // два заполненных квадрата
    lcd.write(255);
    lcd.write(219);  // один пустой квадрат
  } else if (waterLevelLow == 1 && waterLevelHigh == 1) {
    lcd.write(255);  // три заполненных квадрата
    lcd.write(255);
    lcd.write(255);
  }

  lcd.setCursor(0, 3);
  lcd.print(F("Soil 1:"));
  lcd.print(soilMoisture1);
  lcd.print(F("% 2:"));
  lcd.print(soilMoisture2);
  lcd.print(F("% "));

  lcd.setCursor(19, 3);
  lcd.write(isMotionDetected ? 252 : 244);
}

void publishData() {
  char topic[17];
  char data[67];

  snprintf(topic, sizeof(topic), "m/%08X/d/cur", guid);
  snprintf(data, sizeof(data), "{\"1\":%d,\"2\":%d,\"3\":%d,\"4\":%d,\"5\":%d,\"6\":%d,\"7\":%d}",
           airTemp, airHumidity, soilMoisture1, soilMoisture2, waterTemp,
           waterLevelLow ? (waterLevelHigh ? 2 : 1) : 0,
           isDark ? 1 : 0);
  mqtt.publish(topic, data);
}

void publishStates() {
  char topic[18];
  char states[38];

  snprintf(topic, sizeof(topic), "m/%08X/st/cur", guid);
  snprintf(states, sizeof(states), "{\"1\":%c,\"2\":%c,\"3\":%c,\"5\":%c}",
           curVentilationState ? '1' : '0',
           curWateringState1 ? '1' : '0',
           curWateringState2 ? '1' : '0',
           curLightingState ? '1' : '0');
  mqtt.publish(topic, states);
}

void publishParameters() {
  char topic[17];
  char settings[85];

  snprintf(topic, sizeof(topic), "m/%08X/s/cur", guid);
  snprintf(settings, sizeof(settings), "{\"1\":%d,\"2\":%d,\"3\":%d,\"4\":%d,\"5\":%d,\"6\":%d,\"7\":%d,\"8\":%d,\"9\":%d}",
           airTempThreshold, airHumidityThreshold, soilMoistureThreshold1, soilMoistureThreshold2, waterTempThreshold1, waterTempThreshold2, waterLevelThreshold, lightThreshold, motionThreshold);
  mqtt.publish(topic, settings);
}

void updateParameters(const char* payload) {
  sscanf(payload, "{\"1\":%d,\"2\":%d,\"3\":%d,\"4\":%d,\"5\":%d,\"6\":%d,\"7\":%d,\"8\":%d,\"9\":%d}",
         &airTempThreshold, &airHumidityThreshold, &soilMoistureThreshold1, &soilMoistureThreshold2, &waterTempThreshold1, &waterTempThreshold2, &waterLevelThreshold, &lightThreshold, &motionThreshold);

  saveThresholdToEEPROM(ADDR_AIR_TEMP_THRESHOLD, airTempThreshold);
  saveThresholdToEEPROM(ADDR_AIR_HUMIDITY_THRESHOLD, airHumidityThreshold);
  saveThresholdToEEPROM(ADDR_SOIL_MOISTURE_THRESHOLD1, soilMoistureThreshold1);
  saveThresholdToEEPROM(ADDR_SOIL_MOISTURE_THRESHOLD2, soilMoistureThreshold2);
  saveThresholdToEEPROM(ADDR_WATER_TEMP_THRESHOLD1, waterTempThreshold1);
  saveThresholdToEEPROM(ADDR_WATER_TEMP_THRESHOLD2, waterTempThreshold2);
  saveThresholdToEEPROM(ADDR_WATER_LEVEL_THRESHOLD, waterLevelThreshold);
  saveThresholdToEEPROM(ADDR_LIGHT_THRESHOLD, lightThreshold);
  saveThresholdToEEPROM(ADDR_MOTION_THRESHOLD, motionThreshold);

  publishParameters();
}

void updateTimeFromGSM() {
  String dateTime = modem.getGSMDateTime(DATE_FULL);

  if (dateTime != "") {
    int year = dateTime.substring(0, 2).toInt();
    int month = dateTime.substring(3, 5).toInt();
    int date = dateTime.substring(6, 8).toInt();
    int hours = dateTime.substring(9, 11).toInt();
    int minutes = dateTime.substring(12, 14).toInt();
    int seconds = dateTime.substring(15, 17).toInt();

    rtc.setTime(seconds, minutes, hours, date, month, year + 2000);
  }
}

void demoMode() {
  lcd.setCursor(0, 0);
  lcd.print(F("Demo mode is running"));

  controlVentilation(1);
  delay(15000);
  controlWatering(2, 1);
  delay(2500);
  controlWatering(2, 0);
  delay(5500);
  controlWaterLevel(1);
  delay(2500);
  controlWaterLevel(0);
  delay(2900);
  controlLighting(1);
  delay(5000);
  controlLighting(0);
  delay(1300);
  controlVentilation(0);
  delay(7000);
}

void saveThresholdToEEPROM(int address, int value) {
  EEPROM.put(address, value);
}

int readThresholdFromEEPROM(int address) {
  int value;
  EEPROM.get(address, value);
  return value;
}
