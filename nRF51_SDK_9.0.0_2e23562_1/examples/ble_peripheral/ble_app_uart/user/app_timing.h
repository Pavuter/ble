#ifndef __APP_TIMING_H_
#define __APP_TIMING_H_

#include <stdbool.h>
#include <stdint.h>

#include "app_calendar.h"
#include "data_manager.h"

enum flash_operate_e{
  FLASH_OPERATE_OK          = 0x80,
  FLASH_OPERATE_READY_WRITE = 0x40,
  FLASH_OPERATE_READY_ERASE = 0x20,
};

#pragma pack(push, 1)
// 
// 定时事件时间结构体
//
struct timing_time_t{
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t wday;
};

struct timing_t{
  uint8_t  id;
  uint8_t node;
  uint8_t mode;
  uint8_t value;                         // 定时动作使能
  bool    enable;                        // 定时器使能
  struct  timing_time_t time;           // 定时器参数列表
};


struct timer_msg_t{
  struct timing_t msg;
  uint32_t ts;
  uint16_t crc16;
};

//
// 定时器结构体
// 
struct timing_node_t 
{
  struct timing_node_t *next;
  struct timing_t event;
//  uint16_t crc16;
};

#define TIMING_EVENT_PUT_SIZE						( sizeof(struct timing_t) )
#define TIMING_EVENT_MOUNT_SIZE         ( sizeof(struct timing_node_t) ) 
#define TIMING_EVENT_COMPUTE_SIZE       ( TIMING_EVENT_PUT_SIZE+sizeof(uint32_t) ) /* payload + timestamp */
//#define TIMING_EVENT_FLASH_SIZE		      ( TIMING_EVENT_COMPUTE_SIZE+sizeof(((struct timing_node_t*)0)->crc16) )
#define TIMING_EVENT_FLASH_SIZE         ( sizeof(struct timer_msg_t) )

#pragma pack(pop)

// For debug
#define DEBUG_TIMING_LEVEL      				( 2 )
#if ( DEBUG_TIMING_LEVEL == 2 )
#define tim_log(fmt, ...)								app_trace_log("[%d][%05d][tim] "fmt, \
                                        __SYS_REMAIN_HEAP_SIZE_, __LINE__, ##__VA_ARGS__)
  #define tim_log_dump                  app_trace_dump
#elif ( DEBUG_TIMING_LEVEL == 1 )
  #define tim_log(fmt, ...)							app_trace_log("[tim] "fmt, ##__VA_ARGS__)
  #define tim_log_dump                  app_trace_dump
#elif ( DEBUG_TIMING_LEVEL == 0 )
  #define tim_log(...)												
  #define tim_log_dump(...)                            
#endif

#define TIMING_EVENT_ID_MAX_NBR         ( DM_BLOCK_MAX_NUMBER )
#define TIMING_EVENT_PU_MAX_SIZE				( 16 )										// 2bytes crc16

// dummy byte
#define DUMMY_BYTE											( 0xff )

void                    timing_init(void);
void                    timing_event_scan_task(void);
uint32_t                timing_calculate_event_ts(struct timing_t *msg);
uint32_t 								timing_event_delete( struct timing_node_t *node );
struct timing_node_t * 	timing_seek_node( struct timing_t * node_t, uint8_t id);
uint32_t 								timing_event_modify( struct timing_t * timer_new );
uint32_t 								timing_event_reg(struct timing_t * node);
uint32_t  							timing_delete_id( uint8_t d_id );
uint32_t  							timing_reg_id( uint8_t new_id );
uint8_t  		 					  timing_allocate_id( void );
uint32_t 								timing_get_event_nbr(void);
uint32_t                timing_event_list_load( void );
uint8_t                 timing_read_event_infor( uint8_t *evt_infor, uint8_t node_nbr );

void    								ble_flash_data_update_task(void);


#endif 


