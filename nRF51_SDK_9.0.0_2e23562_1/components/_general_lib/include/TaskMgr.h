#ifndef _TASKMGR__H
#define _TASKMGR__H

#include <string.h>
#include "stdint.h"

// This function must be called preodic in about 50ms by interrupt
void FuncTmrQueueCountDown( uint32_t CountDown );

// This function must be called in main while(1) every (n*10)ms
void FuncTmrQueueMgr( void );


#define AddTask_TmrQueue(__DELAY__,__AUTO_RELOAD__,__FUNC__)    do{\
            add_task_tmr_queue(__DELAY__,__AUTO_RELOAD__,__FUNC__,#__FUNC__);\
          }while(0)

#define DelTask_TmrQueue(__FUNC__)     do{\
            del_task_tmr_queue(__FUNC__,#__FUNC__);\
          }while(0)


int8_t add_task_tmr_queue( uint32_t Delay , uint32_t AutoReload , void (*Func)(void) , char *FuncName );
int8_t del_task_tmr_queue( void (*Func)(void), char *FuncName );
int8_t QueryTask_TmrQueue( void (*Func)(void) );


#endif

