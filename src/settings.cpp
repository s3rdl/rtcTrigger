#include "settings.h"

#include <Preferences.h>

namespace {
constexpr char kNamespace[] = "boom";
constexpr char kKeyAuthEnabled[] = "auth_en";
constexpr char kKeyAuthPin[] = "auth_pin";
constexpr char kKeyScheduleEnabled[] = "sched_en";
constexpr char kKeyScheduleHour[] = "sched_hr";
constexpr char kKeyScheduleMinute[] = "sched_min";
constexpr char kKeyOneShotEnabled[] = "one_en";
constexpr char kKeyOneShotYear[] = "one_yr";
constexpr char kKeyOneShotMonth[] = "one_mon";
constexpr char kKeyOneShotDay[] = "one_day";
constexpr char kKeyOneShotHour[] = "one_hr";
constexpr char kKeyOneShotMinute[] = "one_min";
constexpr char kKeyBleFixedWakeEnabled[] = "ble_fix";
constexpr char kKeyBleWakeTimes[] = "ble_times";
constexpr char kKeyBleOneShotEnabled[] = "ble_one_en";
constexpr char kKeyBleOneShotYear[] = "ble_one_yr";
constexpr char kKeyBleOneShotMonth[] = "ble_one_mon";
constexpr char kKeyBleOneShotDay[] = "ble_one_day";
constexpr char kKeyBleOneShotHour[] = "ble_one_hr";
constexpr char kKeyBleOneShotMinute[] = "ble_one_min";
constexpr char kKeyPowerHoldMs[] = "pwr_ms";
constexpr char kKeyBootDelayMs[] = "boot_ms";
constexpr char kKeyModePressMs[] = "mode_ms";
constexpr char kKeyBleWindowMs[] = "ble_ms";
constexpr char kKeyBleIntervalWakeEnabled[] = "ble_int_en";
constexpr char kKeyBleWakeIntervalSec[] = "ble_sec";
constexpr char kKeyBatteryDividerRatioX100[] = "bat_div";
constexpr char kKeyBatteryEmptyMv[] = "bat_emp";
constexpr char kKeyBatteryFullMv[] = "bat_full";
constexpr char kKeyLastEvent[] = "last_evt";
constexpr char kKeyLastEventAt[] = "last_evt_at";
constexpr char kKeySpeakerRuns[] = "spk_runs";
}

AppSettings loadSettings() {
  Preferences preferences;
  preferences.begin(kNamespace, true);

  AppSettings settings{
      .authEnabled = preferences.getBool(kKeyAuthEnabled, Defaults::authEnabled),
      .authPin = preferences.getString(kKeyAuthPin, Defaults::authPin),
      .scheduleEnabled = preferences.getBool(kKeyScheduleEnabled, Defaults::scheduleEnabled),
      .scheduleHour = preferences.getUChar(kKeyScheduleHour, Defaults::scheduleHour),
      .scheduleMinute = preferences.getUChar(kKeyScheduleMinute, Defaults::scheduleMinute),
      .oneShotEnabled = preferences.getBool(kKeyOneShotEnabled, Defaults::oneShotEnabled),
      .oneShotYear = preferences.getUShort(kKeyOneShotYear, Defaults::oneShotYear),
      .oneShotMonth = preferences.getUChar(kKeyOneShotMonth, Defaults::oneShotMonth),
      .oneShotDay = preferences.getUChar(kKeyOneShotDay, Defaults::oneShotDay),
      .oneShotHour = preferences.getUChar(kKeyOneShotHour, Defaults::oneShotHour),
      .oneShotMinute = preferences.getUChar(kKeyOneShotMinute, Defaults::oneShotMinute),
      .bleFixedWakeEnabled = preferences.getBool(kKeyBleFixedWakeEnabled, Defaults::bleFixedWakeEnabled),
      .bleWakeTimes = preferences.getString(kKeyBleWakeTimes, Defaults::bleWakeTimes),
      .bleOneShotEnabled = preferences.getBool(kKeyBleOneShotEnabled, Defaults::bleOneShotEnabled),
      .bleOneShotYear = preferences.getUShort(kKeyBleOneShotYear, Defaults::bleOneShotYear),
      .bleOneShotMonth = preferences.getUChar(kKeyBleOneShotMonth, Defaults::bleOneShotMonth),
      .bleOneShotDay = preferences.getUChar(kKeyBleOneShotDay, Defaults::bleOneShotDay),
      .bleOneShotHour = preferences.getUChar(kKeyBleOneShotHour, Defaults::bleOneShotHour),
      .bleOneShotMinute = preferences.getUChar(kKeyBleOneShotMinute, Defaults::bleOneShotMinute),
      .powerHoldMs = preferences.getUInt(kKeyPowerHoldMs, Defaults::powerHoldMs),
      .bootDelayMs = preferences.getUInt(kKeyBootDelayMs, Defaults::bootDelayMs),
      .modePressMs = preferences.getUInt(kKeyModePressMs, Defaults::modePressMs),
      .bleWindowMs = preferences.getUInt(kKeyBleWindowMs, Defaults::bleWindowMs),
      .bleIntervalWakeEnabled = preferences.getBool(kKeyBleIntervalWakeEnabled, Defaults::bleIntervalWakeEnabled),
      .bleWakeIntervalSec = preferences.getUInt(kKeyBleWakeIntervalSec, Defaults::bleWakeIntervalSec),
      .batteryDividerRatioX100 = preferences.getUShort(kKeyBatteryDividerRatioX100, Defaults::batteryDividerRatioX100),
      .batteryEmptyMv = preferences.getUShort(kKeyBatteryEmptyMv, Defaults::batteryEmptyMv),
      .batteryFullMv = preferences.getUShort(kKeyBatteryFullMv, Defaults::batteryFullMv),
  };

  preferences.end();
  return settings;
}

void saveSettings(const AppSettings &settings) {
  Preferences preferences;
  preferences.begin(kNamespace, false);
  preferences.putBool(kKeyAuthEnabled, settings.authEnabled);
  preferences.putString(kKeyAuthPin, settings.authPin);
  preferences.putBool(kKeyScheduleEnabled, settings.scheduleEnabled);
  preferences.putUChar(kKeyScheduleHour, settings.scheduleHour);
  preferences.putUChar(kKeyScheduleMinute, settings.scheduleMinute);
  preferences.putBool(kKeyOneShotEnabled, settings.oneShotEnabled);
  preferences.putUShort(kKeyOneShotYear, settings.oneShotYear);
  preferences.putUChar(kKeyOneShotMonth, settings.oneShotMonth);
  preferences.putUChar(kKeyOneShotDay, settings.oneShotDay);
  preferences.putUChar(kKeyOneShotHour, settings.oneShotHour);
  preferences.putUChar(kKeyOneShotMinute, settings.oneShotMinute);
  preferences.putBool(kKeyBleFixedWakeEnabled, settings.bleFixedWakeEnabled);
  preferences.putString(kKeyBleWakeTimes, settings.bleWakeTimes);
  preferences.putBool(kKeyBleOneShotEnabled, settings.bleOneShotEnabled);
  preferences.putUShort(kKeyBleOneShotYear, settings.bleOneShotYear);
  preferences.putUChar(kKeyBleOneShotMonth, settings.bleOneShotMonth);
  preferences.putUChar(kKeyBleOneShotDay, settings.bleOneShotDay);
  preferences.putUChar(kKeyBleOneShotHour, settings.bleOneShotHour);
  preferences.putUChar(kKeyBleOneShotMinute, settings.bleOneShotMinute);
  preferences.putUInt(kKeyPowerHoldMs, settings.powerHoldMs);
  preferences.putUInt(kKeyBootDelayMs, settings.bootDelayMs);
  preferences.putUInt(kKeyModePressMs, settings.modePressMs);
  preferences.putUInt(kKeyBleWindowMs, settings.bleWindowMs);
  preferences.putBool(kKeyBleIntervalWakeEnabled, settings.bleIntervalWakeEnabled);
  preferences.putUInt(kKeyBleWakeIntervalSec, settings.bleWakeIntervalSec);
  preferences.putUShort(kKeyBatteryDividerRatioX100, settings.batteryDividerRatioX100);
  preferences.putUShort(kKeyBatteryEmptyMv, settings.batteryEmptyMv);
  preferences.putUShort(kKeyBatteryFullMv, settings.batteryFullMv);
  preferences.end();
}

void resetSettings() {
  Preferences preferences;
  preferences.begin(kNamespace, false);
  preferences.clear();
  preferences.end();
}

EventTelemetry loadEventTelemetry() {
  Preferences preferences;
  preferences.begin(kNamespace, true);

  EventTelemetry telemetry{
      .lastEvent = preferences.getString(kKeyLastEvent, "none"),
      .lastEventAt = preferences.getString(kKeyLastEventAt, ""),
      .speakerRuns = preferences.getUInt(kKeySpeakerRuns, 0),
  };

  preferences.end();
  return telemetry;
}

void saveEventTelemetry(const EventTelemetry &telemetry) {
  Preferences preferences;
  preferences.begin(kNamespace, false);
  preferences.putString(kKeyLastEvent, telemetry.lastEvent);
  preferences.putString(kKeyLastEventAt, telemetry.lastEventAt);
  preferences.putUInt(kKeySpeakerRuns, telemetry.speakerRuns);
  preferences.end();
}

String formatSettings(const AppSettings &settings, float batteryVoltage) {
  String status;
  status.reserve(320);
  status += "auth=";
  status += settings.authEnabled ? "on" : "off";
  status += ",";
  status += "schedule=";
  status += settings.scheduleEnabled ? "on" : "off";
  if (settings.scheduleEnabled) {
    status += ",time=";
    if (settings.scheduleHour < 10) {
      status += '0';
    }
    status += String(settings.scheduleHour);
    status += ':';
    if (settings.scheduleMinute < 10) {
      status += '0';
    }
    status += String(settings.scheduleMinute);
  }
  status += ",oneShot=";
  status += settings.oneShotEnabled ? "on" : "off";
  if (settings.oneShotEnabled) {
    status += ",oneShotAt=";
    status += String(settings.oneShotYear);
    status += '-';
    if (settings.oneShotMonth < 10) {
      status += '0';
    }
    status += String(settings.oneShotMonth);
    status += '-';
    if (settings.oneShotDay < 10) {
      status += '0';
    }
    status += String(settings.oneShotDay);
    status += 'T';
    if (settings.oneShotHour < 10) {
      status += '0';
    }
    status += String(settings.oneShotHour);
    status += ':';
    if (settings.oneShotMinute < 10) {
      status += '0';
    }
    status += String(settings.oneShotMinute);
  }
  status += ",bleFixedWake=";
  status += settings.bleFixedWakeEnabled ? "on" : "off";
  if (settings.bleFixedWakeEnabled) {
    String encodedBleWakeTimes = settings.bleWakeTimes;
    encodedBleWakeTimes.replace(",", "|");
    status += ",bleWakeTimes=" + encodedBleWakeTimes;
  }
  if (settings.bleOneShotEnabled) {
    status += ",bleOneShotAt=";
    status += String(settings.bleOneShotYear);
    status += '-';
    if (settings.bleOneShotMonth < 10) {
      status += '0';
    }
    status += String(settings.bleOneShotMonth);
    status += '-';
    if (settings.bleOneShotDay < 10) {
      status += '0';
    }
    status += String(settings.bleOneShotDay);
    status += 'T';
    if (settings.bleOneShotHour < 10) {
      status += '0';
    }
    status += String(settings.bleOneShotHour);
    status += ':';
    if (settings.bleOneShotMinute < 10) {
      status += '0';
    }
    status += String(settings.bleOneShotMinute);
  }
  status += ",bleWindowMs=" + String(settings.bleWindowMs);
  status += ",bleIntervalWake=";
  status += settings.bleIntervalWakeEnabled ? "on" : "off";
  if (settings.bleIntervalWakeEnabled) {
    status += ",bleWakeSec=" + String(settings.bleWakeIntervalSec);
  }
  status += ",batteryPct=" + String(batteryPercentFromVoltage(settings, batteryVoltage));
  status += ",battery=" + String(batteryVoltage, 2) + "V";
  return status;
}

float scaleBatteryVoltage(const AppSettings &settings, float pinVoltage) {
  return pinVoltage * (static_cast<float>(settings.batteryDividerRatioX100) / 100.0f);
}

uint8_t batteryPercentFromVoltage(const AppSettings &settings, float batteryVoltage) {
  uint16_t batteryMv = static_cast<uint16_t>(batteryVoltage * 1000.0f);
  if (batteryMv <= settings.batteryEmptyMv) {
    return 0;
  }
  if (batteryMv >= settings.batteryFullMv) {
    return 100;
  }

  uint32_t span = settings.batteryFullMv - settings.batteryEmptyMv;
  uint32_t level = batteryMv - settings.batteryEmptyMv;
  return static_cast<uint8_t>((level * 100U) / span);
}
