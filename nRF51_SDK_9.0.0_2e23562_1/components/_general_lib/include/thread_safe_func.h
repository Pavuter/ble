#ifndef __THREAD_SAFE_FUNC__H
#define __THREAD_SAFE_FUNC__H

#include <stdlib.h> 

//void *ts_malloc(size_t n);
//void ts_free(void *p);

__inline void *ts_malloc(size_t n){
  void *p = NULL;
  __disable_irq();
  p = malloc(n);
  __enable_irq();
  return p;
}


__inline void ts_free(void *p){
  
  __disable_irq();
  free(p);
  __enable_irq();
}


#endif


