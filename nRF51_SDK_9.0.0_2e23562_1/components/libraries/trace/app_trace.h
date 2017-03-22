#ifndef __DEBUG_H_
#define __DEBUG_H_

#include <stdint.h>
#include <stdio.h>

#ifndef UART_TX_BUF_SIZE
    #define UART_TX_BUF_SIZE 256                         /**< UART TX buffer size. */
#endif
#ifndef UART_RX_BUF_SIZE
    #define UART_RX_BUF_SIZE 64                         /**< UART RX buffer size. */
#endif

int app_get_heap_size(void);
/**
 * @defgroup app_trace Debug Logger
 * @ingroup app_common
 * @{
 * @brief Enables debug logs/ trace over UART.
 * @details Enables debug logs/ trace over UART. Tracing is enabled only if 
 *          ENABLE_DEBUG_LOG_SUPPORT is defined in the project.
 */
#ifdef ENABLE_DEBUG_LOG_SUPPORT
/**
 * @brief Module Initialization.
 *
 * @details Initializes the module to use UART as trace output.
 * 
 * @warning This function will configure UART using default board configuration. 
 *          Do not call this function if UART is configured from a higher level in the application. 
 */
void app_trace_init(void);

/**
 * @brief Log debug messages.
 *
 * @details This API logs messages over UART. The module must be initialized before using this API.
 *
 * @note Though this is currently a macro, it should be used used and treated as function.
 */
#define app_trace_log printf

/**
 * @brief Dump auxiliary byte buffer to the debug trace.
 *
 * @details This API logs messages over UART. The module must be initialized before using this API.
 * 
 * @param[in] p_buffer  Buffer to be dumped on the debug trace.
 * @param[in] len       Size of the buffer.
 */
void app_trace_dump(uint8_t * p_buffer, uint32_t len);

#define APP_TRACE_HEAP(...)         {app_trace_log("[%4d]", app_get_heap_size());}

#else // ENABLE_DEBUG_LOG_SUPPORT

#define app_trace_init(...)
#define app_trace_log(...)
#define app_trace_dump(...)
#define app_trace_log_heap(...)
#endif // ENABLE_DEBUG_LOG_SUPPORT

/** @} */

#endif //__DEBUG_H_
