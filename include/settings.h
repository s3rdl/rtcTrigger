#pragma once

#include <Arduino.h>

struct AppSettings {
  bool authEnabled;
  String authPin;
  bool scheduleEnabled;
  uint8_t scheduleHour;
  uint8_t scheduleMinute;
  bool oneShotEnabled;
  uint16_t oneShotYear;
  uint8_t oneShotMonth;
  uint8_t oneShotDay;
  uint8_t oneShotHour;
  uint8_t oneShotMinute;
  bool bleFixedWakeEnabled;
  String bleWakeTimes;
  uint32_t powerHoldMs;
  uint32_t bootDelayMs;
  uint32_t modePressMs;
  uint32_t bleWindowMs;
  uint32_t bleWakeIntervalSec;
  uint16_t batteryDividerRatioX100;
  uint16_t batteryEmptyMv;
  uint16_t batteryFullMv;
};

struct EventTelemetry {
  String lastEvent;
  String lastEventAt;
  uint32_t speakerRuns;
};

namespace Pins {
#ifdef BOARD_ADAFRUIT_QTPY_ESP32S3
constexpr uint8_t POWER_RELAY = 18;
constexpr uint8_t MODE_RELAY = 17;
constexpr uint8_t RTC_INT = 16;
constexpr uint8_t I2C_SDA = 41;
constexpr uint8_t I2C_SCL = 40;
constexpr uint8_t BATTERY_ADC = 9;
#elif defined(BOARD_SEEED_XIAO_ESP32C3)
constexpr uint8_t POWER_RELAY = 3;
constexpr uint8_t MODE_RELAY = 4;
constexpr uint8_t RTC_INT = 5;
constexpr uint8_t I2C_SDA = 6;
constexpr uint8_t I2C_SCL = 7;
constexpr uint8_t BATTERY_ADC = 2;
#else
constexpr uint8_t POWER_RELAY = 6;
constexpr uint8_t MODE_RELAY = 7;
constexpr uint8_t RTC_INT = 2;
constexpr uint8_t I2C_SDA = 8;
constexpr uint8_t I2C_SCL = 9;
constexpr uint8_t BATTERY_ADC = 0;
#endif
constexpr uint8_t RECOVERY_BUTTON = 0;
}

namespace Defaults {
constexpr bool authEnabled = true;
constexpr char authPin[] = "123456";
constexpr bool scheduleEnabled = false;
constexpr uint8_t scheduleHour = 8;
constexpr uint8_t scheduleMinute = 0;
constexpr bool oneShotEnabled = false;
constexpr uint16_t oneShotYear = 2026;
constexpr uint8_t oneShotMonth = 1;
constexpr uint8_t oneShotDay = 1;
constexpr uint8_t oneShotHour = 8;
constexpr uint8_t oneShotMinute = 0;
constexpr bool bleFixedWakeEnabled = false;
constexpr char bleWakeTimes[] = "08:00,12:00,18:00";
constexpr uint32_t powerHoldMs = 2200;
constexpr uint32_t bootDelayMs = 8000;
constexpr uint32_t modePressMs = 1000;
constexpr uint32_t bleWindowMs = 10000;
constexpr uint32_t bleWakeIntervalSec = 60;
constexpr uint16_t batteryDividerRatioX100 = 200;
constexpr uint16_t batteryEmptyMv = 3300;
constexpr uint16_t batteryFullMv = 4200;
}

AppSettings loadSettings();
void saveSettings(const AppSettings &settings);
void resetSettings();
EventTelemetry loadEventTelemetry();
void saveEventTelemetry(const EventTelemetry &telemetry);
String formatSettings(const AppSettings &settings, float batteryVoltage);
float scaleBatteryVoltage(const AppSettings &settings, float pinVoltage);
uint8_t batteryPercentFromVoltage(const AppSettings &settings, float batteryVoltage);
