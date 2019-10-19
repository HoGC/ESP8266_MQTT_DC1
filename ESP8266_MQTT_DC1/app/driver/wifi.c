#include "driver/wifi.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "os_type.h"
#include "mem.h"
#include "user_config.h"
#include "smartconfig.h"
#include "airkiss.h"

//#define WIFI_DEBUG_ON

#if defined(WIFI_DEBUG_ON)
#define INFO( format, ... ) os_printf( format, ## __VA_ARGS__ )
#else
#define INFO( format, ... )
#endif

smartconfig_cd_t finish_cd = NULL;
wifconnect_cb_t w_connect = NULL;
wifdisconnect_cb_t w_disconnect = NULL;

LOCAL esp_udp ssdp_udp;
LOCAL struct espconn pssdpudpconn;
LOCAL os_timer_t ssdp_time_serv;
uint8_t lan_buf[200];
uint16_t lan_buf_len;
uint8 udp_sent_cnt = 0;
uint8 smartconfig_flag = 0;
bool connect_flag = 0;
os_timer_t OS_Timer_SM;
os_timer_t OS_Timer_Wifistate;

const airkiss_config_t akconf = { (airkiss_memset_fn) &memset,
		(airkiss_memcpy_fn) &memcpy, (airkiss_memcmp_fn) &memcmp, 0, };
static sm_status sm_comfig_status = SM_STATUS_TIMEOUT;


LOCAL void ICACHE_FLASH_ATTR
airkiss_wifilan_time_callback(void) {
	uint16 i;
	airkiss_lan_ret_t ret;

	if ((udp_sent_cnt++) > 30) {
		udp_sent_cnt = 0;
		os_timer_disarm(&ssdp_time_serv); //s
		//return;
	}
	ssdp_udp.remote_port = DEFAULT_LAN_PORT;
	ssdp_udp.remote_ip[0] = 255;
	ssdp_udp.remote_ip[1] = 255;
	ssdp_udp.remote_ip[2] = 255;
	ssdp_udp.remote_ip[3] = 255;
	lan_buf_len = sizeof(lan_buf);
	ret = airkiss_lan_pack(AIRKISS_LAN_SSDP_NOTIFY_CMD,
	DEVICE_TYPE, DEVICE_ID, 0, 0, lan_buf, &lan_buf_len, &akconf);
	if (ret != AIRKISS_LAN_PAKE_READY) {
		INFO("Pack lan packet error!");
		return;
	}

	ret = espconn_sendto(&pssdpudpconn, lan_buf, lan_buf_len);
	if (ret != 0) {
		INFO("UDP send error!");
	}
	INFO("Finish send notify!\n");
}

LOCAL void ICACHE_FLASH_ATTR
airkiss_wifilan_recv_callbk(void *arg, char *pdata, unsigned short len) {
	uint16 i;
	remot_info* pcon_info = NULL;

	airkiss_lan_ret_t ret = airkiss_lan_recv(pdata, len, &akconf);
	airkiss_lan_ret_t packret;

	switch (ret) {
	case AIRKISS_LAN_SSDP_REQ:
		espconn_get_connection_info(&pssdpudpconn, &pcon_info, 0);
		INFO("remote ip: %d.%d.%d.%d \r\n", pcon_info->remote_ip[0],
				pcon_info->remote_ip[1], pcon_info->remote_ip[2],
				pcon_info->remote_ip[3]);
		INFO("remote port: %d \r\n", pcon_info->remote_port);

		pssdpudpconn.proto.udp->remote_port = pcon_info->remote_port;
		os_memcpy(pssdpudpconn.proto.udp->remote_ip, pcon_info->remote_ip, 4);
		ssdp_udp.remote_port = DEFAULT_LAN_PORT;

		lan_buf_len = sizeof(lan_buf);
		packret = airkiss_lan_pack(AIRKISS_LAN_SSDP_RESP_CMD,
		DEVICE_TYPE, DEVICE_ID, 0, 0, lan_buf, &lan_buf_len, &akconf);

		if (packret != AIRKISS_LAN_PAKE_READY) {
			INFO("Pack lan packet error!");
			return;
		}

		INFO("\r\n\r\n");
		for (i = 0; i < lan_buf_len; i++)
			INFO("%c", lan_buf[i]);
		INFO("\r\n\r\n");

		packret = espconn_sendto(&pssdpudpconn, lan_buf, lan_buf_len);
		if (packret != 0) {
			INFO("LAN UDP Send err!");
		}

		break;
	default:
		INFO("Pack is not ssdq req!%d\r\n", ret);
		break;
	}
}

void ICACHE_FLASH_ATTR
airkiss_start_discover(void) {
	ssdp_udp.local_port = DEFAULT_LAN_PORT;
	pssdpudpconn.type = ESPCONN_UDP;
	pssdpudpconn.proto.udp = &(ssdp_udp);
	espconn_regist_recvcb(&pssdpudpconn, airkiss_wifilan_recv_callbk);
	espconn_create(&pssdpudpconn);

	os_timer_disarm(&ssdp_time_serv);
	os_timer_setfn(&ssdp_time_serv,
			(os_timer_func_t *) airkiss_wifilan_time_callback, NULL);
	os_timer_arm(&ssdp_time_serv, 1000, 1);		//1s
}

/**
 * Smartconfig 状态处理 
 * @param  status: 状态
 * @param  *pdata: AP数据
 * @retval None
 */
void ICACHE_FLASH_ATTR
smartconfig_done(sc_status status, void *pdata) {
	switch (status) {
	case SC_STATUS_WAIT:
		INFO("SC_STATUS_WAIT\n");
		break;
	case SC_STATUS_FIND_CHANNEL:
		INFO("SC_STATUS_FIND_CHANNEL\n");
		break;
	case SC_STATUS_GETTING_SSID_PSWD:
		INFO("SC_STATUS_GETTING_SSID_PSWD\n");
		sc_type *type = pdata;
		if (*type == SC_TYPE_ESPTOUCH) {
			INFO("SC_TYPE:SC_TYPE_ESPTOUCH\n");
		} else {
			INFO("SC_TYPE:SC_TYPE_AIRKISS\n");
		}
		break;
	case SC_STATUS_LINK:
		INFO("SC_STATUS_LINK\n");
		sm_comfig_status = SM_STATUS_GETINFO;
		struct station_config *sta_conf = pdata;

		wifi_station_set_config(sta_conf);
		wifi_station_disconnect();
		wifi_station_connect();
		break;
	case SC_STATUS_LINK_OVER:
		sm_comfig_status = SM_STATUS_FINISH;
		INFO("SC_STATUS_LINK_OVER\n");
		if (pdata != NULL) {
			//SC_TYPE_ESPTOUCH
			uint8 phone_ip[4] = { 0 };

			os_memcpy(phone_ip, (uint8*) pdata, 4);
			INFO("Phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1],
					phone_ip[2], phone_ip[3]);
		} else {
			//SC_TYPE_AIRKISS - support airkiss v2.0
			airkiss_start_discover();
		}
		smartconfig_stop();
		smartconfig_flag = 0;
		connect_flag = 0;
		os_timer_disarm(&OS_Timer_SM);	// 关闭定时器
		finish_cd(sm_comfig_status);
		os_timer_arm(&OS_Timer_Wifistate, 500, 1);  // 使能定时器
		break;
	}

}

/**
 * WIFI连接状态检查   
 * @retval None
 */
void ICACHE_FLASH_ATTR wifi_check(void) {
	uint8 getState;
	LOCAL uint8 count = 0;
	LOCAL uint8 ap_id = 0;
	struct ip_info ipConfig;
	if (smartconfig_flag != 1) {
		wifi_get_ip_info(STATION_IF, &ipConfig);
		getState = wifi_station_get_connect_status();
		//查询 ESP8266 WiFi station 接口连接 AP 的状态
		if (getState == STATION_GOT_IP && ipConfig.ip.addr != 0) {
			if (connect_flag == 0) {
				count = 0;
				connect_flag = 1;
				INFO("wifi connect!\r\n");
				os_timer_arm(&OS_Timer_Wifistate, 5000, 1);
				if(w_connect != NULL){
					w_connect();
				}
			}
		} else {
			count++;
			if (count > 10) {
				count = 0;
				ap_id = wifi_station_get_current_ap_id();
				ap_id = ++ap_id % 2;
				INFO("AP_ID : %d", ap_id);
				wifi_station_ap_change(ap_id);
			}
			if(connect_flag == 1){
				connect_flag = 0;
				INFO("wifi disconnect!\r\n");
				os_timer_arm(&OS_Timer_Wifistate, 1000, 1);
				if(w_connect != NULL){
					w_disconnect();
				}
			}

		}
	}
}

/**
 * 限制SmartConfig配置时间  
 * @retval 
 */
void ICACHE_FLASH_ATTR sm_wait_time(void) {

	LOCAL wait_wait = 0;
	if (wait_wait == 60) {
		wait_wait = 0;
		smartconfig_stop();		// 停止SmartConfig
		wifi_station_connect();
		if (sm_comfig_status != SM_STATUS_GETINFO) {
			sm_comfig_status = SM_STATUS_TIMEOUT;
		}
		smartconfig_flag = 0;
		connect_flag = 0;
		os_timer_disarm(&OS_Timer_SM);	// 关闭定时器
		finish_cd(sm_comfig_status);
		os_timer_arm(&OS_Timer_Wifistate, 500, 1);
	}
	wait_wait++;
}

/**
 * 开始Smartconfig配置  
 * @param  cd: Smartconfig状态回调
 * @retval None
 */
void ICACHE_FLASH_ATTR start_smartconfig(smartconfig_cd_t cd) {
	smartconfig_flag = 1;
	smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS); //SC_TYPE_ESPTOUCH,SC_TYPE_AIRKISS,SC_TYPE_ESPTOUCH_AIRKISS
	wifi_station_disconnect();
	wifi_set_opmode(STATION_MODE);
	finish_cd = cd;
	smartconfig_start(smartconfig_done);
	os_timer_disarm(&OS_Timer_Wifistate);	// 关闭定时器
	if(connect_flag == 1){
		if(w_disconnect != NULL){
			INFO("wifi disconnect!\r\n");
			w_disconnect();
			connect_flag = 0;
		}
	}
	os_timer_disarm(&OS_Timer_SM);	// 关闭定时器
	os_timer_setfn(&OS_Timer_SM, (os_timer_func_t *) sm_wait_time, NULL);// 设置定时器
	os_timer_arm(&OS_Timer_SM, 1000, 1);  // 使能定时器
}

/**
 * 连接目标AP
 * @param  ssid: 名字
 * @param  pass: 密码
 * @retval None
 */
void ICACHE_FLASH_ATTR wifi_connect(uint8_t* ssid, uint8_t* pass){

	struct station_config stationConf;

	INFO("WIFI_INIT\r\n");
	wifi_set_opmode_current(STATION_MODE);

	os_memset(&stationConf, 0, sizeof(struct station_config));

	os_sprintf(stationConf.ssid, "%s", ssid);
	os_sprintf(stationConf.password, "%s", pass);

	wifi_station_set_config_current(&stationConf);

	wifi_station_connect();
}

/**
 * 设置wifi连接、断开回调，并自动尝试连接以储存的多个WIFI 
 * @param  u_connect_cb: 连接回调函数
 * @param  u_disconnect_cb: 断开连接回调函数
 * @retval None
 */
void ICACHE_FLASH_ATTR set_wifistate_cb(wifconnect_cb_t u_connect_cb, wifdisconnect_cb_t u_disconnect_cb){

	w_connect = u_connect_cb;
	w_disconnect = u_disconnect_cb;

	struct station_config config[5];
	int i;
	int	count = wifi_station_get_ap_info(config);
	INFO("ap_num = %d\n", count);
	for(i = 0; i < count; i++)
	{
		INFO("AP%d\n", i+1);
		INFO("ssid : %s\n", config[i].ssid);
		INFO("password : %s\n", config[i].password);
	}

	os_timer_disarm(&OS_Timer_Wifistate);	// 关闭定时器
	os_timer_setfn(&OS_Timer_Wifistate, (os_timer_func_t *) wifi_check, NULL);// 设置定时器
	os_timer_arm(&OS_Timer_Wifistate, 500, 1);  // 使能定时器
}
