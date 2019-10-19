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


#define HTML_INDEX  "<!DOCTYPE html>\n"\
                    "<html>\n"\
                    "<head>\n"\
                    "\t<meta charset=\"utf-8\">\n"\
                    "\t<title>OTA</title>\n"\
                    "\t<link rel=\"stylesheet\" href=\"https://cdn.staticfile.org/twitter-bootstrap/3.3.7/css/bootstrap.min.css\">\n"\
                    "\t<style type=\"text/css\">\n"\
                    "\t.box {\n"\
                    "\t\tposition: absolute;\n"\
                    "\t\ttop: 50%;\n"\
                    "\t\tleft: 50%;"\
                    "\t\tmargin: -150px 0 0 -150px;\n"\
                    "\t\twidth: 300px;\n"\
                    "\t\theight: 300px;\n"\
                    "\t}\n"\
                    "\t</style>\n"\
                    "</head>\n"\
                    "<body>\n"\
                    "\t<div class=\"box\">\n"\
                    "\t\t<form>\n"\
                    "\t\t\t<div class=\"form-group\">\n"\
                    "\t\t\t\t<h1 class=\"text-center\">ESP OTA 升级</h1>\n"\
                    "\t\t\t\t<input type=\"password\" class=\"form-control\" id=\"exampleInputPassword1\" placeholder=\"http://yourdomain.com:9001/ota/\">\n"\
                    "\t\t\t</div>\n"\
                    "\t\t\t<button type=\"submit\" class=\"btn btn-default\" style=\"width: 300px;\">开始升级</button>\n"\
                    "\t\t</form>"\
                    "\t</div>\n"\
                    "</body>\n"\
                    "</html>"

#endif /* APP_INCLUDE_MODULES_OTA_H_ */