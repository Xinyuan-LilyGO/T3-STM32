
#include "log_print.h"
#include "SEGGER_RTT.h"

void print_log(const char * sFormat, ...)
{	
	va_list ParamList;
	va_start(ParamList, sFormat);
	SEGGER_RTT_vprintf(0, sFormat, &ParamList);
	va_end(ParamList);
}

void my_printf(const char * buf)
{
	print_log("%s", buf);
}

void segger_rtt_init(char * str)
{
	SEGGER_RTT_Init();
	print_log(str); 
#if LV_USE_LOG
    lv_log_register_print_cb(my_printf);
#endif	
}

