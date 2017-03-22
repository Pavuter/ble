#include "queue.h"

enum queue_status_t queue_init(struct queue_t *queue, unsigned int size, unsigned char *buf)
{
    queue->buf=buf;          
    queue->remain=size;
    queue->size=size;
    queue->putP=0;                   
    queue->getP=0;                   

    return QUEUE_OK;
}

enum queue_status_t queue_clear(struct queue_t *queue)
{
    unsigned int cnt = 0;

    queue->getP = 0;
    queue->putP = queue->getP;
    queue->remain=queue->size;
    
    for(cnt = 0; cnt < queue->size; cnt ++)
    {
        queue->buf[cnt] = 0;
    }

    return QUEUE_OK;
}
enum queue_status_t queue_put_byte_in(struct queue_t *queue, unsigned char data)
{
    if(queue->remain == 0)
    {
        return QUEUE_OVERRUN;
    }
    queue->buf[queue->putP] = data;
    queue->putP++;
    
    if(queue->putP == queue->size)
    {
        queue->putP = 0;
    }
    queue->remain--;

    return QUEUE_OK;
}

enum queue_status_t queue_push_byte_out(struct queue_t *queue, unsigned char *pdata)
{
    if(queue->remain == queue->size)
    { 
        return QUEUE_EMPTY;
    }
    *pdata = queue->buf[queue->getP];
    queue->getP++;
    if(queue->getP == queue->size)
    {
        queue->getP = 0;
    }
    queue->remain++;
    
    return QUEUE_OK;
}

unsigned int queue_get_used_size(struct queue_t *queue)
{
    return (queue->size-queue->remain);
}

unsigned int queue_get_remain_size(struct queue_t *queue)
{
    return queue->remain;
}




