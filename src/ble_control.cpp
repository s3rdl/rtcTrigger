#include "ble_control.h"

#include <NimBLEDevice.h>
#include <esp_mac.h>

#include "rtc_control.h"
#include "speaker_control.h"
#include "status_led.h"

namespace {
constexpr char kServiceUuid[] = "7a4b1001-4b85-4cc0-98f6-5cb3d5bdb001";
constexpr char kCommandUuid[] = "7a4b1002-4b85-4cc0-98f6-5cb3d5bdb001";
constexpr char kStatusUuid[] = "7a4b1003-4b85-4cc0-98f6-5cb3d5bdb001";
constexpr char kBatteryServiceUuid[] = "180F";
constexpr char kBatteryLevelUuid[] = "2A19";
constexpr uint32_t kStatusNotifyIntervalMs = 2000;

BleRuntimeState *gState = nullptr;
NimBLECharacteristic *gStatusCharacteristic = nullptr;
NimBLECharacteristic *gBatteryCharacteristic = nullptr;
bool gClientConnected = false;

String uniqueDeviceName() {
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_BT);
  char name[32];
  snprintf(name, sizeof(name), "Boom Remote %02X%02X", mac[4], mac[5]);
  return String(name);
}

void persistSettingsIfNeeded() {
  if (gState == nullptr || !gState->settingsChanged || gState->persistSettings == nullptr) {
    return;
  }

  gState->persistSettings(*gState);
  gState->settingsChanged = false;
}

bool commandRequiresAuth(const String &upper) {
  return upper != "GET_STATUS" && !upper.startsWith("AUTH ");
}

String trimCommand(const std::string &value) {
  String command(value.c_str());
  command.trim();
  return command;
}

bool parseClockValue(const String &value, uint8_t &hour, uint8_t &minute) {
  int separator = value.indexOf(':');
  if (separator < 0) {
    return false;
  }

  int parsedHour = value.substring(0, separator).toInt();
  int parsedMinute = value.substring(separator + 1).toInt();
  if (parsedHour < 0 || parsedHour > 23 || parsedMinute < 0 || parsedMinute > 59) {
    return false;
  }

  hour = static_cast<uint8_t>(parsedHour);
  minute = static_cast<uint8_t>(parsedMinute);
  return true;
}

bool parseDateTimeValue(const String &value, uint16_t &year, uint8_t &month, uint8_t &day,
                        uint8_t &hour, uint8_t &minute) {
  if (value.length() < 16) {
    return false;
  }

  year = static_cast<uint16_t>(value.substring(0, 4).toInt());
  month = static_cast<uint8_t>(value.substring(5, 7).toInt());
  day = static_cast<uint8_t>(value.substring(8, 10).toInt());
  hour = static_cast<uint8_t>(value.substring(11, 13).toInt());
  minute = static_cast<uint8_t>(value.substring(14, 16).toInt());

  return year >= 2024 && month >= 1 && month <= 12 && day >= 1 && day <= 31 &&
         hour <= 23 && minute <= 59;
}

bool parseTimingValues(const String &value, uint32_t &powerMs, uint32_t &bootMs,
                       uint32_t &modeMs, uint32_t &bleWindowMs, uint32_t &bleWakeSec) {
  int first = value.indexOf(',');
  int second = value.indexOf(',', first + 1);
  int third = value.indexOf(',', second + 1);
  int fourth = value.indexOf(',', third + 1);
  if (first < 0 || second < 0 || third < 0 || fourth < 0) {
    return false;
  }

  powerMs = value.substring(0, first).toInt();
  bootMs = value.substring(first + 1, second).toInt();
  modeMs = value.substring(second + 1, third).toInt();
  bleWindowMs = value.substring(third + 1, fourth).toInt();
  bleWakeSec = value.substring(fourth + 1).toInt();
  return powerMs > 0 && bootMs > 0 && modeMs > 0 && bleWindowMs > 0 && bleWakeSec > 0;
}

String compactTimesToDisplay(const String &compact) {
  String display;
  for (int i = 0; i + 3 < compact.length(); i += 4) {
    if (display.length() > 0) {
      display += ',';
    }
    display += compact.substring(i, i + 2);
    display += ':';
    display += compact.substring(i + 2, i + 4);
  }
  return display;
}

bool parseFixedWakeValues(const String &value, bool &enabled, String &times) {
  if (value.length() < 2) {
    return false;
  }

  char flag = value.charAt(0);
  if ((flag != '0' && flag != '1') || value.charAt(1) != ' ') {
    return false;
  }

  enabled = flag == '1';
  String rawTimes = value.substring(2);
  rawTimes.trim();
  rawTimes.replace(",", "");
  rawTimes.replace(":", "");
  if (rawTimes.length() % 4 != 0) {
    return false;
  }
  times = compactTimesToDisplay(rawTimes);
  return true;
}

bool parseFixedWakeValuesCompact(const String &value, bool &enabled, String &times) {
  if (value.length() < 1) {
    return false;
  }

  char flag = value.charAt(0);
  if (flag != '0' && flag != '1') {
    return false;
  }

  enabled = flag == '1';
  String rawTimes = value.substring(1);
  rawTimes.trim();
  rawTimes.replace(",", "");
  rawTimes.replace(":", "");
  if (rawTimes.length() % 4 != 0) {
    return false;
  }
  times = compactTimesToDisplay(rawTimes);
  return true;
}

DateTime parseRtcDateTime(const String &value) {
  uint16_t year = 0;
  uint8_t month = 0;
  uint8_t day = 0;
  uint8_t hour = 0;
  uint8_t minute = 0;
  if (!parseDateTimeValue(value, year, month, day, hour, minute)) {
    return DateTime();
  }

  return DateTime(year, month, day, hour, minute, 0);
}

void updateStatusValue() {
  if (gState == nullptr || gStatusCharacteristic == nullptr) {
    return;
  }

  if (gState->refreshRuntimeState != nullptr) {
    gState->refreshRuntimeState(*gState);
  }

  String status = formatSettings(gState->settings, gState->batteryVoltage);
  status += ",authenticated=";
  status += gState->authenticated ? "yes" : "no";
  status += ",rtcNow=" + gState->rtcNow;
  status += ",nextTrigger=" + gState->nextTrigger;
  status += ",lastEvent=" + gState->lastEvent;
  status += ",lastEventAt=" + gState->lastEventAt;
  status += ",speakerRuns=" + String(gState->speakerRuns);
  gStatusCharacteristic->setValue(reinterpret_cast<const uint8_t *>(status.c_str()), status.length());
  if (gBatteryCharacteristic != nullptr) {
    gBatteryCharacteristic->setValue(&gState->batteryPercent, 1);
  }
  if (gClientConnected) {
    gStatusCharacteristic->notify();
    if (gBatteryCharacteristic != nullptr) {
      gBatteryCharacteristic->notify();
    }
  }
}

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *server, ble_gap_conn_desc *desc) override {
    gClientConnected = true;
    setStatusLedConnected(true);
    if (gState != nullptr) {
      gState->authenticated = !gState->settings.authEnabled;
    }
    if (desc != nullptr) {
      server->updateConnParams(desc->conn_handle, 24, 48, 0, 60);
    }
  }

  void onDisconnect(NimBLEServer *, ble_gap_conn_desc *) override {
    gClientConnected = false;
    setStatusLedConnected(false);
    NimBLEDevice::startAdvertising();
  }
};

class CommandCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *characteristic, ble_gap_conn_desc *) override {
    if (gState == nullptr) {
      return;
    }

    String command = trimCommand(characteristic->getValue());
    String upper = command;
    upper.toUpperCase();

    if (upper.startsWith("AUTH ") || upper.startsWith("A ")) {
      int pinOffset = upper.startsWith("A ") ? 2 : 5;
      if (!gState->settings.authEnabled || command.substring(pinOffset) == gState->settings.authPin) {
        gState->authenticated = true;
      }
      updateStatusValue();
      return;
    }

    if (gState->settings.authEnabled && !gState->authenticated && commandRequiresAuth(upper)) {
      updateStatusValue();
      return;
    }

    if (upper == "TURN_ON_NOW") {
      runSpeakerSequence(gState->settings);
    } else if (upper == "PRESS_POWER") {
      pressPowerButton(gState->settings.powerHoldMs);
    } else if (upper == "PRESS_MODE") {
      pressModeButton(gState->settings.modePressMs);
    } else if (upper == "ENABLE_AUTH") {
      gState->settings.authEnabled = true;
      gState->authenticated = true;
      gState->settingsChanged = true;
    } else if (upper == "DISABLE_AUTH") {
      gState->settings.authEnabled = false;
      gState->authenticated = true;
      gState->settingsChanged = true;
    } else if (upper.startsWith("SET_PIN ") || upper.startsWith("PIN ")) {
      String newPin = command.substring(upper.startsWith("PIN ") ? 4 : 8);
      newPin.trim();
      if (newPin.length() >= 4 && newPin.length() <= 12) {
        gState->settings.authPin = newPin;
        gState->authenticated = true;
        gState->settingsChanged = true;
      }
    } else if (upper == "ENABLE_SCHEDULE") {
      gState->settings.scheduleEnabled = true;
      gState->settingsChanged = true;
    } else if (upper == "DISABLE_SCHEDULE") {
      gState->settings.scheduleEnabled = false;
      gState->settingsChanged = true;
    } else if (upper == "GET_STATUS") {
      gState->statusRequested = true;
    } else if (upper == "CLEAR_ONESHOT") {
      gState->settings.oneShotEnabled = false;
      gState->settingsChanged = true;
    } else if (upper.startsWith("SET_SCHEDULE ") || upper.startsWith("SCH ")) {
      uint8_t hour = 0;
      uint8_t minute = 0;
      if (parseClockValue(command.substring(upper.startsWith("SCH ") ? 4 : 13), hour, minute)) {
        gState->settings.scheduleHour = hour;
        gState->settings.scheduleMinute = minute;
        gState->settingsChanged = true;
      }
    } else if (upper.startsWith("SET_ONESHOT ") || upper.startsWith("ONE ")) {
      uint16_t year = 0;
      uint8_t month = 0;
      uint8_t day = 0;
      uint8_t hour = 0;
      uint8_t minute = 0;
      if (parseDateTimeValue(command.substring(upper.startsWith("ONE ") ? 4 : 12), year, month, day, hour, minute)) {
        gState->settings.oneShotEnabled = true;
        gState->settings.oneShotYear = year;
        gState->settings.oneShotMonth = month;
        gState->settings.oneShotDay = day;
        gState->settings.oneShotHour = hour;
        gState->settings.oneShotMinute = minute;
        gState->settingsChanged = true;
      }
    } else if (upper.startsWith("SET_RTC ") || upper.startsWith("RTC ")) {
      DateTime rtcDateTime = parseRtcDateTime(command.substring(upper.startsWith("RTC ") ? 4 : 8));
      if (rtcDateTime.isValid()) {
        setRtcNow(rtcDateTime);
        gState->rtcNow = formatDateTime(rtcDateTime);
        DateTime nextTrigger;
        gState->nextTrigger = nextTriggerForSettings(gState->settings, rtcDateTime, nextTrigger)
                                  ? formatDateTime(nextTrigger)
                                  : "none";
      }
    } else if (upper.startsWith("SET_POWER_MS ") || upper.startsWith("PWR ")) {
      gState->settings.powerHoldMs = command.substring(upper.startsWith("PWR ") ? 4 : 13).toInt();
      gState->settingsChanged = true;
    } else if (upper.startsWith("FW ")) {
      bool enabled = false;
      String times;
      if (parseFixedWakeValues(command.substring(3), enabled, times)) {
        gState->settings.bleFixedWakeEnabled = enabled;
        gState->settings.bleWakeTimes = times;
        gState->settingsChanged = true;
      }
    } else if (upper.startsWith("FW1 ") || upper.startsWith("FW0 ")) {
      bool enabled = false;
      String times;
      if (parseFixedWakeValuesCompact(command.substring(2), enabled, times)) {
        gState->settings.bleFixedWakeEnabled = enabled;
        gState->settings.bleWakeTimes = times;
        gState->settingsChanged = true;
      }
    } else if (upper == "ENABLE_BLE_FIXED_WAKE" || upper == "EBW") {
      gState->settings.bleFixedWakeEnabled = true;
      gState->settingsChanged = true;
    } else if (upper == "DISABLE_BLE_FIXED_WAKE" || upper == "DBW") {
      gState->settings.bleFixedWakeEnabled = false;
      gState->settingsChanged = true;
    } else if (upper.startsWith("BWT ") || upper.startsWith("SET_BLE_WAKE_TIMES ")) {
      gState->settings.bleWakeTimes = command.substring(upper.startsWith("BWT ") ? 4 : 19);
      gState->settings.bleWakeTimes.trim();
      gState->settingsChanged = true;
    } else if (upper.startsWith("SET_TIMING ")) {
      uint32_t powerMs = 0;
      uint32_t bootMs = 0;
      uint32_t modeMs = 0;
      uint32_t bleWindowMs = 0;
      uint32_t bleWakeSec = 0;
      if (parseTimingValues(command.substring(11), powerMs, bootMs, modeMs, bleWindowMs, bleWakeSec)) {
        gState->settings.powerHoldMs = powerMs;
        gState->settings.bootDelayMs = bootMs;
        gState->settings.modePressMs = modeMs;
        gState->settings.bleWindowMs = bleWindowMs;
        gState->settings.bleWakeIntervalSec = bleWakeSec;
        gState->settingsChanged = true;
      }
    } else if (upper.startsWith("SET_BOOT_MS ") || upper.startsWith("BOT ")) {
      gState->settings.bootDelayMs = command.substring(upper.startsWith("BOT ") ? 4 : 12).toInt();
      gState->settingsChanged = true;
    } else if (upper.startsWith("SET_MODE_MS ") || upper.startsWith("MOD ")) {
      gState->settings.modePressMs = command.substring(upper.startsWith("MOD ") ? 4 : 12).toInt();
      gState->settingsChanged = true;
    } else if (upper.startsWith("SET_BLE_WINDOW_MS ") || upper.startsWith("BLW ")) {
      gState->settings.bleWindowMs = command.substring(upper.startsWith("BLW ") ? 4 : 18).toInt();
      gState->settingsChanged = true;
    } else if (upper.startsWith("SET_BLE_WAKE_SEC ") || upper.startsWith("BLS ")) {
      gState->settings.bleWakeIntervalSec = command.substring(upper.startsWith("BLS ") ? 4 : 17).toInt();
      gState->settingsChanged = true;
    } else if (upper.startsWith("SET_BATTERY_DIVIDER_X100 ") || upper.startsWith("BD ")) {
      gState->settings.batteryDividerRatioX100 = command.substring(upper.startsWith("BD ") ? 3 : 25).toInt();
      gState->settingsChanged = true;
    } else if (upper.startsWith("SET_BATTERY_EMPTY_MV ") || upper.startsWith("BE ")) {
      gState->settings.batteryEmptyMv = command.substring(upper.startsWith("BE ") ? 3 : 21).toInt();
      gState->settingsChanged = true;
    } else if (upper.startsWith("SET_BATTERY_FULL_MV ") || upper.startsWith("BF ")) {
      gState->settings.batteryFullMv = command.substring(upper.startsWith("BF ") ? 3 : 20).toInt();
      gState->settingsChanged = true;
    }

    persistSettingsIfNeeded();
    updateStatusValue();
  }
};

void ensureBleInit() {
  static bool initialized = false;
  if (initialized) {
    return;
  }

  NimBLEDevice::init(uniqueDeviceName().c_str());
  NimBLEDevice::setPower(ESP_PWR_LVL_P3);

  NimBLEServer *server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  NimBLEService *service = server->createService(kServiceUuid);

  NimBLECharacteristic *commandCharacteristic = service->createCharacteristic(
      kCommandUuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  commandCharacteristic->setCallbacks(new CommandCallbacks());

  gStatusCharacteristic = service->createCharacteristic(
      kStatusUuid, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

  service->start();

  NimBLEService *batteryService = server->createService(kBatteryServiceUuid);
  gBatteryCharacteristic = batteryService->createCharacteristic(
      kBatteryLevelUuid, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  batteryService->start();

  NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();
  advertising->addServiceUUID(kServiceUuid);
  advertising->addServiceUUID(kBatteryServiceUuid);
  advertising->setScanResponse(true);
  advertising->start();

  initialized = true;
}
}

void startBleWindow(BleRuntimeState &state, uint32_t windowMs) {
  gState = &state;
  gState->statusRequested = false;
  gState->authenticated = !gState->settings.authEnabled;
  ensureBleInit();
  updateStatusValue();
  NimBLEDevice::startAdvertising();

  uint32_t idleStartedAt = millis();
  uint32_t lastStatusNotifyMs = 0;
  while (true) {
    updateStatusLed();

    if (gClientConnected) {
      idleStartedAt = millis();
      if (millis() - lastStatusNotifyMs >= kStatusNotifyIntervalMs) {
        updateStatusValue();
        lastStatusNotifyMs = millis();
      }
    } else if (millis() - idleStartedAt >= windowMs) {
      break;
    }

    if (gState->statusRequested) {
      gState->statusRequested = false;
      updateStatusValue();
    }
    delay(50);
  }

  NimBLEDevice::stopAdvertising();
  setStatusLedConnected(false);
}
