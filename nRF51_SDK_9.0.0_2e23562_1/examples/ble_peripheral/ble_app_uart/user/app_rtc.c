#include "app_calender.h"
#include "app_error.h"

#define CALENDER_TIEMR_INTERVAL    APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER)   /**< 日历定时器定时间隙. */

static uint32_t rtc_time_second = 0;
app_timer_id_t                     m_calender_timer_id;                         /**< 日历定时器. */

/**@brief Function for handling the calender timer timeout.
 * @details This function will be called each time the calender timer expires.
 * @param[in]   p_context   Pointer used for passing some arbitrary information (context) from the
 *                          app_start_timer() call to the timeout handler.
 */
void calender_timeout_handler(void * p_context)
{   
    rtc_time_second++;

    //UNUSED_PARAMETER(p_context);
}


