#ifndef __DATA_MANAGER_H__
#define __DATA_MANAGER_H__

#include <stdint.h>
#include <ysizet.h>

#include "pstorage.h"

#if 0
struct mem_base_t {
  uint32_t start_addr;
  uint32_t end_addr;
  uint32_t page_nbr;
  uint32_t page_size;
};


struct mem_data_t {
  uint32_t offset_addr;
  uint32_t crc32;
  uint16_t length;
  uint8_t *data;
};


struct mem_ll_t {
  struct mem_ll_t *next;
  struct mem_data_t *ds;
};


struct mem_dm_t {
  struct mem_base_t * init;
  struct mem_ll_t   * node;        // data structure.
  uint32_t          * curr_addr;
  uint32_t        ( * write )( size_t start_addr, size_t *p_in_array , size_t length );
	uint32_t        ( * read  )( size_t start_addr, size_t *p_out_array, size_t length );
	uint32_t        ( * erase )( size_t start_addr, size_t  length );
};

#endif

// For DM debug
#define USE_DM_LEVEL																(0)

#if ( USE_DM_LEVEL == 2 )
  #define dm_log(fmt, ...)												app_trace_log("[%d][%05d][dm ] "fmt, \
                                                   __SYS_REMAIN_HEAP_SIZE_, __LINE__, ##__VA_ARGS__)
  #define dm_log_dump                            app_trace_dump
#elif ( USE_DM_LEVEL == 1 )
  #define dm_log(fmt, ...)												app_trace_log("[dm ] "fmt, ##__VA_ARGS__)
  #define dm_log_dump                            app_trace_dump
#elif ( USE_DM_LEVEL == 0 )
  #define dm_log(fmt, ...)												
  #define dm_log_dump                            
#endif

#define DM_BLOCK_MAX_SIZE													(16)
#define DM_BLOCK_MAX_NUMBER												(20)
#define DM_USER_MAX_SIZE													(DM_BLOCK_MAX_SIZE * DM_BLOCK_MAX_NUMBER)

extern pstorage_handle_t 							            m_dm_block_id;
extern pstorage_module_param_t 				            m_dm_param;

extern void power_manage(void);

//
// 初始化flash数据管理器
//
uint32_t 	dm_flash_init( void );
uint32_t 	dm_flash_data_clear( pstorage_size_t id, uint32_t size );
uint32_t 	dm_flash_data_load( pstorage_size_t   id, 
																				 uint8_t          *p_dst, 
																				 pstorage_size_t   byte_nbr, 
																				 pstorage_size_t   offset );

uint32_t  dm_flash_data_update( pstorage_size_t 	 id, 
																					  uint8_t 					*p_src, 
																					  pstorage_size_t   size, 
																					  pstorage_size_t   offset );



#endif



