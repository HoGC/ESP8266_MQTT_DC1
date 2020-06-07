#ifndef _UDP_LOG_H_
#define _UDP_LOG_H_

#include "osapi.h"
#include "user_interface.h"

// #define UDP_LOG_ON

void udp_log_printf(char *msg);;

#define UDP_INFO( format, ... )  do {	\
									char flash_str[1024];	\
									os_sprintf( flash_str, format, ## __VA_ARGS__ ); \
									udp_log_printf(flash_str); \
								} while(0)
                          

void ICACHE_FLASH_ATTR udp_log_init(void);


#endif