#include "app_calendar.h"

#include "app_error.h"
#include "nordic_common.h"
#include "app_trace.h"
#include "nrf_drv_rtc.h"
#include "app_timing.h"
#include "string.h"

#define CALENDER_TIEMR_INTERVAL     APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER)  /**< ������ʱ����ʱ��϶. */

static app_timer_id_t               m_calendar_timer_id;                        /**< ������ʱ��. */
static struct calendar_t            m_calendar_t;                               /**< �����ṹ�� */
static struct gmt_t                 m_gmt;
static uint32_t 					          system_timestamp_bkp = 0;					          /**< ϵͳ��ʼʱ������� */

#define DEBUG_RTC_OPEN							(2)

#if ( DEBUG_RTC_OPEN == 2 )
#define rtc_log(fmt, ...)						app_trace_log("[%d][%05d][rtc]"fmt, \
			                        			__SYS_REMAIN_HEAP_SIZE_, __LINE__, ##__VA_ARGS__);
#elif ( DEBUG_RTC_OPEN == 1 )
#define rtc_log(fmt, ...)						app_trace_log(fmt, ##__VA_ARGS__);
#define rtc_log_dump                app_trace_dump
#else
#define rtc_log(fmt, ...)
#define rtc_log_dump(fmt, ...)
#endif
#if 0
static void rtc_config(void)
{
    uint32_t err_code;

    //Initialize RTC instance
    err_code = nrf_drv_rtc_init(&rtc, NULL, rtc_handler);//��ʼ��RTC
    APP_ERROR_CHECK(err_code);

    //Enable tick event & interrupt
    nrf_drv_rtc_tick_enable(&rtc, true);//ʹ��tick�¼�

    //Set compare channel to trigger interrupt after COMPARE_COUNTERTIME seconds
    err_code = nrf_drv_rtc_cc_set(&rtc, 0, COMPARE_COUNTERTIME * RTC0_CONFIG_FREQUENCY, true);
    APP_ERROR_CHECK(err_code);

    //Power on RTC instance
    nrf_drv_rtc_enable(&rtc);
}
#endif

//
// ��ȡGMT���
//
struct gmt_t app_calendar_get_gmt(void){
  return m_gmt;
}
//
// ��ȡϵͳʱ���
//
uint32_t app_calendar_get_timestamp(void)
{
    return m_gmt.gmt_ts;
}

//
// ��ȡ����������ʱ�估����
//
struct calendar_t app_calendar_handler_get(void)
{
	return m_calendar_t;
}

//
// ����ϵͳʱ���
//
uint32_t app_calendar_set(struct gmt_t *gmt)
{
//	uint32_t err_code = NRF_SUCCESS;
  
  memcpy(&m_gmt, gmt, sizeof(struct gmt_t));
//  
//	// Check params
//	if(gmt.gmt_ts < system_timestamp_bkp)
//	{
//		return NRF_ERROR_INVALID_PARAM;
//	}
//	
//	err_code = app_timer_stop(m_calendar_timer_id);
//	APP_ERROR_CHECK(err_code);
//    
//  // Assignment
//	app_calendar_timestamp_to_date(&m_calendar_t, gmt);
//    
//	// Start calendar timer
//  err_code = app_timer_start(m_calendar_timer_id, 
//                             CALENDER_TIEMR_INTERVAL, 
//                             NULL);
//	APP_ERROR_CHECK(err_code);
	
	return NRF_SUCCESS;
}

//
// �����·ݵ�����
//
static uint8_t calculate_month_day( uint8_t year, uint8_t month )
{
    uint8_t days = 31;

	if((year != 0 && year != 1) || (month > 11))
		return 0xff;
	
    if ( month == 1 ) // feb
    {
        days = ( 28 + year );
    }
    else
    {
        if ( month > 6 ) // aug-dec
        {
            month--;
        }

        if ( month & 1 )
        {
            days = 30;
        }
    }

    return ( days );
}

//
// ����ָ�����ڵ���Ԫ0�꿪ʼ������
//
int calculate_day(int year, int month, int day)
{
    int y, m, d;
    
    m = (month + 9) % 12;
    y = year - m/10;
    d = 365*y + y/4 - y/100 + y/400 + (m*306 + 5)/10 + (day - 1);
    
    return d;
}

/*
  �㷨������

  ���㷨����˼���Ǽ���������ڵ� 0��3��1�յ�������Ȼ���������ȡ�����ļ����
    m1 = (month_start + 9) % 12;
    �����ж������Ƿ����3�£�2�����ж�����ı�ʶ���������ڼ�¼��3�µļ��������
    y1 = year_start - m1/10;
    �����1�º�2�£��򲻰�����ǰ�꣨��Ϊ�Ǽ��㵽0��3��1�յ���������
    d1 = 365*y1 + y1/4 - y1/100 + y1/400 + (m1*306 + 5)/10 + (day_start - 1);
    ����
    365*y1 �ǲ�����������һ���������
    y1/4 - y1/100 + y1/400  �Ǽ���������������һ�죬
    (m2*306 + 5)/10 ���ڼ��㵽��ǰ�µ�3��1�ռ��������
    306=365-31-28��1�º�2�£���5��ȫ���в���31���·ݵĸ���

    (day_start - 1) ���ڼ��㵱ǰ�յ�1�յļ��������

*/
int app_calculate_interval_day( int year_start, // ��ʼ����
                                int month_start, 
                                int day_start,
                                int year_end,   // ��ֹ����
                                int month_end, 
                                int day_end)
{
    // ����ӹ�Ԫ0��ʼ�����ڵ�����
#if 0
    int y2, m2, d2;
    int y1, m1, d1;
    
    m1 = (month_start + 9) % 12;
    y1 = year_start - m1/10;
    d1 = 365*y1 + y1/4 - y1/100 + y1/400 + (m1*306 + 5)/10 + (day_start - 1);

    m2 = (month_end + 9) % 12;
    y2 = year_end - m2/10;
    d2 = 365*y2 + y2/4 - y2/100 + y2/400 + (m2*306 + 5)/10 + (day_end - 1);
    
    return (d2 - d1);
#else
    int ds = 0, de = 0;
    
    ds = calculate_day(year_start, month_start, day_start);
    de = calculate_day(year_end, month_end, day_end);
    
    return (de - ds);
#endif
}


//
// ��ʱ���ת��Ϊ������ʱ��������
//
static void app_calendar_timestamp_to_date(struct calendar_t *calendar_t, struct gmt_t gmt)
{
    uint32_t past_second_of_day = 0, past_days = 0, tmp_days = 0, tmp_year = 0;
    uint8_t mday = 0, day_of_year = 0;
    uint32_t year = 0;	
    uint32_t past_seconds = gmt.gmt_ts+gmt.tz;
    
    // ����ĳһ���ڹ�ȥ������
    past_second_of_day = past_seconds % EVERY_DAY_SECONDS;
    // ���㵱ǰ��ʱ����
    calendar_t->second = past_second_of_day % 60UL;
    calendar_t->minute = (past_second_of_day % 3600UL)/60UL;
//    calendar_t->hour = (past_second_of_day/3600UL + gmt.tz/3600UL)%24;
    calendar_t->hour = past_second_of_day / 3600UL;
    // ���㵱ǰ�����
    past_days = past_seconds / EVERY_DAY_SECONDS;
    
    calendar_t->year = 0;
    year = CALENDAR_BASE_YEAR + calendar_t->year;
    while(1)
    {
        tmp_days = DAY_OF_YEAR(year);
        if(past_days > tmp_days)
        {
            past_days -= tmp_days;
            year++;
        }
        else
        {
            break;
        }
    }
    calendar_t->year = year - CALENDAR_BASE_YEAR;
    day_of_year = past_days + 1;
    // ���㵱ǰ���·�
    calendar_t->month = 0;
    while(1)
    {
        mday = calculate_month_day(IS_LEAP_YEAR(year), calendar_t->month);
		if(mday == 0xff)
		{
			// Do nothing
			break;
		}
        if(past_days > mday)
        {
            past_days -= mday;
            calendar_t->month++;
        }
        else
        {
            break;
        }
    }
    calendar_t->month++;
    // ���㵱ǰ������
    calendar_t->mday = past_days + 1;
    // ��������
    tmp_year = year - 1;
    calendar_t->wday = ( tmp_year+( tmp_year/4 )-( tmp_year/100 )+( tmp_year/400 )+day_of_year )%7;
    
    return;
}

//
// ����ʱ��ת��Ϊϵͳ����ʱ���
//
uint32_t app_calendar_date_to_timestamp(struct calendar_t *calendar_t)
{
	uint32_t past_days = 0, timestamp = 0;
	
	past_days = app_calculate_interval_day( CALENDAR_BASE_YEAR, \
                                            CALENDAR_BASE_MONTH, \
                                            CALENDAR_BASE_DAY,  \
                                            calendar_t->year + CALENDAR_BASE_YEAR, \
                                            calendar_t->month, \
                                            calendar_t->mday);
    // ����ϵͳ��׼ʱ���ʱ�����
    timestamp = past_days*86400UL + calendar_t->hour*3600 + calendar_t->minute * 60UL + calendar_t->second;

	return timestamp;
}

/**@brief Function for handling the calendar timer timeout.
 * @details This function will be called each time the calendar timer expires.
 * @param[in]   p_context   Pointer used for passing some arbitrary information (context) from the
 *                          app_start_timer() call to the timeout handler.
 */
void app_calendar_timeout_handler(void * p_context)
{   
//  system_timestamp_run++;
  m_gmt.gmt_ts++;
// ÿ����һ�μ������Զ�����ϵͳʱ��
  app_calendar_timestamp_to_date(&m_calendar_t, m_gmt);
  
  rtc_log("%4d/%2d/%2d %02d:%02d:%02d %0d\r\n", \
       CALENDAR_BASE_YEAR + m_calendar_t.year, m_calendar_t.month, m_calendar_t.mday, \
       m_calendar_t.hour, m_calendar_t.minute, m_calendar_t.second, m_calendar_t.wday);
  
  UNUSED_PARAMETER(p_context);

//  timing_event_scan_task();
    
	return;
}

//
// ������ʼ������ʱ��
// 
static void app_calendar_set_default(uint32_t *timestamp)
{   
	UNUSED_PARAMETER(timestamp);

	m_calendar_t.year   = 47;
	m_calendar_t.month  = 3;
	m_calendar_t.mday   = 20;
	m_calendar_t.hour   = 1;
	m_calendar_t.minute = 8;
	m_calendar_t.second = 32;
	m_calendar_t.wday   = 1;

	m_gmt.gmt_ts = app_calendar_date_to_timestamp(&m_calendar_t);
	system_timestamp_bkp = m_gmt.gmt_ts;

	return;
}

//
// Initializes the calendar
//
void app_calendar_init(void)
{
    uint32_t err_code = NRF_SUCCESS;
  
    // Initializes the calendar structure
    app_calendar_set_default(0);
    
    // Create calendar timer
    err_code = app_timer_create(&m_calendar_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                app_calendar_timeout_handler);
    APP_ERROR_CHECK(err_code);
    
    // Start calendar timer
    err_code = app_timer_start(m_calendar_timer_id, 
                               CALENDER_TIEMR_INTERVAL, 
                               NULL);
    APP_ERROR_CHECK(err_code);

	return;
}


