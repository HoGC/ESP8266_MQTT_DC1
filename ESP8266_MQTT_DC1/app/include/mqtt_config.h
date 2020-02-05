#ifndef __MQTT_CONFIG_H__
#define __MQTT_CONFIG_H__

/*DEFAULT CONFIGURATIONS*/

#define MQTT_HOST			"mqtt.yourdomain.com"       //HOST or "mqtt.yourdomain.com"
#define MQTT_PORT			1883
#define MQTT_BUF_SIZE		2048
#define MQTT_KEEPALIVE		120	            /*second*/

#define MQTT_USER			"username"
#define MQTT_PASS			"password"


#define OTA_TOPIC           "/esp8266/ota/%s"       //ota升级话题
#define LWT_TOPIC           "/dc1/lwt/%s"           //遗嘱话题
#define BIRTH_TOPIC         "/dc1/birth/%s"         //出生话题

#define STATUS_TOPIC        "/dc1/%s/status%d"      //状态话题
#define CONTROL_TOPIC       "/dc1/%s/control%d"     //控制话题

#define INFO_CONTROL_TOPIC  "/dc1/%s/info"          //信息控制话题

#define POWER_TOPIC         "/dc1/%s/power"         //功率信息话题
#define VOLTAGE_TOPIC       "/dc1/%s/voltage"       //电压信息话题
#define CURRENT_TOPIC       "/dc1/%s/current"       //电流信息话题

#define LWT_MESSAGE         "offline"

#define MQTT_RECONNECT_TIMEOUT 	5	        /*second*/

#define DEFAULT_SECURITY	0
#define QUEUE_BUFFER_SIZE		 		2048

//#define PROTOCOL_NAMEv31	                /*MQTT version 3.1 compatible with Mosquitto v0.15*/
#define PROTOCOL_NAMEv311			        /*MQTT version 3.11 compatible with https://eclipse.org/paho/clients/testing/*/

#endif // __MQTT_CONFIG_H__
