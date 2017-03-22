#include "nordic_common.h"
#include <time.h>


#include "ptl.h"
//#include "ptl_lowlayer.h"
//#include "ptl_midlayer.h"
#include "queue_mgr.h"
#include "TaskMgr.h"

#include "app_calendar.h"
#include "app_timing.h"
//#include "orvibo_common/database.h"

struct ack_payload_t {
  uint8_t *payload;
  int16_t payload_length;
};

enum module2mcu_cmd_e {
  CMD_UNDEFINED = 0,
  CMD_MODULE_STATE_INFORM = 0x02,
  CMD_DATA_COMM,
  CMD_OTA,

	CMD_LOCAL_TIME_SYNC = 0X02,
	
};


struct ble_switch_t m_ble_switch_status = {
//  .sub_cmd = sub_cmd_01,
//  .point   = 0,           // 默认节点为0
//  .mode    = 0,           // 默认模式为控制开关
//  .value   = 0            // 默认开关为关闭状态
//  sub_cmd_01,
  0,           // 默认节点为0
  0,           // 默认模式为控制开关
  0            // 默认开关为关闭状态
};

// 状态
uint8_t ble_switch_status_changed = true;

/**************** CMD : MCU -> MODULE -- START *****************/

bool cmd_device_info_setting(xPayloadHandler h);
bool cmd_device_infor_get( xPayloadHandler h );
bool cmd_module_control(xPayloadHandler h);
bool cmd_data_communication(xPayloadHandler h);
bool cmd_ota_operation(xPayloadHandler h);

bool module_heart_beat_sync(void);
void ble_device_status_update_report(void);        // 设备状态上传

//         ----------- sub cmd ------------
bool module_ctrl_set_local_time(    void *rx_msg, 
                                    int16_t length, 
                                    uint8_t serial, 
                                    struct ack_payload_t *ack_payload );

bool module_ctrl_device_control(    void *rx_msg, 
                                    int16_t length, 
                                    uint8_t serial, 
                                    struct ack_payload_t *ack_payload );

uint8_t module_ctrl_create_timer(   void *rx_msg, 
                                    int16_t length, 
                                    uint8_t serial, 
                                    struct ack_payload_t *ack_payload );

bool module_ctrl_modify_timer(      void *rx_msg, 
                                    int16_t length, 
                                    uint8_t serial, 
                                    struct ack_payload_t *ack_payload );

bool module_ctrl_delete_timer(      void *rx_msg, 
                                    int16_t length, 
                                    uint8_t serial, 
                                    struct ack_payload_t *ack_payload );
bool module_ctrl_inquire_timer(     void *rx_msg, 
                                    int16_t length, 
                                    uint8_t serial, 
                                    struct ack_payload_t *ack_payload );

bool device_model_setting(          void *rx_msg, 
                                    int16_t length, 
                                    uint8_t serial, 
                                    struct ack_payload_t *ack_payload );

bool module_infor_model_get(        void *rx_msg, 
                                    int16_t length, 
                                    uint8_t serial, 
                                    struct ack_payload_t *ack_payload );

bool mcu_factory_set_module(        void *rx_msg, 
                                    int16_t length, 
                                    uint8_t serial, 
                                    struct ack_payload_t *ack_payload );

bool mcu_set_module_work_mode(      void *rx_msg, 
                                    int16_t length, 
                                    uint8_t serial, 
                                    struct ack_payload_t *ack_payload );

bool mcu_modify_ble_transmit_interval(  void *rx_msg, 
                                        int16_t length, 
                                        uint8_t serial, 
                                        struct ack_payload_t *ack_payload );

bool mcu_data_comm_uplink(              void *rx_msg, 
                                        int16_t length, 
                                        uint8_t serial, 
                                        struct ack_payload_t *ack_payload );




/**************** CMD : MCU -> MODULE --  END  *****************/



/**************** ACK : MODULE -> MCU -- START *****************/

bool ack_module_status_inform(xPayloadHandler h);
bool ack_data_communication(xPayloadHandler h);
bool ack_ota_operation(xPayloadHandler h);

/**************** ACK : MODULE -> MCU --  END  *****************/
extern int32_t transmit_interval_ms;
bool modify_device_model_info(struct private_model_t *p);
bool modify_transmit_interval(uint32_t ms);

bool device_model_set_flag = false;

#define LENGTH_WITHOUT_PAYLOAD()    (sizeof(struct sub_payload_t) - sizeof(sub_payload->payload))


#pragma pack(push , 1)
struct sub_payload_t {
  uint16_t sub_cmd;
  uint8_t  payload;
};
#pragma pack(pop)


#define MID_LEVEL_RECV_FRAMEWORK( __CODE__ )      do{\
          struct sub_payload_t *sub_payload = ( struct sub_payload_t * )lowlayer_get_payload( h );\
          struct ack_payload_t ack_payload = {0x0};\
          \
          if( sub_payload != NULL ){\
            uint16_t sub_payload_length = lowlayer_get_payload_length(h) - LENGTH_WITHOUT_PAYLOAD();\
            if( sub_payload_length >= 0 ){\
              \
              __CODE__\
              \
              if( ack_payload.payload != NULL ){\
                struct sub_payload_t *ack = (struct sub_payload_t *)(ack_payload.payload);\
                ack->sub_cmd = sub_payload->sub_cmd;\
                uint16_t serial = lowlayer_get_serial( h );\
                \
                ble_lowlayer_build_packet(  lowlayer_get_cmd( h ), \
                                            &serial,\
                                            false,\
                                            true, \
                                            (uint8_t *)ack,\
                                            ack_payload.payload_length, \
                                            NULL );\
                free( ack_payload.payload );\
              }\
            }\
          }\
        }while(0)


              
/******************************************************************************

          --------------------------------------------------
          |                                                |
          |           Direction :  APP -> Module           |
          |                                                |
          --------------------------------------------------

******************************************************************************/

							
bool cmd_device_info_setting(xPayloadHandler h){
  MID_LEVEL_RECV_FRAMEWORK(
    switch( sub_payload->sub_cmd ){
      case 1 :  device_model_setting( &(sub_payload->payload),
                                      sub_payload_length,
                                      lowlayer_get_serial(h),
                                      &(ack_payload) );
                break;
      default : break;
    }
  );
  return true;
}

//
// sub_cmd = 0x01
//
bool cmd_device_infor_get( xPayloadHandler h ){
	MID_LEVEL_RECV_FRAMEWORK(
		ptl_log("cmd_device_infor_get --> cmd = 0x%02x\r\n", sub_payload->sub_cmd);
		switch( sub_payload->sub_cmd ){
			case 1:
				module_infor_model_get( &(sub_payload->payload),
			                            sub_payload_length,
			                            lowlayer_get_serial(h),
			                            &(ack_payload) );
			break;
			case 0x09:
				ptl_log("[cmd_device_infor_get] recive cmd = 0x09;\r\n");
			default:
				ptl_log("error!\r\n");
			break;
		}
	);
  return true;
}

//
// sub_cmd = 0x02
//
bool cmd_module_control( xPayloadHandler h ){
  MID_LEVEL_RECV_FRAMEWORK(
//    ptl_log( "sub_cmd = 0x%02x\r\n", sub_payload->sub_cmd );
//    ptl_log_dump( &sub_payload->payload, sub_payload_length+2 );
    switch( sub_payload->sub_cmd ){
      case sub_cmd_01 :				// APP设置本地时间
      module_ctrl_set_local_time( &(sub_payload->payload),
                                    sub_payload_length,
                                    lowlayer_get_serial(h),
                                    &(ack_payload) );
      break;
      case sub_cmd_02 :  
				module_ctrl_device_control( &(sub_payload->payload),
                                      sub_payload_length,
                                      lowlayer_get_serial(h),
                                      &(ack_payload) );
      break;
      case sub_cmd_03 :  
				module_ctrl_create_timer( 	&(sub_payload->payload),
                                  		sub_payload_length,
                                  		lowlayer_get_serial(h),
                                  	&(ack_payload) );
      break;
      case sub_cmd_04 :  
				module_ctrl_modify_timer( 	&(sub_payload->payload),
                                    	sub_payload_length,
                                    	lowlayer_get_serial(h),
                                    &(ack_payload) );
      break;
			case sub_cmd_05 :
				module_ctrl_delete_timer(		&(sub_payload->payload),
                                     sub_payload_length,
                                   	 lowlayer_get_serial(h),
                                    &(ack_payload) );
			break;
      case sub_cmd_06 :
        module_ctrl_inquire_timer( &(sub_payload->payload),
                                     sub_payload_length,
                                   	 lowlayer_get_serial(h),
                                    &(ack_payload) );
      break;
			default : 
			break;
    }
  );
  return true;
}


bool cmd_data_communication(xPayloadHandler h){
  MID_LEVEL_RECV_FRAMEWORK(
    switch( sub_payload->sub_cmd ){
      case 1 :  
      break;
      default : 
      break;
    }
  );
  return true;
}


bool device_model_setting(  void *rx_msg, 
                            int16_t length, 
                            uint8_t serial, 
                            struct ack_payload_t *ack_payload ){
  #pragma pack(push , 1)
  struct payload_t {
    char model_id[32];
    char software_version;
//    char hardware_version;
  } *payload = ( struct payload_t * )rx_msg;      // <! unfinish function

  struct ack_t {
    uint16_t sub_cmd;
    uint8_t  err_no;
  } *ack = NULL;
  #pragma pack(pop)

  //#define MIN(a,b)      (a<b)?a:b
  enum {
    PARSE_ALL_DONE,
    PARSE_MODEL_ID,
    PARSE_SW_VER,
    PARSE_HW_VER,
  } parse_state = PARSE_MODEL_ID;

  if( payload != NULL ){

    struct private_model_t *p = ( struct private_model_t * )malloc( sizeof( struct private_model_t ) );
    if( p == NULL ){ goto SEND_ACK; }
    memset( p, 0x00, sizeof( struct private_model_t ) );
    
    /********************** parse model id ***************************/
    
    if( length < sizeof( payload->model_id ) ){ goto SEND_ACK; }
    memcpy( ( p->model_id ), &( payload->model_id ), sizeof( payload->model_id ) / sizeof( char ) );
    length -= sizeof(payload->model_id);
    parse_state = PARSE_SW_VER;

    /********************** parse software version ***************************/

    if( length <= 0 ){ goto SEND_ACK; }
    uint8_t sw_len = MIN( strnlen( &( payload->software_version ), 40),
                          strnlen( &( payload->software_version ), length ) );
    if( ( sw_len >= 40 ) || ( sw_len <= 0 ) ){ goto SEND_ACK; }
    memcpy( ( p->sw_ver ), &( payload->software_version ), sw_len / sizeof( uint8_t ) );
    length -= sw_len + 1;
    parse_state = PARSE_HW_VER;

    /********************** parse hardware version ***************************/

    if( length <= 0 ){ goto SEND_ACK; }
    char *hareware_version = &( payload->software_version ) + sw_len + 1;
    uint8_t hw_len = MIN( strnlen(hareware_version, 40),
                          strnlen(hareware_version, length) );
    if( (hw_len >= 40) || (hw_len <= 0)  ){ goto SEND_ACK; }
    memcpy( (p->hw_ver), hareware_version, hw_len / sizeof( hw_len ) );
    length -= hw_len + 1;
    parse_state = PARSE_ALL_DONE;

    modify_device_model_info( p );
    free( p );
  }

SEND_ACK :

  ack = (struct ack_t *)malloc( sizeof(struct ack_t) );

  if( ack != NULL ){
    ack->err_no = parse_state;
  }
  ack_payload->payload        = (uint8_t *)ack;
  ack_payload->payload_length = sizeof( struct ack_t );
  
  return true;
}


bool module_infor_model_get( void *rx_msg, 
                             int16_t length, 
                             uint8_t serial, 
                             struct ack_payload_t *ack_payload ){
  #pragma pack(push , 1)
//  struct payload_t {
//    
//  } *payload = (struct payload_t *)rx_msg;

  struct ack_t {
    uint16_t sub_cmd;
    uint8_t  module_model;
  } *ack = NULL;
  #pragma pack(pop)

SEND_ACK :
  ack = (struct ack_t *)malloc( sizeof( struct ack_t ) );
  enum {
    UNDEFINED = 0,
    WIFI_ESP_8266,
    BLE_NRF51822,
  };

  if( ack != NULL ){
    ack->module_model = BLE_NRF51822;
  }
  ack_payload->payload        = (uint8_t *)ack;
  ack_payload->payload_length = sizeof(struct ack_t);
  
  return true;
}

//
// 时间戳同步
// fcmd     0x02
// sub_cmd  0x01
//
bool module_ctrl_set_local_time( void *rx_msg, 
                                 int16_t length, 
                                 uint8_t serial, 
                                 struct ack_payload_t *ack_payload ){
	uint8_t err_code = PTL_ERROR_NONE;
//  struct set_time_t{
//		uint32_t gmt_ts;			      // GMT时间戳，格林威治标准时间
//		uint32_t tz;					      // 时区-单位秒
//		uint32_t tz_dst;						// 时区-夏令时，单位秒
//		uint32_t tz_dst_start;			// 夏令时开始时间 GMT 时间戳
//		uint32_t tz_dst_stop;				// 夏令时结束时间 GMT 时间戳
//	}*p_time = (struct set_time_t *)rx_msg;
#pragma pack(push , 1)
	struct ack_msg_t{
		uint16_t sub_cmd;
		uint8_t  err_code;
	}*ack_msg = NULL;
#pragma pack(pop)  
  struct gmt_t *s_time = (struct gmt_t*)malloc( sizeof(struct gmt_t) );
  if(NULL != s_time){
    memcpy( (uint8_t*)s_time, (uint8_t*)rx_msg, sizeof(struct gmt_t) );
  }
  
//  ptl_log_dump( (uint8_t*)p_time, sizeof(struct set_time_t) );
//  ptl_log("ts = 0x%08x.\r\n", s_time->gmt_ts);
  err_code = app_calendar_set(s_time);
	if(NRF_SUCCESS != err_code){
		err_code = PTL_ERROR_NONE; // 操作失败
	}else{
		err_code = PTL_ERROR_OPERATE;
	}
	ack_msg = (struct ack_msg_t * )malloc( sizeof(struct ack_msg_t) );
	if( NULL != ack_msg ){
		ack_msg->err_code = err_code;
	}
	ack_payload->payload        = ( uint8_t * )ack_msg;
  ack_payload->payload_length = sizeof( struct ack_msg_t );
	free(s_time);
  
	return (bool)err_code;
}


//
// 设备控制指令
// fcmd     0x02
// sub_cmd  0x02
//
bool module_ctrl_device_control(  void *rx_msg, 
	                            int16_t length, 
	                            uint8_t serial, 
	                            struct ack_payload_t *ack_payload ){
	                            
	enum { MODE_SWITCH, MODE_LED_MESH };
	
#pragma pack(push , 1)
	struct dev_ctrl_t{
		uint8_t  point;
		uint8_t  mode;
		uint8_t  value;
	}*p_set = ( struct dev_ctrl_t * )rx_msg;

	struct ack_msg_t{
    uint16_t sub_cmd;
		uint8_t  err_code;
	}*ack_msg = NULL;
#pragma pack(pop)
  ptl_log("length = %02x, serial = %02x\r\n", length, serial);
  ptl_log_dump( (uint8_t*)rx_msg, length );
	uint8_t err_code = PTL_ERROR_NONE;
  ack_msg = ( struct ack_msg_t * )malloc( sizeof( struct ack_msg_t ) );	
  
  if( NULL != p_set ){
    if( MODE_SWITCH == p_set->mode ){
      m_ble_switch_status.value = p_set->value;
      ble_switch_status_changed = SWITCH_STATUS_OPERATE;   // 设置状态已经更新
      err_code = PTL_ERROR_NONE;
    }else if( MODE_LED_MESH== p_set->mode ){
      // add led control code...
      err_code = PTL_ERROR_NONE;
    }else{
      err_code = PTL_ERROR_NONE_MODE;
      ptl_log("mode error. mode = %02x.\r\n", p_set->mode);
    }
    
    if( NULL!= ack_msg ){
      ack_msg->err_code = err_code;
    }
  }
	ack_payload->payload = (uint8_t *)ack_msg;
	ack_payload->payload_length = sizeof( struct ack_msg_t );
  
	return true;
}


//
// 创建一个定时器
// fcmd     0x02
// sub_cmd  0x03
//
uint8_t module_ctrl_create_timer( void *rx_msg, 
                                 int16_t length, 
                                 uint8_t serial, 
                                 struct ack_payload_t *ack_payload ){
  uint32_t err_code = PTL_ERROR_NONE;
#pragma pack(push , 1)
  struct ack_msg_t {
    uint16_t sub_cmd;
    uint8_t err_code;
    uint8_t id;
  }*ack_msg = NULL;
#pragma pack(pop)
  struct timing_t *msg = NULL;
  if( NULL != rx_msg ){
    msg = (struct timing_t *)malloc( sizeof(struct timing_t) );
    if( NULL != msg ){
      memcpy( (uint8_t*)&(msg->id), (uint8_t *)rx_msg, TIMING_EVENT_PUT_SIZE );
      err_code = timing_event_reg(msg);
    }else{
      err_code = PTL_ERROR_NONE_MEM;
      ptl_log("malloc error.\r\n");
    }
  }
	ack_msg = (struct ack_msg_t*)malloc( sizeof(struct ack_msg_t) );
  if( NULL != ack_msg ){
    ack_msg->id = msg->id; 
    ack_msg->err_code = err_code;  
  }
  ack_payload->payload        = (uint8_t *)ack_msg;
  ack_payload->payload_length = sizeof( struct ack_msg_t );
  free(msg);
  
  return PTL_ERROR_NONE;
}


//
// 修改一个定时器内容
// fcmd     0x02
// sub_cmd  0x04
//
bool module_ctrl_modify_timer(   void *rx_msg, 
                                 int16_t length, 
                                 uint8_t serial, 
                                 struct ack_payload_t *ack_payload ){
	uint32_t err_code = PTL_ERROR_NONE;
#pragma pack(push , 1)
  struct ack_msg_t{
    uint16_t sub_cmd;
    uint8_t err_code;
  }*ack_msg = NULL;
#pragma pack(pop)
	
	struct timing_t *msg = (struct timing_t *)malloc(sizeof(struct timing_t));
  if(NULL != msg){
    memcpy((uint8_t*)&(msg->id), (uint8_t*)&(((struct timing_t*)rx_msg)->id), sizeof(struct timing_t));
    err_code = timing_event_modify(msg);
    if( PTL_ERROR_NONE == err_code ){
      ptl_log( "timing event modify successed.0x%02x\r\n", err_code );
    }else{
      ptl_log( "timing event modify failed.0x%02x\r\n", err_code );
    }   
  }else{
    err_code = PTL_ERROR_NONE_MEM;
  }
	ack_msg = (struct ack_msg_t*)malloc( sizeof(struct ack_msg_t) );
  if(NULL != ack_msg){
    ack_msg->err_code = err_code;
  }
  ack_payload->payload        = (uint8_t *)ack_msg;
  ack_payload->payload_length = sizeof(struct ack_msg_t); 
	free(msg);
//  ptl_log("ack_payload:");
//  ptl_log_dump(ack_payload->payload, ack_payload->payload_length);
	return true;
}


//
// 删除一个定时器
// fcmd     0x02
// sub_cmd  0x05
//
bool module_ctrl_delete_timer( void *rx_msg, 
                               int16_t length, 
                               uint8_t serial, 
                               struct ack_payload_t *ack_payload ){
  uint32_t err_code = PTL_ERROR_NONE;
  #pragma pack(push , 1)
  struct rx_msg_t{
    uint8_t id;
  };   
	
  struct ack_msg_t {
    uint16_t sub_cmd;
    uint8_t err_code;
  }*ack_msg = NULL;
	#pragma pack(pop)
  
  if(NULL != rx_msg){
    struct timing_node_t *p_seek = NULL;
    struct rx_msg_t *p_rx_msg = (struct rx_msg_t*)malloc( sizeof(struct rx_msg_t) );
    if(NULL != p_rx_msg){
      memcpy( p_rx_msg, rx_msg, sizeof(struct rx_msg_t) );
      p_seek = timing_seek_node(NULL, p_rx_msg->id);
      if( NULL != p_seek ){
        err_code = timing_event_delete( p_seek );
      }else{
        ptl_log("seek PTL_ERROR_NONE_EVT.\r\n");
        err_code = PTL_ERROR_NONE_EVT;
      }
      free(p_rx_msg);
    }else{
      err_code = PTL_ERROR_NONE_EVT;
    }
  }else{
      err_code = PTL_ERROR_NONE_EVT;
   }
  ack_msg = ( struct ack_msg_t *)malloc( sizeof( struct ack_msg_t ) );
  if( NULL != ack_msg ){
    ack_msg->err_code = err_code;
  }else{
    ptl_log("ack_msg malloc error.\r\n");
  }
  ack_payload->payload = ( uint8_t * )ack_msg;
  ack_payload->payload_length = sizeof( struct ack_msg_t );
  return true;
}

//
// 查询定时事件信息
// fcmd     0x02
// sub_cmd  0x06
//
bool module_ctrl_inquire_timer( void *rx_msg, 
                               int16_t length, 
                               uint8_t serial, 
                               struct ack_payload_t *ack_payload ){
  
  struct ack_msg_t{
    uint16_t sub_cmd;
    uint8_t err_code;
    uint8_t evt_cnt;
  };
  uint8_t err_code = PTL_ERROR_NONE;
  uint8_t read_node_nbr = 0, evt_infor_length = 0;
  uint8_t *evt_infor = NULL;
  uint8_t node_nbr = timing_get_event_nbr();
  
  ptl_log("read node_nbr = %d\r\n", node_nbr);
  if( node_nbr > 0 ){
    evt_infor_length = sizeof(struct timing_t) *node_nbr;
    evt_infor = (uint8_t *)malloc(evt_infor_length);    
    if( NULL != evt_infor ){
      memset( evt_infor, 0x00, evt_infor_length );
      read_node_nbr = timing_read_event_infor( evt_infor, node_nbr );
      ptl_log("read cnt actually = %d\r\n", read_node_nbr );
    }else{
      err_code = PTL_ERROR_OPERATE;
      ptl_log("malloc error.\r\n");
    }
  }
  if( read_node_nbr == node_nbr ){
    err_code = PTL_ERROR_NONE;
  }else{
    err_code = PTL_ERROR_OPERATE;
  }
  ptl_log("read data_1:");
  ptl_log_dump( evt_infor, evt_infor_length );
  
  uint8_t ack_size = sizeof(struct ack_msg_t);  
  struct ack_msg_t *ack_msg = (struct ack_msg_t *)malloc( ack_size + evt_infor_length );
  if( NULL != ack_msg ){
		ack_msg->err_code = err_code;
    ack_msg->evt_cnt  = read_node_nbr;
    memcpy( (ack_msg+1), evt_infor, evt_infor_length );
  }
    
  ack_payload->payload        = (uint8_t*)ack_msg;
  ack_payload->payload_length = ack_size + evt_infor_length;
  
  ptl_log("read data_2:");
  ptl_log_dump( (uint8_t*)ack_msg, evt_infor_length + ack_size );
  free(evt_infor);
  return true;                                  
}


//
//
//
bool mcu_factory_set_module(  void *rx_msg, 
                              int16_t length, 
                              uint8_t serial, 
                              struct ack_payload_t *ack_payload ){
  #pragma pack(push , 1)
//  struct payload_t {
//    
//  } *payload = (struct payload_t *)rx_msg;

  struct ack_t {
    uint16_t sub_cmd;
    uint8_t  err_no;
  } *ack = NULL;
  #pragma pack( pop )

  //bool ret = factory_set();
  bool ret = true;

SEND_ACK :
  
  ack = (struct ack_t *)malloc( sizeof(struct ack_t ) );

  enum { 
    OPERATION_SUCCESSED = 0,
    OPERATION_FAILED,
  };

  if( ack != NULL ){
    if( ret ){ ack->err_no = OPERATION_SUCCESSED; }
    else     { ack->err_no = OPERATION_FAILED;    }
  }
  ack_payload->payload        = (uint8_t *)ack;
  ack_payload->payload_length = sizeof(struct ack_t);
  
  return true;
}


bool mcu_set_module_work_mode(void *rx_msg, 
					                                  int16_t length, 
					                                  uint8_t serial, 
					                                  struct ack_payload_t *ack_payload ){
  #pragma pack(push , 1)
  struct payload_t {
    uint8_t work_mode;
  } *payload = (struct payload_t *)rx_msg;
  #pragma pack(pop)

  enum {
    WORK_MOVE_UNDEFINED = 0,
    WORK_MODE_AP_CONFIG,
    WORK_MODE_STATION,
  };

  switch( payload->work_mode ){
    case WORK_MODE_AP_CONFIG :  
      if( device_model_set_flag ){
        //ap_config();
      }
    break;
    case WORK_MODE_STATION   :  
	    //back_to_station();
	  break;
    default : 
		break;
  }
  return true;
}


bool mcu_modify_ble_transmit_interval(void *rx_msg, 
                                      int16_t length, 
                                      uint8_t serial, 
                                      struct ack_payload_t *ack_payload
                                      ){
  #pragma pack(push , 1)
  struct payload_t {
    uint16_t transmit_interval;
  } *payload = (struct payload_t *)rx_msg;
  
  struct ack_t {
    uint16_t sub_cmd;
    uint8_t  err_no;
  } *ack = NULL;
  #pragma pack(pop)
  
SEND_ACK :
  
  ack = ( struct ack_t * )malloc( sizeof( struct ack_t ) );

  enum { 
    OPERATION_SUCCESSED = 0,
    OPERATION_FAILED,
  };

  if( ack != NULL ){
    if( modify_transmit_interval( payload->transmit_interval ) ){
      ack->err_no = OPERATION_SUCCESSED;
    }else{
      ack->err_no = OPERATION_FAILED;
    }
  }
  ack_payload->payload        = (uint8_t *)ack;
  ack_payload->payload_length = sizeof(struct ack_t);
  
  return true;
}

bool mcu_data_comm_uplink(  void *rx_msg, 
                            int16_t length, 
                            uint8_t serial, 
                            struct ack_payload_t *ack_payload  ){
  #pragma pack(push , 1)
  struct payload_t {
    uint8_t  device_payload;
  } *payload = (struct payload_t *)rx_msg;
  
   struct ack_t {
    uint16_t sub_cmd;
    uint8_t  err_no;
  } *ack = NULL;
  #pragma pack(pop)

  uint8_t *dev_ptr = &(payload->device_payload);
  char *js_payload = (char *)malloc( length * 2 + 1 );
  char *js_ptr = js_payload;
  int16_t dev_len = length;

  while( dev_len > 0 ){
    sprintf( js_ptr , "%02x", *dev_ptr );
    dev_ptr++;   dev_len--;
    js_ptr += 2;
  }

  //js_build_data_uplink( js_payload );
  free( js_payload ); 

SEND_ACK :
  
  ack = (struct ack_t *)malloc( sizeof(struct ack_t ) );

  enum { 
    OPERATION_SUCCESSED = 0,
    NO_ENOUGH_RAM,
    SERVER_DISCONNECTED,
  };

  if( ack != NULL ){
    ack->err_no = OPERATION_SUCCESSED;
  }
  ack_payload->payload        = (uint8_t *)ack;
  ack_payload->payload_length = sizeof(struct ack_t);
  
  return true;
}



/******************************************************************************

          --------------------------------------------------
          |                                                |
          |           Direction :  Module -> MCU           |
          |                                                |
          --------------------------------------------------

******************************************************************************/

bool ack_module_status_inform(xPayloadHandler h){
  MID_LEVEL_RECV_FRAMEWORK(
    switch( sub_payload->sub_cmd ){
//      case 1 :  device_model_setting( &(sub_payload->payload),
//                                      sub_payload_length,
//                                      lowlayer_get_serial(h),
//                                      &(ack_payload)
//                                    );
//                break;
      default : break;
    }
  );
  return true;
}

bool ack_data_communication( xPayloadHandler h ){
  MID_LEVEL_RECV_FRAMEWORK(
    switch( sub_payload->sub_cmd ){
      case 2 :  
                break;
      case 3 :  
                //reply_ack_wait( lowlayer_get_serial(h), true );
                break;
      default : break;
    }
  );
  return true;
}


bool module_init_work_mode_inform(enum work_mode_e mode){
  #pragma pack(push , 1)
  struct tx_msg_t {
    uint16_t sub_cmd;
    uint8_t  init_mode;
  } msg, *tx_msg = &msg;
  #pragma pack(pop)

  tx_msg->sub_cmd = 0x01;
  tx_msg->init_mode = mode;

  ble_lowlayer_build_packet( CMD_MODULE_STATE_INFORM,
                              NULL,
                              true,
                              false,
                              (uint8_t *)tx_msg,
                              sizeof(struct tx_msg_t),
                              NULL );
  return true;
}


bool module_wifi_connection_inform( enum wifi_event_e event ){
  #pragma pack(push , 1)
  struct tx_msg_t {
    uint16_t sub_cmd;
    uint8_t  event;
  } msg, *tx_msg = &msg;
  #pragma pack(pop)

  static enum wifi_event_e local_event = WIFI_STATE_UNDEFINED;

  if( local_event == event ){ return true; }
  local_event = event;

  tx_msg->sub_cmd = 0x02;
  tx_msg->event = event;

  ble_lowlayer_build_packet( CMD_MODULE_STATE_INFORM,
                              NULL,
                              true,
                              false,
                              (uint8_t *)tx_msg,
                              sizeof(struct tx_msg_t),
                              NULL );
  return true;
}


bool module_server_connection_inform( enum server_event_e event ){
  #pragma pack(push , 1)
  struct tx_msg_t {
    uint16_t sub_cmd;
    uint8_t  event;
  } msg, *tx_msg = &msg;
  #pragma pack(pop)

  tx_msg->sub_cmd = 0x03;
  tx_msg->event = event;

  ble_lowlayer_build_packet( CMD_MODULE_STATE_INFORM,
                              NULL,
                              true,
                              false,
                              (uint8_t *)tx_msg,
                              sizeof(struct tx_msg_t),
                              NULL );
  return true;
}


bool module_local_time_sync(void){
  #pragma pack(push , 1)
  struct tx_msg_t {
    uint16_t sub_cmd;
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
  } msg, *tx_msg = &msg;
  #pragma pack(pop)

  uint32_t local_time = app_calendar_get_timestamp();
  if( local_time < 946684800UL ){ return false; }
  struct tm *tm = localtime( (time_t *)&local_time );   // <! unfinish function
  
  tx_msg->sub_cmd = sub_cmd_01;
  tx_msg->year    = tm->tm_year + 1900;
  tx_msg->month   = tm->tm_mon + 1;
  tx_msg->day     = tm->tm_mday;
  tx_msg->hour    = tm->tm_hour;
  tx_msg->minute  = tm->tm_min;
  tx_msg->second  = tm->tm_sec;

  ble_lowlayer_build_packet( fcmd_02, 
                              NULL,
                              false,
                              false,
                              (uint8_t *)tx_msg,
                              sizeof(struct tx_msg_t),
                              NULL );
  return true;
}


static void device_heart_beat_timeout(uint8_t serial){

            // <! unfinish function
  
}


void device_status_update_report_timeout( uint8_t serial ){
  // 
}

bool module_heart_beat_sync(void){
  #pragma pack(push , 1)
  struct tx_msg_t {
    uint16_t sub_cmd;
  } msg, *tx_msg = &msg;
  #pragma pack(pop)

  tx_msg->sub_cmd = sub_cmd_02;
//  ptl_log("heat payload len = %d.\r\n", sizeof(struct tx_msg_t));
  ble_lowlayer_build_packet( fcmd_03,
                              NULL,
                              true,
                              false,
                              (uint8_t *)tx_msg,
                              sizeof(struct tx_msg_t),
                              device_heart_beat_timeout );
  return true;
}

//
// 设备状态更新自动上报
// fcmd    0x02
// sub_cmd 0x01
//
bool module_status_update_report( void ){
  
  if( ble_switch_status_changed == SWITCH_STATUS_UPDATE ){
    ble_switch_status_changed = SWITCH_STATUS_DEFAULT;  
#pragma pack(push, 1)
    struct ack_msg_t{
      uint16_t sub_cmd;
      uint8_t  point;
      uint8_t  mode;
      uint8_t  value;
    }*ack_msg = NULL;
#pragma pack(pop)    
    ack_msg = (struct ack_msg_t *)malloc( sizeof(struct ack_msg_t) );
    memcpy( (uint8_t *)ack_msg+sizeof(ack_msg->sub_cmd), &m_ble_switch_status, sizeof(struct ble_switch_t) );
    ack_msg->sub_cmd = sub_cmd_01;
    ble_lowlayer_build_packet( fcmd_02,
                               NULL,
                               false, // 状态上报不需要对端设备应答
                               false,
                               (uint8_t *)ack_msg,
                               sizeof(struct ack_msg_t),
                               NULL );
    free(ack_msg);
  }
  
  return true;
}


static void data_comm_downlink_timeout(uint8_t serial){
  //reply_ack_wait(serial, false);
}


bool data_comm_downlink(char *js_payload, bool ack_require, uint16_t *serial){
  #pragma pack(push , 1)
  struct tx_msg_t {
    uint16_t sub_cmd;
  } *tx_msg = NULL;
  #pragma pack(pop)
  uint32_t val=0;
  int16_t length = strlen(js_payload) / 2;
  tx_msg = (struct tx_msg_t *)malloc( sizeof(struct tx_msg_t) + length );

  if( tx_msg != NULL ){

    tx_msg->sub_cmd = 0x03;
    
    uint8_t *data = (uint8_t *)(tx_msg + 1);
    int16_t cover_length = length * 2;
    while( cover_length > 0 ){    
      sscanf( js_payload , "%2x", &val );
      *data = (uint8_t)val;

      js_payload   += 2;   data++;
      cover_length -= 2;
    }
    

    bool ret = ble_lowlayer_build_packet(  CMD_DATA_COMM,
                                            serial,
                                            ack_require,
                                            false,
                                            (uint8_t *)tx_msg,
                                            sizeof(struct tx_msg_t) + length,
                                            data_comm_downlink_timeout );
    free( tx_msg );
    return ret;
  }
  
  return false;
}




/******************************************************************************

          --------------------------------------------------
          |                                                |
          |           local :  control function            |
          |                                                |
          --------------------------------------------------

******************************************************************************/


void ble_device_heartbeat(void){
  module_heart_beat_sync();
//  ptl_log("huicun.\r\m\n");
}


void ble_device_time_sync(void){
  module_local_time_sync();
}

void ble_device_status_update_report_task(void){
  module_status_update_report();
}


bool read_device_model_info(void){  
#if 0
  #define PAGE_SIZE     4096

  static bool init_flag = false;
  if( !init_flag ){
    if( DB_SUCCESS != db_param_init( 112*PAGE_SIZE , 10 * PAGE_SIZE , PAGE_SIZE ) ){
      FATAL_ERR( "database param init failed\n" );
    }
    if( DB_SUCCESS != db_create_table(DEVICE_MODEL_TABLE_NAME , 1 * PAGE_SIZE) ){
      FATAL_ERR( "create %s table failed\n", DEVICE_MODEL_TABLE_NAME );
    }
    init_flag = true;
  }

  bool set_flag = false;

  struct private_model_t *p = (struct private_model_t *)malloc( sizeof(struct private_model_t) );
  if( p == NULL ){ return false; }
  memset( p, 0x0, sizeof(struct private_model_t) );
  
  xDataBaseHandler h = db_open_table(DEVICE_MODEL_TABLE_NAME);  // <! basic routine , this must be done
  if( h != NULL ){
    struct db_record_t *r = db_get_first_record(h);
    while( r != NULL ){
      if( cJSON_GetObjectItem(r->js, "custom_device") != NULL ){

        if( (cJSON_GetObjectItem(r->js, "model_id") != NULL)          &&
            (cJSON_GetObjectItem(r->js, "sw_ver") != NULL)            &&
            (cJSON_GetObjectItem(r->js, "hw_ver") != NULL)            &&
            (cJSON_GetObjectItem(r->js, "transmit_interval") != NULL)   ){

          if( (strcmp( cJSON_GetObjectItem(r->js, "model_id")->valuestring, "" ) != 0x0) &&
              (strcmp( cJSON_GetObjectItem(r->js, "sw_ver")->valuestring, "" ) != 0x0)     &&
              (strcmp( cJSON_GetObjectItem(r->js, "hw_ver")->valuestring, "" ) != 0x0)       ){

            strncpy( (p->model_id), 
                     cJSON_GetObjectItem(r->js, "model_id")->valuestring, 
                     sizeof(p->model_id) - 1 
                   );
            strncpy( (p->sw_ver), 
                     cJSON_GetObjectItem(r->js, "sw_ver")->valuestring, 
                     sizeof(p->sw_ver) - 1 
                   );
            strncpy( (p->hw_ver), 
                     cJSON_GetObjectItem(r->js, "hw_ver")->valuestring, 
                     sizeof(p->hw_ver) - 1 
                   );
            set_private_device_model( p );
            device_model_set_flag = true;
            ptl_log( "got modelID:%s, sw_ver:%s, hw_ver:%s\n", p->model_id, p->sw_ver, p->hw_ver );
          }
          transmit_interval_ms = (cJSON_GetObjectItem(r->js, "transmit_interval")->valueint);
          ptl_log( "got transmit_interval:%dms\n", transmit_interval_ms );
          set_flag = true;
                    
        }
        break;
      }
      r = db_get_next_record(h);
    }
    if( DB_SUCCESS != db_close_table(h) ){          // <! basic routine , this must be done !!!!!
      FATAL_ERR( "close table failed\n" );
    }
  }

  if( ! set_flag ){ free( p ); }

  return set_flag;
#endif
  return true;
}


bool modify_device_model_info(struct private_model_t *p){
  if( p == NULL ){ return false; }
#if 0
  bool set_flag = false;
  enum DB_Status ret = DB_FAILED;
  xDataBaseHandler h = db_open_table(DEVICE_MODEL_TABLE_NAME);
  if( h != NULL ){

    struct db_record_t *r = db_get_first_record(h);
    while( r != NULL ){
      if( cJSON_GetObjectItem(r->js, "custom_device") != NULL ){
        if( (cJSON_GetObjectItem(r->js, "model_id") != NULL)    &&
            (cJSON_GetObjectItem(r->js, "sw_ver") != NULL)      &&
            (cJSON_GetObjectItem(r->js, "hw_ver") != NULL)      && 
            (cJSON_GetObjectItem(r->js, "transmit_interval") != NULL) ){

          if( (strcmp( cJSON_GetObjectItem(r->js, "model_id")->valuestring, p->model_id ) == 0x0) &&
              (strcmp( cJSON_GetObjectItem(r->js, "sw_ver")->valuestring, p->sw_ver ) == 0x0)     &&
              (strcmp( cJSON_GetObjectItem(r->js, "hw_ver")->valuestring, p->hw_ver ) == 0x0)     &&
              (transmit_interval_ms = (cJSON_GetObjectItem(r->js, "transmit_interval")->valueint))){

            goto CLOSE_TABLE;                                  // <! same info, no need to update
          }
        }
        db_delete_current_record( h );                          // <! delete all the record
      }
      r = db_get_next_record(h);
    }
    
    r = (struct db_record_t *)db_malloc_record(NEW_TYPE_JSON);  // <! basic routine
    if( r != NULL ){                              
      cJSON_AddBoolToObject  ( r->js, "custom_device",      true                );
      cJSON_AddStringToObject( r->js, "model_id",           p->model_id         );
      cJSON_AddStringToObject( r->js, "sw_ver",             p->sw_ver           );
      cJSON_AddStringToObject( r->js, "hw_ver",             p->hw_ver           );
      cJSON_AddNumberToObject( r->js, "transmit_interval",  transmit_interval_ms);

      ret = db_add_record(h, r);    // <! add record to database
      
      db_free_record(r);

      set_flag = true;
      ptl_log( "set modelID:%s, sw_ver:%s, hw_ver:%s, transmit_interval:%dms\n", p->model_id, p->sw_ver, p->hw_ver, transmit_interval_ms );
    }

CLOSE_TABLE:
  
    if( DB_SUCCESS != db_close_table(h) ){ // <! basic routine , this must be done !!!!!
      FATAL_ERR( "close table failed\n" );
    }
  }

  if( set_flag && (DB_SUCCESS == ret) ){
    return read_device_model_info();
  }

  return (DB_SUCCESS == ret)?true:false;
#endif
  return true;
}


static bool ble_midlayer_init( void ){

  /* fcmd 0x01 设备信息获取 */
  cmd_callback_reg( sub_cmd_01, cmd_device_infor_get );
  
  /* fcmd 0x02 控制设备 */
  cmd_callback_reg( sub_cmd_02, cmd_module_control );    
  
  /* fcmd 0x03 数据通信命令 */
  cmd_callback_reg( sub_cmd_03, cmd_data_communication );
 
  /*  */
//  ack_callback_reg( 0x02, ack_module_status_inform );
//  ack_callback_reg( 0x03, ack_data_communication );
  
	ptl_log( "ble mid level init done\n" );

  return true;
}

void app_protocol_init(void){
  
  ble_lowlayer_init();
	ble_midlayer_init();
}




// --------------------  debug purpose code -------------------------- //

/*
bool cmd_device_info_setting(xPayloadHandler h){
  #pragma pack(push , 1)
  struct sub_payload_t {
    uint16_t sub_cmd;
    uint8_t  payload;
  } *sub_payload = (struct sub_payload_t *)lowlayer_get_payload(h);
  struct ack_payload_t ack_payload = {0x0};
  #pragma pack(pop)

  #define LENGTH_WITHOUT_PAYLOAD()    (sizeof(struct sub_payload_t) - sizeof(sub_payload->payload))

  if( sub_payload != NULL ){
    uint16_t sub_payload_length = lowlayer_get_payload_length(h);
    if( sub_payload_length >= LENGTH_WITHOUT_PAYLOAD() ){

      switch( sub_payload->sub_cmd ){
        case 1 :  device_model_setting( &(sub_payload->payload),
                                        sub_payload_length - LENGTH_WITHOUT_PAYLOAD(),
                                        lowlayer_get_serial(h),
                                        &(ack_payload)
                                      );
                  break;
        default : break;

      }

      if( ack_payload.payload != NULL ){
        struct sub_payload_t *ack = (struct sub_payload_t *)(ack_payload.payload);
        ack->sub_cmd = sub_payload->sub_cmd;
        uint8_t serial = lowlayer_get_serial(h);

        ble_lowlayer_build_packet(
                                    lowlayer_get_cmd(h), 
                                    &serial,
                                    false,
                                    true,
                                    (uint8_t *)ack,
                                    ack_payload.payload_length
                                   );
        free( ack_payload.payload );
      }
    }
  }
}

*/

