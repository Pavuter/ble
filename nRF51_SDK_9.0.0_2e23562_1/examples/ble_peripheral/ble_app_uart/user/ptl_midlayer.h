#ifndef __UART_MIDLEVEL__H
#define __UART_MIDLEVEL__H


//#include "esp_common.h"

//#include "cJSON.h"

#include <stdbool.h>
#include <stdint.h>


#define DEVICE_MODEL_TABLE_NAME       "dev_model"

enum work_mode_e {
  INIT_WORK_MODE_UNDEFINED = 0,
  INIT_WORK_MODE_STATION,
  INIT_WORK_MODE_AP_CONFIG,
};

enum wifi_event_e {
  WIFI_STATE_UNDEFINED = 0,
  WIFI_STATE_DISCONNECTED,
  WIFI_STATE_OK,
};

enum switch_status_e{
	SWITCH_STATUS_DEFAULT = 0x00,
  SWITCH_STATUS_OPERATE = 0x10,
  SWITCH_STATUS_UPDATE  = 0x80
};

enum server_event_e {
  SERVER_STATE_UNDEFINED = 0,
  SERVER_STATE_DISCONNECTED,
  SERVER_STATE_CONNECTED,
};

enum sub_cmd_e{
	sub_cmd_00 = 0x0000,
	sub_cmd_01 = 0x0001,
	sub_cmd_02 = 0x0002,
	sub_cmd_03 = 0x0003,
	sub_cmd_04 = 0x0004,
	sub_cmd_05 = 0x0005,
	sub_cmd_06 = 0x0006,
};


enum fcmd_e{
	fcmd_00 = 0x00,
	fcmd_01,
	fcmd_02,
	fcmd_03,
	fcmd_04,
//  fcmd_10 = 0x10,
//  fcmd_11,
//  fcmd_12,
//  fcmd_13,
};

#pragma pack( push, 1 )
struct ble_switch_t {
//  uint16_t sub_cmd;
  uint8_t  point;
  uint8_t  mode;
  uint8_t  value;
};
#pragma pack(pop)

extern struct ble_switch_t  m_ble_switch_status;
extern uint8_t              ble_switch_status_changed;

bool module_init_work_mode_inform(enum work_mode_e mode);
bool module_wifi_connection_inform(enum wifi_event_e event);
bool module_server_connection_inform(enum server_event_e event);
bool data_comm_downlink(char *js_payload, bool ack_require, uint16_t *serial);
bool read_device_model_info(void);
void app_protocol_init(void);
void ble_device_heartbeat(void);
void ble_device_time_sync(void);
void ble_device_status_update_report_task(void);



#endif

