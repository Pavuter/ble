#include "queue.h"

QUEUE_STATUS queue_init(queue_byte_t *queue, unsigned int size, unsigned char *buf)
{
    queue->pbuf=buf;
    queue->remain=size;
    queue->size=size;
    queue->put=0;
    queue->get=0;

    return QUEUE_OK;
}

QUEUE_STATUS queue_clear(queue_byte_t *queue)
{
    unsigned int cnt = 0;

    queue->get = 0;
    queue->put = queue->get;
    queue->remain=queue->size;

    for(cnt = 0; cnt < queue->size; cnt ++)
    {
        queue->pbuf[cnt] = 0;
    }

    return QUEUE_OK;
}
QUEUE_STATUS queue_put_byte(queue_byte_t *queue, unsigned char data)
{
    if(queue->remain == 0)
    {
        return QUEUE_OVERRUN;
    }
    queue->pbuf[queue->put] = data;
    queue->put++;

    if(queue->put == queue->size)
    {
        queue->put = 0;
    }
    queue->remain--;

    return QUEUE_OK;
}

QUEUE_STATUS queue_get_byte(queue_byte_t *queue, unsigned char *pdata)
{
    if(queue->remain == queue->size)
    {
        return QUEUE_EMPTY;
    }
    *pdata = queue->pbuf[queue->get];
    queue->get++;
    if(queue->get == queue->size)
    {
        queue->get = 0;
    }
    queue->remain++;

    return QUEUE_OK;
}

unsigned int queue_get_used_buff_size(queue_byte_t *queue)
{
    return (queue->size-queue->remain);
}

unsigned int queue_get_remain_buff_size(queue_byte_t *queue)
{
    return queue->remain;
}




