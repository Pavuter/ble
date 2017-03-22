#ifndef _U_QUEUE_H_
#define _U_QUEUE_H_

/**
 *@brief   :queue state.
 */
typedef enum
{
    QUEUE_OK,
    QUEUE_OVERRUN,
    QUEUE_EMPTY,
    QUEUE_REMAIN
}QUEUE_STATUS;

/**
 *@brief   :Queue control module.
 */
typedef struct _QUEUE_
{
  unsigned char *pbuf;   // point to data cache.
  unsigned int put;      // input queue bytes counter.
  unsigned int get;      // output queue bytes counter.
  unsigned int size;     // queue size.
  unsigned int remain;   // queue remain size.
}queue_byte_t;

/* user function definition --------------------------------------------- */
QUEUE_STATUS queue_init(queue_byte_t *queue, unsigned int size, unsigned char *buf);
QUEUE_STATUS queue_put_byte(queue_byte_t *queue, unsigned char data);
QUEUE_STATUS queue_get_byte(queue_byte_t *queue, unsigned char *pdata);
unsigned int queue_get_used_buff_size(queue_byte_t *queue);
unsigned int queue_get_remain_buff_size(queue_byte_t *queue);

#endif





