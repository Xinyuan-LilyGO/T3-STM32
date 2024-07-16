#ifndef __LIB_LOG_H__
#define __LIB_LOG_H__

#define LIB_LOG_LEVEL LIB_LOG_TRACE

#define LIB_LOG_TRACE 0 /**< A lot of logs to give detailed information*/
#define LIB_LOG_INFO  1 /**< Log important events*/
#define LIB_LOG_WARN  2 /**< Log if something unwanted happened but didn't caused problem*/
#define LIB_LOG_ERROR 3 /**< Only critical issue, when the system may fail*/
#define LIB_LOG_USER  4 /**< Custom logs from the user*/
#define LIB_LOG_NONE  5 /**< Do not log anything*/

void log_print(int level, const char *fmt, ...);

#if LIB_LOG_LEVEL <= LIB_LOG_TRACE
#define LOG_TRACE(fmt, ...) log_print(LIB_LOG_TRACE, fmt, ##__VA_ARGS__);
#else
#define LOG_TRACE(fmt, ...) do {}while(0);
#endif

#if LIB_LOG_LEVEL <= LIB_LOG_INFO
#define LOG_INFO(fmt, ...) log_print(LIB_LOG_INFO, fmt, ##__VA_ARGS__);
#else
#define LOG_INFO(fmt, ...) do {}while(0);
#endif

#if LIB_LOG_LEVEL <= LIB_LOG_WARN
#define LOG_WARN(fmt, ...) log_print(LIB_LOG_WARN, fmt, ##__VA_ARGS__);
#else
#define LOG_WARN(fmt, ...) do {}while(0);
#endif

#if LIB_LOG_LEVEL <= LIB_LOG_ERROR
#define LOG_ERROR(fmt, ...) log_print(LIB_LOG_ERROR, fmt, ##__VA_ARGS__);
#else
#define LOG_ERROR(fmt, ...) do {}while(0);
#endif

#if LIB_LOG_LEVEL <= LIB_LOG_USER
#define LOG_USER(fmt, ...) log_print(LIB_LOG_USER, fmt, ##__VA_ARGS__);
#else
#define LOG_USER(fmt, ...) do {}while(0);
#endif

int print_log(const char * sFormat, ...);
int my_printf(const char * buf);
void segger_rtt_init(char * str);


#endif
