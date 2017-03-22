

#include "thread_safe_func.h"

#include "stm32f0xx.h"
#include <string.h>
#include <stdlib.h>


//__inline void *ts_malloc(size_t n){
//  void *p = NULL;
//  __disable_irq();
//  p = malloc(n);
//  __enable_irq();
//  return p;
//}


//__inline void ts_free(void *p){
//  
//  __disable_irq();
//  free(p);
//  __enable_irq();
//}




