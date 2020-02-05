#include "ets_sys.h"
#include "os_type.h"
#include "c_types.h"
#include "osapi.h"
#include "mem.h"
#include "gpio.h"
#include "user_interface.h"

#include "mqtt/mqtt.h"
#include "driver/uart.h"
#include "driver/wifi.h"
#include "driver/ota.h"
#include "driver/dc1.h"
#include "driver/webserver.h"

//#define MAIN_DEBUG_ON
#if defined(MAIN_DEBUG_ON)
#define INFO( format, ... ) os_printf( format, ## __VA_ARGS__ )
#else
#define INFO( format, ... )
#endif

#define SENDINFO_TIME							3000

u8 mac_str[13];									//mac地址
u8 ota_topic[32]={""};							//ota升级话题
u8 lwt_topic[32]={""};							//遗嘱话题
u8 birth_topic[30]={""};						//出生话题

u8 info_control_topic[30]={""};						//信息控制话题
u8 status_topic[7][30]={"","","",""};				//状态话题
u8 control_topic[4][30]={"","","",""};				//控制话题

MQTT_Client mqttClient;
bool smartconfig_mode = false;
os_timer_t OS_Timer_LED;
os_timer_t OS_Timer_SendInfo;

uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set(void) {
	enum flash_size_map size_map = system_get_flash_size_map();
	uint32 rf_cal_sec = 0;

	switch (size_map) {
	case FLASH_SIZE_4M_MAP_256_256:
		rf_cal_sec = 128 - 5;
		break;

	case FLASH_SIZE_8M_MAP_512_512:
		rf_cal_sec = 256 - 5;
		break;

	case FLASH_SIZE_16M_MAP_512_512:
	case FLASH_SIZE_16M_MAP_1024_1024:
		rf_cal_sec = 512 - 5;
		break;

	case FLASH_SIZE_32M_MAP_512_512:
	case FLASH_SIZE_32M_MAP_1024_1024:
		rf_cal_sec = 1024 - 5;
		break;

	case FLASH_SIZE_64M_MAP_1024_1024:
		rf_cal_sec = 2048 - 5;
		break;
	case FLASH_SIZE_128M_MAP_1024_1024:
		rf_cal_sec = 4096 - 5;
		break;
	default:
		rf_cal_sec = 0;
		break;
	}

	return rf_cal_sec;
}


/**
 * 	smartconfig配置回调
 */
void smartconfig_cd(sm_status status){

	switch (status)
	{
		case SM_STATUS_FINISH:
			INFO("smartconfig_finish\n");
			break;
	
		case SM_STATUS_GETINFO:
			INFO("wifiinfo_error\n");
			break;
		case SM_STATUS_TIMEOUT:
			INFO("smartconfig_timeout\n");
			break;
	}
	smartconfig_mode = false;
}

/**
 * 	ota升级回调
 */
void ota_finished_callback(void * arg) {
	struct upgrade_server_info *update = arg;
	os_timer_disarm(&OS_Timer_LED);
	if (update->upgrade_flag == true) {
		INFO("OTA  Success ! rebooting!\n");
		system_upgrade_reboot();
	} else {
		INFO("OTA Failed!\n");
	}
}


/**
 * 	WIFI连接回调
 */
void wifi_connect_cb(void){

	INFO("wifi connect!\r\n");
	os_timer_disarm(&OS_Timer_LED);
	wifi_led_switch(1);
	MQTT_Connect(&mqttClient);
}

/**
 * 	WIFI断开回调
 */
void wifi_disconnect_cb(void){
	INFO("wifi disconnect!\r\n");
	if(smartconfig_mode != true){
		os_timer_arm(&OS_Timer_LED, 500, 1);
	}
	MQTT_Disconnect(&mqttClient);
}

/**
 * 	MQTT连接回调
 */
void mqttConnectedCb(uint32_t *args) {
	
	u8 i;
	uint8_t gpio_ret;
	uint8_t switch_bit=0x80;
	MQTT_Client* client = (MQTT_Client*) args;
	INFO("MQTT: Connected\r\n");
	MQTT_Publish(client, birth_topic, "online", os_strlen("online"), 0,0);
	MQTT_Subscribe(client,ota_topic, 0);
	MQTT_Subscribe(client,info_control_topic, 0);
	if(updata_status_check()){
		MQTT_Publish(client, ota_topic, "updata_finish", os_strlen("updata_finish"), 0,0);
	}
	gpio_ret = dc1_read_gpio();
	INFO("gpio_ret:%02X",gpio_ret);
	for (i = 0; i < 4; i++){
		if((gpio_ret & switch_bit) == switch_bit){
			MQTT_Publish(client, status_topic[i], "1", os_strlen("1"), 0,1);
		}else{
			MQTT_Publish(client, status_topic[i], "0", os_strlen("0"), 0,1);
		}
		switch_bit = switch_bit >> 1;
	}
	for (i = 0; i < 4; i++){
        MQTT_Subscribe(client,control_topic[i], 0);
    }
	os_timer_arm(&OS_Timer_SendInfo, SENDINFO_TIME, 1);
}


/**
 * 	MQTT断开连接回调
 */
void mqttDisconnectedCb(uint32_t *args) {
	MQTT_Client* client = (MQTT_Client*) args;
	INFO("MQTT: Disconnected\r\n");
}


/**
 * 	MQTT发布消息回调
 */
void mqttPublishedCb(uint32_t *args) {
	MQTT_Client* client = (MQTT_Client*) args;
	INFO("MQTT: Published\r\n");
}

/**
 * 	MQTT接收数据回调
 */
void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len,
		const char *data, uint32_t data_len) {
	char *topicBuf = (char*) os_zalloc(topic_len + 1), *dataBuf =
			(char*) os_zalloc(data_len + 1);
	
	u8 i;	
	u8 ret = 0;	

	MQTT_Client* client = (MQTT_Client*) args;

	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;

	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;

	INFO("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);

	//data = {"url"="http://yourdomain.com:9001/ota/"}
	if (os_strcmp(topicBuf, ota_topic) == 0) {
		char url_data[200];
		if(ota_get_josn_str(dataBuf,"url",url_data)){
			INFO("ota_start\n");
			os_timer_disarm(&OS_Timer_SendInfo);
			os_timer_arm(&OS_Timer_LED, 100, 1);
			MQTT_Publish(client, ota_topic, "ota_start", os_strlen("ota_start"), 0,0);
			ota_upgrade(url_data,ota_finished_callback);
		}
	}else if (os_strcmp(topicBuf, info_control_topic) == 0) {
		if(os_strcmp(dataBuf, "1") == 0){
            os_timer_arm(&OS_Timer_SendInfo, SENDINFO_TIME, 1);
        }else if(os_strcmp(dataBuf, "0") == 0){
            os_timer_disarm(&OS_Timer_SendInfo);
        }
	}else if (os_strcmp(topicBuf, control_topic[0]) == 0) {
        if(os_strcmp(dataBuf, "1") == 0){
            ret = set_switch(0,1);
			if(ret){
				MQTT_Publish(client, status_topic[0], "1", os_strlen("1"), 0,1);
			}
        }else if(os_strcmp(dataBuf, "0") == 0){
            ret = set_switch(0,0);
			if(ret){
				MQTT_Publish(client, status_topic[0], "0", os_strlen("0"), 0,1);
				MQTT_Publish(client, status_topic[1], "0", os_strlen("0"), 0,1);
				MQTT_Publish(client, status_topic[2], "0", os_strlen("0"), 0,1);
				MQTT_Publish(client, status_topic[3], "0", os_strlen("0"), 0,1);
			}
        }
    }else{
		for ( i = 1; i < 4; i++)
    	{
			if (os_strcmp(topicBuf, control_topic[i]) == 0) {
				if(os_strcmp(dataBuf, "1") == 0){
					ret = set_switch(i,1);
					if(ret){
						MQTT_Publish(client, status_topic[i], "1", os_strlen("1"), 0,1);
						MQTT_Publish(client, status_topic[0], "1", os_strlen("1"), 0,1);
					}
					INFO("MQTT_Publish\n");
				}else if(os_strcmp(dataBuf, "0") == 0){
					ret = set_switch(i,0);
					if(ret){
						MQTT_Publish(client, status_topic[i], "0", os_strlen("0"), 0,1);
					}
				}
			}
		}
    }

	os_free(topicBuf);
	os_free(dataBuf);
}

/**
 * 	MQTT初始化
 */
void ICACHE_FLASH_ATTR mqtt_init(void) {

	MQTT_InitConnection(&mqttClient, MQTT_HOST, MQTT_PORT, DEFAULT_SECURITY);
	MQTT_InitClient(&mqttClient, mac_str, MQTT_USER,MQTT_PASS, MQTT_KEEPALIVE, 1);
	MQTT_InitLWT(&mqttClient, lwt_topic, LWT_MESSAGE, 0, 0);
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);
}


/**
 * 获取MAC
 */
void ICACHE_FLASH_ATTR get_mac(void) {

	u8 i;
	u8 mac[6];
	wifi_get_macaddr(STATION_IF, mac);
	os_sprintf(mac_str,"%02x%02x%02x%02x%02x%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	INFO("mac:%s\n", mac_str);

    os_sprintf(ota_topic,OTA_TOPIC,mac_str);
	os_sprintf(lwt_topic,LWT_TOPIC,mac_str);
    os_sprintf(birth_topic,BIRTH_TOPIC,mac_str);

    for (i = 0; i < 4; i++)
    {
        os_sprintf(control_topic[i],CONTROL_TOPIC,mac_str,i);
        os_sprintf(status_topic[i],STATUS_TOPIC,mac_str,i);
    }
	os_sprintf(info_control_topic,INFO_CONTROL_TOPIC,mac_str);
	os_sprintf(status_topic[4],VOLTAGE_TOPIC,mac_str);
	os_sprintf(status_topic[5],CURRENT_TOPIC,mac_str);
	os_sprintf(status_topic[6],POWER_TOPIC,mac_str);
}


void ICACHE_FLASH_ATTR led_flash(void){
    LOCAL bool status=0;
    wifi_led_switch(status);
    status=~status;
}


/**
 * 定时发送参数
 */
void ICACHE_FLASH_ATTR send_info(void){

	u8 i = 0;
	uint16_t electric_data[3];
	char electric_str[3][6] = {""};

	get_electric_data(electric_data);

	for (i = 0; i < 3; i++)
	{
		os_sprintf(electric_str[i],"%d.%d",electric_data[i]/10,electric_data[i]%10);
		MQTT_Publish(&mqttClient, status_topic[4+i], electric_str[i], os_strlen(electric_str[i]), 0,0);
	}
}


void ICACHE_FLASH_ATTR key0_short(bool status){

    u8 i=0;
    INFO("keyShortPress\n");
	if(status){
		MQTT_Publish(&mqttClient, status_topic[0], "1", os_strlen("1"), 0,1);
	}else{
		for(i = 0; i < 4; i++){
			MQTT_Publish(&mqttClient, status_topic[i], "0", os_strlen("0"), 0,1);
		}
	}
	
}

void ICACHE_FLASH_ATTR key0_long(void){
    INFO("keyLongPress\n");
	smartconfig_mode = true;
    os_timer_arm(&OS_Timer_LED, 100, 1);
    start_smartconfig(smartconfig_cd);
}

void ICACHE_FLASH_ATTR key1_short(bool status){
    if(status){
        MQTT_Publish(&mqttClient, status_topic[1], "1", os_strlen("1"), 0,1);
        MQTT_Publish(&mqttClient, status_topic[0], "1", os_strlen("1"), 0,1);
    }else{
        MQTT_Publish(&mqttClient, status_topic[1], "0", os_strlen("0"), 0,1);
    }
}

void ICACHE_FLASH_ATTR key1_long(void)
{   
	INFO("start webserver\n"); 
	
	os_timer_arm(&OS_Timer_LED, 100, 1);
	
	wifi_set_opmode(STATIONAP_MODE);
	struct softap_config stationConf;
	wifi_softap_get_config(&stationConf);
	os_strcpy(stationConf.ssid, "Webserver");
	stationConf.ssid_len = os_strlen("Webserver");
	wifi_softap_set_config_current(&stationConf);

	webserver_init(NULL);

	wifi_station_disconnect();
}

void ICACHE_FLASH_ATTR key2_short(bool status){
    if(status){
        MQTT_Publish(&mqttClient, status_topic[2], "1", os_strlen("1"), 0,1);
        MQTT_Publish(&mqttClient, status_topic[0], "1", os_strlen("1"), 0,1);
    }else{
        MQTT_Publish(&mqttClient, status_topic[2], "0", os_strlen("0"), 0,1);
    }
}

void ICACHE_FLASH_ATTR key3_short(bool status){
    if(status){
        MQTT_Publish(&mqttClient, status_topic[3], "1", os_strlen("1"), 0,1);
        MQTT_Publish(&mqttClient, status_topic[0], "1", os_strlen("1"), 0,1);
    }else{
        MQTT_Publish(&mqttClient, status_topic[3], "0", os_strlen("0"), 0,1);
    }
}


void ICACHE_FLASH_ATTR led_flash_init(void){
    os_timer_disarm(&OS_Timer_LED);
    os_timer_setfn(&OS_Timer_LED, (os_timer_func_t *)led_flash, NULL);
    os_timer_arm(&OS_Timer_LED, 500, 1);
}

void ICACHE_FLASH_ATTR send_info_init(void){
    os_timer_disarm(&OS_Timer_SendInfo);
    os_timer_setfn(&OS_Timer_SendInfo, (os_timer_func_t *)send_info, NULL);
}

void user_init(void) {

	// 串口初始化
	UART_SetPrintPort(1);
	uart_init(4800, 115200);
	os_delay_us(60000);
	UART_SetParity(0,EVEN_BITS);
	system_uart_swap();
	set_uart_cb(dc1_uart_data_handler);

	get_mac();
	mqtt_init();

	led_flash_init();
	send_info_init();

	dc1_init(key0_short,key0_long,key1_short,key1_long,key2_short,NULL,key3_short,NULL);

	wifi_set_opmode(STATION_MODE);
	set_wifistate_cb(wifi_connect_cb, wifi_disconnect_cb);
}

