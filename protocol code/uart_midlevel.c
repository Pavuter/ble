#include <stdio.h>

#include "uart_lowlevel.h"
#include "uart_midlevel.h"
#include "driver/uart_fsm.h"
#include "assist/queue_mgr.h"
#include "assist/db_print.h"
#include "assist/c_time.h"
#include "assist/TaskMgr.h"

#include "orvibo_common/database.h"


#include "protocol.h"


struct ack_payload_t {
  uint8_t *payload;
  int16_t payload_length;
};


/**************** CMD : MCU -> MODULE -- START *****************/

bool cmd_device_info_setting(xPayloadHandler h);
bool cmd_module_control(xPayloadHandler h);
bool cmd_data_communication(xPayloadHandler h);
bool cmd_ota_operation(xPayloadHandler h);

//         ----------- sub cmd ------------
bool device_model_setting(
                                    void *rx_msg, 
                                    int16_t length, 
                                    uint8_t serial, 
                                    struct ack_payload_t *ack_payload
                                  );
bool mcu_get_module_model(
                                    void *rx_msg, 
                                    int16_t length, 
                                    uint8_t serial, 
                                    struct ack_payload_t *ack_payload
                                  );

bool mcu_factory_set_module(
                                    void *rx_msg, 
                                    int16_t length, 
                                    uint8_t serial, 
                                    struct ack_payload_t *ack_payload
                                  );
bool mcu_set_module_work_mode(
                                    void *rx_msg, 
                                    int16_t length, 
                                    uint8_t serial, 
                                    struct ack_payload_t *ack_payload
                                  );
bool mcu_modify_uart_transmit_interval(
                                    void *rx_msg, 
                                    int16_t length, 
                                    uint8_t serial, 
                                    struct ack_payload_t *ack_payload
                                  );
bool mcu_data_comm_uplink(
                                    void *rx_msg, 
                                    int16_t length, 
                                    uint8_t serial, 
                                    struct ack_payload_t *ack_payload
                                  );




/**************** CMD : MCU -> MODULE --  END  *****************/



/**************** ACK : MODULE -> MCU -- START *****************/

bool ack_module_status_inform(xPayloadHandler h);
bool ack_data_communication(xPayloadHandler h);
bool ack_ota_operation(xPayloadHandler h);

/**************** ACK : MODULE -> MCU --  END  *****************/


bool modify_device_model_info(struct private_model_t *p);



bool device_model_set_flag = false;


bool uart_mid_level_init(void){

  cmd_callback_reg( 0x1, cmd_device_info_setting );
  cmd_callback_reg( 0x2, cmd_module_control );
  cmd_callback_reg( 0x3, cmd_data_communication );
  //cmd_callback_reg( 0x4, cmd_ota_operation );

  ack_callback_reg( 0x2, ack_module_status_inform );
  ack_callback_reg( 0x3, ack_data_communication );
  //ack_callback_reg( 0x4, ack_ota_operation );

  DB_PRINT( "uart mid level init done\n" );
  return true;
}


#define LENGTH_WITHOUT_PAYLOAD()    (sizeof(struct sub_payload_t) - sizeof(sub_payload->payload))


#pragma pack(push , 1)
struct sub_payload_t {
  uint16_t sub_cmd;
  uint8_t  payload;
};
#pragma pack(pop)


#define MID_LEVEL_RECV_FRAMEWORK( __CODE__ )      do{\
          struct sub_payload_t *sub_payload = (struct sub_payload_t *)lowlevel_get_payload(h);\
          struct ack_payload_t ack_payload = {0x0};\
          \
          if( sub_payload != NULL ){\
            int16_t sub_payload_length = lowlevel_get_payload_length(h) - LENGTH_WITHOUT_PAYLOAD();\
            DB_PRINT( "sub payload len: %d\n", sub_payload_length );\
            if( sub_payload_length >= 0 ){\
              \
              __CODE__\
              \
              if( ack_payload.payload != NULL ){\
                struct sub_payload_t *ack = (struct sub_payload_t *)(ack_payload.payload);\
                ack->sub_cmd = sub_payload->sub_cmd;\
                uint8_t serial = lowlevel_get_serial(h);\
                \
                uart_lowlevel_build_packet(\
                                            lowlevel_get_cmd(h), \
                                            &serial,\
                                            false,\
                                            true, \
                                            (uint8_t *)ack,\
                                            ack_payload.payload_length, \
                                            NULL\
                                           );\
                free( ack_payload.payload );\
              }\
            }\
          }\
        }while(0)


              
/******************************************************************************

          --------------------------------------------------
          |                                                |
          |           Direction :  MCU -> Module           |
          |                                                |
          --------------------------------------------------

******************************************************************************/


bool cmd_device_info_setting(xPayloadHandler h){
  MID_LEVEL_RECV_FRAMEWORK(
    switch( sub_payload->sub_cmd ){
      case 1 :  device_model_setting( &(sub_payload->payload),
                                      sub_payload_length,
                                      lowlevel_get_serial(h),
                                      &(ack_payload)
                                    );
                break;
      default : break;
    }
  );
}

bool cmd_module_control(xPayloadHandler h){
  MID_LEVEL_RECV_FRAMEWORK(
    DB_PRINT( "sub cmd: 0x%02X\n", sub_payload->sub_cmd );
    switch( sub_payload->sub_cmd ){
      case 1 :  mcu_get_module_model( &(sub_payload->payload),
                                      sub_payload_length,
                                      lowlevel_get_serial(h),
                                      &(ack_payload)
                                    );
                break;
      case 2 :  mcu_factory_set_module( &(sub_payload->payload),
                                      sub_payload_length,
                                      lowlevel_get_serial(h),
                                      &(ack_payload)
                                    );
                break;
      case 3 :  mcu_set_module_work_mode( &(sub_payload->payload),
                                      sub_payload_length,
                                      lowlevel_get_serial(h),
                                      &(ack_payload)
                                    );
                break;
      case 4 :  mcu_modify_uart_transmit_interval( &(sub_payload->payload),
                                      sub_payload_length,
                                      lowlevel_get_serial(h),
                                      &(ack_payload)
                                    );
                break;
      default : break;
    }
  );
}

bool cmd_data_communication(xPayloadHandler h){
  MID_LEVEL_RECV_FRAMEWORK(
    switch( sub_payload->sub_cmd ){
      case 1 :  mcu_data_comm_uplink( &(sub_payload->payload),
                                      sub_payload_length,
                                      lowlevel_get_serial(h),
                                      &(ack_payload)
                                    );
                break;
      default : break;
    }
  );
}



bool device_model_setting(
                                  void *rx_msg, 
                                  int16_t length, 
                                  uint8_t serial, 
                                  struct ack_payload_t *ack_payload
                                  ){
  #pragma pack(push , 1)
  struct payload_t {
    char model_id[32];
    char software_version;
//    char hardware_version;
  } *payload = (struct payload_t *)rx_msg;      // <! unfinish function

  struct ack_t {
    uint16_t sub_cmd;
    uint8_t  err_no;
  } *ack = NULL;
  #pragma pack(pop)

  #define MIN(a,b)      (a<b)?a:b

  enum {
    PARSE_ALL_DONE,
    PARSE_MODEL_ID,
    PARSE_SW_VER,
    PARSE_HW_VER,
  } parse_state = PARSE_MODEL_ID;

  if( payload != NULL ){

    struct private_model_t *p = (struct private_model_t *)malloc(sizeof(struct private_model_t));
    if( p == NULL ){ goto SEND_ACK; }
    memset( p, 0x0, sizeof(struct private_model_t));
    
    /********************** parse model id ***************************/
    
    if( length < sizeof(payload->model_id) ){ goto SEND_ACK; }
    memcpy( (p->model_id), &(payload->model_id), sizeof(payload->model_id) );
    length -= sizeof(payload->model_id);
    parse_state = PARSE_SW_VER;

    /********************** parse software version ***************************/

    if( length <= 0 ){ goto SEND_ACK; }
    uint8_t sw_len = MIN( strnlen(&(payload->software_version), 40),
                          strnlen(&(payload->software_version), length)  );
    if( (sw_len >= 40) || (sw_len <= 0) ){ goto SEND_ACK; }
    memcpy( (p->sw_ver), &(payload->software_version), sw_len );
    length -= sw_len + 1;
    parse_state = PARSE_HW_VER;

    /********************** parse hardware version ***************************/

    if( length <= 0 ){ goto SEND_ACK; }
    char *hareware_version = &(payload->software_version) + sw_len + 1;
    uint8_t hw_len = MIN( strnlen(hareware_version, 40),
                          strnlen(hareware_version, length)  );
    if( (hw_len >= 40) || (hw_len <= 0)  ){ goto SEND_ACK; }
    memcpy( (p->hw_ver), hareware_version, hw_len );
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
  ack_payload->payload_length = sizeof(struct ack_t);
}

bool mcu_get_module_model(
                                  void *rx_msg, 
                                  int16_t length, 
                                  uint8_t serial, 
                                  struct ack_payload_t *ack_payload
                                  ){
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

  ack = (struct ack_t *)malloc( sizeof(struct ack_t ) );
  
  enum {
    UNDEFINED = 0,
    WIFI_ESP_8266,
  };

  if( ack != NULL ){
    ack->module_model = WIFI_ESP_8266;
  }
  ack_payload->payload        = (uint8_t *)ack;
  ack_payload->payload_length = sizeof(struct ack_t);
}


bool mcu_factory_set_module(
                                  void *rx_msg, 
                                  int16_t length, 
                                  uint8_t serial, 
                                  struct ack_payload_t *ack_payload
                                  ){
  #pragma pack(push , 1)
//  struct payload_t {
//    
//  } *payload = (struct payload_t *)rx_msg;

  struct ack_t {
    uint16_t sub_cmd;
    uint8_t  err_no;
  } *ack = NULL;
  #pragma pack(pop)

  bool ret = factory_set();

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
}


bool mcu_set_module_work_mode(
                                  void *rx_msg, 
                                  int16_t length, 
                                  uint8_t serial, 
                                  struct ack_payload_t *ack_payload
                                  ){
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
                                  ap_config();
                                }
                                break;
    case WORK_MODE_STATION   :  
                                back_to_station();
                                break;
    default : break;
  }

}

bool mcu_modify_uart_transmit_interval(
                                  void *rx_msg, 
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
  
  ack = (struct ack_t *)malloc( sizeof(struct ack_t ) );

  enum { 
    OPERATION_SUCCESSED = 0,
    OPERATION_FAILED,
  };

  if( ack != NULL ){
    DB_PRINT( "set transmit interval: %d\n", payload->transmit_interval );
    if( modify_transmit_interval(payload->transmit_interval) ){
      ack->err_no = OPERATION_SUCCESSED;
    }else{
      ack->err_no = OPERATION_FAILED;
    }
  }
  ack_payload->payload        = (uint8_t *)ack;
  ack_payload->payload_length = sizeof(struct ack_t);
    
}

bool mcu_data_comm_uplink(
                                  void *rx_msg, 
                                  int16_t length, 
                                  uint8_t serial, 
                                  struct ack_payload_t *ack_payload
                                  ){
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

  js_build_data_uplink( js_payload );
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
//                                      lowlevel_get_serial(h),
//                                      &(ack_payload)
//                                    );
//                break;
      default : break;
    }
  );
}

bool ack_data_communication(xPayloadHandler h){
  MID_LEVEL_RECV_FRAMEWORK(
    switch( sub_payload->sub_cmd ){
      case 2 :  
                break;
      case 3 :  
                reply_ack_wait( lowlevel_get_serial(h), true );
                break;
      default : break;
    }
  );
}



enum module2mcu_cmd_e {
  CMD_UNDEFINED = 0,
  CMD_MODULE_STATE_INFORM = 0x2,
  CMD_DATA_COMM,
  CMD_OTA,
};

bool module_init_work_mode_inform(enum work_mode_e mode){
  #pragma pack(push , 1)
  struct tx_msg_t {
    uint16_t sub_cmd;
    uint8_t  init_mode;
  } msg, *tx_msg = &msg;
  #pragma pack(pop)

  tx_msg->sub_cmd = 0x01;
  tx_msg->init_mode = mode;

  uart_lowlevel_build_packet( CMD_MODULE_STATE_INFORM,
                              NULL,
                              true,
                              false,
                              (uint8_t *)tx_msg,
                              sizeof(struct tx_msg_t),
                              NULL
                             );
  return true;
}


bool module_wifi_connection_inform(enum wifi_event_e event){
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

  uart_lowlevel_build_packet( CMD_MODULE_STATE_INFORM,
                              NULL,
                              true,
                              false,
                              (uint8_t *)tx_msg,
                              sizeof(struct tx_msg_t),
                              NULL
                             );
  return true;
}

bool module_server_connection_inform(enum server_event_e event){
  #pragma pack(push , 1)
  struct tx_msg_t {
    uint16_t sub_cmd;
    uint8_t  event;
  } msg, *tx_msg = &msg;
  #pragma pack(pop)

  tx_msg->sub_cmd = 0x03;
  tx_msg->event = event;

  uart_lowlevel_build_packet( CMD_MODULE_STATE_INFORM,
                              NULL,
                              true,
                              false,
                              (uint8_t *)tx_msg,
                              sizeof(struct tx_msg_t),
                              NULL
                             );
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

  uint32_t local_time = get_time();
  if( local_time < 946684800UL ){ return false; }
  struct tm *tm = localtime( (time_t *)&local_time );   // <! unfinish function
  
  tx_msg->sub_cmd = 0x01;
  tx_msg->year    = tm->tm_year + 1900;
  tx_msg->month   = tm->tm_mon + 1;
  tx_msg->day     = tm->tm_mday;
  tx_msg->hour    = tm->tm_hour;
  tx_msg->minute  = tm->tm_min;
  tx_msg->second  = tm->tm_sec;

  uart_lowlevel_build_packet( CMD_DATA_COMM, 
                              NULL,
                              false,
                              false,
                              (uint8_t *)tx_msg,
                              sizeof(struct tx_msg_t),
                              NULL
                             );
  return true;
}


static void device_heart_beat_timeout(uint8_t serial){

            // <! unfinish function
  
}

bool module_heart_beat_sync(void){
  #pragma pack(push , 1)
  struct tx_msg_t {
    uint16_t sub_cmd;
  } msg, *tx_msg = &msg;
  #pragma pack(pop)

  tx_msg->sub_cmd = 0x02;

  uart_lowlevel_build_packet( CMD_DATA_COMM,
                              NULL,
                              true,
                              false,
                              (uint8_t *)tx_msg,
                              sizeof(struct tx_msg_t),
                              device_heart_beat_timeout
                             );
  return true;
}


static void data_comm_downlink_timeout(uint8_t serial){
  reply_ack_wait(serial, false);
}

bool data_comm_downlink(char *js_payload, bool ack_require, uint8_t *serial){
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
    

    bool ret = uart_lowlevel_build_packet( CMD_DATA_COMM,
                                serial,
                                ack_require,
                                false,
                                (uint8_t *)tx_msg,
                                sizeof(struct tx_msg_t) + length,
                                data_comm_downlink_timeout
                              );
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

void uart_device_heartbeat(void){
  module_heart_beat_sync();
}

void uart_device_time_sync(void){
  module_local_time_sync();
}


bool read_device_model_info(void){
  bool set_flag = false;

  struct private_model_t *p = (struct private_model_t *)malloc( sizeof(struct private_model_t) );
  if( p == NULL ){ return false; }
  memset( p, 0x0, sizeof(struct private_model_t) );
  
  xDataBaseHandler h = db_open_table(DEVICE_MODEL_TABLE_NAME);  // <! basic routine , this must be done
  if( h != NULL ){
    struct db_record_t *r = db_get_first_record(h);
    while( r != NULL ){
      if( cJSON_GetObjectItem(r->js, "custom_device") != NULL ){

        if( (cJSON_GetObjectItem(r->js, "model_id") != NULL)    &&
            (cJSON_GetObjectItem(r->js, "sw_ver") != NULL) &&
            (cJSON_GetObjectItem(r->js, "hw_ver") != NULL)   ){

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
          set_flag = true;
          device_model_set_flag = true;

          DB_PRINT( "got modelID:%s, sw_ver:%s, hw_ver:%s\n", p->model_id, p->sw_ver, p->hw_ver );
          
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
}


bool modify_device_model_info(struct private_model_t *p){
  if( p == NULL ){ return false; }

  bool set_flag = false;
  enum DB_Status ret = DB_FAILED;
  xDataBaseHandler h = db_open_table(DEVICE_MODEL_TABLE_NAME);
  if( h != NULL ){

    struct db_record_t *r = db_get_first_record(h);
    while( r != NULL ){
      if( cJSON_GetObjectItem(r->js, "custom_device") != NULL ){
        if( (cJSON_GetObjectItem(r->js, "model_id") != NULL)    &&
            (cJSON_GetObjectItem(r->js, "sw_ver") != NULL) &&
            (cJSON_GetObjectItem(r->js, "hw_ver") != NULL)   ){

          if( (strcmp( cJSON_GetObjectItem(r->js, "model_id")->valuestring, p->model_id ) == 0x0)
            && (strcmp( cJSON_GetObjectItem(r->js, "sw_ver")->valuestring, p->sw_ver ) == 0x0)
            && (strcmp( cJSON_GetObjectItem(r->js, "hw_ver")->valuestring, p->hw_ver ) == 0x0) ){

            goto CLOSE_TABLE;                                  // <! same info, no need to update
          }
        }
        db_delete_current_record( h );                          // <! delete all the record
      }
      r = db_get_next_record(h);
    }
    
    r = (struct db_record_t *)db_malloc_record(NEW_TYPE_JSON);  // <! basic routine
    if( r != NULL ){
      cJSON_AddBoolToObject  ( r->js, "custom_device",  true        );
      cJSON_AddStringToObject( r->js, "model_id",       p->model_id );
      cJSON_AddStringToObject( r->js, "sw_ver",         p->sw_ver   );
      cJSON_AddStringToObject( r->js, "hw_ver",         p->hw_ver   );

      ret = db_add_record(h, r);    // <! add record to database
      
      db_free_record(r);

      set_flag = true;
      DB_PRINT( "set modelID:%s, sw_ver:%s, hw_ver:%s\n", p->model_id, p->sw_ver, p->hw_ver );
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
}



bool uart_lowlevel_init(void){

  uart_mid_level_init();
  
  AddTask_TmrQueue(   200,    50, uart_lowlevel_preodic_recv );
  AddTask_TmrQueue(   200,    50, uart_lowlevel_preodic_send );
  
  AddTask_TmrQueue( 60000, 60000, uart_device_heartbeat      );
  AddTask_TmrQueue( 60000, 60000, uart_device_time_sync      );

  return true;
}




// --------------------  debug purpose code -------------------------- //

/*
bool cmd_device_info_setting(xPayloadHandler h){
  #pragma pack(push , 1)
  struct sub_payload_t {
    uint16_t sub_cmd;
    uint8_t  payload;
  } *sub_payload = (struct sub_payload_t *)lowlevel_get_payload(h);
  struct ack_payload_t ack_payload = {0x0};
  #pragma pack(pop)

  #define LENGTH_WITHOUT_PAYLOAD()    (sizeof(struct sub_payload_t) - sizeof(sub_payload->payload))

  if( sub_payload != NULL ){
    uint16_t sub_payload_length = lowlevel_get_payload_length(h);
    if( sub_payload_length >= LENGTH_WITHOUT_PAYLOAD() ){

      switch( sub_payload->sub_cmd ){
        case 1 :  device_model_setting( &(sub_payload->payload),
                                        sub_payload_length - LENGTH_WITHOUT_PAYLOAD(),
                                        lowlevel_get_serial(h),
                                        &(ack_payload)
                                      );
                  break;
        default : break;

      }

      if( ack_payload.payload != NULL ){
        struct sub_payload_t *ack = (struct sub_payload_t *)(ack_payload.payload);
        ack->sub_cmd = sub_payload->sub_cmd;
        uint8_t serial = lowlevel_get_serial(h);

        uart_lowlevel_build_packet(
                                    lowlevel_get_cmd(h), 
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

