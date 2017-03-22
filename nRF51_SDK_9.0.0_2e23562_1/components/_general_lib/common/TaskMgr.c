#include <string.h>

#include "TaskMgr.h"

#include <string.h>
#include <stdlib.h> 
#include <stdbool.h>

#if 0
#include "db_print.h"
#else
#define DB_PRINT(__FMT__ , ...)
#endif

#define FuncTmrQueueNUM			8			  // <! Maximun task num in queue

#define FuncTmrQueueFSMInitFlag	0x57932469	  // <! specialise code
typedef enum QueueFSM_State_enum {
  QueueFSM_Suspended = 0,
  QueueFSM_Ready
}QueueFSM_State_TypeDef;

#define FUNCTION_NAME_MAX_LEN         24
struct FuncTmrQueue_t {
  struct FuncTmrQueue_t *Next;
  int32_t   Delay;
  uint32_t  AutoReload;
  uint32_t  KillFlag;
  void      (*Func)(void);
  //char      FuncName[FUNCTION_NAME_MAX_LEN];
};

struct FuncTmrQueue_FSM_t {
  uint32_t  WakeUpFlag;
  uint32_t  WakeUpImmediately;
  uint32_t  WakeUpTimeCountDown;
  uint32_t  WakeUpTime;
  struct FuncTmrQueue_t *Queue_p;
  uint32_t  InitFlag;
  struct FuncTmrQueue_t *FuncQueue;
};

bool TaskMgrRunningFlag = false;

static struct FuncTmrQueue_FSM_t FuncTmrQueue_FSM;
static struct FuncTmrQueue_t FuncTmrQueue[FuncTmrQueueNUM];

#define GET_RUNNING_TASK()      (FuncTmrQueue_FSM.Queue_p)
#define SET_RUNNING_TASK(p)     (FuncTmrQueue_FSM.Queue_p = p)

void FuncTmrQueueMgr_Init( void );

/***************************** debug menu *************************/

// xMenuHandler taskmgr_menu_handler = NULL;
// static void taskmgr_entering_callback(xMenuHandler h);
// static void taskmgr_leaving_callback(xMenuHandler h);

// static void taskmgr_entering_callback(xMenuHandler h){
//   struct FuncTmrQueue_t *p , *q = NULL;
//   p = FuncTmrQueue_FSM.FuncQueue;
//   char *m_name = malloc(MENU_NAME_MAX_LEN);
//   if( m_name != NULL ){
//     while( p != NULL ){
//       memset(m_name , 0x0 , MENU_NAME_MAX_LEN );
//       snprintf( m_name , MENU_NAME_MAX_LEN , "%.24s,%u,%u" , 
//                p->FuncName, p->Delay , p->AutoReload );
//       menu_add( m_name , h , NULL );

//       p = p->Next;
//     }
//     free(m_name);
//   }
// }

// static void taskmgr_leaving_callback(xMenuHandler h){
//   menu_del_child(h);
// }



// // ======================== FuncTmrQueueCountDown demo ========================= //
// void TaskMgr_Tick_Callback( hftimer_handle_t htimer ){
//   static uint32_t LastTick,NewTick,DeltaTick;

//   NewTick = hfsys_get_time ();
//   if( NewTick > LastTick )  DeltaTick = NewTick - LastTick;
//   else{
//     DeltaTick = 50;
//   }
//   LastTick = NewTick;
//   FuncTmrQueueCountDown( DeltaTick );
//     // Reload IWDG counter 
// //    IWDG_ReloadCounter();       // <! Free the dog
// }

// ======================= timer execute queue count down (unit:ms) =============== //
void FuncTmrQueueCountDown( uint32_t CountDown ){
  static uint32_t Accumulate = 0;
  if( FuncTmrQueue_FSM.InitFlag != FuncTmrQueueFSMInitFlag ) return;  // <! system hasn't been initialize
  if( FuncTmrQueue_FSM.WakeUpFlag == 0 ){      // <! FSM is not suspended
    CountDown += Accumulate;
    if( FuncTmrQueue_FSM.WakeUpTime > 0 ){    // <! FSM is initialize
      if( FuncTmrQueue_FSM.WakeUpTimeCountDown > CountDown ){   // <! check if the value is overflow
        FuncTmrQueue_FSM.WakeUpTimeCountDown -= CountDown;
      }else{                                  // <! overflow situation
        FuncTmrQueue_FSM.WakeUpTimeCountDown = 0;    // <! countdown set to 0
        FuncTmrQueue_FSM.WakeUpFlag = 1;             // <! wake up the FSM
        FuncTmrQueue_FSM.WakeUpImmediately = 0x0;
      }
      // <! need immediately wakeup
      if( FuncTmrQueue_FSM.WakeUpImmediately ){
        DB_PRINT("WakeUpImmediately,WakeTime:%d\r\n" , FuncTmrQueue_FSM.WakeUpTime);
        DB_PRINT("CountDown: %d\r\n" , FuncTmrQueue_FSM.WakeUpTimeCountDown);
        FuncTmrQueue_FSM.WakeUpTime = FuncTmrQueue_FSM.WakeUpTime
                                    - FuncTmrQueue_FSM.WakeUpTimeCountDown;
        FuncTmrQueue_FSM.WakeUpTimeCountDown = 0;    // <!  countdown set to 0
        FuncTmrQueue_FSM.WakeUpFlag = 1;             // <! wake up the FSM
        FuncTmrQueue_FSM.WakeUpImmediately = 0x0;
        DB_PRINT("WakeUpTime:%d\r\n" , FuncTmrQueue_FSM.WakeUpTime);
      }
    }
    Accumulate = 0;
  }else if( FuncTmrQueue_FSM.WakeUpFlag == 1 ){ // <! FSM is suspended
    Accumulate += CountDown;            // <! record the missing time
  }
}

void FuncTmrQueueMgr_Init( void ){
//  if( FuncTmrQueue_FSM.InitFlag != FuncTmrQueueFSMInitFlag ){
    uint8_t i;
    for( i=0; i<FuncTmrQueueNUM; i++ ){
      memset( (void *)&(FuncTmrQueue[i]) , 0x00, sizeof( struct FuncTmrQueue_t ) );
    }
    FuncTmrQueue_FSM.FuncQueue = NULL;
    
    memset( &FuncTmrQueue_FSM, 0x0 , sizeof(struct FuncTmrQueue_FSM_t) );
    FuncTmrQueue_FSM.WakeUpFlag = 1;
    FuncTmrQueue_FSM.InitFlag = FuncTmrQueueFSMInitFlag;

//     if( taskmgr_menu_handler == NULL ){
//       taskmgr_menu_handler = menu_add( "TaskMgr" , NULL , NULL );
//       menu_set_entering_callback( taskmgr_menu_handler , taskmgr_entering_callback );
//       menu_set_leaving_callback( taskmgr_menu_handler , taskmgr_leaving_callback );
//     }
//  }
}

// ===========================  ?“那㊣?∩DD?車芍D1邦角赤?¯ ======================= // 
void FuncTmrQueueMgr( void ){
//  if( FuncTmrQueue_FSM.InitFlag != FuncTmrQueueFSMInitFlag ){
//    uint8_t i;
//    for( i=0; i<FuncTmrQueueNUM; i++ )
//      memset( &(FuncTmrQueue[i]) , 0x00, sizeof( struct FuncTmrQueue_t ) );
//    FuncTmrQueue_FSM.WakeUpFlag = 0;
//    FuncTmrQueue_FSM.WakeUpTimeCountDown = 0;
//    FuncTmrQueue_FSM.WakeUpTime = 0;
//    FuncTmrQueue_FSM.InitFlag = FuncTmrQueueFSMInitFlag;
//    FuncTmrQueue_FSM.FuncQueue = NULL;
//  }
  TaskMgrRunningFlag = true;
  if( FuncTmrQueue_FSM.WakeUpFlag == 1 ){       // <! FSM needed to wake up
    uint32_t  MinWakeUpTime = 60000;            // <! the longest time to wakeup
    
    if( FuncTmrQueue_FSM.FuncQueue != NULL ){     // <! task existed
      struct FuncTmrQueue_t *p , *q = NULL;
      p = FuncTmrQueue_FSM.FuncQueue;
      while( p != NULL ){
        if( p -> Delay <= FuncTmrQueue_FSM.WakeUpTime ){ // <! time's up , need to execute
          if( p -> KillFlag ){              // <! needed to be kill
            p -> AutoReload = 0;            // <! reset all the value         
            p -> Delay = 0;
            p -> Func = NULL;
          }else{                            // <! normal task
            p -> Delay = 0;                 // <! 
            SET_RUNNING_TASK(p);
            DB_PRINT( "Run Func : %s\r\n" , (p -> FuncName) );
            p -> Func();                    // <! execute the callback function
            SET_RUNNING_TASK(NULL);
          }
          if( p -> AutoReload > 0 ){          // <! if not run once task
            if( p -> Delay == 0x0 ){          // <! if delay has been set by callback , then we don't set reset here
              p -> Delay = p -> AutoReload;   // <! reload the timer
            }
          }else{                    // <! it's run once task , need to delete from the queue
            if( p  ==  FuncTmrQueue_FSM.FuncQueue ){  // <! if it's the first task
              FuncTmrQueue_FSM.FuncQueue = p -> Next;
              memset( p , 0x00 , sizeof( struct FuncTmrQueue_t ) );  // <! clear the block
              p = FuncTmrQueue_FSM.FuncQueue;
              continue;
            }else{           // <! if it's not the first , just change the pointer
              q -> Next = p -> Next;
              memset( p , 0x00 , sizeof( struct FuncTmrQueue_t ) ); // <! clear the block
              p = q->Next;
              continue;
            }
          }
        }else{                    // <! delay time not yet finish, keep decresing
          p -> Delay -= FuncTmrQueue_FSM.WakeUpTime;  // <! sub the sleep time
        }
        if( p -> Delay < MinWakeUpTime && p -> Delay > 0 ){ // <! get the shortest wakeup time
          MinWakeUpTime = p -> Delay;
        }
        q = p;
        p = p -> Next;
      }
      FuncTmrQueue_FSM.WakeUpTimeCountDown = MinWakeUpTime;// <! set as the shortest sleep time
      FuncTmrQueue_FSM.WakeUpTime = MinWakeUpTime;        // <! set the sleep time
      
    }else{  // <! if there is no task in queue , we need to wakeup every 60s
      FuncTmrQueue_FSM.WakeUpTimeCountDown = MinWakeUpTime;// <! set as the shortest sleep time
      FuncTmrQueue_FSM.WakeUpTime = MinWakeUpTime;        // <! set the sleep time
    }
    FuncTmrQueue_FSM.WakeUpFlag = 0;                    // <! clear the flag
  }
  TaskMgrRunningFlag = false;
}

// ===================== Add task to queue ====================== //
int8_t add_task_tmr_queue( uint32_t Delay , 
                           uint32_t AutoReload ,
                           void (*Func)(void) ,
                           char *FuncName)
{
  #define GET_TIMESTAMP()     (FuncTmrQueue_FSM.WakeUpTime - FuncTmrQueue_FSM.WakeUpTimeCountDown)
  uint8_t i,repeat=0;
  if( FuncTmrQueue_FSM.InitFlag != FuncTmrQueueFSMInitFlag ){   //??????????, ?????????
    FuncTmrQueueMgr_Init();
  }
  //if( TaskMgrRunningFlag ){ u_printf( "TaskMgr is sheduling. this might get error\n" ); }
  while(repeat<2){
    for( i=0; i<FuncTmrQueueNUM; i++ ){       // <! Go through the task list
      if( repeat==0 ){
        if( FuncTmrQueue[i].Func == Func ){   // <! check the existing functions
          if( GET_RUNNING_TASK() == &FuncTmrQueue[i] ){   // <! if support multi-thread , the function might be running at the time
            DB_PRINT( "Func %s is running..\r\n" , FuncTmrQueue[i].FuncName );
            if( FuncTmrQueue[i].AutoReload == 0x0 || FuncTmrQueue[i].KillFlag > 0x0 ){  // <! one time task or going to be killed
              continue;   // <! Keep going , force it to add new task.
            }else{
              FuncTmrQueue[i].Delay = Delay;            // <! set the delay value
              FuncTmrQueue[i].AutoReload = AutoReload;  // <! set auto-reload value
              FuncTmrQueue[i].Func = Func;              // <! set the callback function
              FuncTmrQueue[i].KillFlag = 0;             // <! of course can't be killed
              
              //snprintf( FuncTmrQueue[i].FuncName, FUNCTION_NAME_MAX_LEN, "%s", FuncName );
              
              FuncTmrQueue_FSM.WakeUpImmediately = 1;   // <! wake up the fsm immediately
              DB_PRINT( "func : %s is changed\r\n" , FuncTmrQueue[i].FuncName);
              return 0;
            }
          }else{
            Delay += GET_TIMESTAMP();                 // <! Add the timestamp from now
            DB_PRINT("delay changed to %d\r\n" , Delay );
            FuncTmrQueue[i].Delay = Delay;            // <! set the delay value
            FuncTmrQueue[i].AutoReload = AutoReload;  // <! set auto-reload value
            FuncTmrQueue[i].Func = Func;              // <! set the callback function
            FuncTmrQueue[i].KillFlag = 0;             // <! of course can't be killed
            
            //snprintf( FuncTmrQueue[i].FuncName, FUNCTION_NAME_MAX_LEN, "%s", FuncName );
            
            FuncTmrQueue_FSM.WakeUpImmediately = 1;   // <! wake up the fsm immediately
            DB_PRINT( "func : %s is changed\r\n" , FuncTmrQueue[i].FuncName );
            return 0;
          }
        }
      }else{
        if( FuncTmrQueue[i].Func == NULL ){         // <! search available block for task
          FuncTmrQueue[i].Next = NULL;
          Delay += GET_TIMESTAMP();                 // <! Add the timestamp from now
          DB_PRINT("delay changed to %d\r\n" , Delay );
            
          FuncTmrQueue[i].Delay = Delay;            // <! set the delay value
          FuncTmrQueue[i].AutoReload = AutoReload;  // <! set auto-reload value
          FuncTmrQueue[i].Func = Func;              // <! set the callback function
          FuncTmrQueue[i].KillFlag = 0;             // <! of course can't be killed
          
          //snprintf( FuncTmrQueue[i].FuncName, FUNCTION_NAME_MAX_LEN, "%s", FuncName );
          
          if( FuncTmrQueue_FSM.FuncQueue == NULL ){ // <! if the list has no task
            FuncTmrQueue_FSM.FuncQueue = &FuncTmrQueue[i];  // <! put it at the front
          }else{                                    // <! if the list already has tasks
            struct FuncTmrQueue_t *p , *q;
            for( p=FuncTmrQueue_FSM.FuncQueue; p!=NULL; p=p->Next ){  // <! get the last pointer
              q = p;
            }
            q -> Next = &FuncTmrQueue[i];           // <! put the task to last
          }
          FuncTmrQueue_FSM.WakeUpImmediately = 1;   // <! wake up the fsm immediately
          DB_PRINT( "func :%s is added\r\n" , FuncTmrQueue[i].FuncName );
          return 0;
        }
      }
    }
    repeat++;
  }
  return -1;     //?T???D?迆∩??谷㊣㏒∩?D??⊿,﹞米??∩赤?車
}
// =====================  ∩車?“那㊣?∩DD?車芍D谷?3y豕??? ====================== //
int8_t del_task_tmr_queue( void (*Func)(void), char *FuncName ){
  uint8_t i;
  if( FuncTmrQueue_FSM.InitFlag != FuncTmrQueueFSMInitFlag ){   // <! system hasn't been initialize
    FuncTmrQueueMgr_Init();
  }
  DB_PRINT( "del func : %s \r\n" , FuncName );
  for( i=0; i<FuncTmrQueueNUM; i++ ){   // <! go through the queue
    if( FuncTmrQueue[i].Func == Func ){  // <! find the block
      DB_PRINT( "found func [%d] \r\n" , i );
      FuncTmrQueue[i].KillFlag = 1;     // <! kill it
      FuncTmrQueue[i].Delay = 1;
      FuncTmrQueue_FSM.WakeUpImmediately = 1;
      return 1;
    } 
  }
  DB_PRINT( "func not found \r\n" );
  return 0;
}
// ================  ∩車?“那㊣?∩DD?車芍D2谷?辰豕???那?﹞?∩??迆 =================== //
int8_t QueryTask_TmrQueue( void (*Func)(void) ){
  uint8_t i;
  if( FuncTmrQueue_FSM.InitFlag != FuncTmrQueueFSMInitFlag ){   // <! system hasn't been initialize
    FuncTmrQueueMgr_Init();
  }
  for( i=0; i<FuncTmrQueueNUM; i++ ){    // <! go through the queue
    if( FuncTmrQueue[i].Func == Func ){  // <! find the block
      return 1;
    } 
  }
  return 0;
}




