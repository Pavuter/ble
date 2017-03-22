#include "nordic_common.h"
#include "nrf_delay.h"
#include "ble_nus.h"
#include "app_error.h"
#include "app_timer.h"
#include "app_trace.h"

#include "ptl.h"
#include "ptl_lowlayer.h"

#include "queue_mgr.h"
#include "TaskMgr.h"

/****************** Test case **************************

char *_test_case [] = {

  { 0x55, 0xAA, 0x89, 0x13 },   // <! length err
  { 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA },  // <! continous head
  {  }, // <! chksum error
  {  }, // <! incomplete packet at first
  {  }, // <! incomplete packet at the end
  {  }, // <! packet overflow 300byte buffer
  {  }, // <! continous sticky packet

  // <! continous sync pack sending.
  // <! continous async pack sending.
  // <! continous sync & async pack sending.




****************** Test case **************************/

#define BLE_LOWLAYER_PREODIC_TIME    (50)

#pragma pack(push , 1)

#define BLE_LOWLEVEL_PROTOCOL_HEAD       (0xAA55)

typedef uint8_t         chksum_t;

struct ble_lowlayer_protocol_t {
  uint16_t frame_head;
  uint16_t frame_length;
  uint16_t frame_serial;
  
  uint8_t  frame_cmd:6;
  uint8_t  frame_cmd_ack_flag:1;
  uint8_t  frame_cmd_ack_require:1;

  uint8_t  payload;
//uint8_t  chksum;
};

#pragma pack(pop)


#define DEFAULT_MALLOC_LENGTH         (100)

struct recv_ctrl_t {
  uint8_t *recv_buff;
  uint16_t recv_len;
  uint16_t malloc_length;
};

struct send_ctrl_t {
  struct send_ctrl_t *next;
  struct ble_lowlayer_protocol_t *pack;
  void   (*ack_timeout_callback)(uint8_t serial);
  
  int16_t retry_ttl;
  int8_t  retry_cnt;
};


struct ble_data_t{
  struct ble_data_t *next;
  uint8_t *pdata;
  uint16_t length;
};
static struct ble_data_t  *m_llp_recive = NULL, *m_llp_send = NULL;
static struct send_ctrl_t *send_ctrl_queue = NULL;

static uint32_t m_ble_tx_counter = 0, m_ble_rx_counter = 0;
app_timer_id_t m_ble_communicate_timer_id;

/**
	Statement
	*/
static uint16_t parse_protocol(void *msg, uint16_t length);
static void parse_frame_cmd(struct ble_lowlayer_protocol_t *p);

/**
	Function 
	*/


void llp_tx_timer_cnt_set( uint32_t ticks ){
	m_ble_tx_counter = ticks;
}

uint32_t llp_tx_timer_cnt_get( void ){
	return m_ble_tx_counter;
}

void llp_tx_timer_cnt_increase( void ){
	m_ble_tx_counter++;
}

void llp_rx_timer_cnt_set( uint32_t ticks ){
	m_ble_rx_counter = ticks;
}

uint32_t llp_rx_timer_cnt_get( void ){
	return m_ble_rx_counter;
}

void llp_rx_timer_cnt_increase( void ){
	m_ble_rx_counter++;
}



static bool recv_buff_resize( struct recv_ctrl_t *recv_ctrl, 
                              uint16_t loose_length,
                              uint16_t append_length){
  uint8_t *buff = NULL;                            
  
  if( recv_ctrl == NULL ){ return false; }
  if( recv_ctrl->recv_buff == NULL ){ return false; }

  uint16_t new_malloc_length = recv_ctrl->malloc_length - loose_length + append_length;
  new_malloc_length += DEFAULT_MALLOC_LENGTH - ( new_malloc_length % DEFAULT_MALLOC_LENGTH );
  ptl_log( "new_malloc_len = %d - %d + %d = %d -> %d\r\n", 
            recv_ctrl->malloc_length, loose_length, append_length,
            (recv_ctrl->malloc_length - loose_length + append_length),
            new_malloc_length );

  if( loose_length > 0 ){
    append_length = 0;
    
    if( loose_length >= recv_ctrl->recv_len ){
      
      goto ABANDON_BUFFER;
    }else{

      goto RESIZE_BUFFER;
    }
    
  }else if( append_length > 0 ){
    loose_length = 0;

    goto RESIZE_BUFFER;
  }

RESIZE_BUFFER :

  ptl_log( "resize buffer\r\n" );
  buff = (uint8_t *)malloc( new_malloc_length );
  if( buff != NULL ){             // <! transfer remain data to new buffer mem

    ptl_log( "recv_len = %d - %d = %d\r\n", recv_ctrl->recv_len, loose_length,
                                         recv_ctrl->recv_len - loose_length );
    memcpy( buff, recv_ctrl->recv_buff + loose_length, recv_ctrl->recv_len - loose_length );
    recv_ctrl->recv_len -= loose_length;
    free( recv_ctrl->recv_buff );

    recv_ctrl->malloc_length = new_malloc_length;
    recv_ctrl->recv_buff = buff;
  }else{                          // <! malloc failed

    goto ABANDON_BUFFER;
  }

  return true;

ABANDON_BUFFER :

  ptl_log( "abandon buffer\r\n" );
  free( recv_ctrl->recv_buff );
  recv_ctrl->recv_buff = (uint8_t *)malloc( DEFAULT_MALLOC_LENGTH );

  recv_ctrl->malloc_length = DEFAULT_MALLOC_LENGTH;
  recv_ctrl->recv_len = 0;

  return true;

}

struct ble_data_t **llp_get_recive_header( void ){
  return &m_llp_recive;
}

struct ble_data_t **llp_get_send_header( void ){
  return &m_llp_send;
}

void ble_communicate_timeout_callback( void *p_context ){
	UNUSED_PARAMETER(p_context);
	llp_tx_timer_cnt_increase();
	llp_rx_timer_cnt_increase();
}


void ble_lowlayer_init( void ){
//	uint32_t err_code = NRF_SUCCESS;
#if 0
  struct ble_data_t p_set = {
    .next   = NULL,
    .length = 0,
    .pdata  = NULL
  };
  m_llp_recive = ( struct ble_data_t * )malloc( sizeof( struct ble_data_t ) );
  memcpy( m_llp_recive, &p_set, sizeof( struct ble_data_t ) );

	m_llp_send = ( struct ble_data_t * )malloc( sizeof( struct ble_data_t ) );
  memcpy( m_llp_send, &p_set, sizeof( struct ble_data_t ) );
#endif
//	err_code = app_timer_create( 	&m_ble_communicate_timer_id, \
//																APP_TIMER_MODE_SINGLE_SHOT, \
//																ble_communicate_timeout_callback );
//	APP_ERROR_CHECK(err_code);

	/* lowlayer protocol tx and rx timer init */
//	llp_tx_timer_cnt_set(0);
//	llp_rx_timer_cnt_set(0);

}


uint8_t llp_add_node( struct ble_data_t **node_t, uint8_t *data, uint16_t length ){
  struct ble_data_t *p = ( struct ble_data_t * )malloc( ( sizeof( struct ble_data_t ) ) );
	
  if( NULL != p ){
		memset( p, 0, sizeof(struct ble_data_t) );
	  p->length = length;
	  p->pdata = ( uint8_t * )malloc( p->length * sizeof(uint8_t) );

	  if( NULL != p->pdata ){
	    memcpy(p->pdata, data, p->length);
	    MOUNT(*node_t, p);
      return 0;
	  }else{
	    ptl_log("node malloc error.\r\n");
			free( p );
	    return 1;
	  }
  }
  return 2;
}


//
//
//
uint32_t ble_recive_add_node( uint8_t *data, uint16_t length ){
  llp_add_node(&m_llp_recive, data, length);
}

    
uint16_t ble_read_list_withe_delete( struct ble_data_t ** node_t, uint8_t *rd_data, uint16_t read_byte_length ){
  uint16_t byte_cnt = 0;
  struct ble_data_t * p = NULL;
  
  while( NULL != ( p = *node_t ) ){
    if( byte_cnt < read_byte_length ){
      //ptl_log("brefore free heap = %d\r\n", __SYS_REMAIN_HEAP_SIZE_ );
      uint16_t single_read_length = GET_SMALLER_VALUE( p->length, (read_byte_length - byte_cnt) );
      memcpy( rd_data + byte_cnt, p->pdata, sizeof( uint8_t ) * single_read_length ); /* 拷贝节点数据 */
      byte_cnt += single_read_length;

			/* 当前节点数据还有剩余，但是指定长度的字节已经读取完毕， */
      if(single_read_length < p->length){ 
        memset( p->pdata, 0, sizeof( uint8_t ) * single_read_length );
				
				/* 更新当前内存块字符长度计数器 */ 
        p->length -= single_read_length; 

				/* 将缓存区中未读取的字节移动到当前节点缓存区首地址 */
        memcpy( p->pdata, p->pdata+single_read_length, p->length ); 
        break;
      }else{
        UNMOUNT( *node_t, p ); // 删除节点
        free( p->pdata );
        free(p);
        //ptl_log("after free heap = %d\r\n", __SYS_REMAIN_HEAP_SIZE_ );
      }
    }
    else{
      break; 
    }
  }
  return byte_cnt;
}


void ble_send_task( void ){
	extern ble_nus_t 		 m_nus;
	uint32_t 						 err_code = NRF_SUCCESS;
	struct ble_data_t *p = NULL;
	
  if( NULL != ( p = m_llp_send ) ){
		if( m_nus.is_notification_enabled ){ // 对端设备连接并使能通知功能
			err_code = ble_nus_string_send(&m_nus, p->pdata, p->length);
			if ( err_code != NRF_SUCCESS ){
				ptl_log("*** error ***\r\n");
				ptl_log("ble send error.code = %d, length = %d addr = 0x%p 0x%p\r\n", err_code, p->length, p->pdata, &m_nus);
				ptl_log("*************\r\n");
//				APP_ERROR_CHECK(err_code);
			}else{
        ptl_log( "ble send ok." );
        ptl_log_dump(p->pdata, p->length);
      }
    }
		else{
      ptl_log( "ack data:" );
      ptl_log_dump(p->pdata, p->length);
    	ptl_log("peer device has disabled notification or disconnected.\r\n");
      
    }
    UNMOUNT( m_llp_send, p );
    free( p->pdata );
    free( p );
  }
}

				
void llp_show_list(struct ble_data_t  *llp){
  uint16_t index = 0, node_nbr = 0;
  struct ble_data_t *p_rec = NULL;
  
  p_rec = llp;
	ptl_log("\r\n\r\n\r\n");
  while( NULL != p_rec ){
    ptl_log( "node_%03d, %02d: ", node_nbr, p_rec->length );
    uint8_t *p_node_data = NULL;
    p_node_data = p_rec->pdata;
    for( index = 0; index < p_rec->length; index++ ){
      ptl_log( "%02X ", *(p_node_data++) );
      nrf_delay_ms(2);
    }  
    ptl_log("\r\n\r\n\r\n");
    node_nbr++;
    p_rec = p_rec->next;
  }
}


void ble_lowlayer_preodic_recv(void){
  static struct recv_ctrl_t recv_ctrl;
  static uint16_t  void_data_cnt= 0;
  static enum {
    STATE_INIT,
    STATE_RECV,
  } state = STATE_INIT;
	
  switch( state ){
    case STATE_INIT :
      if( recv_ctrl.recv_buff == NULL ){
        recv_ctrl.malloc_length = DEFAULT_MALLOC_LENGTH;
        recv_ctrl.recv_buff = (uint8_t *)malloc(recv_ctrl.malloc_length);
        if( recv_ctrl.recv_buff == NULL ){ break; } // 创建失败
        ptl_log( "recv buff init done\r\n" );
      }
      state = STATE_RECV;
          
    case STATE_RECV :
      if( recv_ctrl.recv_buff != NULL ){
        uint16_t delta_len = 0;
        delta_len = ble_read_list_withe_delete( llp_get_recive_header(), \
                                 recv_ctrl.recv_buff + recv_ctrl.recv_len, \
                                 recv_ctrl.malloc_length - recv_ctrl.recv_len );
        if( delta_len > 0 ){
          recv_ctrl.recv_len += delta_len;
          void_data_cnt = 0;
          
          ptl_log( "recv new: %d, total: %d\r\n", delta_len, recv_ctrl.recv_len );
          ptl_log_dump( recv_ctrl.recv_buff, delta_len );
          
          uint16_t parsed_length = parse_protocol( recv_ctrl.recv_buff, recv_ctrl.recv_len );
          if( parsed_length > 0 ){

            ptl_log( "parsed: %d, shink buff\r\n", parsed_length );
            recv_buff_resize( &recv_ctrl, parsed_length, 0 );
          }else{    // <! could not parse anything.. maybe packet ain't complete
          
            if( recv_ctrl.malloc_length <= recv_ctrl.recv_len ){// <! buffer is full , expension is needed
              ptl_log( "buff is full, appending...\r\n" );
              recv_buff_resize( &recv_ctrl, 0, DEFAULT_MALLOC_LENGTH );
            }
            ptl_log( "pack not complete\r\n" );
          }
        }else{          // <! no new data is received
          if( recv_ctrl.recv_len == 0 ){      // <! never recv anything
                                              // <! wait for next time in
          }else{
          	LOG_STRING(recv_ctrl.recv_buff, recv_ctrl.recv_len);
            void_data_cnt += BLE_LOWLAYER_PREODIC_TIME;
            ptl_log( "data break for %d ms\r\n", void_data_cnt );
            if( void_data_cnt >= 100 ){       // <! data already broke
              ptl_log( "data break time too long\r\n" );
              // <! to do
              while( recv_ctrl.recv_len > 0 ){    // <! parse all data immediately.
                uint16_t parsed_length = parse_protocol( recv_ctrl.recv_buff, recv_ctrl.recv_len );

                if( parsed_length <= 0 ){
                  parsed_length = sizeof( ((struct ble_lowlayer_protocol_t *)0)->frame_head );
                }
                recv_buff_resize( &recv_ctrl, parsed_length, 0 );
              }
            }
          }
        }
      }
			else{
      state = STATE_INIT;
    }
    break;
    default : state = STATE_INIT; 
		break;
  }

  return;

}

static chksum_t chksum(void *data, uint32_t length){
  uint8_t *d = (uint8_t *)data;
  chksum_t sum = 0;

  while( length > 0 ){
    sum += *d++;
    length--;
  }
  
  return sum;
}


static uint16_t parse_protocol(void *msg, uint16_t length){
  uint16_t parsed_length = 0;

  struct ble_lowlayer_protocol_t *p = (struct ble_lowlayer_protocol_t *)msg;

  //ptl_log( "len: %d\r\n", length );
  
  if( msg == NULL ){ return 0; }

  while( (length - parsed_length) > (sizeof(p->frame_head) + sizeof(p->frame_length)) ){

    if( p->frame_head == BLE_LOWLEVEL_PROTOCOL_HEAD ){
      ptl_log( "head found @%d, frame_length: %04X\r\n", parsed_length , p->frame_length );
      if( p->frame_length <= 5000 ){
        if( (p->frame_length <= (length - parsed_length) ) && \
            (p->frame_length >= sizeof(struct ble_lowlayer_protocol_t)) ){
          chksum_t __chksum = *(chksum_t *)((uint8_t *)&(p->frame_head) + p->frame_length - sizeof(chksum_t));
          chksum_t local_chksum = chksum(&(p->frame_head), p->frame_length - sizeof(chksum_t)) ^ 0x33;
          if( __chksum == local_chksum ){
            parse_frame_cmd( p );
            parsed_length += p->frame_length;
          }else{
            ptl_log( "chksum err, pack: 0x%02X, local: 0x%02X\r\n", __chksum, local_chksum );
            parsed_length++;                // <! chksum err, parse next !!
          }
        }else{
          ptl_log( "pack incomplete\r\n" );
          break;
        }
      }else{
        parsed_length++;                    // <! length too large, parse next !!
        ptl_log( "frame length err\r\n" );
      }
    }else{
      parsed_length++;                      // <! frame head err, parse next !!
      ptl_log( "head error...\r\n" );
    }
    p = (struct ble_lowlayer_protocol_t *)( (uint8_t *)msg + parsed_length );
  }

  ptl_log( "parsed_length : %d\r\n", parsed_length );
  return parsed_length;
}

// -------------------- frame sending control -------------------- //

#define BLE_SEND_MAX_RETRY_CNT     (5)

static int32_t transmit_interval_ms = 0;

bool modify_transmit_interval(uint32_t ms){
  transmit_interval_ms = ms;
	
  return true;
}


void ble_lowlayer_preodic_send(void){
  struct send_ctrl_t *p = NULL;
  bool async_only_enable = false;
	
  //if( llp_tx_timer_cnt_get() < transmit_interval_ms ){ return; }  // <! frame interval control
  p = send_ctrl_queue;
  while( NULL != p ){
    if( async_only_enable ){
      if( p->pack->frame_cmd_ack_require ){
        p = p->next; 
        continue;
      }
    }
    p->retry_ttl -= BLE_LOWLAYER_PREODIC_TIME;
    if( p->retry_ttl <= 0 ){
      uint8_t already_send_length = 0, frame_length = p->pack->frame_length;
      uint8_t pkt_length = 0;
			/* 往发送链表填充一整个数据包 */
			#if 1
//      ptl_log("read form send_ctrl_queue:");
//      ptl_log_dump(&(p->pack->payload), p->pack->frame_length-8);
      while( already_send_length < frame_length ){
        pkt_length = GET_SMALLER_VALUE( BLE_NUS_MAX_DATA_LEN, frame_length - already_send_length );
				uint8_t *p_little_pkt = ( uint8_t * )malloc( pkt_length / sizeof(uint8_t) );
				if( NULL != p_little_pkt ){
					memcpy( p_little_pkt, (uint8_t *)( (uint8_t *)&(p->pack->frame_head)+already_send_length), pkt_length );
					if ( 0 != llp_add_node( &m_llp_send, p_little_pkt, pkt_length )){
						ptl_log("malloc error.\r\n");
						return;
					}
//					ptl_log_dump( p_little_pkt, pkt_length );
					already_send_length += pkt_length;
					free( p_little_pkt );
				}else{
					ptl_log("malloc error.\r\n");
					goto DROP_PACK; 
				}
      }
//      ptl_log("already = %d, pkt = %d\r\n", already_send_length, pkt_length );
//      llp_show_list(m_llp_send);
//      ptl_log( "send cmd:0x%02x serial:0x%04x len:0x%04x, retry:%d\r\n",
//                p->pack->frame_cmd,
//                p->pack->frame_serial,
//                p->pack->frame_length,
//                p->retry_cnt);
			#endif
      if( ! p->pack->frame_cmd_ack_require ){
        goto DROP_PACK;
      }else{
      
        if( --p->retry_cnt > 0 ){
          p->retry_ttl = 100;               // <! need to figure out a way to calculate
          break; // add 02.28
        }else{

          if( p->ack_timeout_callback ){    // <! ack timeout
            p->ack_timeout_callback( p->pack->frame_serial );
          }
          goto DROP_PACK;
        }
      }
    }
    async_only_enable = true;
    p = p->next;
  }
  return;

DROP_PACK :

  UNMOUNT( send_ctrl_queue, p );
  free( p->pack );
  free( p );
  return;
}


bool ble_lowlayer_build_packet( uint8_t  frame_cmd, 
                                uint16_t *frame_serial,
                                bool     ack_require,
                                bool     ack_flag,
                                uint8_t *payload, 
                                uint16_t payload_length,
                                void   (*ack_timeout_callback)(uint8_t serial) ){

  static uint8_t local_serial = 0;

  if( ack_require && ack_flag ){ return false; }
  if( ack_flag && (frame_serial == NULL) ){ return false; }
  if( ack_flag || (!ack_require) ){ ack_timeout_callback = NULL; }
  
  struct ble_lowlayer_protocol_t *p = (struct ble_lowlayer_protocol_t *)\
        malloc( sizeof(struct ble_lowlayer_protocol_t) + payload_length );
  
  if( p != NULL ){

    p->frame_head             = BLE_LOWLEVEL_PROTOCOL_HEAD;
    p->frame_length           = sizeof(struct ble_lowlayer_protocol_t) + payload_length;
    if( ack_flag ){
      p->frame_serial         = *frame_serial;      // <! copy rx serial if this is an ack.
    }else{
      p->frame_serial         = local_serial;       // <! create tx serial for each new packet
    }
    p->frame_cmd              = frame_cmd;
    p->frame_cmd_ack_flag     = ack_flag;
    p->frame_cmd_ack_require  = ack_require;

    memcpy( &(p->payload), payload, payload_length );

    chksum_t *__chksum = ((uint8_t *)p) + p->frame_length - sizeof(chksum_t);
    *__chksum = chksum( p, p->frame_length - sizeof(chksum_t) ) ^ 0x33;

    /******************* mount to send queue *******************/

    struct send_ctrl_t *s = (struct send_ctrl_t *)malloc( sizeof(struct send_ctrl_t) );
    if( s != NULL ){

      memset( s, 0x0, sizeof(struct send_ctrl_t) );

      s->pack = p;
      s->ack_timeout_callback = ack_timeout_callback;
      s->retry_cnt = BLE_SEND_MAX_RETRY_CNT;
      s->retry_ttl = 0;

      MOUNT( send_ctrl_queue, s );

//      ptl_log( "build frame cmd:0x%02X serial:0x%04X len:%d\r\n", frame_cmd, local_serial, p->frame_length );
//			ptl_log_dump(&s->pack->payload, s->pack->frame_length-8);
      /********************** log frame serial *******************/

      if( !ack_flag ){                                              // <! only work if this is not an ack.
        if( frame_serial != NULL ){ *frame_serial = local_serial; } // <! sometimes upper level need this serial
        local_serial++;
      }

      return true;
    }
    free( p );
  }
  ptl_log( "out of memory\r\n" );
  return false;
}


// -------------------- frame command callback ------------------- //

struct frame_cmd_callback_t {
  struct frame_cmd_callback_t *next;
  bool  (*callback)(xPayloadHandler h);
  uint8_t frame_cmd;
};

static struct frame_cmd_callback_t  *cmd_callback_queue = NULL,
                                    *ack_callback_queue = NULL;

static struct frame_cmd_callback_t  *callback_handler_query(struct frame_cmd_callback_t *queue, uint8_t frame_cmd)
{
  struct frame_cmd_callback_t *p = queue;
  
  while( NULL != p ){
    
    if( p->frame_cmd == frame_cmd ){
      break;
    }
    p = p->next;
  }
  return p;
}


static void parse_frame_cmd( struct ble_lowlayer_protocol_t *f ){

  if( f == NULL ){ return; }
  struct frame_cmd_callback_t *p = NULL;

  //ptl_log( "[parse_frame_cmd] recv cmd: 0x%02X\r\n", f->frame_cmd );

  if( f->frame_cmd_ack_flag ){
    
    // <! search if we really send a ack-require packet.
    
    struct send_ctrl_t *s = send_ctrl_queue;
    while( s != NULL ){
      if( s->retry_cnt < BLE_SEND_MAX_RETRY_CNT ){       // <! already sent
        if( ( s->pack->frame_cmd_ack_require ) &&           // <! frame is ack require
            ( s->pack->frame_cmd == f->frame_cmd ) &&       // <! frame cmd matching
            ( s->pack->frame_serial == f->frame_serial ) ){ // <! frame serial matching   
          p = callback_handler_query( ack_callback_queue, f->frame_cmd );
          ptl_log( "recv ack of frame %u\r\n", f->frame_serial );
          UNMOUNT( send_ctrl_queue, s );            // <! ack recved, drop from send queue
          free( s->pack );                          // <! release resource
          free( s );
          break;
        }
      }
      s = s->next;
    }
  }else{
    p = callback_handler_query( cmd_callback_queue, f->frame_cmd );
  }

  if( p != NULL ){
    if( p->callback != NULL ){
      ptl_log( "callback found\r\n" );
      p->callback( (xPayloadHandler) f );
    }
  }
}


bool cmd_callback_reg( 	uint8_t frame_cmd,
                      						bool  (*callback)(xPayloadHandler h) ){
  if( callback == NULL ){ return false; }
  
  struct frame_cmd_callback_t *p = callback_handler_query( cmd_callback_queue, frame_cmd );
  if( p == NULL ){
    p = (struct frame_cmd_callback_t *)malloc( sizeof(struct frame_cmd_callback_t) );
  }

  if( p != NULL ){
    memset( p, 0x0, sizeof(struct frame_cmd_callback_t) );

    p->callback = callback;
    p->frame_cmd = frame_cmd;

    MOUNT( cmd_callback_queue, p );

    ptl_log( "reg cmd:0x%02x callback\r\n", frame_cmd );
    return true;
  }
  
  return false;
}


bool ack_callback_reg(uint8_t frame_cmd,
                      bool  (*callback)(xPayloadHandler h)){
  if( callback == NULL ){ return false; }
  
  struct frame_cmd_callback_t *p = callback_handler_query( ack_callback_queue, frame_cmd );
  if( p == NULL ){
    p = (struct frame_cmd_callback_t *)malloc( sizeof(struct frame_cmd_callback_t) );
  }

  if( p != NULL ){
    memset( p, 0x0, sizeof(struct frame_cmd_callback_t) );

    p->callback = callback;
    p->frame_cmd = frame_cmd;

    MOUNT( ack_callback_queue, p );

    ptl_log( "reg ack cmd:%u callback\r\n", frame_cmd );
    return true;
  }
  
  return false;
}


void* lowlayer_get_payload(xPayloadHandler handler){
  if( handler != NULL ){
    struct ble_lowlayer_protocol_t *p = (struct ble_lowlayer_protocol_t *)handler;
    return &(p->payload);
  }
  return NULL;
}


uint16_t lowlayer_get_payload_length(xPayloadHandler handler){
  if( handler != NULL ){
    struct ble_lowlayer_protocol_t *p = ( struct ble_lowlayer_protocol_t * )handler;
//    ptl_log("p->frame_length = 0x%d  sizeof(struct ble_lowlayer_protocol_t) = 0x%02x",p->frame_length, sizeof(struct ble_lowlayer_protocol_t));
    return (p->frame_length - sizeof(struct ble_lowlayer_protocol_t) ); // 12 -8
  }
  return 0;
}


uint16_t lowlayer_get_serial(xPayloadHandler handler){
  if( handler != NULL ){
    struct ble_lowlayer_protocol_t *p = (struct ble_lowlayer_protocol_t *)handler;
    return (p->frame_serial);
  }
  return 0;
}


uint8_t lowlayer_get_cmd(xPayloadHandler handler){
  if( handler != NULL ){
    struct ble_lowlayer_protocol_t *p = (struct ble_lowlayer_protocol_t *)handler;
    return (p->frame_cmd);
  }
  return 0;
}


