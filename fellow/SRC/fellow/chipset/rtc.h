#pragma once

bool rtcSetEnabled(bool enabled);
void rtcMap();

#ifdef _DEBUG
#define RTC_LOG
#endif
