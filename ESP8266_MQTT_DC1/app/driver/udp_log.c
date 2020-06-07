#include "driver/udp_log.h"

#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"

#include <stdarg.h>

static struct espconn user_udp_espconn;

void udp_log_printf(char *msg){
    espconn_sent(&user_udp_espconn, msg, os_strlen(msg));
}

void ICACHE_FLASH_ATTR udp_log_init(void){
    
    user_udp_espconn.type = ESPCONN_UDP;
    user_udp_espconn.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
    user_udp_espconn.proto.udp->local_port = espconn_port();  // set a available  port
    
    const char udp_remote_ip[4] = {255, 255, 255, 255}; 
    
    os_memcpy(user_udp_espconn.proto.udp->remote_ip, udp_remote_ip, 4); // ESP8266 udp remote IP
    
    user_udp_espconn.proto.udp->remote_port =  8266;  // ESP8266 udp remote port
    
    espconn_create(&user_udp_espconn);   // create udp
}