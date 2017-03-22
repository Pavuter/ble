#include "uart_lowlevel.h"
#include "driver/uart_fsm.h"
#include "assist/queue_mgr.h"
#include "assist/db_print.h"


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



#define UART_LOWLEVEL_PREODIC_TIME    (50)


#pragma pack(push , 1)

#define UART_LOWLEVEL_PROTOCOL_HEAD       (0xAA55)

typedef uint8_t         chksum_t;

struct uart_lowlevel_protocol_t {
  uint16_t frame_head;
  uint16_t frame_length;
  uint8_t  frame_serial;
  
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

static uint16_t parse_protocol(void *msg, uint16_t length);
static void parse_frame_cmd(struct uart_lowlevel_protocol_t *p);


static bool recv_buff_resize(struct recv_ctrl_t *recv_ctrl, 
                             uint16_t loose_length,
                             uint16_t append_length){
  if( recv_ctrl == NULL ){ return false; }
  if( recv_ctrl->recv_buff == NULL ){ return false; }

  uint16_t new_malloc_length = recv_ctrl->malloc_length - loose_length + append_length;
  new_malloc_length += DEFAULT_MALLOC_LENGTH - ( new_malloc_length % DEFAULT_MALLOC_LENGTH );
  DB_PRINT( "new_malloc_len = %d - %d + %d = %d -> %d\n", 
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

  DB_PRINT( "resize buffer\n" );
  uint8_t *buff = (uint8_t *)zalloc( new_malloc_length );
  if( buff != NULL ){             // <! transfer remain data to new buffer mem

    DB_PRINT( "recv_len = %d - %d = %d\n", recv_ctrl->recv_len, loose_length,
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

  DB_PRINT( "abandon buffer\n" );
  free( recv_ctrl->recv_buff );
  recv_ctrl->recv_buff = (uint8_t *)zalloc( DEFAULT_MALLOC_LENGTH );

  recv_ctrl->malloc_length = DEFAULT_MALLOC_LENGTH;
  recv_ctrl->recv_len = 0;

  return true;

}


void uart_lowlevel_preodic_recv(void){

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
            recv_ctrl.recv_buff = (uint8_t *)zalloc(recv_ctrl.malloc_length);
            if( recv_ctrl.recv_buff == NULL ){ break; }
            DB_PRINT( "recv buff init done\n" );
          }
          state = STATE_RECV;
          
    case STATE_RECV :
          if( recv_ctrl.recv_buff != NULL ){
            uint16_t delta_len = uart_recv( recv_ctrl.recv_buff + recv_ctrl.recv_len, 
                                            recv_ctrl.malloc_length - recv_ctrl.recv_len );
            if( delta_len > 0 ){
              recv_ctrl.recv_len += delta_len;
              void_data_cnt = 0;

              DB_PRINT( "recv new: %d, total: %d\n", delta_len, recv_ctrl.recv_len );
              uint16_t parsed_length = parse_protocol( recv_ctrl.recv_buff, recv_ctrl.recv_len );
              if( parsed_length > 0 ){

                DB_PRINT( "parsed: %d, shink buff\n", parsed_length );
                recv_buff_resize( &recv_ctrl, parsed_length, 0 );
              }else{    // <! could not parse anything.. maybe packet ain't complete
              
                if( recv_ctrl.malloc_length <= recv_ctrl.recv_len ){// <! buffer is full , expension is needed
                  DB_PRINT( "buff is full, appending\n" );
                  recv_buff_resize( &recv_ctrl, 0, DEFAULT_MALLOC_LENGTH );
                }
                DB_PRINT( "pack not complete\n" );
              }
              
            }else{          // <! no new data is received
            
              if( recv_ctrl.recv_len == 0 ){      // <! never recv anything
                                                  // <! wait for next time in
              }else{
                void_data_cnt += UART_LOWLEVEL_PREODIC_TIME;
                DB_PRINT( "data break for %d ms\n", void_data_cnt );
                
                if( void_data_cnt >= 100 ){       // <! data already broke

                  DB_PRINT( "data break time too long\n" );
                  // <! to do

                  while( recv_ctrl.recv_len > 0 ){    // <! parse all data immediately.
                    uint16_t parsed_length = parse_protocol( recv_ctrl.recv_buff, recv_ctrl.recv_len );

                    if( parsed_length <= 0 ){
                      parsed_length = sizeof( ((struct uart_lowlevel_protocol_t *)0)->frame_head );
                    }
                    recv_buff_resize( &recv_ctrl, parsed_length, 0 );
                  }
                }
              }
            }
          }else{
            state = STATE_INIT;
          }
          break;
          
    default : state = STATE_INIT; break;
          
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

  struct uart_lowlevel_protocol_t *p = (struct uart_lowlevel_protocol_t *)msg;

  DB_PRINT( "len: %d\n", length );
  
  if( msg == NULL ){ return 0; }

  while( (length - parsed_length) > (sizeof(p->frame_head) + sizeof(p->frame_length)) ){

    if( p->frame_head == UART_LOWLEVEL_PROTOCOL_HEAD ){
      DB_PRINT( "head found @%d, frame_length: %d\n", parsed_length , p->frame_length );
      if( p->frame_length <= 5000 ){
        if( (p->frame_length <= (length - parsed_length)) && \
		    (p->frame_length >= sizeof(struct uart_lowlevel_protocol_t)) ){

          chksum_t __chksum = *(chksum_t *)((uint8_t *)&(p->frame_head) + p->frame_length - sizeof(chksum_t));
          chksum_t local_chksum = chksum(&(p->frame_head), p->frame_length - sizeof(chksum_t)) ^ 0x33;
          if( __chksum == local_chksum ){

            DB_PRINT( "parse %d byte data\n", p->frame_length );

            parse_frame_cmd( p );
            parsed_length += p->frame_length;
            
          }else{
            DB_PRINT( "chksum err, pack: 0x%02X, local: 0x%02X\n", __chksum, local_chksum );
            parsed_length++;                // <! chksum err, parse next !!
          }
          
        }else{
          DB_PRINT( "pack incomplete\n" );
          break;
        }
      }else{
        parsed_length++;                    // <! length too large, parse next !!
        DB_PRINT( "frame length err\n" );
      }
    }else{
      parsed_length++;                      // <! frame head err, parse next !!
      DB_PRINT( "head err\n" );
    }
    p = (struct uart_lowlevel_protocol_t *)( (uint8_t *)msg + parsed_length );
  }

  DB_PRINT( "parsed_length : %d\n", parsed_length );
  return parsed_length;
}

// -------------------- frame sending control -------------------- //

#define UART_SEND_MAX_RETRY_CNT     (5)

static int32_t transmit_interval_ms = 0;

bool modify_transmit_interval(int32_t ms){
  transmit_interval_ms = ms;
  return true;
}

struct send_ctrl_t {
  struct send_ctrl_t *next;
  struct uart_lowlevel_protocol_t *pack;
  void   (*ack_timeout_callback)(uint8_t serial);
  
  int16_t retry_ttl;
  int8_t  retry_cnt;
};

static struct send_ctrl_t *send_ctrl_queue;

void uart_lowlevel_preodic_send(void){
  struct send_ctrl_t *p = send_ctrl_queue;
  bool async_only_enable = false;

  if( get_uart_tx_idle_time() < transmit_interval_ms ){ return; }  // <! frame interval control

  while( NULL != p ){

    if( async_only_enable ){
      if( p->pack->frame_cmd_ack_require ){
        p = p->next; continue;
      }
    }

    p->retry_ttl -= UART_LOWLEVEL_PREODIC_TIME;
    if( p->retry_ttl <= 0 ){
      uart_send( (uint8_t *)&(p->pack->frame_head), p->pack->frame_length );
      DB_PRINT( "send cmd:%u serial:%u len:%u, retry:%d\n",
                p->pack->frame_cmd,
                p->pack->frame_serial,
                p->pack->frame_length,
                p->retry_cnt);

      if( ! p->pack->frame_cmd_ack_require ){

        goto DROP_PACK;
      }else{
      
        if( --p->retry_cnt > 0 ){
          p->retry_ttl = 100;               // <! need to figure out a way to calculate
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


bool uart_lowlevel_build_packet(
                              uint8_t  frame_cmd, 
                              uint8_t *frame_serial,
                              bool     ack_require,
                              bool     ack_flag,
                              uint8_t *payload, 
                              uint16_t payload_length,
                              void   (*ack_timeout_callback)(uint8_t serial)
                             ){

  static uint8_t local_serial = 0;

  
  if( ack_require && ack_flag ){ return false; }
  if( ack_flag && (frame_serial == NULL) ){ return false; }
  if( ack_flag || (!ack_require) ){ ack_timeout_callback = NULL; }
  
  struct uart_lowlevel_protocol_t *p = 
         (struct uart_lowlevel_protocol_t *)malloc( 
                 sizeof(struct uart_lowlevel_protocol_t) + payload_length );
  
  if( p != NULL ){

    p->frame_head             = UART_LOWLEVEL_PROTOCOL_HEAD;
    p->frame_length           = sizeof(struct uart_lowlevel_protocol_t) + payload_length;
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
      s->retry_cnt = UART_SEND_MAX_RETRY_CNT;
      s->retry_ttl = 0;

      MOUNT( send_ctrl_queue, s );

      DB_PRINT( "build frame cmd:%u serial:%u len:%u\n",
                frame_cmd, local_serial, p->frame_length );

      /********************** log frame serial *******************/

      if( !ack_flag ){                                              // <! only work if this is not an ack.
        if( frame_serial != NULL ){ *frame_serial = local_serial; } // <! sometimes upper level need this serial
        local_serial++;
      }

      return true;
    }

    free( p );
  }
  DB_PRINT( "out of memory\n" );
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

static void parse_frame_cmd(struct uart_lowlevel_protocol_t *f){

  if( f == NULL ){ return; }
  struct frame_cmd_callback_t *p = NULL;

  DB_PRINT( "recv cmd: 0x%02X\n", f->frame_cmd );

  if( f->frame_cmd_ack_flag ){
    
    // <! search if we really send a ack-require packet.
    
    struct send_ctrl_t *s = send_ctrl_queue;
    while( s != NULL ){
      if( s->retry_cnt < UART_SEND_MAX_RETRY_CNT ){       // <! already sent
        if( (s->pack->frame_cmd_ack_require) &&           // <! frame is ack require
            (s->pack->frame_cmd == f->frame_cmd) &&       // <! frame cmd matching
            (s->pack->frame_serial == f->frame_serial) ){ // <! frame serial matching   
            
          p = callback_handler_query( ack_callback_queue, f->frame_cmd );
          DB_PRINT( "recv ack of frame %u\n", f->frame_serial );

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
      DB_PRINT( "callback found\n" );
      p->callback( (xPayloadHandler)f );
    }
  }
}


bool cmd_callback_reg(
                      uint8_t frame_cmd,
                      bool  (*callback)(xPayloadHandler h)
                     ){
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

    DB_PRINT( "reg cmd:%u callback\n", frame_cmd );
    return true;
  }
  
  return false;
}


bool ack_callback_reg(
                      uint8_t frame_cmd,
                      bool  (*callback)(xPayloadHandler h)
                     ){

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

    DB_PRINT( "reg ack cmd:%u callback\n", frame_cmd );
    return true;
  }
  
  return false;
}


void* lowlevel_get_payload(xPayloadHandler handler){
  if( handler != NULL ){
    struct uart_lowlevel_protocol_t *p = (struct uart_lowlevel_protocol_t *)handler;
    return &(p->payload);
  }
  return NULL;
}

uint16_t lowlevel_get_payload_length(xPayloadHandler handler){
  if( handler != NULL ){
    struct uart_lowlevel_protocol_t *p = (struct uart_lowlevel_protocol_t *)handler;
    return (p->frame_length - sizeof(struct uart_lowlevel_protocol_t) );
  }
  return 0;
}

uint8_t lowlevel_get_serial(xPayloadHandler handler){
  if( handler != NULL ){
    struct uart_lowlevel_protocol_t *p = (struct uart_lowlevel_protocol_t *)handler;
    return (p->frame_serial);
  }
  return 0;
}

uint8_t lowlevel_get_cmd(xPayloadHandler handler){
  if( handler != NULL ){
    struct uart_lowlevel_protocol_t *p = (struct uart_lowlevel_protocol_t *)handler;
    return (p->frame_cmd);
  }
  return 0;
}





