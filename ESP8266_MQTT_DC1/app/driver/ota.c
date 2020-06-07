/*
 * ota driver
 * Author: HoGC 
 */
#include "ets_sys.h"
#include "os_type.h"
#include "c_types.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "upgrade.h"
#include "espconn.h"
#include "driver/ota.h"
#include "spi_flash.h"

//#define OTA_DEBUG_ON

#if defined(OTA_DEBUG_ON)
#define INFO( format, ... ) os_printf( format, ## __VA_ARGS__ )
#else
#define INFO( format, ... )
#endif

char host[32];
uint16_t port;
char path[208];
unsigned short re_dns;
struct espconn user_tcp_conn;

ip_addr_t addr;

upgrade_states_check_callback ota_cb;

void ICACHE_FLASH_ATTR ota_start_upgrade(const char *server_ip, uint16_t port,
		const char *path, upgrade_states_check_callback ota_check_cb) {
	char file[21]="";
	u8 bin=0;
	u8 spi_size_map=0;

	enum flash_size_map size_map = system_get_flash_size_map();
	switch (size_map) {
		case FLASH_SIZE_8M_MAP_512_512:
			spi_size_map=2;
            break;
		case FLASH_SIZE_16M_MAP_512_512:
			spi_size_map=3;
            break;
        case FLASH_SIZE_16M_MAP_1024_1024:
			spi_size_map=5;
            break;
        case FLASH_SIZE_32M_MAP_512_512:
			spi_size_map=4;
            break;
        case FLASH_SIZE_32M_MAP_1024_1024:
			spi_size_map=6;
            break;
        default:
			INFO("NOT MAP! \n\n");
            return;
    }
	uint8_t userBin = system_upgrade_userbin_check();
	switch (userBin) {
		case UPGRADE_FW_BIN1:
			bin=2;
			break;
		case UPGRADE_FW_BIN2:
			bin=1;
			break;
		default:
			INFO("Fail read system_upgrade_userbin_check! \n\n");
			return;
	}
	if(spi_size_map == 4 || spi_size_map==6){
		os_sprintf(file,"user%d.4096.new.%d.bin",bin,spi_size_map);
	}
	else if(spi_size_map == 3 || spi_size_map==5){
		os_sprintf(file,"user%d.2048.new.%d.bin",bin,spi_size_map);
	}else if(spi_size_map == 2){
		os_sprintf(file,"user%d.1024.new.%d.bin",bin,spi_size_map);
	}else{
		return;
	}
	INFO("file name: %s\n\n",file);
	
	struct upgrade_server_info* update =
			(struct upgrade_server_info *) os_zalloc(
					sizeof(struct upgrade_server_info));

	update->pespconn = (struct espconn *) os_zalloc(sizeof(struct espconn));
	os_memcpy(update->ip, server_ip, 4);
	update->port = port;
	update->check_cb = ota_check_cb;
	update->check_times = 10000;
	update->url = (uint8 *) os_zalloc(4096);

	INFO(
			"Http Server Address:%d.%d.%d.%d ,port: %d,filePath: %s,fileName: %s \n",
			IP2STR(update->ip), update->port, path, file);

	os_sprintf((char*) update->url, "GET /%s%s HTTP/1.1\r\n"
			"Host: "IPSTR":%d\r\n"
	"Connection: keep-alive\r\n"
	"\r\n", path, file, IP2STR(update->ip), update->port);

	if (system_upgrade_start(update) == false) {
		INFO(" Could not start upgrade\n");
		os_free(update->pespconn);
		os_free(update->url);os_free(update);
	} else {
		INFO(" upgrading...\n");
	}
}

uint8_t ICACHE_FLASH_ATTR OTA_StrToIP(const int8_t* str, void *ip)
{
	    int i;
	    const char * start;

	    start = str;
	    for (i = 0; i < 4; i++) {
	        char c;
	        int n = 0;
	        while (1) {
	            c = * start;
	            start++;
	            if (c >= '0' && c <= '9') {
	                n *= 10;
	                n += c - '0';
	            }
	            else if ((i < 3 && c == '.') || i == 3) {
	                break;
	            }
	            else {
	                return 0;
	            }
	        }
	        if (n >= 256) {
	            return 0;
	        }
	        ((uint8_t*)ip)[i] = n;
	    }
	    return 1;

}

//解析URL
void ICACHE_FLASH_ATTR url_parse(char *URL, char *host, unsigned short *port, char *path){

	char *PA;
	char *PB;

	memset(path, 0, sizeof(path));
	memset(host, 0, sizeof(host));
	*port = 0;

	if (!(*URL)) {
		return;
	}
	PA = URL;

	if (!strncmp(PA, "http://", strlen("http://"))) {
		PA = URL + strlen("http://");
	} else if (!strncmp(PA, "https://", strlen("https://"))) {
		PA = URL + strlen("https://");
	}

	PB = strchr(PA, '/');

	if (PB) {
		memcpy(host, PA, strlen(PA) - strlen(PB));
		if (PB + 1) {
			memcpy(path, PB + 1, strlen(PB - 1));
			if(path[strlen(PB) - 2] != '/'){
				path[strlen(PB) - 1] = '/';
				path[strlen(PB)] = 0;
			}else{
				path[strlen(PB) - 1] = 0;
			}
		}
		host[strlen(PA) - strlen(PB)] = 0;

	} else {
		memcpy(host, PA, strlen(PA));
		host[strlen(PA)] = 0;
		path[0]='/';
		path[1]='\0';
	}

	PA = strchr(host, ':');

	if (PA) {
		*port = atoi(PA + 1);
		host[strlen(host) - strlen(PA)] = 0;
	} else {
		*port = 80;
	}
}

//寻找DNS解析，并且配置
void ICACHE_FLASH_ATTR user_esp_dns_found(const char *name, ip_addr_t *ipaddr,
		void *arg) {
			
	if (ipaddr == NULL) {
		INFO("user_dns_found NULL \r\n");
		if(re_dns++ < 10){
			espconn_gethostbyname(&user_tcp_conn, host, &addr, user_esp_dns_found);
		}
		return;
	}
	u8 server_ip[4];
	re_dns = 0;
	memcpy(server_ip, ipaddr, 4);
	INFO("ip:%d.%d.%d.%d\n",server_ip[0],server_ip[1],server_ip[2],server_ip[3]);
	INFO("start_upgrade\n");
	ota_start_upgrade(server_ip, port, path, ota_cb);
}

uint8 ICACHE_FLASH_ATTR userbin_check(void) {
	uint8 ret;
	uint8 userbin;
	userbin = system_upgrade_userbin_check();
	if (userbin == UPGRADE_FW_BIN1) {
		ret = 1;
		INFO("\nupgrade_userbin:user1 \n");
	} else if (userbin == UPGRADE_FW_BIN2) {
		ret = 2;
		INFO("\nupgrade_userbin:nuser2 \n");
	} else {
		ret = 0;
		INFO("\nnot_upgrade_userbin \n");
	}
	return ret;
}

/**
 * 检查是否OTA升级过，
 * @retval 1：已升级 0：未升级 2：读取失败
 */
uint8 ICACHE_FLASH_ATTR updata_status_check(void) {
	uint8 userbin;
	uint8 updata_ret;
	uint32 updata_status;
	SpiFlashOpResult flash_ret = 0;
	uint8 upgrade_status_flash = 0;
	enum flash_size_map size_map = system_get_flash_size_map();
	switch (size_map) {
		case FLASH_SIZE_8M_MAP_512_512:
			upgrade_status_flash = 124;
            break;
		case FLASH_SIZE_16M_MAP_512_512:
			upgrade_status_flash = 124;
            break;
        case FLASH_SIZE_16M_MAP_1024_1024:
			upgrade_status_flash = 252;
            break;
        case FLASH_SIZE_32M_MAP_512_512:
			upgrade_status_flash = 124;
            break;
        case FLASH_SIZE_32M_MAP_1024_1024:
			upgrade_status_flash = 252;
            break;
        default:
			upgrade_status_flash = 124;
            return;
    }

	flash_ret = spi_flash_read(upgrade_status_flash * SPI_FLASH_SEC_SIZE, (uint32 *) &updata_status, 4);
	os_delay_us(1000);
	if (flash_ret == SPI_FLASH_RESULT_OK) {
		INFO("updata_status_read_ok\r\n");
		userbin = system_upgrade_userbin_check();
		if (userbin != updata_status) {
			updata_status = (uint32)userbin;
			INFO("upgrade_userbin_read_finish\r\n");
			spi_flash_erase_sector(upgrade_status_flash);
			flash_ret =spi_flash_write(upgrade_status_flash * SPI_FLASH_SEC_SIZE, (uint32 *)&updata_status, 4);
			if(flash_ret == SPI_FLASH_RESULT_OK){
				INFO("upgrade_userbin_write_finish\r\n");
			}
			updata_ret = 1;
		}else{
			updata_ret = 0;
		}
	}else{
		updata_ret = 2;
		INFO("updata_status_read_error\r\n");
	}
	return updata_ret;
}

/**
 * OTA升级
 * @param  url: 文件下载的网址
 * @param  ota_check_cb: OTA升级状态的回调
 * @retval None
 */
void ICACHE_FLASH_ATTR ota_upgrade(char *url,
		upgrade_states_check_callback ota_check_cb) {

	u8 server_ip[4];
	ota_cb = ota_check_cb;
	url_parse(url,host,&port,path);
	if (OTA_StrToIP(host, server_ip)) {
		INFO("url:%d.%d.%d.%d:%d%s\r\n",server_ip[0],server_ip[1],server_ip[2],server_ip[3],port,path);
		ota_start_upgrade(server_ip, port, path, ota_cb);
	}
	else { 
		INFO("url:%s:%d%s\r\n",host,port,path);
		espconn_gethostbyname(&user_tcp_conn, host, &addr, user_esp_dns_found);
	}
}
