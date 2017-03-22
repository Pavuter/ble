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
    AT_EVT_TEST,            // ��������
    AT_EVT_SET_MAC,         // ����6�ֽڳ��ȵ�MAC��ַ�������������״̬������ģ�鱻�޸�MAC��ַ����Ҫ�ӶϿ�֮��Ż����µ�MAC��ַ�㲥
    AT_EVT_GET_MAC,         // ��ȡMAC��ַ
    AT_EVT_SET_BAUD,
    AT_EVT_GET_BAUD,
    AT_EVT_SET_DEV_NAME,
    AT_EVT_GET_DEV_NAME,
    AT_EVT_GET_DEV_PARAM,
    AT_EVT_SET_RFPM,
    AT_EVT_GET_RFPM,
    AT_EVT_SET_RESET,
    AT_EVT_SET_DEFAULT,
    AT_EVT_GET_VERSION,     // ��ȡģ��汾��
    AT_EVT_SET_ADV_DATA,    // ��󳤶Ȳ�����16Bytes
    AT_EVT_SET_PID,         // ģ�����к�֧�� 0x[0000 ~ FFFF]
    AT_EVT_SET_ADV_ITV,     // ���ù㲥�������Ч��ΧΪ100ms~4000ms
    AT_EVT_SET_CIT,         // �������Ӽ������������Ч
    AT_EVT_SET_SLEEP        // ���������͹���˯��ģʽ
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




