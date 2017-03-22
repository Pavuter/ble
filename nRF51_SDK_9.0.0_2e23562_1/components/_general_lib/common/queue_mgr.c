

#include "queue_mgr.h"

#include <string.h>
#include <stdlib.h> 
#include <stdint.h>


/***********************************

struct timer_t {
  struct timer_t *next;

  uint8_t id;
  uint8_t year;
};

struct timer_t *timer_queue = NULL;

void create_timer(){
  struct timer_t *p = (struct timer_t *)malloc( sizeof(struct timer_t) );
  if( p != NULL ){
	p->year = 2017;


	MOUNT( timer_queue, p );
  }

}

void delete_timer(struct timer_t *p ){
  if( p != NULL ){
	UNMOUNT( timer_queue, p );

  }
}

void find_timer(uint8_t id){
	struct timer_t *p = timer_queue;
	while( p != NULL ){
		// do something
		if( p->id == id ){
			break;
		}
		
		p = p->next;
	}
}

**********************************/


struct chain_t {
  struct chain_t *next;
};


void mount(void **__chain , void *__p){
  struct chain_t **chain=(struct chain_t **)__chain;
  struct chain_t *p = (struct chain_t *)__p;
  if( p == NULL ){ return; }
  if( *chain == NULL ){
    *chain = p;
    p->next = NULL;
  }else{
    struct chain_t *q;
    for( q=*chain; q->next!=NULL; q=q->next );
    q->next = p;
    p->next = NULL;
  }
}

void unmount(void **__chain , void *__p){
  struct chain_t **chain=(struct chain_t **)__chain;
  struct chain_t *p = (struct chain_t *)__p;
  if( p == NULL ){ return; }
  if( *chain == NULL ){ return; }
  else{
    struct chain_t *q;
    if( p == *chain ){ *chain = p->next; p->next = NULL; return; }
    else{
      for( q=*chain; q->next!=NULL; q=q->next ){
        if( q->next == p ){ 
          q->next = p->next; 
          p->next = NULL; 
          return; 
        }
      }
    }
  }
}


void drop_queue(void **__chain){
  struct chain_t *p=(struct chain_t *)(*__chain);
  struct chain_t *q=NULL;
  while(p!=NULL){
    q = p->next;
    unmount( __chain, p );
    free(p);
    p = q;
  }
}

void merge_queue(void **__chain, void **__queue){
//  struct chain_t **chain=(struct chain_t **)__chain;
  struct chain_t **q = (struct chain_t **)__queue;
  struct chain_t *p  = NULL;
  while( NULL != (p = *q) ){
    unmount(__queue, p);
    mount(__chain, p);
  }
}






