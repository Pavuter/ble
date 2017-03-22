#ifndef __UART_MIDLEVEL__H
#define __UART_MIDLEVEL__H


#include "esp_common.h"

#include "cJSON.h"

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

enum server_event_e {
  SERVER_STATE_UNDEFINED = 0,
  SERVER_STATE_DISCONNECTED,
  SERVER_STATE_CONNECTED,
};


bool module_init_work_mode_inform(enum work_mode_e mode);
bool module_wifi_connection_inform(enum wifi_event_e event);
bool module_server_connection_inform(enum server_event_e event);

bool data_comm_downlink(char *js_payload, bool ack_require, uint8_t *serial);


bool read_device_model_info(void);


bool uart_lowlevel_init(void);




#endif

