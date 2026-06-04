#include "rtc_control.h"

#include <Wire.h>
#include <esp_sleep.h>

namespace {
RTC_DS3231 rtc;
bool rtcReady = false;

bool isValidOneShot(const AppSettings &settings) {
  return settings.oneShotMonth >= 1 && settings.oneShotMonth <= 12 &&
         settings.oneShotDay >= 1 && settings.oneShotDay <= 31 &&
         settings.oneShotHour <= 23 && settings.oneShotMinute <= 59;
}

DateTime oneShotAt(const AppSettings &settings) {
  return DateTime(settings.oneShotYear, settings.oneShotMonth, settings.oneShotDay,
                  settings.oneShotHour, settings.oneShotMinute, 0);
}

bool isValidBleOneShot(const AppSettings &settings) {
  return settings.bleOneShotMonth >= 1 && settings.bleOneShotMonth <= 12 &&
         settings.bleOneShotDay >= 1 && settings.bleOneShotDay <= 31 &&
         settings.bleOneShotHour <= 23 && settings.bleOneShotMinute <= 59;
}

DateTime bleOneShotAt(const AppSettings &settings) {
  return DateTime(settings.bleOneShotYear, settings.bleOneShotMonth, settings.bleOneShotDay,
                  settings.bleOneShotHour, settings.bleOneShotMinute, 0);
}

bool parseWakeTimeToken(const String &token, uint8_t &hour, uint8_t &minute) {
  String trimmed = token;
  trimmed.trim();
  if (trimmed.length() != 5 || trimmed.charAt(2) != ':') {
    return false;
  }

  int parsedHour = trimmed.substring(0, 2).toInt();
  int parsedMinute = trimmed.substring(3, 5).toInt();
  if (parsedHour < 0 || parsedHour > 23 || parsedMinute < 0 || parsedMinute > 59) {
    return false;
  }

  hour = static_cast<uint8_t>(parsedHour);
  minute = static_cast<uint8_t>(parsedMinute);
  return true;
}
}

bool initRtc() {
  Wire.begin(Pins::I2C_SDA, Pins::I2C_SCL);
  rtcReady = rtc.begin();
  Serial.print("RTC begin: ");
  Serial.println(rtcReady ? "OK" : "FAIL");
  if (!rtcReady) {
    return false;
  }

  rtc.disable32K();
  rtc.writeSqwPinMode(DS3231_OFF);
  clearRtcAlarmLatch();
  return true;
}

DateTime getRtcNow() {
  return rtc.now();
}

void setRtcNow(const DateTime &dateTime) {
  if (!rtcReady) {
    return;
  }

  rtc.adjust(dateTime);
}

bool rtcLostPower() {
  return rtcReady && rtc.lostPower();
}

bool rtcAlarmPending() {
  return rtcReady && rtc.alarmFired(1);
}

bool oneShotDue(const AppSettings &settings, const DateTime &now) {
  if (!settings.oneShotEnabled || !isValidOneShot(settings)) {
    return false;
  }

  return oneShotAt(settings) <= now;
}

bool bleOneShotDue(const AppSettings &settings, const DateTime &now) {
  if (!settings.bleOneShotEnabled || !isValidBleOneShot(settings)) {
    return false;
  }

  return bleOneShotAt(settings) <= now;
}

bool nextTriggerForSettings(const AppSettings &settings, const DateTime &now, DateTime &nextTrigger) {
  bool hasTarget = false;

  if (settings.scheduleEnabled) {
    DateTime daily(now.year(), now.month(), now.day(), settings.scheduleHour, settings.scheduleMinute, 0);
    if (daily <= now) {
      daily = daily + TimeSpan(1, 0, 0, 0);
    }
    nextTrigger = daily;
    hasTarget = true;
  }

  if (settings.oneShotEnabled && isValidOneShot(settings)) {
    DateTime exact = oneShotAt(settings);
    if (exact > now && (!hasTarget || exact < nextTrigger)) {
      nextTrigger = exact;
      hasTarget = true;
    }
  }

  return hasTarget;
}

bool nextBleWakeForSettings(const AppSettings &settings, const DateTime &now, DateTime &nextWake) {
  bool hasTarget = false;
  if (settings.bleFixedWakeEnabled) {
    String remaining = settings.bleWakeTimes;
    while (remaining.length() > 0) {
      int separator = remaining.indexOf(',');
      String token = separator >= 0 ? remaining.substring(0, separator) : remaining;
      remaining = separator >= 0 ? remaining.substring(separator + 1) : "";

      uint8_t hour = 0;
      uint8_t minute = 0;
      if (!parseWakeTimeToken(token, hour, minute)) {
        continue;
      }

      DateTime candidate(now.year(), now.month(), now.day(), hour, minute, 0);
      if (candidate <= now) {
        candidate = candidate + TimeSpan(1, 0, 0, 0);
      }

      if (!hasTarget || candidate < nextWake) {
        nextWake = candidate;
        hasTarget = true;
      }
    }
  }

  if (settings.bleOneShotEnabled && isValidBleOneShot(settings)) {
    DateTime exact = bleOneShotAt(settings);
    if (exact > now && (!hasTarget || exact < nextWake)) {
      nextWake = exact;
      hasTarget = true;
    }
  }

  return hasTarget;
}

void syncRtcFromBuildTime() {
  if (!rtcReady) {
    return;
  }

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

bool configureRtcAlarm(const AppSettings &settings, ScheduledWakeKind &kind, DateTime &triggerAt) {
  if (!rtcReady) {
    Serial.println("RTC alarm setup skipped: RTC not ready");
    return false;
  }

  DateTime now = rtc.now();
  DateTime nextSpeaker = now + TimeSpan(365, 0, 0, 0);
  DateTime nextBle = now + TimeSpan(365, 0, 0, 0);
  bool hasSpeaker = nextTriggerForSettings(settings, now, nextSpeaker);
  bool hasBle = nextBleWakeForSettings(settings, now, nextBle);
  Serial.print("RTC now: ");
  Serial.println(formatDateTime(now));
  Serial.print("Has speaker trigger: ");
  Serial.println(hasSpeaker ? "yes" : "no");
  if (hasSpeaker) {
    Serial.print("Next speaker trigger: ");
    Serial.println(formatDateTime(nextSpeaker));
  }
  Serial.print("Has BLE trigger: ");
  Serial.println(hasBle ? "yes" : "no");
  if (hasBle) {
    Serial.print("Next BLE trigger: ");
    Serial.println(formatDateTime(nextBle));
  }
  if (!hasSpeaker && !hasBle) {
    Serial.println("RTC alarm setup skipped: no upcoming trigger");
    return false;
  }

  if (hasSpeaker && (!hasBle || nextSpeaker <= nextBle)) {
    kind = ScheduledWakeKind::Speaker;
    triggerAt = nextSpeaker;
  } else {
    kind = ScheduledWakeKind::Ble;
    triggerAt = nextBle;
  }

  rtc.clearAlarm(1);
  rtc.disableAlarm(2);
  Serial.print("Programming RTC alarm kind: ");
  Serial.println(kind == ScheduledWakeKind::Speaker ? "speaker" : "ble");
  Serial.print("Programming RTC alarm at: ");
  Serial.println(formatDateTime(triggerAt));
  bool result = rtc.setAlarm1(triggerAt, DS3231_A1_Date);
  Serial.print("RTC setAlarm1 result: ");
  Serial.println(result ? "success" : "fail");
  return result;
}

void disableRtcAlarm() {
  if (!rtcReady) {
    return;
  }

  rtc.disableAlarm(1);
  rtc.disableAlarm(2);
  clearRtcAlarmLatch();
}

void clearRtcAlarmLatch() {
  if (!rtcReady) {
    return;
  }

  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
}

WakeTrigger detectWakeTrigger() {
  if (rtcAlarmPending()) {
    Serial.println("Wake cause: RTC alarm pending");
    return WakeTrigger::RtcAlarm;
  }

  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  Serial.print("esp_sleep wake cause: ");
  Serial.println(static_cast<int>(cause));
  if (cause == ESP_SLEEP_WAKEUP_GPIO || cause == ESP_SLEEP_WAKEUP_EXT1 || cause == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Wake cause: GPIO/EXT RTC wake");
    return WakeTrigger::RtcAlarm;
  }
  if (cause == ESP_SLEEP_WAKEUP_TIMER) {
    return WakeTrigger::Timer;
  }
  return WakeTrigger::Other;
}

void configureWakeSources(const AppSettings &settings, bool rtcWakeConfigured) {
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  if (settings.bleIntervalWakeEnabled && (!settings.bleFixedWakeEnabled || !rtcWakeConfigured)) {
    esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(settings.bleWakeIntervalSec) * 1000000ULL);
  }
  pinMode(Pins::RTC_INT, INPUT_PULLUP);
#ifdef BOARD_ADAFRUIT_QTPY_ESP32S3
  esp_sleep_enable_ext1_wakeup_io(1ULL << Pins::RTC_INT, ESP_EXT1_WAKEUP_ANY_LOW);
#else
  esp_deep_sleep_enable_gpio_wakeup(1ULL << Pins::RTC_INT, ESP_GPIO_WAKEUP_GPIO_LOW);
#endif
}

String formatDateTime(const DateTime &dateTime) {
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "%04u-%02u-%02uT%02u:%02u:%02u",
           dateTime.year(), dateTime.month(), dateTime.day(),
           dateTime.hour(), dateTime.minute(), dateTime.second());
  return String(buffer);
}
