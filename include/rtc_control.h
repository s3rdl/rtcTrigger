#pragma once

#include <Arduino.h>
#include <RTClib.h>

#include "settings.h"

enum class WakeTrigger {
  Timer,
  RtcAlarm,
  Other,
};

enum class ScheduledWakeKind {
  None,
  Speaker,
  Ble,
};

bool initRtc();
DateTime getRtcNow();
void setRtcNow(const DateTime &dateTime);
bool rtcLostPower();
bool rtcAlarmPending();
bool oneShotDue(const AppSettings &settings, const DateTime &now);
void syncRtcFromBuildTime();
bool nextTriggerForSettings(const AppSettings &settings, const DateTime &now, DateTime &nextTrigger);
bool nextBleWakeForSettings(const AppSettings &settings, const DateTime &now, DateTime &nextWake);
bool configureRtcAlarm(const AppSettings &settings, ScheduledWakeKind &kind, DateTime &triggerAt);
void disableRtcAlarm();
void clearRtcAlarmLatch();
WakeTrigger detectWakeTrigger();
void configureWakeSources(const AppSettings &settings, bool rtcWakeConfigured);
String formatDateTime(const DateTime &dateTime);
