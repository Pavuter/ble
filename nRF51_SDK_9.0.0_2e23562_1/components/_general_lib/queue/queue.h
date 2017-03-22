#ifndef _U_QUEUE_H_
#define _U_QUEUE_H_

/**
 *@brief   :queue state.
 */
enum queue_status_t
{
    QUEUE_OK,
    QUEUE_OVERRUN,
    QUEUE_EMPTY,
    QUEUE_REMAIN
};

/**
 *@brief   :Queue control module. 
 */
struct queue_t
{
  unsigned char *buf;     // point to data cache.
  unsigned int putP;      // input queue bytes counter.
  unsigned int getP;      // output queue bytes counter.
  unsigned int size;      // queue size.
  unsigned int remain;    // queue remain size.
};

/* user function definition --------------------------------------------- */
enum queue_status_t queue_init(struct queue_t *queue, unsigned int size, unsigned char *buf);
enum queue_status_t queue_put_byte_in(struct queue_t *queue, unsigned char data);
enum queue_status_t queue_push_byte_out(struct queue_t *queue, unsigned char *pdata);
unsigned int queue_get_used_size(struct queue_t *queue);
unsigned int queue_get_remain_size(struct queue_t *queue);

#endif





