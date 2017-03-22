#ifndef __LED_HANDLER_
#define __LED_HANDLER_

#include "stdint.h"
#include <stdbool.h>

typedef void*  xLedHandler;
typedef void*  xLedProfile;


xLedHandler led_hw_pwm_reg( void (*hw_pwm_ctrl_callback)(uint16_t duty) );
xLedHandler led_hw_pwm_reg_with_param( 
            void (*hw_pwm_ctrl_callback)(void *param , uint16_t duty) , void *param);

xLedHandler led_gpio_reg( void (*gpio_ctrl_callback)(bool on_off) );
xLedHandler led_gpio_reg_with_param( 
            void (*gpio_ctrl_callback)(void *param , bool on_off) , void *param);


bool led_unreg( xLedHandler led_handler );

xLedProfile led_breath_profile_create( 
                                       int16_t min,
                                       int16_t max,
                                       int32_t cycle_period
                                     );

xLedProfile led_blink_profile_create( 
                                      int16_t min,
                                      int16_t max,
                                      int32_t on_time,
                                      int32_t off_time
                                    );

xLedProfile led_move_to_profile_create( 
                                      int16_t target_duty,
                                      int32_t time
                                    );

/********************** usage *********************
  xLedProfile n = led_cascade_profile_create( profiles_count , 
                    led_profile, loop_num, ... );
                    
  e.g.
  
  xLedProfile sos = led_cascade_profile_create( 5, 
                     blink_fast, 3,
                     always_off, 1,
                     blink_slow, 3,
                     blink_fast, 3,
                     always_off, 3
                    );
  led_set( led1, sos );
********************** ***** *********************/
xLedProfile led_cascade_profile_create(int num, ...);

bool led_set( 
              xLedHandler led_handler,
              xLedProfile led_profile
            );

bool set_led_ctrl_period(uint16_t delta_ms);
void led_ctrl_fsm(void);
                                    
// blink_mode 变成链表, 可以多变时间

extern xLedProfile always_on;
extern xLedProfile always_off;
extern xLedProfile blink_slow;
extern xLedProfile blink_mid;
extern xLedProfile blink_fast;
extern xLedProfile sos_blink;
extern xLedProfile blink_once;
extern xLedHandler led_blue, led_red;
/*
xLedProfile breath_slow = led_breath_profile_create( );
xLedProfile breath_fast = led_breath_profile_create( );

xLedProfile always_on  = led_blink_profile_create( 0, 1000, 1000,    0 );
xLedProfile always_off = led_blink_profile_create( 0, 1000,    0, 1000 );
xLedProfile blink_slow = led_blink_profile_create( 0, 1000, 1000, 1000 );
xLedProfile blink_fast = led_blink_profile_create( 0, 1000,  100,  100 );
xLedProfile sos_blink  = led_cascade_profile_create( 5, 
                           blink_fast, 3,
                           always_off, 1,
                           blink_slow, 3,
                           blink_fast, 3,
                           always_off, 3
                          );
   
led_set( led1, sos );

void led_set( led1_handler , breath_fast );
void led_set( led1_handler , blink_slow );
*/


#endif


