#include "bsp_gpio_ble.h"
#include "nordic_common.h"
#include "app_timer.h"
#include "app_trace.h"
#include "ble_advertising.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "ble.h"
#include "bsp.h"

#include "nrf_gpio.h"
#include "key_handler.h"
#include "led_handler.h"
#include "TaskMgr.h"

#include "ptl.h"

/* Private variables ---------------------------------------------------------*/
#define BSP_SIMPLE					(1)
#define bsp_button_led_delay_time	(327UL) // 1638

typedef void*         				xPinHandler;

typedef struct PIN_INFOR_T
{
//  char *fmt;
  uint32_t pin_number;
}pin_info_t;

xLedProfile always_on;
xLedProfile always_off;
xLedProfile blink_slow;
xLedProfile blink_mid;
xLedProfile blink_fast;
xLedProfile sos_blink;
xLedProfile blink_once;

xLedHandler led_blue, led_red, led3, led4;

static app_timer_id_t app_time_btn_led;

extern int bsp_key_event_callback(enum KeyEvent_e key_event);

//
//
//
void led_ctrl_callback(void *param , bool on_off)
{
  pin_info_t *port = param;
  uint8_t value = 0;

  if(on_off) value = 1;
  else value = 0;

  nrf_gpio_pin_write(port->pin_number, value);

  return;
}

//
// 读取一个IO口状态
//
static bool read_pin(void *pin_info, enum TriggerLevel_e trigger_level)
{
	pin_info_t *p = pin_info;

	if( nrf_gpio_pin_read(p->pin_number) ==  trigger_level )
	{
		return true;
  }

	return false;
}

//
// 增加一个IO口定义
//

static xPinHandler add_pin(uint32_t type, uint32_t pin_number)
{
    pin_info_t *p = (pin_info_t *)malloc(sizeof(pin_info_t));

    //UNUSED_PARAMETER(fmt);
#if ((BUTTONS_NUMBER > 0) && (defined BSP_SIMPLE))
    if (type & BSP_INIT_BUTTONS)
    {
        p->pin_number = pin_number;
        nrf_gpio_cfg_input(pin_number, BUTTON_PULL);
    }
    
#endif // (BUTTONS_NUMBER > 0) && !(defined BSP_SIMPLE)

#if ((LEDS_NUMBER > 0) && (defined BSP_SIMPLE))

    if (type & BSP_INIT_LED)
    {
        p->pin_number = pin_number;
        nrf_gpio_cfg_output(pin_number);
    }
#endif // (LEDS_NUMBER > 0) && !(defined BSP_SIMPLE)

    return p;
}


//
// led初始化
//
void _led_init(void)
{
    set_led_ctrl_period(50);           // <! must done before every led setting
    led_red = led_gpio_reg_with_param( led_ctrl_callback, add_pin(BSP_INIT_LED, LED_RED) );
    led_blue = led_gpio_reg_with_param( led_ctrl_callback, add_pin(BSP_INIT_LED, LED_BLUE) );
//    blink_mid  = led_blink_profile_create( 0, 100,  500,  500 );
    always_on  = led_blink_profile_create( 0, 100, 1000,    0 );
    always_off = led_blink_profile_create( 0, 100,    0, 1000 );
    blink_fast = led_blink_profile_create( 0, 100,  100,  100 );
//    blink_slow = led_blink_profile_create( 0, 100, 1000, 1000 );

    sos_blink = led_cascade_profile_create(  5,\
							                 blink_fast, 3,\
							                 always_off, 1,\
							                 blink_mid,  3,\
							                 blink_fast, 3,\
							                 always_off, -1);
   // printf("heap = %d\r\n", app_get_heap_size());
    //blink_once = led_cascade_profile_create( 2,\
							                 blink_slow, 1,\
							                 always_off, -1);

    //AddTask_TmrQueue( 1000, 0, set_led1_later );
    //AddTask_TmrQueue( 3000, 0, set_led2_later );
    //AddTask_TmrQueue( 1000, 0, set_led3_later );
    //AddTask_TmrQueue( 1200, 0, set_led4_later );

//    led_set( led1, always_off );
//    led_set( led2, always_on );
//    led_set( led3, blink_slow );
//    led_set( led4, blink_mid );

}


//
// 按键初始化
//
 void _key_init(void)
{
	key_reg( add_pin(BSP_INIT_BUTTONS, BUTTON_HOME), 0, 1000, bsp_key_event_callback, read_pin );
//	key_reg( add_pin(BSP_INIT_BUTTONS, BUTTON_2), 0, 1000, key_handler, read_pin );
}

//
// 按键控制回掉函数
//
void app_button_led_callback(void * p_context)
{
	static uint8_t counter = 0;
  uint32_t error_code = NRF_SUCCESS;
  uint32_t delay_ms = 10;

  UNUSED_PARAMETER(p_context);
	if( counter++ >= 5 ){
		counter = 0;
		led_ctrl_fsm();               // <! 50ms ctrl
	}
  key_scan( delay_ms );         // <! 50ms scan
  FuncTmrQueueCountDown( delay_ms );
  error_code = app_timer_start( app_time_btn_led,\
                                bsp_button_led_delay_time,\
                                NULL );
  APP_ERROR_CHECK(error_code);

  return;
}

void switch_control_task( void ){
	
  if( ble_switch_status_changed == SWITCH_STATUS_OPERATE ){
		ble_switch_status_changed = SWITCH_STATUS_UPDATE; // 设置上传状态报告
    if( 0 == m_ble_switch_status.value ){ // 设置关闭状态
      ptl_log("switch control turn on.\r\n");
      nrf_gpio_pin_clear( SWITCH_PIN );
    }else{                                // 设置打开状态
      ptl_log("switch control turn off.\r\n");
      nrf_gpio_pin_set( SWITCH_PIN );
    }
  }
}


void ble_switch_pin_init(void){
  
  nrf_gpio_cfg_output( SWITCH_PIN );
  
}


//
// 初始化按键及LED控制
//
void bsp_button_led_init(void)
{
  uint32_t error_code;

  _key_init();
  _led_init();
  ble_switch_pin_init();
  
  error_code = app_timer_create( &app_time_btn_led,
                                 APP_TIMER_MODE_SINGLE_SHOT,   // APP_TIMER_MODE_REPEATED   APP_TIMER_MODE_SINGLE_SHOT
                                 app_button_led_callback);
  APP_ERROR_CHECK(error_code);
  error_code = app_timer_start( app_time_btn_led,
                                bsp_button_led_delay_time,
                                NULL);
  APP_ERROR_CHECK(error_code);
}





