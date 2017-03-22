
#include "led_handler.h"
#include "queue_mgr.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define __ABS(__NUM)      (((__NUM)<0)?(-(__NUM)):(__NUM))

#define LED_CTRL_FSM_PERIOD       (led_ctrl_period)

static uint16_t led_ctrl_period = 50;

bool set_led_ctrl_period(uint16_t delta_ms){
  if( delta_ms == 0 ){ return false; }
  
  led_ctrl_period = delta_ms;
  return true;
}

/**************************************************************/
enum LedProfileType_e{
  LED_PROFILE_TYPE_NONE = 0,
  LED_PROFILE_TYPE_BREATH,
  LED_PROFILE_TYPE_BLINK,
  LED_PROFILE_TYPE_MOVE_TO,
  LED_PROFILE_TYPE_CASCADE_QUEUE,
  LED_PROFILE_TYPE_CASCADE,
};

struct led_config_profile_t {
  struct led_config_profile_t *next;
  enum LedProfileType_e profile_type;
};

struct led_breath_config_profile_t {
  struct led_breath_config_profile_t *next;
  enum LedProfileType_e profile_type;
  int16_t min;
  int16_t max;
  int16_t inc_delta;
  int16_t inc_period;
  int32_t cycle_period;
};

struct led_blink_config_profile_t {
  struct led_blink_config_profile_t *next;
  enum LedProfileType_e profile_type;
  int16_t min;
  int16_t max;
  int32_t on_time;
  int32_t off_time;
  int32_t cycle_period;
};

struct led_move_to_config_profile_t {
  struct led_move_to_config_profile_t *next;
  enum LedProfileType_e profile_type;
  int16_t inc_delta;
  int16_t target_duty;
  int32_t cycle_period;
};

struct led_cascade_config_profile_t {
  struct led_cascade_config_profile_t *next;
  enum LedProfileType_e profile_type;
  union {
          struct led_config_profile_t *config_profile;
          struct led_breath_config_profile_t  *breath_config_profile;
          struct led_blink_config_profile_t   *blink_config_profile;
          struct led_move_to_config_profile_t *move_to_config_profile;
          struct led_cascade_config_profile_t *cascade_queue;
        } profile_u;
  int16_t loop_num;
  //int16_t loop_cnt;
};

struct led_config_profile_t         *led_config_profile_queue = NULL;

/**************************************************************/

enum CallBackFunctionType_e {
  CALLBACK_TYPE_GPIO = 0,
  CALLBACK_TYPE_GPIO_WITH_PARAM,
  CALLBACK_TYPE_HW_PWM,
  CALLBACK_TYPE_HW_PWM_WITH_PARAM,
};

struct led_ctrl_t {
  struct led_ctrl_t *next;
  union {
          struct led_config_profile_t *config_profile;
          struct led_breath_config_profile_t  *breath_config_profile;
          struct led_blink_config_profile_t   *blink_config_profile;
          struct led_move_to_config_profile_t *move_to_config_profile;
          struct led_cascade_config_profile_t *cascade_config_profile;
        } profile_u;
  int16_t  inc_period_cnt;
  int32_t  cycle_period_cnt;
  int16_t  current_duty;
  int16_t  min_duty;
  int32_t  loop_cnt;
  enum CallBackFunctionType_e    callback_type;
  union {
          void (*hw_pwm_ctrl_callback)(uint16_t duty);
          void (*hw_pwm_ctrl_callback_with_param)(void *param , uint16_t duty);
          void (*gpio_ctrl_callback)(bool on_off);
          void (*gpio_ctrl_callback_with_param)(void *param , bool on_off);
        }callback_u;
  void *callback_param;
};

struct led_ctrl_t *led_ctrl_queue = NULL;



static struct led_config_profile_t *is_profile_valid(xLedProfile led_profile);
static bool led_breath_calc(struct led_ctrl_t *p, struct led_breath_config_profile_t *profile);
static bool led_blink_calc(struct led_ctrl_t *p, struct led_blink_config_profile_t *profile);
static bool led_move_to_calc(struct led_ctrl_t *p, struct led_move_to_config_profile_t *profile);
static bool led_cascade_calc(struct led_ctrl_t *p, struct led_cascade_config_profile_t *cascade_profile);


// what if on time < off time .???

static bool led_breath_calc(struct led_ctrl_t *p, struct led_breath_config_profile_t *profile){
  
  p->inc_period_cnt += LED_CTRL_FSM_PERIOD;
  p->min_duty = profile->min;
  
  if( p->inc_period_cnt >= profile->inc_period ){ // <! little inc period
    p->inc_period_cnt = 0;
    p->current_duty += profile->inc_delta;
    if( p->current_duty > profile->max ){
      p->current_duty -= ((profile->max)<<1);     // <! shouldn't larger than max
      return true;
    }
    if( profile->min > 0 ){   // <! configed the pwm_min > 0
      if( __ABS(p->current_duty) <= (profile->min) ){
        p->current_duty = ((profile->min)<<1) + p->current_duty; // min + (current - (-min) )
      }
    }
  }
  return false;
}

static bool led_blink_calc(struct led_ctrl_t *p, struct led_blink_config_profile_t *profile){
  
  p->min_duty = profile->min;
  
  p->cycle_period_cnt += LED_CTRL_FSM_PERIOD;
  if( p->cycle_period_cnt < profile->on_time ){
    p->current_duty = profile->max;
  }else{
    p->current_duty = profile->min;
  }
  
  if( p->cycle_period_cnt >= profile->cycle_period ){
    p->cycle_period_cnt = 0x0;
    p->inc_period_cnt = 0x0;
    if( profile->on_time > 0 ){
      p->current_duty = profile->max;
    }else{
      p->current_duty = profile->min;
    }
    return true;
  }
  return false;
}

static bool led_move_to_calc(struct led_ctrl_t *p, struct led_move_to_config_profile_t *profile){

  p->min_duty = 0;
  
  if( p->current_duty != profile->target_duty ){
    if( __ABS(p->current_duty - profile->target_duty) > __ABS(profile->inc_delta) ){
      p->current_duty += profile->inc_delta;
    }else{
      p->current_duty = profile->target_duty;
    }
  }else{
    free( profile );
    profile = NULL;
    return true;
  }
  return false;
}

static bool led_cascade_calc(struct led_ctrl_t *p, struct led_cascade_config_profile_t *cascade_profile){
  
  struct led_cascade_config_profile_t *profile = cascade_profile->profile_u.cascade_queue;
  
  int32_t loop_cnt = 0;
  
AGAIN:
  loop_cnt = 0;
  profile = cascade_profile->profile_u.cascade_queue;
  
  while( profile != NULL ){
    if( profile->loop_num > 0 ){
      loop_cnt += profile->loop_num;
    
      if( p->loop_cnt < loop_cnt ){
        break;
      }
    }else{
      break;
    }
    profile = profile->next;
  }
  
  if( profile == NULL ){
    p->loop_cnt = 0;
    profile = cascade_profile->profile_u.cascade_queue;
  }
  
  if( profile != NULL ){
    bool finish_flag = false;
    switch( profile->profile_u.config_profile->profile_type ){
      case LED_PROFILE_TYPE_BREATH  : if( led_breath_calc(p, profile->profile_u.breath_config_profile) ){
                                        finish_flag = true;
                                      } break;
      case LED_PROFILE_TYPE_BLINK   : if( led_blink_calc(p, profile->profile_u.blink_config_profile) ){
                                        finish_flag = true;
                                      } break;
      case LED_PROFILE_TYPE_MOVE_TO : if( led_move_to_calc(p, profile->profile_u.move_to_config_profile) ){
                                        finish_flag = true;
                                      } break;
      default : break;
    }
    
    if( finish_flag ){
      if( profile->loop_num > 0 ){
        p->loop_cnt++;
        goto AGAIN;
      }
    }
  }
  
  return true;
}

/**
		注意: led 控制函数的调用间隔为50ms
	*/ 
void led_ctrl_fsm(void)
{
    struct led_ctrl_t *p = led_ctrl_queue;

    while( p != NULL )
    {

        /******************************* calculation ***********************************/
        if( p->profile_u.config_profile != NULL ){
        switch( p->profile_u.config_profile->profile_type )
        {
            case LED_PROFILE_TYPE_BREATH  : 
                led_breath_calc(p, p->profile_u.breath_config_profile);   
                break;
            case LED_PROFILE_TYPE_BLINK   : 
                led_blink_calc(p, p->profile_u.blink_config_profile);     
                break;
            case LED_PROFILE_TYPE_MOVE_TO : 
                led_move_to_calc(p, p->profile_u.move_to_config_profile); 
                break;
            case LED_PROFILE_TYPE_CASCADE_QUEUE : 
                led_cascade_calc(p, p->profile_u.cascade_config_profile); 
                break;
            default : 
                break;
        }
    }
      
        /******************************* callback *************************************/
        if( p->callback_u.gpio_ctrl_callback != NULL )
        {
            switch(p->callback_type)
            {
                case CALLBACK_TYPE_GPIO :
                    p->callback_u.gpio_ctrl_callback( (p->current_duty != p->min_duty) );
                    break;
                case CALLBACK_TYPE_GPIO_WITH_PARAM :
                    p->callback_u.gpio_ctrl_callback_with_param(p->callback_param, (p->current_duty != p->min_duty) );
                    break;
                case CALLBACK_TYPE_HW_PWM :
                    p->callback_u.hw_pwm_ctrl_callback( __ABS(p->current_duty) );
                    break;
                case CALLBACK_TYPE_HW_PWM_WITH_PARAM :
                    p->callback_u.hw_pwm_ctrl_callback_with_param( p->callback_param, __ABS(p->current_duty) );
                    break;
                default : break;
            }
        }
        p = p->next;
    }
}

xLedHandler led_hw_pwm_reg( void (*hw_pwm_ctrl_callback)(uint16_t duty) ){
  struct led_ctrl_t *p = (struct led_ctrl_t *)malloc(sizeof(struct led_ctrl_t));
  if( p == NULL ){ return NULL; }

  memset( p , 0x0 , sizeof(struct led_ctrl_t) );
  p->callback_type = CALLBACK_TYPE_HW_PWM;
  p->callback_u.hw_pwm_ctrl_callback = hw_pwm_ctrl_callback;
  
  MOUNT( led_ctrl_queue , p );
  return p;
}

xLedHandler led_hw_pwm_reg_with_param( void (*hw_pwm_ctrl_callback)(void *param , uint16_t duty) , void *param){
  struct led_ctrl_t *p = (struct led_ctrl_t *)malloc(sizeof(struct led_ctrl_t));
  if( p == NULL ){ return NULL; }

  memset( p , 0x0 , sizeof(struct led_ctrl_t) );
  p->callback_type = CALLBACK_TYPE_HW_PWM_WITH_PARAM;
  p->callback_u.hw_pwm_ctrl_callback_with_param = hw_pwm_ctrl_callback;
  p->callback_param = param;
  
  MOUNT( led_ctrl_queue , p );
  return p;
}

xLedHandler led_gpio_reg( void (*gpio_ctrl_callback)(bool on_off) ){
  struct led_ctrl_t *p = (struct led_ctrl_t *)malloc(sizeof(struct led_ctrl_t));
  if( p == NULL ){ return NULL; } 

  memset( p , 0x0 , sizeof(struct led_ctrl_t) );
  p->callback_type = CALLBACK_TYPE_GPIO;
  p->callback_u.gpio_ctrl_callback = gpio_ctrl_callback;
  
  MOUNT( led_ctrl_queue , p );
  return p;
}

xLedHandler led_gpio_reg_with_param( void (*gpio_ctrl_callback)(void *param , bool on_off) , void *param){
  struct led_ctrl_t *p = (struct led_ctrl_t *)malloc(sizeof(struct led_ctrl_t));
  if( p == NULL ){ return NULL; } 

  memset( p , 0x0 , sizeof(struct led_ctrl_t) );
  p->callback_type = CALLBACK_TYPE_GPIO_WITH_PARAM;
  p->callback_u.gpio_ctrl_callback_with_param = gpio_ctrl_callback;
  p->callback_param = param;
  
  MOUNT( led_ctrl_queue , p );
  return p;
}

xLedProfile led_breath_profile_create( 
                                       int16_t min,
                                       int16_t max,
                                       int32_t cycle_period
                                     ){
                                       
  #define IS_IN_RANGE(value)            ( (value >= 0) && (value < 16384) )
                                  
  struct led_breath_config_profile_t *p=NULL;
                                       
  if( ! IS_IN_RANGE(min) && IS_IN_RANGE(max) ){ return NULL; }
  if( ! (min < max) ){ return NULL; }
  if( ! (cycle_period >= 0) ){ return NULL; }
  
  if( cycle_period % (2*LED_CTRL_FSM_PERIOD) >= LED_CTRL_FSM_PERIOD ){
    cycle_period += 2*LED_CTRL_FSM_PERIOD - (cycle_period % (2*LED_CTRL_FSM_PERIOD));
  }else{
    cycle_period -= (cycle_period % (2*LED_CTRL_FSM_PERIOD));
  }
  
  p = (struct led_breath_config_profile_t *)malloc(sizeof(struct led_breath_config_profile_t));                        
  if( p == NULL ){ return NULL; }                                     
  
  p->profile_type = LED_PROFILE_TYPE_BREATH;
  p->min = min;
  p->max = max;
  p->inc_delta =( 2*LED_CTRL_FSM_PERIOD*(max - min) )/cycle_period;
  p->inc_period = 50;
  p->cycle_period = cycle_period;

  MOUNT( led_config_profile_queue , p );
  return p;
}

xLedProfile led_blink_profile_create( 
                                      int16_t min,
                                      int16_t max,
                                      int32_t on_time,
                                      int32_t off_time
                                    ){
                                      
  #define IS_IN_RANGE(value)            ( (value >= 0) && (value < 16384) ) 
  struct led_blink_config_profile_t *p=NULL;
                                      
  if( ! IS_IN_RANGE(min) && IS_IN_RANGE(max) ){ return NULL; }
  if( ! (min < max) ){ return NULL; }
  if( ! (on_time >= 0) ){ return NULL; }
  if( ! (off_time >= 0) ){ return NULL; }
  
  p = (struct led_blink_config_profile_t *)malloc(sizeof(struct led_blink_config_profile_t));
  if( p == NULL ){ return NULL; }
  
  p->profile_type = LED_PROFILE_TYPE_BLINK;
  p->min = min;
  p->max = max;
  p->on_time = on_time;
  p->off_time = off_time;
  p->cycle_period = on_time + off_time;

  MOUNT( led_config_profile_queue , p );
  return p;
}

xLedProfile led_move_to_profile_create( 
                                        int16_t target_duty,
                                        int32_t time
                                      ){
                                      
  #define IS_IN_RANGE(value)            ( (value >= 0) && (value < 16384) ) 
  struct led_move_to_config_profile_t *p=NULL;
                                      
  if( ! IS_IN_RANGE(target_duty) ){ return NULL; }
  if( ! (time >= 0) ){ return NULL; }
  
  p = (struct led_move_to_config_profile_t *)malloc(sizeof(struct led_move_to_config_profile_t));
  if( p == NULL ){ return NULL; }
  
  p->profile_type = LED_PROFILE_TYPE_MOVE_TO;
  p->target_duty = target_duty;
  p->inc_delta = 1;
  p->cycle_period = time;

  MOUNT( led_config_profile_queue , p );  
  return p;
}

xLedProfile led_cascade_profile_create(int num, ...){ //xLedProfile profile, int loop_num, 
  
  struct led_cascade_config_profile_t *p = NULL, *q = NULL;
  
  struct cascade_format_t {
    xLedProfile profile;
    int loop_num;
  } *arg = (struct cascade_format_t *)(&(num) + 1);
  
  if( num <= 0 ){ return NULL; }
  
  /************************** parent profile ***************************/
  q = (struct led_cascade_config_profile_t *)malloc( sizeof(struct led_cascade_config_profile_t) );
  if( q == NULL ){ return NULL; }
  memset( q, 0x0, sizeof(struct led_cascade_config_profile_t) );
  q->profile_type = LED_PROFILE_TYPE_CASCADE_QUEUE;
  MOUNT( led_config_profile_queue , q );
  
  while(num--){
    if( is_profile_valid(arg->profile) == NULL ){  goto ERR;  }
    
    /*********************** check profile type ************************/
    switch( ((struct led_config_profile_t *)(arg->profile))->profile_type ){
      case LED_PROFILE_TYPE_BREATH :
      case LED_PROFILE_TYPE_BLINK  : break;
      default                      : goto ERR;
    }
    
    /************************* create profile **************************/   
    p = (struct led_cascade_config_profile_t *)malloc(sizeof(struct led_cascade_config_profile_t));
    if( p == NULL ){  goto ERR;  }
    memset(p, 0x0, sizeof(struct led_cascade_config_profile_t));

    /************************** profile info ***************************/
    p->profile_type = LED_PROFILE_TYPE_CASCADE;
    p->loop_num = arg->loop_num;
    p->profile_u.config_profile = (struct led_config_profile_t *)(arg->profile);
    
   /***************************** mount *******************************/
    MOUNT( q->profile_u.cascade_queue, p );
    
    arg++;
  }
  
  return q;
  
  
ERR:
  DROP_QUEUE(q->profile_u.cascade_queue);
  free(q);
  return NULL;
}


static struct led_config_profile_t * is_profile_valid(xLedProfile led_profile){

  struct led_config_profile_t *p = led_config_profile_queue;
  while( p != NULL ){
    if( (xLedProfile)p == led_profile ){ break; }
    p = p->next;
  }

  return p;
}

bool led_set( xLedHandler led_handler , xLedProfile led_profile ){
  struct led_ctrl_t *p = (struct led_ctrl_t *)led_handler;

  if( p == NULL ){ return false; }
  if( is_profile_valid(led_profile) == NULL ){ return false; }
  
  #define RESET_PARAM(__P)    do{\
        __P->current_duty = 0x0;\
        __P->cycle_period_cnt = 0x0;\
        __P->inc_period_cnt = 0x0;\
        __P->loop_cnt = 0x0;\
      }while(0)
  
  if( p->profile_u.config_profile != NULL ){
    switch( p->profile_u.config_profile->profile_type ){
      case LED_PROFILE_TYPE_MOVE_TO : 
              if( p->profile_u.move_to_config_profile != NULL ){
                free( p->profile_u.move_to_config_profile );
              } break;
      default : break;
    }
  }
  p->profile_u.config_profile = NULL;
    
  switch( ((struct led_config_profile_t *)led_profile)->profile_type ){
    case LED_PROFILE_TYPE_NONE : break;
    case LED_PROFILE_TYPE_BREATH :
          if( (p->callback_type == CALLBACK_TYPE_HW_PWM) || (p->callback_type == CALLBACK_TYPE_HW_PWM_WITH_PARAM) ){
            p->profile_u.breath_config_profile = (struct led_breath_config_profile_t *)led_profile;
            RESET_PARAM(p);
            return true;
          }
          
    case LED_PROFILE_TYPE_BLINK :
          p->profile_u.blink_config_profile = (struct led_blink_config_profile_t *)led_profile;
          RESET_PARAM(p);
          return true;
    
    case LED_PROFILE_TYPE_MOVE_TO :
          p->profile_u.move_to_config_profile = (struct led_move_to_config_profile_t *)led_profile;
          p->current_duty = __ABS( p->current_duty );
          p->profile_u.move_to_config_profile->inc_delta = (p->profile_u.move_to_config_profile->target_duty - p->current_duty)
                                               / (p->profile_u.move_to_config_profile->cycle_period / LED_CTRL_FSM_PERIOD);
          if( p->profile_u.move_to_config_profile->inc_delta == 0 ){
            p->profile_u.move_to_config_profile->inc_delta = (p->profile_u.move_to_config_profile->target_duty > p->current_duty)?1:-1;
          }
          p->cycle_period_cnt = 0;
          p->inc_period_cnt = 0;
          return true;
          
    case LED_PROFILE_TYPE_CASCADE_QUEUE :
          p->profile_u.cascade_config_profile = (struct led_cascade_config_profile_t *)led_profile;
          RESET_PARAM(p);
          //reset_cascade_loop_cnt(p->profile_u.cascade_config_profile);
          return true;

    default : break;
  }
  
  return false;
}


