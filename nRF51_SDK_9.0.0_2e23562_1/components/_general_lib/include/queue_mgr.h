#ifndef __QUEUE_MGR__H
#define __QUEUE_MGR__H


#define MOUNT( __CHAIN , __P )                  mount( (void *)&(__CHAIN) , __P )
#define UNMOUNT( __CHAIN , __P )                unmount( (void *)&(__CHAIN) , __P )
#define DROP_QUEUE( __CHAIN )                   drop_queue( (void *)&(__CHAIN) )
#define MERGE_QUEUE( __CHAIN1 , __CHAIN2)       merge_queue( (void *)&(__CHAIN1) , (void *)&(__CHAIN2) )


void mount(void **__chain , void *__p);
void unmount(void **__chain , void *__p);
void drop_queue(void **__chain);
void merge_queue(void **__chain, void **__p);

#endif


