#pragma once

bool rtcSetEnabled(bool enabled);
void rtcMap(void);

#ifdef _DEBUG
#define RTC_LOG
#endif
