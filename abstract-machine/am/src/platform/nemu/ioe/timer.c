#include <am.h>
#include <nemu.h>

// 使用静态变量模拟时间
static uint64_t boot_time = 0;
static uint64_t mock_time = 0;

void __am_timer_init() {
  boot_time = 0;
  mock_time = 0;
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  // 尝试读取实际RTC值
  uint32_t lo = inl(RTC_ADDR);
  uint32_t hi = inl(RTC_ADDR + 4);
  uint64_t rtc_time = ((uint64_t)hi << 32) | lo;
  
  // 如果RTC无效，使用模拟时钟
  if (rtc_time == 0) {
    mock_time += 3;  // 每次增加100ms
    uptime->us = mock_time;
  } else {
    uptime->us = rtc_time;
  }
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}

void __am_timer_config(AM_TIMER_CONFIG_T *cfg) {
  cfg->present = true;
  cfg->has_rtc = true;
}
