/*
 * ota driver
 * Author: HoGC 
 */
#ifndef _OTA_H_
#define _OTA_H_
#include "upgrade.h"

uint8 ICACHE_FLASH_ATTR userbin_check(void);

uint8 ICACHE_FLASH_ATTR updata_status_check(void);

void ICACHE_FLASH_ATTR ota_upgrade(char *url_buf,upgrade_states_check_callback ota_check_cb);

void ICACHE_FLASH_ATTR url_parse(char *URL, char *host, unsigned short *port, char *path);

#endif /* APP_INCLUDE_MODULES_OTA_H_ */
