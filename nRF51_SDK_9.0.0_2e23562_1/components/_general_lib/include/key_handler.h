#ifndef __KEY_HANDLER_
#define __KEY_HANDLER_

#include "stdint.h"
#include <stdbool.h>

typedef void*               xKeyHandler;


enum KeyEvent_e {
  KEY_NONE = 0,
  KEY_CLICK,
  KEY_HOLD,
  KEY_HOLDING,
  KEY_RELEASE
};

enum TriggerLevel_e {
  TRIGGER_LEVEL_LOW = 0,
  TRIGGER_LEVEL_HIGH,
};

xKeyHandler key_reg(
              void *pin_info,
              uint8_t  trigger_level,         // <! trigger level
              uint16_t hold_threshold,        // <! threshold for hold event
              int (*event_callback)(enum KeyEvent_e key_event),  // <! callback function for eventv
			  bool (*key_trigger_callback)(void *pin_info, enum TriggerLevel_e trigger_level)
           );

xKeyHandler key_reg_with_param(
              void *pin_info,
              uint8_t  trigger_level,         // <! trigger level
              uint16_t hold_threshold,        // <! threshold for hold event
              int (*event_callback)(void *param , enum KeyEvent_e key_event),  // <! callback function for event
              void *callback_param,
			  bool (*key_trigger_callback)(void *pin_info, enum TriggerLevel_e trigger_level)
           );


xKeyHandler combine_key_reg(
              void *pin_info, 
              uint8_t  trigger_level,         // <! trigger level
              xKeyHandler combind_with        // <! the primary key
           );

void key_unreg_callback( int (*event_callback)(enum KeyEvent_e key_event) );


void key_event_fsm(void); // key_event_detector
void key_scan(uint32_t delta_ms);

/*
  // registering key event
  
  key_register( 13,
                TRIGGER_LEVEL_LOW,
                1000,
                key1_handler,
                NULL );
  
  // registering combine key event
  
  xKeyHandler new_xKeyHandler;
  new_xKeyHandler = key_register( GPIOA,
                                  GPIO_Pin_3,
                                  TRIGGER_LEVEL_LOW,
                                  1000,
                                  key1_handler,
                                  NULL );
  new_xKeyHandler = key_register( GPIOA,
                                  GPIO_Pin_7,
                                  TRIGGER_LEVEL_LOW,
                                  1000,
                                  NULL,
                                  new_xKeyHandler );
  
  // unregistering key event by pin and callback_func
  
  key_unreg_pin( GPIOA , GPIO_Pin_7 );
  key_unreg_callback( key1_handler );

  
int key1_handler(enum KeyEvent_e key_event){
  switch(key_event){
    case KEY_CLICK :
      break;
    case KEY_HOLD :
      break;
    case KEY_RELEASE :
      break;
    default : break;
  }
}
*/

#endif
