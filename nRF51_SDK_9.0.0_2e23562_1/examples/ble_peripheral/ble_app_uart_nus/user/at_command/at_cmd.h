#ifndef __AT_CMD_H
#define __AT_CMD_H

#if (APP_USE_AT_CMD_MOUDLE)

#include <stdint.h>
#include "queue.h"

#include "nrf_gpio.h"

#define AT_CMD_ENABLE_PIN           20
#define AT_QUEUE_MAX_SIZE           64

#define SYS_WORK_INDICATE_PIN       30

typedef enum _AT_MODE_
{
    MODE_AT_CMD,
    MODE_NORMAL
}at_mode_t;


typedef enum _AT_EVENT_
{
    AT_EVT_NONE,
    AT_EVT_TEST,            // 测试命令
    AT_EVT_SET_MAC,         // 设置6字节长度的MAC地址，如果处于连接状态的蓝牙模块被修改MAC地址，需要从断开之后才会以新的MAC地址广播
    AT_EVT_GET_MAC,         // 获取MAC地址
    AT_EVT_SET_BAUD,
    AT_EVT_GET_BAUD,
    AT_EVT_SET_DEV_NAME,
    AT_EVT_GET_DEV_NAME,
    AT_EVT_GET_DEV_PARAM,
    AT_EVT_SET_RFPM,
    AT_EVT_GET_RFPM,
    AT_EVT_SET_RESET,
    AT_EVT_SET_DEFAULT,
    AT_EVT_GET_VERSION,     // 获取模块版本号
    AT_EVT_SET_ADV_DATA,    // 最大长度不超过16Bytes
    AT_EVT_SET_PID,         // 模块序列号支持 0x[0000 ~ FFFF]
    AT_EVT_SET_ADV_ITV,     // 设置广播间隔，有效范围为100ms~4000ms
    AT_EVT_SET_CIT,         // 设置连接间隔，充重启生效
    AT_EVT_SET_SLEEP        // 设置蓝牙低功耗睡眠模式
}at_event_t;

typedef struct _AT_HANDLE_
{
    at_event_t event;
    uint8_t    *data_p;
    uint32_t   length;
}at_handle_t;

//
#define IS_WORK_IN_BRIDGE_MODE()  ((nrf_gpio_pin_read(SYS_WORK_INDICATE_PIN)&&0x01)?true:false)


//
// External params
//
extern queue_byte_t at_queue_t;

//
// Function
//
uint32_t at_cmd_init(void);
uint32_t at_cmd_paraise(uint8_t *rec_data, uint32_t len);

void hex_to_string(uint8_t *pbDest, uint8_t *pbSrc, int nLen);

#endif

#endif




