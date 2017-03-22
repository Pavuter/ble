#ifndef __UART_LOWLEVEL__H
#define __UART_LOWLEVEL__H


#include "esp_common.h"
#include "driver/uart.h"


typedef void*      xPayloadHandler;

void uart_lowlevel_preodic_recv(void);
void uart_lowlevel_preodic_send(void);

bool modify_transmit_interval(int32_t ms);


bool uart_lowlevel_build_packet(uint8_t  frame_cmd, 
								uint8_t *frame_serial,
								bool     ack_require,
								bool     ack_flag,
								uint8_t *payload, 
								uint16_t payload_length,
								void   (*ack_timeout_callback)(uint8_t serial));


bool cmd_callback_reg(uint8_t frame_cmd,
                      bool  (*callback)(xPayloadHandler h));

bool ack_callback_reg(
                      uint8_t frame_cmd,
                      bool  (*callback)(xPayloadHandler h)
                     );


void* lowlevel_get_payload(xPayloadHandler handler);
uint16_t lowlevel_get_payload_length(xPayloadHandler handler);
uint8_t lowlevel_get_serial(xPayloadHandler handler);
uint8_t lowlevel_get_cmd(xPayloadHandler handler);





#endif

