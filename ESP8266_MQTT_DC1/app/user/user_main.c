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
#include "driver/clock.h"
#include "driver/cJSON.h"
#include "driver/webserver.h"

#define MAIN_DEBUG_ON
#if defined(MAIN_DEBUG_ON)
#define INFO( format, ... ) os_printf( format, ## __VA_ARGS__ )
#else
#define INFO( format, ... )
#endif

#define SWITCH_TASK_ID				200
#define SWITCH0_OFF_TASK_ID			200
#define SWITCH0_ON_TASK_ID			201
#define SWITCH1_OFF_TASK_ID			202
#define SWITCH1_ON_TASK_ID			203
#define SWITCH2_OFF_TASK_ID			204
#define SWITCH2_ON_TASK_ID			205
#define SWITCH3_OFF_TASK_ID			206
#define SWITCH3_ON_TASK_ID			207

#define SENDINFO_TIME							3000

u8 mac_str[13];									//mac地址
u8 ota_topic[32]={""};							//ota升级话题
u8 lwt_topic[32]={""};							//遗嘱话题
u8 birth_topic[30]={""};						//出生话题

u8 timer_config_topic[50]={""};						//定时配置话题
u8 timer_info_topic[50]={""};						//定时输出信息话题
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

int set_dc1_switch(int switch_mun, uint8_t status){
	int ret = 0, i = 0;
	if((0<=switch_mun) && (switch_mun<=3)){
		if(switch_mun == 0){
			if(status == 1){
				ret = set_switch(0,1);
				if(ret){
					MQTT_Publish(&mqttClient, status_topic[0], "1", os_strlen("1"), 0,1);
				}
			}else if(status == 0){
				ret = set_switch(0,0);
				if(ret){
					for ( i = 0; i < 4; i++)
					{
						MQTT_Publish(&mqttClient, status_topic[i], "0", os_strlen("0"), 0,1);
					}
				}
			}
		}else{
			if(status == 1){
				ret = set_switch(switch_mun,1);
				if(ret){
					MQTT_Publish(&mqttClient, status_topic[0], "1", os_strlen("1"), 0,1);
					MQTT_Publish(&mqttClient, status_topic[switch_mun], "1", os_strlen("1"), 0,1);
				}
			}else if(status == 0){
				ret = set_switch(switch_mun,0);
				if(ret){
					MQTT_Publish(&mqttClient, status_topic[switch_mun], "0", os_strlen("0"), 0,1);
				}
			}
		}
	}
	return ret;
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
	MQTT_Subscribe(client,timer_config_topic, 0);
	MQTT_Subscribe(client,info_control_topic, 0);
	if(updata_status_check()){
		MQTT_Publish(client, ota_topic, "updata_finish", os_strlen("updata_finish"), 0,0);
	}
	gpio_ret = dc1_read_gpio();
	for (i = 0; i < 4; i++){
		if((gpio_ret & switch_bit) == switch_bit){
			INFO("switch%d_status: on\n", i);
			MQTT_Publish(client, status_topic[i], "1", os_strlen("1"), 0,1);
		}else{
			INFO("switch%d_status: off\n", i);
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

	//data = {"url":"http://yourdomain.com:9001/ota/"}
	if (os_strcmp(topicBuf, ota_topic) == 0) {

		cJSON *root = cJSON_Parse(dataBuf);
		cJSON *json = cJSON_GetObjectItem(root, "url");
		if ((json != NULL) && (json->type == cJSON_String))
    	{
			INFO("ota_start\n");
			os_timer_disarm(&OS_Timer_SendInfo);
			os_timer_arm(&OS_Timer_LED, 100, 1);
			MQTT_Publish(client, ota_topic, "ota_start", os_strlen("ota_start"), 0,0);
			ota_upgrade(json->valuestring,ota_finished_callback);
		}
		cJSON_Delete(root);
	}else if (os_strcmp(topicBuf, info_control_topic) == 0) {
		if(os_strcmp(dataBuf, "1") == 0){
			INFO("dc1_info: on\n");
            os_timer_arm(&OS_Timer_SendInfo, SENDINFO_TIME, 1);
        }else if(os_strcmp(dataBuf, "0") == 0){
			INFO("dc1_info: off\n");
            os_timer_disarm(&OS_Timer_SendInfo);
        }
	}else if (os_strcmp(topicBuf, control_topic[0]) == 0) {
        if(os_strcmp(dataBuf, "1") == 0){
			INFO("set_switch0: on\n");
            set_dc1_switch(0, 1);
        }else if(os_strcmp(dataBuf, "0") == 0){
			INFO("set_switch0: off\n");
            set_dc1_switch(0, 0);
        }
    }else{
		for ( i = 1; i < 4; i++)
    	{
			if (os_strcmp(topicBuf, control_topic[i]) == 0) {
				if(os_strcmp(dataBuf, "1") == 0){
					INFO("set_switch%d: on\n", i);
					set_dc1_switch(i, 1);
				}else if(os_strcmp(dataBuf, "0") == 0){
					INFO("set_switch%d: off\n", i);
					set_dc1_switch(i, 0);
				}
			}
		}
    }


	if (os_strcmp(topicBuf, timer_config_topic) == 0) {
		uint8_t num = 0;
		cJSON *root, *root_sub, *json;
		char *pjson;

		clock_time_t time;
		uint8_t week_bit = 0;
		int task_id = 0;
		int timer_id = 0;

		root = cJSON_Parse(dataBuf);
		pjson = cJSON_Print(root);
		os_free(pjson);

		//swiech_topic_config
		root_sub = cJSON_GetObjectItem(root, "addTimer");
		if(root_sub != NULL){
			json = cJSON_GetObjectItem(root_sub, "time");
			if ((json != NULL) && (json->type == cJSON_String))
			{
				INFO("add timer time:%s\n", json->valuestring);
				clock_str_to_time(json->valuestring, &time);
				json = NULL; 
				json = cJSON_GetObjectItem(root_sub, "switchNum");
				if ((json != NULL) && (json->type == cJSON_Number)){
					task_id = SWITCH_TASK_ID + (json->valueint)*2;
					json = NULL; 
					json = cJSON_GetObjectItem(root_sub, "value");
					if ((json != NULL) && (json->type == cJSON_Number)){
						if(json->valueint == 1){
							task_id = task_id + 1;
						}
						INFO("add timer task_id: %d\n",task_id);
						json = NULL; 
						json = cJSON_GetObjectItem(root_sub, "week");
						if ((json != NULL) && (json->type == cJSON_String))
						{
							INFO("add timer week: %s\n",json->valuestring);
							clock_str_to_week_bit(json->valuestring, &week_bit);
							timer_id = clock_add_timer(time, week_bit, task_id);
						}else{
							INFO("add today timer task\n");
							timer_id = clock_add_today_timer(time, task_id);
						}
						char out_info[30] = ""; 
						if(timer_id >= TIMER_STATIC_ID){
							os_strcpy(out_info, "add timer task succeed");
						}else if(timer_id == TIMER_REPEAT){
							os_strcpy(out_info, "add timer task repeat");
						}else if(timer_id == TIMER_FULL){
							os_strcpy(out_info, "add timer task full");
						}else if(timer_id == TIMER_MODIFY){
							os_strcpy(out_info, "add timer task modify");
						}else{
							os_strcpy(out_info, "add timer task failure");
						}
						INFO("%s\n",out_info);
						MQTT_Publish(client, timer_info_topic, out_info, os_strlen(out_info), 0,0);
					}
				}
			}
		}

		json = cJSON_GetObjectItem(root, "closeTimer");
		if ((json != NULL) && (json->type == cJSON_Number)){
			int ret = clock_close_timer(json->valueint);
			if(ret == TIMER_SUCCEED){
				INFO("close timer succeed\n");
				MQTT_Publish(client, timer_info_topic, "close timer succeed", os_strlen("close timer succeed"), 0,0);
			}else if(ret == TIMER_NONENTITY){
				INFO("close timer nonentity\n");
				MQTT_Publish(client, timer_info_topic, "close timer nonentity", os_strlen("close timer nonentity"), 0,0);
			}
		}

		json = cJSON_GetObjectItem(root, "openTimer");
		if ((json != NULL) && (json->type == cJSON_Number)){
			int ret = clock_open_timer(json->valueint);
			if(ret == TIMER_SUCCEED){
				INFO("open timer succeed\n");
				MQTT_Publish(client, timer_info_topic, "open timer succeed", os_strlen("open timer succeed"), 0,0);
			}else if(ret == TIMER_NONENTITY){
				INFO("open timer nonentity\n");
				MQTT_Publish(client, timer_info_topic, "open timer nonentity", os_strlen("open timer nonentity"), 0,0);
			}
		}

		json = cJSON_GetObjectItem(root, "delTimer");
		if ((json != NULL) && (json->type == cJSON_Number)){
			int ret = clock_delete_timer(json->valueint);
			if(ret == TIMER_SUCCEED){
				INFO("add timer del succeed\n");
				MQTT_Publish(client, timer_info_topic, "add timer del succeed", os_strlen("add timer del succeed"), 0,0);
			}else if(ret == TIMER_NONENTITY){
				INFO("add timer nonentity\n");
				MQTT_Publish(client, timer_info_topic, "add timer nonentity", os_strlen("add timer nonentity"), 0,0);
			}
		}

		json = cJSON_GetObjectItem(root, "getTimer");
		if ((json != NULL) && (json->type == cJSON_Number)){
			if(json->valueint == 0){
				char info_str[1024] = "";
				clock_get_timer_task_info_json(info_str);
				MQTT_Publish(client, timer_info_topic, info_str, os_strlen(info_str), 0,0);
			}
		}
		
		cJSON_Delete(root);
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

	os_sprintf(timer_info_topic,TIMER_INFO_TOPIC,mac_str);
	os_sprintf(timer_config_topic,TIMER_CONFIG_TOPIC,mac_str);
	os_sprintf(status_topic[6],POWER_TOPIC,mac_str);
	os_sprintf(status_topic[4],VOLTAGE_TOPIC,mac_str);
	os_sprintf(status_topic[5],CURRENT_TOPIC,mac_str);
	os_sprintf(info_control_topic,INFO_CONTROL_TOPIC,mac_str);
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
	if(status){
   		INFO("set_switch0: on\n");
		MQTT_Publish(&mqttClient, status_topic[0], "1", os_strlen("1"), 0,1);
	}else{
   		INFO("set_switch0: off\n");
		for(i = 0; i < 4; i++){
			MQTT_Publish(&mqttClient, status_topic[i], "0", os_strlen("0"), 0,1);
		}
	}
	
}

void ICACHE_FLASH_ATTR key0_long(void){
    INFO("start_smartconfig\n");
	smartconfig_mode = true;
    os_timer_arm(&OS_Timer_LED, 100, 1);
    start_smartconfig(smartconfig_cd);
}

void ICACHE_FLASH_ATTR key1_short(bool status){
    if(status){
   		INFO("set_switch1: on\n");
        MQTT_Publish(&mqttClient, status_topic[1], "1", os_strlen("1"), 0,1);
        MQTT_Publish(&mqttClient, status_topic[0], "1", os_strlen("1"), 0,1);
    }else{
   		INFO("set_switch1: off\n");
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
	os_strcpy(stationConf.ssid, "DC1");
	stationConf.ssid_len = os_strlen("DC1");
	wifi_softap_set_config_current(&stationConf);

	webserver_init(NULL);
}

void ICACHE_FLASH_ATTR key2_short(bool status){
    if(status){
   		INFO("set_switch2: on\n");
        MQTT_Publish(&mqttClient, status_topic[2], "1", os_strlen("1"), 0,1);
        MQTT_Publish(&mqttClient, status_topic[0], "1", os_strlen("1"), 0,1);
    }else{
   		INFO("set_switch2: off\n");
        MQTT_Publish(&mqttClient, status_topic[2], "0", os_strlen("0"), 0,1);
    }
}

void ICACHE_FLASH_ATTR key3_short(bool status){
    if(status){
   		INFO("set_switch3: on\n");
        MQTT_Publish(&mqttClient, status_topic[3], "1", os_strlen("1"), 0,1);
        MQTT_Publish(&mqttClient, status_topic[0], "1", os_strlen("1"), 0,1);
    }else{
   		INFO("set_switch3: off\n");
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

void ICACHE_FLASH_ATTR clock_handler_cd(user_timer_t timer){
	INFO("clock handler to task_id: %d\n",timer.task_id);

	int ret = 0, i = 0;
	switch (timer.task_id)
	{
		case SWITCH0_ON_TASK_ID:
			INFO("timer_up_switch0_on\n");
			set_dc1_switch(0, 1);
			break;
		case SWITCH0_OFF_TASK_ID:
			INFO("timer_up_switch0_off\n");
			set_dc1_switch(0, 0);
			break;
		case SWITCH1_ON_TASK_ID:
			INFO("timer_up_switch1_on\n");
			set_dc1_switch(1, 1);
			break;
		case SWITCH1_OFF_TASK_ID:
			INFO("timer_up_switch1_off\n");
			set_dc1_switch(1, 0);
			break;
		case SWITCH2_ON_TASK_ID:
			INFO("timer_up_switch2_on\n");
			set_dc1_switch(2, 1);
			break;
		case SWITCH2_OFF_TASK_ID:
			INFO("timer_up_switch2_off\n");
			set_dc1_switch(2, 0);
			break;
		case SWITCH3_ON_TASK_ID:
			INFO("timer_up_switch3_on\n");
			set_dc1_switch(3, 1);
			break;
		case SWITCH3_OFF_TASK_ID:
			INFO("timer_up_switch3_off\n");
			set_dc1_switch(3, 0);
			break;
		default:
			INFO("timer_task_id_error\n");
			break;
	}
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
	
	clock_init(clock_handler_cd);

	wifi_set_opmode(STATION_MODE);
	set_wifistate_cb(wifi_connect_cb, wifi_disconnect_cb);


	udp_log_init();
}

