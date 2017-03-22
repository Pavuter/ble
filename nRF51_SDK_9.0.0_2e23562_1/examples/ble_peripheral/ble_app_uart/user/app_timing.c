#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "app_timing.h"
#include "nordic_common.h"
#include "ble_flash.h"
#include "nrf_delay.h"
#include "ptl.h"

#include "app_calendar.h"

#include "queue_mgr.h"
#include "TaskMgr.h"


static uint32_t m_timing_event_id_list = 0x00000000;

// ��ʱʱ�¼��б�
static struct timing_node_t *timing_node = NULL;
	
// ��ѯ��ʱʱ���Ƿ�ﵽ 
static void timing_event( uint8_t value )
{
  tim_log("timing_event run...action =%d.\r\n", value);
  
  ble_switch_status_changed = SWITCH_STATUS_OPERATE;
  m_ble_switch_status.value = value;
}


//
// ��ȡ��ǰϵͳ�Ķ�ʱ�¼��ܵĸ���
//
uint32_t timing_get_event_nbr(void){
  uint32_t node_total_nbr = 0;
  uint32_t list = m_timing_event_id_list;
  tim_log("m_timing_event_id_list = 0x%08x...\r\n", m_timing_event_id_list);
  for(uint32_t cnt = 0; cnt < TIMING_EVENT_ID_MAX_NBR; cnt++){
    if( list & 0x01 ){
      node_total_nbr++;
    }
    list >>= 1;
  }
  return node_total_nbr;
}

//
// ����һ�����õ�ID��
//
uint8_t timing_allocate_id( void ){
  uint32_t i = 0;
  uint32_t node_total_nbr= timing_get_event_nbr();
  
  if( node_total_nbr < TIMING_EVENT_ID_MAX_NBR ){
    for( i = 0; i < TIMING_EVENT_ID_MAX_NBR; i++ ){
      if( 0 == ( m_timing_event_id_list & (1 << i) ) ){
        return (i+1);
      }
    }
  }
  return DUMMY_BYTE;
}


//
// ע��һ���µ�ID��
//
uint32_t timing_reg_id( uint8_t new_id ){
  uint32_t tmp_id = new_id - 1;
  uint32_t bit_pos = 1<<tmp_id;
  
  if( tmp_id < TIMING_EVENT_ID_MAX_NBR ){
    if( 1 != (m_timing_event_id_list & bit_pos) ){
      m_timing_event_id_list |= bit_pos;
      return NRF_SUCCESS;
    }
  }
  return NRF_ERROR_NOT_SUPPORTED;  
}

//
// ɾ��һ��ָ����ID��
//
uint32_t timing_delete_id( uint8_t d_id ){
  if( d_id < TIMING_EVENT_ID_MAX_NBR ){
    m_timing_event_id_list &= ~( 1 << (d_id-1) );
    return 0;
  }
  return NRF_ERROR_NOT_SUPPORTED;
}


//
// ��ȡ��ʱ�¼�����Ч��Ϣ
//
uint8_t timing_read_event_infor( uint8_t *evt_infor_addr, uint8_t node_nbr ){
	uint32_t tmp_node_nbr = 0;
	uint32_t tmp_addr = 0;
	uint8_t size = sizeof(struct timing_t);
	struct timing_node_t *p = NULL;
	
	p = timing_node;
	if( (1 > node_nbr) || (NULL == evt_infor_addr) ){
    return 0;
  }
  do{
      memcpy( evt_infor_addr + tmp_addr, &(p->event.id), size );
      p = p->next;
      tmp_addr += size;
      tmp_node_nbr++;
  }while( (--node_nbr)&&(NULL != p) );
  
  return tmp_node_nbr;
}


//
// ͨ��ID�Ų���ĳ���ڵ�
// id �� node_t ������һ�����ɣ�ȱʧ��ʹ��NULL���
//
struct timing_node_t * timing_seek_node(struct timing_t * node_t , uint8_t id){
  struct timing_node_t * p = NULL;

	if( NULL != node_t ){
    p = timing_node;
		while( NULL != p ){
			if( 0 == ( memcmp((uint8_t*)&(p->event.time.hour), (uint8_t*)&(node_t->time.hour), 3)) ){
        if( (p->event.time.wday>>7) & (node_t->time.wday>>7) ){ // ѭ����ʱ�¼�
          if( 0 != ( ( (p->event.time.wday)<<1 )&( (node_t->time.wday)<<1) ) ){
            tim_log("node -- seek node addr:0x%p...\r\n", p);
            break;
          }
        }else{ // ���ζ�ʱ�¼�
          break;
        }
			}
      p = p->next;
		}
    return p;
	}

	if( NULL != id ){
    p = timing_node;
		while( NULL != p ){
			if( p->event.id == id ){
        tim_log("id -- seek node addr:0x%p...\r\n", p);
				break;
			}
      p = p->next;
		}
    return p;
	}
	
  return NULL;
}


//
// ��ʱʱ��ע��
//
uint32_t timing_event_reg(struct timing_t *p_in_node){
  uint32_t err_code = NRF_SUCCESS;
  
	if( NULL == p_in_node ){
		return PTL_ERROR_INVALID_PARAM;	
	}
  /* �����Ƿ������ͬ��ʱ�¼� */
  if(NULL != timing_seek_node(p_in_node, NULL)){ /* �˴������㷨�����Ż�--03.16 */
    tim_log("PTL_ERROR_SAME_EVT.\r\n");
    return PTL_ERROR_SAME_EVT;
  }
  /* ����id */
  p_in_node->id = timing_allocate_id();
  if( TIMING_EVENT_ID_MAX_NBR < p_in_node->id ){
    tim_log("PTL_ERROR_OUT_OF_EVT.\r\n");
    return PTL_ERROR_OUT_OF_EVT;
  }
  ptl_log("allocate id = 0x%02x. max = 0x%02x\r\n", p_in_node->id, TIMING_EVENT_ID_MAX_NBR);
  /* ׼���������� */
  struct timing_node_t *p_mount = (struct timing_node_t *)malloc(TIMING_EVENT_MOUNT_SIZE);
  if( NULL == p_mount){
    tim_log("NRF_ERROR_NO_MEM.\r\n");
    return PTL_ERROR_NONE_MEM;
  }
  memcpy( (uint8_t*)&(p_mount->event.id), (uint8_t*)&(p_in_node->id), TIMING_EVENT_PUT_SIZE );
  /* ׼��flash�Ĵ洢���� */
  struct timer_msg_t *timer_msg = (struct timer_msg_t *)malloc(TIMING_EVENT_FLASH_SIZE);
  if( NULL == timer_msg ){
    return PTL_ERROR_NONE_MEM;
  }
  memcpy( (uint8_t *)&(timer_msg->msg.id), (uint8_t*)&(p_in_node->id), TIMING_EVENT_PUT_SIZE );
  timer_msg->ts = timing_calculate_event_ts(p_in_node);
  timer_msg->crc16 = ble_flash_crc16_compute( (uint8_t*)timer_msg, TIMING_EVENT_COMPUTE_SIZE, NULL );
  tim_log("tim flash data: ts = %08x crc = %08x.", timer_msg->ts, timer_msg->crc16);
  tim_log_dump((uint8_t *)&(timer_msg->msg.id), TIMING_EVENT_FLASH_SIZE);
  /* ����дflash���� */
  err_code = dm_flash_data_update( timer_msg->msg.id, (uint8_t*)timer_msg, DM_BLOCK_MAX_SIZE, 0 );
  if(NRF_SUCCESS != err_code){
    tim_log("dm flash data update failed.\r\n");
    free(timer_msg);
    return PTL_ERROR_OPERATE;
  }
  /* ִ�й��ؼ�ע����� */
  MOUNT( timing_node, p_mount );
  timing_reg_id(p_in_node->id);
  
  return PTL_ERROR_NONE;
}

//
// ɾ��������һ���ڵ�
//
uint32_t timing_event_delete( struct timing_node_t *p )
{
  uint32_t err_code = NRF_SUCCESS;
  
	if( NULL == p || p->event.id > TIMING_EVENT_ID_MAX_NBR ){
		tim_log("ID anvalid. id = %02x\r\n", p->event.id);
		return PTL_ERROR_OUT_OF_EVT;
	}
	err_code = dm_flash_data_clear( p->event.id, DM_BLOCK_MAX_SIZE );
  if(NRF_SUCCESS != err_code){
    tim_log("dm flash data clear failed.\r\n");
    return PTL_ERROR_OPERATE;
  }
  UNMOUNT(timing_node, p);
  timing_delete_id(p->event.id);
  free(p);
  tim_log("dm flash data clear successfully.\r\n");
  
	return PTL_ERROR_NONE;
}


//
// ���Ķ�ʱ�¼���Ϣ
//
uint32_t timing_event_modify( struct timing_t * timer_new ){
	uint32_t err_code = NRF_SUCCESS;
  
  if( NULL == timer_new ){
		return PTL_ERROR_INVALID_PARAM;	
	}
	struct timing_node_t *timer_old = timing_seek_node(NULL, timer_new->id);
  if( NULL == timer_old ){
    return PTL_ERROR_NONE_EVT;
  }
  /* ׼��ˢ�¹��ؽڵ������ */
	struct timing_t *p_new = (struct timing_t*)malloc( TIMING_EVENT_PUT_SIZE );
  if(NULL == p_new){
    return PTL_ERROR_NONE_MEM;
  }
  memcpy( (uint8_t*)&(p_new->id), (uint8_t*)&(timer_new->id), TIMING_EVENT_PUT_SIZE );
  /* ׼��flash�Ĵ洢���� */
  struct timer_msg_t *timer_msg = (struct timer_msg_t *)malloc(TIMING_EVENT_FLASH_SIZE);
  if( NULL == timer_msg ){
    return PTL_ERROR_NONE_MEM;
  }
  memcpy( (uint8_t *)&(timer_msg->msg.id), (uint8_t*)&(timer_new->id), TIMING_EVENT_PUT_SIZE );
  timer_msg->ts = timing_calculate_event_ts(timer_new);
  timer_msg->crc16 = ble_flash_crc16_compute( (uint8_t*)&(timer_msg->msg.id), TIMING_EVENT_COMPUTE_SIZE, NULL );
	/* ����дflash���� */
  err_code = dm_flash_data_update( timer_msg->msg.id, (uint8_t *)&(timer_msg->msg.id), DM_BLOCK_MAX_SIZE, 0);
  if(NRF_SUCCESS != err_code){
    tim_log("dm flash data update failed.\r\n");
    return PTL_ERROR_OPERATE;
  }
  
  memcpy( (uint8_t*)&(timer_old->event.id), (uint8_t*)&(p_new->id), TIMING_EVENT_PUT_SIZE ); 
  tim_log("dm flash data update successfully.\r\n");
  free(p_new);
  
	return PTL_ERROR_NONE;
}

//
// ɨ�趨ʱ�¼��Ƿ񵽴�
//
void timing_event_scan_task(void)
{
	struct calendar_t calendar = app_calendar_handler_get();
	struct timing_node_t *node = timing_node;
  
  struct timer_msg_t *evt = (struct timer_msg_t *)malloc(DM_BLOCK_MAX_SIZE);
  if(NULL != evt){
    uint8_t size = (&(((struct timing_time_t *)0)->wday))-(&(((struct timing_time_t *)0)->hour));
    while( NULL != node ){
      if( node->event.enable ){	
        if( 0x80 == (node->event.time.wday & 0x80) ){ /* �����������ѭ��ִ���¼� */
//          tim_log("%p, %p\r\n", node, timing_node);
//          tim_log("calendar.wday=%d..node->event.time.wday=%02x..size=%02x\r\n", \
//            calendar.wday, node->event.time.wday, size);
//          tim_log("((node->event.time.wday&0x7F)&(1<<(calendar.wday-1))...%02x & %02x=%02x.\r\n", \
//            (uint8_t)(node->event.time.wday&0x7F), (uint8_t)(1<<(calendar.wday-1)), \
//              ((uint8_t)(node->event.time.wday&0x7F) & (uint8_t)(1<<(calendar.wday-1))));
//          tim_log_dump(&(calendar.hour), 4);
//          tim_log_dump(&(node->event.time.hour), 4);
          
          if( 0 != ( (uint8_t)(node->event.time.wday&0x7F) & (uint8_t)(1<<(calendar.wday-1)) ) ){
            if( 0 == memcmp( (uint8_t*)&(calendar.hour), (uint8_t*)&(node->event.time.hour), size ) ){
              timing_event( node->event.value );
            }
          }
        }else{ /* ����ִ���¼� */ 
          uint32_t sys_ts = app_calendar_get_timestamp();
          if(NRF_SUCCESS != dm_flash_data_load( node->event.id, (uint8_t *)&(evt->msg.id), DM_BLOCK_MAX_SIZE, 0 )){
            tim_log("timing_event_scan_task load failed.\r\n");
          }
//          tim_log("sys_ts = %08d, evt_ts = %08d...%02x..%02x\r\n", sys_ts, evt->ts, node->event.time.wday, node->event.time.wday & 0x80 );
          if(sys_ts == evt->ts){ 
            timing_event( node->event.value );
            node->event.enable = false;
            timing_event_modify(&(node->event));
//            if(NRF_SUCCESS != dm_flash_data_update( node->event.id, (uint8_t *)&(node->event.id), DM_BLOCK_MAX_SIZE, 0 )){
//              tim_log("timing_event_scan_task update failed.\r\n");
//            }
          }
        }
      }
      node = node->next;
    }
    free(evt);
  }
}



//
// ��ȡ��ʱ�¼��б�ͷ
//
struct timing_node_t * timing_get_event_list(void)
{
    return timing_node;
}


//
// ����ǵ���ִ�����������ִ������
// ��ǰ�趨ʱ��Ϊ��ȥʱ����ִ���¼��趨Ϊ��һ�����죬�����趨Ϊ��ǰ����
//
uint32_t timing_calculate_event_ts(struct timing_t *msg){
  if( 1 != (msg->time.wday & 0x80) ){
    struct calendar_t calendar = app_calendar_handler_get(); 
    struct gmt_t gmt = app_calendar_get_gmt();
    uint32_t ts_local = calendar.hour*3600UL + calendar.minute*60UL + calendar.second;
    uint32_t ts_creat = msg->time.hour*3600UL + msg->time.minute*60UL + msg->time.second;
    ptl_log("ts_local = %d ts_creat = %d.\r\n", ts_local , ts_creat);
		uint32_t past_days = app_calculate_interval_day( CALENDAR_BASE_YEAR, \
				                                             CALENDAR_BASE_MONTH, \
				                                             CALENDAR_BASE_DAY,  \
				                                             calendar.year + CALENDAR_BASE_YEAR, \
				                                             calendar.month, \
				                                             calendar.mday);
    if( ts_local >= ts_creat ){ // ��ȥʱ�� 
			past_days += 1;
    }
		uint32_t set_ts = past_days*86400UL + ts_creat - gmt.tz;
		return set_ts;
  }
	return 0;
}

//
// ���� flash ��ʱ�¼��б�
// ע�⣡������dmϵ�к�����size����һ��Ҫ��4�ı��������ǲ��ܳ����û�flash��С
//
static uint32_t timing_flash_data_update(void){
#if 0
	uint32_t err_code = NRF_SUCCESS;
	struct timing_node_t *p_tim_msg = timing_node;
  
  if( NULL == p_tim_msg ){
    return NRF_ERROR_INVALID_PARAM;
  }
  p_tim_msg = timing_node;
  while( (NULL != p_tim_msg) ){
    if( FLASH_OPERATE_OK != p_tim_msg->w_flash ){
      break;
    }else{
      p_tim_msg = p_tim_msg->next;
    }
  }
  if(NULL != p_tim_msg && p_tim_msg->event.id > 0){
    switch( p_tim_msg->w_flash ){ /* size����һ��Ҫ��4�ı��� */
      case FLASH_OPERATE_READY_WRITE: 
        err_code = dm_flash_data_update( p_tim_msg->event.id, \
                                        (uint8_t *) &(p_tim_msg->event), \
                                         DM_BLOCK_MAX_SIZE, \
                                         0);
        if(NRF_SUCCESS == err_code){
          p_tim_msg->w_flash = FLASH_OPERATE_OK;
          tim_log("dm flash data update successfully.\r\n");
        }else{
          tim_log("dm flash data update failed.\r\n");
        }
      break;
      case FLASH_OPERATE_READY_ERASE:
        err_code = dm_flash_data_clear( p_tim_msg->event.id, DM_BLOCK_MAX_SIZE );
        if(NRF_SUCCESS == err_code){
          UNMOUNT(timing_node, p_tim_msg);
          free(p_tim_msg);
          tim_log("dm flash data clear successfully.\r\n");
        }else{
          tim_log("dm flash data clear failed.\r\n");
        }
      break;
      case FLASH_OPERATE_OK:
      break;
      default:
        tim_log("w_flash error.0x%02x\r\n", p_tim_msg->w_flash);
      break;
    }
  }
#endif
  return NRF_SUCCESS;
}


//
// ����flash�еĶ�ʱ�¼��б�
//
void ble_flash_data_update_task(void){
  timing_flash_data_update();
}


//
// ����flash�еĶ�ʱ�¼��б�
// ϵͳ������ʱ�����
//
uint32_t timing_event_list_load( void ){
	uint8_t evt_cnt = 1;
//  uint8_t p_load[TIMING_EVENT_FLASH_SIZE];
	uint32_t err_code = NRF_SUCCESS;
  uint32_t pending_count = 0;
  
	do{
    pstorage_access_status_get(&pending_count);
    while(pending_count > 5);
    struct timer_msg_t *p_tim_load = (struct timer_msg_t *)malloc(TIMING_EVENT_FLASH_SIZE);
    if( NULL == p_tim_load ){		
			tim_log("node timing malloc error.\r\n");
			return NRF_ERROR_NO_MEM;
		}	
		err_code = dm_flash_data_load( evt_cnt, (uint8_t *)p_tim_load, DM_BLOCK_MAX_SIZE, 0 );
		if( NRF_SUCCESS != err_code ){
      free(p_tim_load);
      tim_log("flash data load error. = %d.\r\n", err_code);
			continue;
		}
//    nrf_delay_ms(40);
    tim_log_dump( (uint8_t*)&(p_tim_load->msg.id), TIMING_EVENT_FLASH_SIZE );
    if( DUMMY_BYTE == p_tim_load->msg.id ){
      free(p_tim_load);
			tim_log("load id is invalid.\r\n");
			continue;
		}
		uint16_t cal_crc16 = ble_flash_crc16_compute( (uint8_t *)&(p_tim_load->msg.id), TIMING_EVENT_COMPUTE_SIZE, NULL );
    if( cal_crc16 != p_tim_load->crc16 ){
			tim_log( "crc can't match... len = %d cal_crc16 = %04X load_crc16 = %04X.\r\n", TIMING_EVENT_COMPUTE_SIZE, cal_crc16, p_tim_load->crc16 );
      free( p_tim_load );
			continue;
		}
    struct timing_node_t *p_mount = (struct timing_node_t *)malloc( sizeof(struct timing_node_t) );
    if( NULL == p_mount){
      return NRF_ERROR_NO_MEM;
    }
    memcpy( (uint8_t*)&(p_mount->event.id), (uint8_t*)&(p_tim_load->msg.id), TIMING_EVENT_PUT_SIZE );
    MOUNT( timing_node, p_mount );
		timing_reg_id( p_mount->event.id );
    tim_log( "evt_cnt = %02d, read id = %02d.\r\n" , evt_cnt, p_mount->event.id );
	}while( (++evt_cnt) <= DM_BLOCK_MAX_NUMBER );
  tim_log("m_timing_event_id_list = 0x%08x\r\n", m_timing_event_id_list);
  return NRF_SUCCESS;
}


//
// ��ʱ��������ʼ��
// �����ݴ�falsh��ȡ���������µ�ϵͳ�Ķ�ʱ�¼�������
//
void timing_init(void)
{
  if( 0 != timing_event_list_load() ){
    tim_log("timing event list load failed.\r\n");
  }
}





/* end of app_timing.h ------------------------------------------------------ */ 
