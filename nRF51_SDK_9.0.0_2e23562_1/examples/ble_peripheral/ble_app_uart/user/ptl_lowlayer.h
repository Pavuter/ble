#ifndef __UART_LOWLEVEL__H
#define __UART_LOWLEVEL__H


//#include "esp_common.h"
//#include "driver/uart.h"
#include <stdbool.h>
#include <stdint.h>

typedef void*      xPayloadHandler;

__INLINE static void *zalloc( size_t length){
//	if(length < (__SYS_REMAIN_HEAP_SIZE_ - 200)){
	  void *p = (void * )malloc( length );
	  
	  if(NULL != p){
	    memset( p, 0, length );
	  }
		return p;
//	}
//	else{
//		LOG("zalloc error. __SYS_REMAIN_HEAP_SIZE_ is not enough.Remain size little than 200");
//		return NULL;
//	}
};


bool 									modify_transmit_interval(uint32_t ms);


bool 									ble_lowlayer_build_packet(	uint8_t  frame_cmd,\
                                                  uint16_t *frame_serial,\
                                                  bool     ack_require,\
                                                  bool     ack_flag,\
                                                  uint8_t *payload,\
                                                  uint16_t payload_length,\
                                                  void   (*ack_timeout_callback)(uint8_t serial) );


bool 									cmd_callback_reg(uint8_t frame_cmd, bool  (*callback)(xPayloadHandler h) );

bool 									ack_callback_reg(uint8_t frame_cmd, bool  (*callback)(xPayloadHandler h) );


void* 								lowlayer_get_payload( xPayloadHandler handler );
uint16_t 							lowlayer_get_payload_length( xPayloadHandler handler );
uint16_t 							lowlayer_get_serial( xPayloadHandler handler );
uint8_t 							lowlayer_get_cmd( xPayloadHandler handler );


struct ble_data_t   **llp_get_recive_header( void );
struct ble_data_t   **llp_get_send_header( void );
//uint8_t               llp_add_node( struct ble_data_t **node_t, uint8_t *data, uint16_t length );
uint32_t              ble_recive_add_node( uint8_t *data, uint16_t length );
void                  llp_show_list( struct ble_data_t  *llp );
void     							llp_tx_timer_cnt_set( uint32_t ticks );
uint32_t 							llp_tx_timer_cnt_get( void );
void 									llp_tx_timer_cnt_increase( void );
void     							llp_rx_timer_cnt_set( uint32_t ticks );
uint32_t 							llp_rx_timer_cnt_get( void );
void 									llp_rx_timer_cnt_increase( void );
uint16_t              ble_read_list_withe_delete( struct ble_data_t ** node_t, uint8_t *rd_data, uint16_t read_byte_length );
void 									ble_lowlayer_init( void );

void                  ble_lowlayer_preodic_recv(void);
void                  ble_lowlayer_preodic_send(void);
void                  ble_send_task( void );
#endif

