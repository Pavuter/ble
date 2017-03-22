
#if (APP_USE_AT_CMD_MOUDLE)

#include <stdio.h>
#include <stdint.h>
#include <string.h>
//#include <malloc.h>

#include "SEGGER_RTT.h"
#include "nrf_drv_gpiote.h"
#include "at_cmd.h"
#include "queue.h"
#include "app_uart.h"

static at_handle_t at_handle;
static at_mode_t   nrf_work_mode = MODE_NORMAL;
static uint8_t     at_rec_data[20];
static uint8_t     queue_buff[AT_QUEUE_MAX_SIZE];

queue_byte_t at_queue_t = {
    queue_buff,
    0,
    0,
    AT_QUEUE_MAX_SIZE,
    AT_QUEUE_MAX_SIZE
};



//
// AT指令集的初始化
//
uint32_t at_cmd_init(void)
{
    ret_code_t error_code = NRF_SUCCESS;

    at_handle.data_p = at_rec_data;
    at_handle.event  = AT_EVT_NONE;
    at_handle.length = 0;
    
    return error_code;
}

//
// AT指令集的数据包解析
//
uint32_t at_cmd_paraise(uint8_t *rec_data, uint32_t length)
{
    //static uint32_t tmp_cnt = 0;
    //uint8_t tmp_data[50];

    if(rec_data == NULL)
        return NRF_ERROR_NULL;
    if(length <= 0)
        return NRF_ERROR_DATA_SIZE;

    //hex_to_string(rec_data, tmp_data, len);
    //sprintf((char*)&tmp_data[0], "%s", rec_data);
    for (uint32_t i = 0; i < length; i++)
    {
        while(app_uart_put(rec_data[i]) != NRF_SUCCESS);
    }
    return 0;
}

void at_cmd_rsp(void)
{

}

//
// 检测模块当前的工作模式
// High 为正常工作模式，Low 为透传模式
//
at_mode_t at_cmd_mode_detect(uint32_t mode_pin)
{
    at_mode_t mode = MODE_AT_CMD;



    return mode;
}
#endif




