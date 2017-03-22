#ifndef __PTL_H__
#define __PTL_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "compiler_abstraction.h"

#include "ptl_lowlayer.h"
#include "ptl_midlayer.h"

#define DEBUG_PROTOCOL_OPEN         (2)

// for debug
#if (DEBUG_PROTOCOL_OPEN == 2)
#define ptl_log(fmt, ...)							app_trace_log("[%d][%05d] "fmt, \
                       								__SYS_REMAIN_HEAP_SIZE_, __LINE__, ##__VA_ARGS__)
#define ptl_log_dump                  app_trace_dump
#elif (DEBUG_PROTOCOL_OPEN == 1)
#define ptl_log         							app_trace_log
#define ptl_log_dump                  app_trace_dump
#else
#define ptl_log(...)
#define ptl_log_dump(...)
#endif

// protocol error code
#define PTL_ERROR_BASE                  	(0x00)
#define PTL_ERROR_NONE                  	(PTL_ERROR_BASE + 0)
#define PTL_ERROR_OPERATE					        (PTL_ERROR_BASE + 1)
#define PTL_ERROR_GET_MODEL_ID				    (PTL_ERROR_BASE + 2)
#define PTL_ERROR_GET_MODEL_SOFT_VERSION	(PTL_ERROR_BASE + 3)
#define PTL_ERROR_GET_MODEL_HW_VERISON		(PTL_ERROR_BASE + 4)
#define PTL_ERROR_NONE_POINT   			      (PTL_ERROR_BASE + 5)
#define PTL_ERROR_NONE_MODE					      (PTL_ERROR_BASE + 6)
#define PTL_ERROR_OUT_OF_EVT				      (PTL_ERROR_BASE + 7)
#define PTL_ERROR_FULL_EVT					      (PTL_ERROR_BASE + 8)
#define PTL_ERROR_SAME_EVT					      (PTL_ERROR_BASE + 9)
#define PTL_ERROR_NONE_EVT					      (PTL_ERROR_BASE + 10)
#define PTL_ERROR_NONE_MEM					      (PTL_ERROR_BASE + 11)
#define PTL_ERROR_SERVICE_NOT_CONNECT		  (PTL_ERROR_BASE + 12)
#define PTL_ERROR_INVALID_PARAM           (PTL_ERROR_BASE + 13)
#define GET_SMALLER_VALUE(x, y)     ((x)>(y)?(y):(x)) 

struct private_model_t {
  char model_id[33];
  char sw_ver[40];
  char hw_ver[40];
};

#endif


