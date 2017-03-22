#ifndef __APP_CALENDER_H
#define __APP_CALENDER_H

#include "app_timer.h"

#define APP_CALENDAR_TIMERS_NUMBER              (1)

#define CALENDAR_BASE_YEAR                      (1970)
#define CALENDAR_BASE_MONTH                     (1)
#define CALENDAR_BASE_DAY                       (1)
// 四年一闰，百年不闰，四百年再闰
#define	IS_LEAP_YEAR(year)	                    (!((year) % 400) || (((year) % 100) && !((year) % 4)))
#define DAY_OF_YEAR(year)                       (IS_LEAP_YEAR(year)?366:365)
#define EVERY_DAY_SECONDS                       (0x15180)

#define WDAY_CYCLE_ENABLE                       (0x80)
#pragma pack(push , 1)
//
// 
//
struct calendar_t
{
//  uint8_t wday:7;
//  uint8_t wday_en:1;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
	uint8_t wday;
  uint8_t mday;
  uint8_t month;
  uint8_t year;
    //uint32_t timestamp;
};


struct gmt_t{
  uint32_t gmt_ts;			      // GMT时间戳，格林威治标准时间
  uint32_t tz;					      // 时区-单位秒
  uint32_t tz_dst;						// 时区-夏令时，单位秒
  uint32_t tz_dst_start;			// 夏令时开始时间 GMT 时间戳
  uint32_t tz_dst_stop;				// 夏令时结束时间 GMT 时间戳
};
#pragma pack(pop)

void 				        app_calendar_init(void);
void 				        app_calendar_timeout_handler(void * p_context);
struct calendar_t 	app_calendar_handler_get(void);
uint32_t            app_calendar_set(struct gmt_t *gmt);
void 				        app_calendar_timestamp_to_date(struct calendar_t *calendar_t, struct gmt_t gmt);
uint32_t            app_calendar_date_to_timestamp(struct calendar_t *calendar_t);
int 				        app_calculate_interval_day( int year_start, \
                                                int month_start, \
                                                int day_start,  \
                                                int year_end, \
                                                int month_end, \
                                                int day_end);
uint32_t            app_calendar_get_timestamp(void);
struct gmt_t        app_calendar_get_gmt(void);

#endif


