
#include "lib_log.h"
#include "SEGGER_RTT.h"
#include <stdio.h>
#include <stdint.h>

#define Terminal_ID 0

void log_print_color(int level)
{
	switch (level)
    {
		case LIB_LOG_TRACE: SEGGER_RTT_TerminalOut(Terminal_ID, RTT_CTRL_TEXT_BRIGHT_WHITE); break;
		case LIB_LOG_INFO:  SEGGER_RTT_TerminalOut(Terminal_ID, RTT_CTRL_TEXT_BRIGHT_GREEN); break; 
		case LIB_LOG_WARN:  SEGGER_RTT_TerminalOut(Terminal_ID, RTT_CTRL_TEXT_BRIGHT_YELLOW); break; 
		case LIB_LOG_ERROR: SEGGER_RTT_TerminalOut(Terminal_ID, RTT_CTRL_TEXT_BRIGHT_RED);  break;
		case LIB_LOG_USER:  SEGGER_RTT_TerminalOut(Terminal_ID, RTT_CTRL_TEXT_BRIGHT_MAGENTA); break;
		default: break;
    }
}

void log_print_buf(const char *info,  uint8_t *buf, uint32_t len)
{
	int ret = 0;
	int tmp = 0;
	char buffer[512];

	if(info != NULL) {
		ret = snprintf((char *const)(buffer), 512, "%s: ", info);
	}
	
	for(int i = 0; i < len; i++) {
		tmp = snprintf((char *const)(buffer + ret), 512, "0x%02x ", buf[i]);
		ret += tmp;
	}
	snprintf((char *const)(buffer + ret), 512, "\n");
	
	SEGGER_RTT_TerminalOut(Terminal_ID, RTT_CTRL_TEXT_BRIGHT_WHITE);
	SEGGER_RTT_TerminalOut(Terminal_ID, buffer);
	SEGGER_RTT_TerminalOut(Terminal_ID, RTT_CTRL_RESET);
}

void log_print(int level, const char *fmt, ...)
{
	int ret = 0;
	char *prefix;
	va_list ParamList;
	va_start(ParamList, fmt);

	char buffer[512];

    switch (level)
    {
		case LIB_LOG_TRACE: prefix = "[T]"; break;
		case LIB_LOG_INFO:  prefix = "[I]"; break; 
		case LIB_LOG_WARN:  prefix = "[W]"; break; 
		case LIB_LOG_ERROR: prefix = "[E]";  break;
		case LIB_LOG_USER:  prefix = "[U]"; break;
		default: break;
    }
	ret = snprintf((char *const)buffer, 512, "%s ", prefix);
	ret += vsnprintf((char *const)(buffer + ret), 512, fmt, ParamList);

	va_end(ParamList);

	log_print_color(level);
	SEGGER_RTT_TerminalOut(Terminal_ID, buffer);
	SEGGER_RTT_TerminalOut(Terminal_ID, RTT_CTRL_RESET);
}

int print_log(const char *sFormat, ...)
{
	int r;
	va_list ParamList;
	va_start(ParamList, sFormat);
	r = SEGGER_RTT_vprintf(Terminal_ID, sFormat, &ParamList);
	va_end(ParamList);
	return r;
}

int my_printf(const char *buf)
{
	return print_log("%s", buf);
}

void segger_rtt_init(char *str)
{
	SEGGER_RTT_Init();
	// SEGGER_RTT_ConfigUpBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
	print_log(str);
#if LV_USE_LOG
	lv_log_register_print_cb(my_printf);
#endif
}
