#include <stdio.h>
#include <stdlib.h>

#include "data_manager.h"
#include "nordic_common.h"
#include "nrf_error.h"
#include "ble_flash.h"
#include "app_trace.h"
#include "TaskMgr.h"

pstorage_handle_t 							m_dm_block_id;
pstorage_module_param_t 				m_dm_param;

volatile static uint8_t dm_op_update_state = 0;
volatile static uint8_t dm_op_store_state = 0;
volatile static uint8_t dm_op_load_state = 0;
volatile static uint8_t dm_op_clear_state = 0;

static uint8_t pstorage_wait_flag = 0;
static pstorage_block_t pstorage_wait_handle = 0;

//
//
//
uint32_t dm_flash_status_get( void ){
	uint32_t    retval;
	uint32_t    count = 0;   
	
	// Request clearing of blocks
	retval = pstorage_access_status_get(&count);
  
  if( NRF_SUCCESS == retval ){
    if (count == 0){
      // No pending operations, safe to power off or enter radio intense operations.
      return 0;
    }
    else{
      // Storage access pending, wait!
      //dm_log("pstorage_access_status_get busy.\r\n");
      return 1;
    }
  }else{
    dm_log("pstorage_access_status_get error.\r\n");
    return 2;
  }
}


//
// This API is used to clear data in storage blocks. The event notified using a 
// registered callback will indicate when this operation is complete. The event 
// result indicates whether the operation was successful or not.
// The size requested to be erased has to be equal or a multiple of the block size.
//
uint32_t dm_flash_data_clear( pstorage_size_t id, uint32_t size ){
	uint32_t retval = NRF_SUCCESS; 	 
	pstorage_handle_t base_handle;
		
	if( size > DM_USER_MAX_SIZE ){
		return 3;
	}
	id -= 1; // 兼容APP
	retval = pstorage_block_identifier_get( &m_dm_block_id, id, &base_handle );
  if( NRF_SUCCESS != retval){
    return NRF_ERROR_INVALID_STATE;
  }

  pstorage_wait_handle = base_handle.block_id;
  pstorage_wait_flag = 1;
  retval = pstorage_clear(&base_handle, size);
  if( NRF_SUCCESS != retval){
    return NRF_ERROR_INVALID_STATE;
  }else{
    while( pstorage_wait_flag ){ power_manage(); }
  }
  
  return NRF_SUCCESS;
}


//
// This API is used to read data from a storage block. It is permitted to read a 
// part of the block using the offset field. The application should ensure that 
// the destination has enough memory to copy data from the storage block to the 
// destination pointer provided in the API.
//
// Note: The block size and offset in load and store should be a multiple of word size (4 bytes).
//
uint32_t dm_flash_data_load( pstorage_size_t   id, 
                              uint8_t          *p_dst, 
                              pstorage_size_t   byte_nbr, 
                              pstorage_size_t   offset ){
	uint32_t retval = NRF_SUCCESS;
	pstorage_handle_t base_handle;
  
  id -= 1; //兼容APP
	if( NULL == p_dst || id > DM_BLOCK_MAX_NUMBER ){
    return NRF_ERROR_INVALID_STATE;
  }
  
  retval = pstorage_block_identifier_get( &m_dm_block_id, id, &base_handle );
  if( NRF_SUCCESS != retval){
    return NRF_ERROR_INVALID_STATE;
  }
  
  pstorage_wait_handle = base_handle.block_id;
  pstorage_wait_flag = 1;
  retval = pstorage_load( p_dst, &base_handle, byte_nbr, offset );
  if( NRF_SUCCESS != retval ){
    return NRF_ERROR_INVALID_STATE;
  }else{
//     while( pstorage_wait_flag ){ power_manage(); };
    while( pstorage_wait_flag );
  }
  return NRF_SUCCESS;
}

//
/**
	Function for updating persistently stored data of length 'size' contained in 
  the 'p_src' address in the storage module at 'p_dest' address.

	Parameters
	[in]	p_src	  Source address containing data to be stored. API assumes this to 
								be resident memory and no intermediate copy of data is made by the API.
	[in]	size	  Size of data to be stored expressed in bytes. Must be word aligned 
								and size + offset must be <= block size.
	[in]	offset	Offset in bytes to be applied when writing to the block. For example, 
								if within a block of 100 bytes, the application wishes to write 20 
								bytes at an offset of 12 bytes, then this field should be set to 12. 
								Must be word aligned.
 */
//
uint32_t dm_flash_data_update( pstorage_size_t 	 id, 
                               uint8_t 					*p_src, 
                               pstorage_size_t   size, 
                               pstorage_size_t   offset ){
	uint32_t retval = NRF_SUCCESS;
	pstorage_handle_t base_handle;
  
  if( NULL == p_src || id > DM_BLOCK_MAX_NUMBER ){
    return NRF_ERROR_INVALID_STATE;
  }
  dm_log("get id --> p_in_id = %d.\r\n", id);
  dm_log_dump(p_src, size);
#if 1
  id -= 1; // 兼容APP
  retval = pstorage_block_identifier_get( &m_dm_block_id, id, &base_handle );
  if( NRF_SUCCESS != retval){
    return NRF_ERROR_INVALID_STATE;
  }
//  while( ble_wait_radio_free() ){ power_manage(); };
  pstorage_wait_handle = base_handle.block_id;
  pstorage_wait_flag = 1;
  if( NRF_SUCCESS != pstorage_clear( &base_handle, size ) ){
    return NRF_ERROR_INVALID_STATE;
  }
  while( pstorage_wait_flag ){ power_manage(); };
    
  uint8_t p_load_data[DM_BLOCK_MAX_SIZE] = {0};
  
  pstorage_load( &p_load_data[0], &base_handle, size, 0 );
  dm_log_dump( p_load_data, size );
//  while( ble_wait_radio_free() ){ power_manage(); }; // 等待radio空闲
  pstorage_wait_handle = base_handle.block_id;
  pstorage_wait_flag = 1;
  if( NRF_SUCCESS != pstorage_store( &base_handle, p_src, size, offset)){
    return NRF_ERROR_INVALID_STATE;
  }
  while( pstorage_wait_flag ){ power_manage(); };
  
  pstorage_load( &p_load_data[0], &base_handle, size, 0 );
  dm_log_dump( p_load_data, size );
  
  /* 读取校验数据 */
//  if( 0 != memcpy( p_load_data, p_src, size ) ){
//    
//  }
  
//  free( p_src );
#endif
  return NRF_SUCCESS;   
}


uint8_t *p_data = NULL;


void dm_test_update( void ){
#if 0
	uint8_t buf[DM_BLOCK_MAX_SIZE] = {0};
  static uint8_t cnt = 0;
  uint8_t retval = 0;
  
	for( uint8_t i = 0; i < DM_BLOCK_MAX_SIZE ; i++ ){
		buf[i] = 0x00;
	}
  buf[0] += cnt;
	//dm_log_dump( buf, DM_BLOCK_MAX_SIZE );
	uint8_t *data = ( uint8_t * )malloc( DM_BLOCK_MAX_SIZE );
	if( NULL != data ){
		memcpy( data, buf, DM_BLOCK_MAX_SIZE );
		p_data = data;
		retval = dm_flash_data_update(1, p_data, DM_BLOCK_MAX_SIZE, 0);
    if( NRF_SUCCESS == retval){
      cnt++;
			//return 0;
		}else{
			dm_log("pstorage update data failed.\r\n");
			//return 1;
		}
	}else{
		dm_log("get block identidier failed.\r\n");
		//return 2;
	}
#else
  dm_flash_data_clear( 1, DM_USER_MAX_SIZE );
#endif
}

void dm_test_load( void ){
  static uint8_t buf[DM_BLOCK_MAX_SIZE] = {0};

  // read
  //dm_log(str);
  dm_log("test load:");
  dm_flash_data_load(1, buf, DM_BLOCK_MAX_SIZE, 0);
  dm_log_dump( buf, DM_BLOCK_MAX_SIZE );
  
  dm_flash_data_load(1, buf, DM_BLOCK_MAX_SIZE, 0);
  dm_log_dump( buf, DM_BLOCK_MAX_SIZE );
  
  dm_flash_data_load(2, buf, DM_BLOCK_MAX_SIZE, 0);
  dm_log_dump( buf, DM_BLOCK_MAX_SIZE );
  
  dm_flash_data_load(3, buf, DM_BLOCK_MAX_SIZE, 0);
  dm_log_dump( buf, DM_BLOCK_MAX_SIZE );
    
}

void dm_test_final_test( void ){
  
  /* ==========================================================================
  
  */
  //  m_dm_block_id
  pstorage_handle_t block_0_handle;
  pstorage_handle_t block_1_handle;
  pstorage_handle_t block_2_handle;
  pstorage_handle_t block_3_handle;
  
  uint32_t retval = NRF_SUCCESS;
  uint8_t buf[DM_BLOCK_MAX_SIZE] = {0};
  
	for( uint8_t i = 0; i < DM_BLOCK_MAX_SIZE ; i++ ){
		buf[i] = i;
	}
  uint8_t *p_read_data = ( uint8_t * )malloc( DM_BLOCK_MAX_SIZE );
  uint8_t *p_write_data = ( uint8_t * )malloc( DM_BLOCK_MAX_SIZE );
	if( NULL == p_write_data || NULL == p_read_data ){
		dm_log("malloc error.\r\n");
  }
  memcpy( p_write_data, buf, DM_BLOCK_MAX_SIZE );
  memset( p_read_data, 0, DM_BLOCK_MAX_SIZE );
  
  retval = pstorage_block_identifier_get( &m_dm_block_id, 0, &block_0_handle ); 
  retval = pstorage_block_identifier_get( &m_dm_block_id, 1, &block_1_handle );
  retval = pstorage_block_identifier_get( &m_dm_block_id, 2, &block_2_handle );
  retval = pstorage_block_identifier_get( &m_dm_block_id, 3, &block_3_handle );
  if( NRF_SUCCESS != retval ){
    dm_log( "get id error.\r\n" );
  }
  /* load ------------------------------------------------------------------- */
  dm_log( "start load...\r\n" );
  dm_log( "======================================================\r\n" );
  pstorage_load( p_read_data, &block_0_handle, DM_BLOCK_MAX_SIZE, 0 );
  dm_log_dump( p_read_data, DM_BLOCK_MAX_SIZE );
  
  pstorage_load( p_read_data, &block_1_handle, DM_BLOCK_MAX_SIZE, 0 );
  dm_log_dump( p_read_data, DM_BLOCK_MAX_SIZE );
  
  pstorage_load( p_read_data, &block_2_handle, DM_BLOCK_MAX_SIZE, 0 );
  dm_log_dump( p_read_data, DM_BLOCK_MAX_SIZE );
  
  pstorage_load( p_read_data, &block_3_handle, DM_BLOCK_MAX_SIZE, 0 );
  dm_log_dump( p_read_data, DM_BLOCK_MAX_SIZE );
  
  /* clear ------------------------------------------------------------------ */
  dm_log( "start clear all area...\r\n" );
  dm_log( "======================================================\r\n" );
  dm_op_clear_state = 0;
  pstorage_wait_handle = block_0_handle.block_id;
  pstorage_wait_flag = 1; 
  retval = pstorage_clear( &block_0_handle, DM_BLOCK_MAX_SIZE * 32 ); // 清除4个block
  if( NRF_SUCCESS != retval ){
    dm_log( "clear error.\r\n" );
  }
  while( pstorage_wait_flag ){ power_manage(); };

  pstorage_load( p_read_data, &block_0_handle, DM_BLOCK_MAX_SIZE, 0 );
  dm_log_dump( p_read_data, DM_BLOCK_MAX_SIZE );
  
  pstorage_load( p_read_data, &block_1_handle, DM_BLOCK_MAX_SIZE, 0 );
  dm_log_dump( p_read_data, DM_BLOCK_MAX_SIZE );
  
  pstorage_load( p_read_data, &block_2_handle, DM_BLOCK_MAX_SIZE, 0 );
  dm_log_dump( p_read_data, DM_BLOCK_MAX_SIZE );
  
  pstorage_load( p_read_data, &block_3_handle, DM_BLOCK_MAX_SIZE, 0 );
  dm_log_dump( p_read_data, DM_BLOCK_MAX_SIZE );
  
  /* store ------------------------------------------------------------------ */
//  dm_log( "start store...\r\n" );
//  dm_log( "======================================================\r\n" );
//  retval = pstorage_store( &block_0_handle, p_write_data, DM_BLOCK_MAX_SIZE, 0 );
//  pstorage_wait_handle = block_1_handle.block_id;
//  pstorage_wait_flag = 1; 
//  retval = pstorage_store( &block_1_handle, p_write_data, DM_BLOCK_MAX_SIZE, 0 );
//  if( NRF_SUCCESS != retval ){
//    dm_log( "update error.\r\n" );
//  }
//  while( pstorage_wait_flag ){ power_manage(); };
//
//  retval = pstorage_store( &block_2_handle, p_write_data, DM_BLOCK_MAX_SIZE, 0 );
//  pstorage_wait_handle = block_3_handle.block_id;
//  pstorage_wait_flag = 1; 
//  retval = pstorage_store( &block_3_handle, p_write_data, DM_BLOCK_MAX_SIZE, 0 );
//  if( NRF_SUCCESS != retval ){
//    dm_log( "update error.\r\n" );
//  }
//  while( pstorage_wait_flag ){ power_manage(); };
//
//  /* load ------------------------------------------------------------------- */
//  dm_log( "start load...\r\n" );
//  dm_log( "======================================================\r\n" );
//  pstorage_load( p_read_data, &block_0_handle, DM_BLOCK_MAX_SIZE, 0 );
//  dm_log_dump( p_read_data, DM_BLOCK_MAX_SIZE );
//  
//  pstorage_load( p_read_data, &block_1_handle, DM_BLOCK_MAX_SIZE, 0 );
//  dm_log_dump( p_read_data, DM_BLOCK_MAX_SIZE );
//  
//  pstorage_load( p_read_data, &block_2_handle, DM_BLOCK_MAX_SIZE, 0 );
//  dm_log_dump( p_read_data, DM_BLOCK_MAX_SIZE );
//  
//  pstorage_load( p_read_data, &block_3_handle, DM_BLOCK_MAX_SIZE, 0 );
//  dm_log_dump( p_read_data, DM_BLOCK_MAX_SIZE );
//  
//  /* clear after load ------------------------------------------------------- */
//  dm_log( "clear after load...\r\n" );
//  dm_log( "======================================================\r\n" );
//  pstorage_wait_handle = block_0_handle.block_id;
//  pstorage_wait_flag = 1;;
//  retval = pstorage_clear( &block_0_handle, DM_BLOCK_MAX_SIZE ); // 清除 block_0
//  while( pstorage_wait_flag ){ power_manage(); };
//
//  /* load after clear ------------------------------------------------------- */
//  dm_log( "load after clear...\r\n" );
//  dm_log( "======================================================\r\n" );
//  pstorage_load( p_read_data, &block_0_handle, DM_BLOCK_MAX_SIZE, 0 );
//  dm_log_dump( p_read_data, DM_BLOCK_MAX_SIZE );
//  
//  dm_log( "load...\r\n" );
//  pstorage_load( p_read_data, &block_1_handle, DM_BLOCK_MAX_SIZE, 0 );
//  dm_log_dump( p_read_data, DM_BLOCK_MAX_SIZE );
//  
//  pstorage_load( p_read_data, &block_2_handle, DM_BLOCK_MAX_SIZE, 0 );
//  dm_log_dump( p_read_data, DM_BLOCK_MAX_SIZE );
//  
//  pstorage_load( p_read_data, &block_3_handle, DM_BLOCK_MAX_SIZE, 0 );
//  dm_log_dump( p_read_data, DM_BLOCK_MAX_SIZE );
//  dm_log( "======================================================\r\n" );
  
  free( p_read_data );
  free( p_write_data );
  
}


//
// 存储管理器事件回调函数
// 当前flash的数据更新完成就会进入该事件回调函数
//
static void dm_event_callback( pstorage_handle_t *handle, 
																					uint8_t op_code, 
																					uint32_t result, 
																					uint8_t *p_data, 
																					uint32_t length ){
  uint32_t retval = NRF_SUCCESS;
  if(handle->block_id == pstorage_wait_handle) { pstorage_wait_flag = 0; }
  switch( op_code ){
		case PSTORAGE_STORE_OP_CODE:		// 数据存储
			if( NRF_SUCCESS == retval ){ 
				dm_log("flash data store successed.\r\n");
			}else{
				dm_log("flash data store failed.\r\n");
			}
			//这里可以释放缓存
			free( p_data );
		break;
		case PSTORAGE_LOAD_OP_CODE:			// 数据读取
			if( NRF_SUCCESS == retval ){ 
				dm_log("flash data load successed.\r\n");
			}else{
				dm_log("flash data load failed.\r\n");
			}
		break;
		case PSTORAGE_CLEAR_OP_CODE:		// 数据清理
			if( NRF_SUCCESS == retval ){ 
				dm_log("flash data clear successed.\r\n");
			}else{
				dm_log("flash data clear failed.\r\n");
			}
		break;
		case PSTORAGE_UPDATE_OP_CODE:		// 数据更新
			if( NRF_SUCCESS == retval ){ 
				dm_log("flash data update successed.\r\n");
			}else{
				dm_log("flash data update failed.\r\n");
			}
			//这里可以释放缓存
			// ...
//			free( p_data );
		break;
		default:
		break;
	}
}

//
// flash数据管理器
// 申请物理存储块数量为 DM_BLOCK_MAX_NUMBER = 32.
// 申请物理存储块的单个存储空间为16Bytes.
//
// 注意:规定最小申请空间大小为16Bytes.
//
uint32_t dm_flash_init( void ){
  uint32_t err_code = NRF_SUCCESS;
	
	m_dm_param.block_count = DM_BLOCK_MAX_NUMBER;
	m_dm_param.block_size  = DM_BLOCK_MAX_SIZE;
	m_dm_param.cb          = dm_event_callback;

	err_code = pstorage_init();
	if( NRF_SUCCESS != err_code ){
		dm_log("pstorage init error.\r\n");
		return NRF_ERROR_INVALID_STATE;
	}else{
		dm_log("pstorage init ok.\r\n");
	}
	err_code = pstorage_register( &m_dm_param, &m_dm_block_id );
	if( NRF_SUCCESS != err_code ){
		dm_log("pstorage register error.\r\n");
		return NRF_ERROR_INVALID_STATE;
	}else{
		dm_log("pstorage register ok.\r\n");
	}
  
//  dm_test_final_test();
//  AddTask_TmrQueue(5000, 0, dm_test_update);
//  AddTask_TmrQueue(5000, 1000, dm_test_final_test);
  
  return NRF_SUCCESS;
}


/* End of file -------------------------------------------------------------- */

