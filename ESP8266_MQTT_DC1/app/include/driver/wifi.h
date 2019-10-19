/*
 * wifi driver
 * Author: HoGC 
 */
#ifndef USER_WIFI_H_
#define USER_WIFI_H_
#include "ets_sys.h"
#include "os_type.h"

#define DEVICE_TYPE 		"gh_9e2cff3dfa51" //wechat public number
#define DEVICE_ID 			"122475" //model ID

#define DEFAULT_LAN_PORT 	12476

typedef enum {
    SM_STATUS_FINISH = 0,
    SM_STATUS_GETINFO,
    SM_STATUS_TIMEOUT,
} sm_status;

typedef void (*smartconfig_cd_t)(sm_status status);
typedef void (*wifconnect_cb_t)(void);
typedef void (*wifdisconnect_cb_t)(void);

void ICACHE_FLASH_ATTR wifi_connect(uint8_t* ssid, uint8_t* pass);
void ICACHE_FLASH_ATTR set_wifistate_cb(wifconnect_cb_t connect_cb, wifdisconnect_cb_t disconnect_cb);
void ICACHE_FLASH_ATTR start_smartconfig(smartconfig_cd_t cd);

#endif /* USER_WIFI_H_ */
