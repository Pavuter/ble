
#include <string.h>
#include <stdlib.h> 

#include "key_handler.h"
#include "queue_mgr.h"

#define __inline                    inline

enum KeyState_e {
  KEY_STATE_IDLE = 0,
  KEY_STATE_PRESS,
  KEY_STATE_HOLD,
  KEY_STATE_RELEASE,
};


struct key_ctrl_t 
{
  struct key_ctrl_t *next;
  struct key_ctrl_t *combind_with;
  void *pin_info;
  uint16_t  trigger_cnt;
  uint16_t  hold_threshold;
  enum TriggerLevel_e trigger_level;
  enum KeyState_e     key_state;
  enum KeyEvent_e     key_event_flag;
  int (*event_callback)(enum KeyEvent_e key_event);
  int (*event_callback_with_param)(void *param , enum KeyEvent_e key_event);
  bool (*key_trigger_callback)(void *pin_info, enum TriggerLevel_e trigger_level);
  void *callback_param;
};

static struct key_ctrl_t *key_queue = NULL;


static xKeyHandler key_register(                                                                
              void *pin_info,                                                                       
              uint8_t  trigger_level,                                                           
              uint16_t hold_threshold,                                                          
              int (*event_callback)(enum KeyEvent_e key_event),                                 
              int (*event_callback_with_param)(void *param , enum KeyEvent_e key_event),        
              void *callback_param,                                                             
			  bool (*key_trigger_callback)(void *pin_info, enum TriggerLevel_e trigger_level),  
              xKeyHandler combind_with        /* <! the primary key */);

static void unmount_and_free(struct key_ctrl_t *p);

				
xKeyHandler key_reg(void *pin_info,
                    uint8_t  trigger_level,         // <! trigger level
                    uint16_t hold_threshold,        // <! threshold for hold event
                    int (*event_callback)(enum KeyEvent_e key_event),  // <! callback function for event
                    bool (*key_trigger_callback)(void *pin_info, enum TriggerLevel_e trigger_level))
{
  return key_register( pin_info,
                       trigger_level, 
                       hold_threshold, 
                       event_callback, 
                       NULL, NULL,
                       key_trigger_callback,
					   NULL );
}

xKeyHandler key_reg_with_param(void *pin_info,
                              uint8_t  trigger_level,         // <! trigger level
                              uint16_t hold_threshold,        // <! threshold for hold event
                              int (*event_callback)(void *param , enum KeyEvent_e key_event),  // <! callback function for event
                              void *callback_param,
                              bool (*key_trigger_callback)(void *pin_info, enum TriggerLevel_e trigger_level))
{
  return key_register( pin_info,
                       trigger_level, hold_threshold, 
                       NULL,
                       event_callback, callback_param,
					   key_trigger_callback,
                       NULL );
}

xKeyHandler combine_key_reg(
              void *pin_info,
              uint8_t  trigger_level,         // <! trigger level
              xKeyHandler combind_with        // <! the primary key
           ){
  return key_register( pin_info,
                       trigger_level, 0x0, 
                       NULL, 
                       NULL, NULL,
					   NULL,
                       combind_with );
}

/* <! xKeyHandler combind_with: the primary key */ 
__inline static xKeyHandler key_register(   void *pin_info,                                                                           
                                            uint8_t  trigger_level,                                                           
                                            uint16_t hold_threshold,                                                          
                                            int (*event_callback)(enum KeyEvent_e key_event),                                     
                                            int (*event_callback_with_param)(void *param , enum KeyEvent_e key_event),        
                                            void *callback_param,                                                             
                                            bool (*key_trigger_callback)(void *pin_info, enum TriggerLevel_e trigger_level),  
                                            xKeyHandler combind_with)                                                       
{
    struct key_ctrl_t *p = (struct key_ctrl_t *)malloc( sizeof(struct key_ctrl_t) );
    if( p == NULL ){ return NULL; }

    p->next = NULL;
    p->combind_with = NULL;
    p->pin_info = pin_info;
    p->trigger_level = (enum TriggerLevel_e)trigger_level;   // <! trigger level
    p->hold_threshold = hold_threshold; // <! threshold for hold event
    p->event_callback = event_callback; // <! callback function for event
    p->event_callback_with_param = event_callback_with_param;  // <! callback with param function for event
    p->callback_param = callback_param;
    p->key_trigger_callback = key_trigger_callback;

    p->key_state      = KEY_STATE_IDLE; // <! init state : idle
    p->key_event_flag = KEY_NONE;       // <! init state : no event
    p->trigger_cnt    = 0;              // <! init cnt = 0;

    if( combind_with == NULL )
    { 
        MOUNT( key_queue , p );
    }
    else
    {
        ((struct key_ctrl_t *)combind_with)->combind_with = p;
        p->event_callback = NULL;
    }
    return p;
}

void key_unreg_callback( int (*event_callback)(enum KeyEvent_e key_event) ){
  struct key_ctrl_t *p = key_queue , *q = NULL; 
  while(p != NULL)
  {
    q = p->next;
    if( p->event_callback == event_callback ){
      unmount_and_free(p);
    }
    p = q;
  }
}

static void unmount_and_free(struct key_ctrl_t *p){
  struct key_ctrl_t  *cb = NULL ;
  UNMOUNT( key_queue , p );
  cb = p;
  while( p != NULL ){
    cb = p->combind_with;
    free(p);
    p = cb;
  }
}


void key_scan(uint32_t delta_ms){
  #define IS_LONG_PRESS()         (p->trigger_cnt >= p->hold_threshold)

  struct key_ctrl_t *p = key_queue , *q = NULL;
  while( p != NULL ){
    switch( p->key_state ){
      
      case KEY_STATE_IDLE :
            q = p;
            while( q != NULL ){
              if( ! p->key_trigger_callback(p->pin_info, p->trigger_level) ){ break; }   // <! if no trigger , break the while
              q = q->combind_with;
            }
            if( q == NULL ){  // <! if go through all combind block , then all pins are trigger
              p->key_state = KEY_STATE_PRESS;       // <! jump to press
            }
            break;
      case KEY_STATE_PRESS :
            p->key_state = KEY_STATE_HOLD;
            break;
      case KEY_STATE_HOLD :
            q = p;
            while( q != NULL ){
              if( ! p->key_trigger_callback(p->pin_info, p->trigger_level) ){ break; }   // <! if no trigger , break the while
              q = q->combind_with;
            }
            if( q == NULL ){  // <! if go through all combind block , then all pins are trigger
              p->trigger_cnt += delta_ms;       // <! press ++
              
              if( IS_LONG_PRESS() ){
                if( p->key_event_flag != KEY_HOLDING ){
                  p->key_event_flag = KEY_HOLD;
                }
              }
            }else{
              p->key_state = KEY_STATE_RELEASE;
            }
            break;
      case KEY_STATE_RELEASE :
            if( IS_LONG_PRESS() ){
              p->key_event_flag = KEY_RELEASE;
            }else{
              p->key_event_flag = KEY_CLICK;
            }
            p->trigger_cnt = 0;
            p->key_state = KEY_STATE_IDLE;
            break;
      
      default : p->key_state = KEY_STATE_IDLE; break;
    }
    p = p->next;
  }
}

void key_event_fsm(void){
  struct key_ctrl_t *p = key_queue;
  while( p != NULL ){
    if( p->key_event_flag != KEY_NONE ){
      if( p->event_callback != NULL ){
        p->event_callback(p->key_event_flag);
      }
      if( p->event_callback_with_param != NULL ){
        p->event_callback_with_param(p->callback_param, p->key_event_flag);
      }
      switch(p->key_event_flag){
        case KEY_HOLD :     p->key_event_flag = KEY_HOLDING; break;
        case KEY_HOLDING :  break;
        default :           p->key_event_flag = KEY_NONE; break;
      }
    }
    p = p->next;
  }
}

