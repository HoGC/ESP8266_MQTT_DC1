/*
 * webconfig driver
 * Author: HoGC 
 */
#ifndef __WEBSERVER_H__
#define __WEBSERVER_H__

typedef void (*webserver_cd_t)(char *pusrdata, unsigned short length);

#define BURSIZE 2048

#define SERVER_PORT 80
#define SERVER_SSL_PORT 443

#define FLASH_READ_SIZE 2052
#define HTML_FILE_SIZE 2052

#define URLSize 10

typedef enum Result_Resp {
    RespFail = 0,
    RespSuc,
} Result_Resp;

typedef enum ProtocolType {
    GET = 0,
    POST,
} ProtocolType;

typedef enum _ParmType {
    SWITCH_STATUS = 0,
    INFOMATION,
    WIFI,
    SCAN,
	REBOOT,
    DEEP_SLEEP,
    LIGHT_STATUS,
    CONNECT_STATUS,
    USER_BIN
} ParmType;

typedef struct URL_Frame {
    enum ProtocolType Type;
    char pSelect[URLSize];
    char pCommand[URLSize];
    char pFilename[URLSize];
} URL_Frame;

typedef struct _rst_parm {
    ParmType parmtype;
    struct espconn *pespconn;
} rst_parm;

void webserver_init( webserver_cd_t u_webserver_cd);
void webserver_recon(void *arg, sint8 err);
void webserver_listen(void *arg);
void webserver_sent(void *arg);
void webserver_discon(void *arg);
void webserver_recv(void *arg, char *pusrdata, unsigned short length);
void data_send(void *arg, bool responseOK, char *psend);
void parse_url(char *precv, URL_Frame *purl_frame);
void webserver_wifi_connect(char *psend);
void webserver_ota_url(char *psend);


#define HTML_INDEX  "<!DOCTYPE html>\n"\
                    "<html>\n"\
                    "<head>\n"\
                    "\t<meta charset=\"UTF-8\">\n"\
                    "\t<title>OTA</title>\n"\
                    "\t<style type=\"text/css\">\n"\
                    "\t*{\n"\
                    "\t\tbox-sizing: border-box;\n"\
                    "\t}\n"\
                    "\tbody {\n"\
                    "\t\tline-height: 1.42857143;\n"\
                    "\t}\n"\
                    "\ta {\n"\
                    "\t\ttext-decoration: none;\n"\
                    "\t}\n"\
                    "\tp {\n"\
                    "\t\tmargin: 0 0 10px;\n"\
                    "\t}\n"\
                    "\th1 {\n"\
                    "\t\tdisplay: block;\n"\
                    "\t\tfont-size: 36px;\n"\
                    "\t\tmargin-block-start: 0.67em;\n"\
                    "\t\tmargin-block-end: 0.67em;\n"\
                    "\t\tmargin-inline-start: 0px;\n"\
                    "\t\tmargin-inline-end: 0px;\n"\
                    "\t\tfont-family: inherit;\n"\
                    "\t\tfont-weight: 500;\n"\
                    "\t\tline-height: 1.1;\n"\
                    "\t\tcolor: inherit;\n"\
                    "\t\tmargin-top: 20px;\n"\
                    "\t\tmargin-bottom: 10px;\n"\
                    "\t}\n"\
                    "\t.box {\n"\
                    "\t\tposition: absolute;\n"\
                    "\t\ttop: 50%;\n"\
                    "\t\tleft: 50%;\n"\
                    "\t\tmargin: -150px 0 0 -150px;\n"\
                    "\t\twidth: 300px;\n"\
                    "\t\theight: 300px;\n"\
                    "\t}\n"\
                    "\t.text-center {\n"\
                    "\t\ttext-align: center;\n"\
                    "\t}\n"\
                    "\t.btn {\n"\
                    "\t\tdisplay: inline-block;\n"\
                    "\t\tpadding: 6px 12px;\n"\
                    "\t\tmargin-bottom: 0;\n"\
                    "\t\tfont-size: 14px;\n"\
                    "\t\tfont-weight: 400;\n"\
                    "\t\tline-height: 1.42857143;\n"\
                    "\t\ttext-align: center;\n"\
                    "\t\twhite-space: nowrap;\n"\
                    "\t\tvertical-align: middle;\n"\
                    "\t\ttouch-action: manipulation;\n"\
                    "\t\tcursor: pointer;\n"\
                    "\t\tuser-select: none;\n"\
                    "\t\tbackground-image: none;\n"\
                    "\t\tborder: 1px solid transparent;\n"\
                    "\t\tborder-radius: 4px;\n"\
                    "\t}\n"\
                    "\t.btn-success{\n"\
                    "\t\tcolor:#fff;\n"\
                    "\t\tbackground-color:#5cb85c;\n"\
                    "\t\tborder-color:#4cae4c\n"\
                    "\t}\n"\
                    "\t.btn-success:hover {\n"\
                    "\t\tcolor: #fff;\n"\
                    "\t\tbackground-color: #449d44;\n"\
                    "\t\tborder-color: #398439;\n"\
                    "\t}\n"\
                    "\t.btn-primary{\n"\
                    "\t\tcolor:#fff;\n"\
                    "\t\tbackground-color:#337ab7;\n"\
                    "\t\tborder-color:#2e6da4\n"\
                    "\t}\n"\
                    "\t.btn-primary:hover{\n"\
                    "\t\tcolor:#fff;\n"\
                    "\t\tbackground-color:#286090;\n"\
                    "\t\tborder-color:#204d74\n"\
                    "\t}\n"\
                    "\t</style>\n"\
                    "</head>\n"\
                    "<body>\n"\
                    "\t<div class=\"box\">\n"\
                    "\t\t<h1 class=\"text-center\">WebServer</h1>\n"\
                    "\t\t<br>\n"\
                    "\t\t<a href=\"wifi\" class=\"btn btn-success\" style=\"width: 300px;\">WIFI配置</a>\n"\
                    "\t\t</br>\n"\
                    "\t\t</br>\n"\
                    "\t\t<a href=\"ota\" class=\"btn btn-primary\" style=\"width: 300px;\">OTA升级</a>\n"\
                    "\t\t<p align=center><font size=\"0.7\" color=\"#333333\">HoGC</font></p>\n"\
                    "\t</div>\n"\
                    "</body>\n"\
                    "</html>"


#define OTA_HTML_INDEX  "<!DOCTYPE html>\n"\
                        "<html>\n"\
                        "<head>\n"\
                        "\t<meta charset=\"UTF-8\">\n"\
                        "\t<title>OTA</title>\n"\
                        "\t<style type=\"text/css\">\n"\
                        "\t* {\n"\
                        "\t\tbox-sizing: border-box;\n"\
                        "\t}\n"\
                        "\tbody {\n"\
                        "\t\tline-height: 1;\n"\
                        "\t}\n"\
                        "\ta {\n"\
                        "\t\ttext-decoration: none;\n"\
                        "\t}\n"\
                        "\tp {\n"\
                        "\t\tmargin: 0 0 10px;\n"\
                        "\t}\n"\
                        "\th1 {\n"\
                        "\t\tdisplay: block;\n"\
                        "\t\tfont-size: 36px;\n"\
                        "\t\tmargin-block-start: 0.67em;\n"\
                        "\t\tmargin-block-end: 0.67em;\n"\
                        "\t\tmargin-inline-start: 0px;\n"\
                        "\t\tmargin-inline-end: 0px;\n"\
                        "\t\tfont-family: inherit;\n"\
                        "\t\tfont-weight: 500;\n"\
                        "\t\tline-height: 1.1;\n"\
                        "\t\tcolor: inherit;\n"\
                        "\t\tmargin-top: 20px;\n"\
                        "\t\tmargin-bottom: 20px\n"\
                        "\t}\n"\
                        "\t.box {\n"\
                        "\t\tposition: absolute;\n"\
                        "\t\ttop: 50%;\n"\
                        "\t\tleft: 50%;\t\tmargin: -150px 0 0 -150px;\n"\
                        "\t\twidth: 300px;\n"\
                        "\t\theight: 300px\n"\
                        "\t}\n"\
                        "\t.text-center {\n"\
                        "\t\ttext-align: center;\n"\
                        "\t}\n"\
                        "\t.form-control {\n"\
                        "\t\tdisplay: block;\n"\
                        "\t\twidth: 100%;\n"\
                        "\t\theight: 34px;\n"\
                        "\t\tpadding: 6px 12px;\n"\
                        "\t\tfont-size: 14px;\n"\
                        "\t\tline-height: 1.42857143;\n"\
                        "\t\tcolor: #555;\n"\
                        "\t\tbackground-color: #fff;\n"\
                        "\t\tbackground-image: none;\n"\
                        "\t\tborder: 1px solid #ccc;\n"\
                        "\t\tborder-radius: 4px;\n"\
                        "\t\tbox-shadow: inset 0 1px 1px rgba(0,0,0,.075);\n"\
                        "\t\ttransition: border-color ease-in-out .15s,box-shadow ease-in-out .15s\n"\
                        "\t}\n"\
                        "\t.btn {\n"\
                        "\t\tdisplay: inline-block;\n"\
                        "\t\tpadding: 6px 12px;\n"\
                        "\t\tmargin-bottom: 0;\n"\
                        "\t\tfont-size: 14px;\n"\
                        "\t\tfont-weight: 400;\n"\
                        "\t\tline-height: 1.42857143;\n"\
                        "\t\ttext-align: center;\n"\
                        "\t\twhite-space: nowrap;\n"\
                        "\t\tvertical-align: middle;\n"\
                        "\t\ttouch-action: manipulation;\n"\
                        "\t\tcursor: pointer;\n"\
                        "\t\tuser-select: none;\n"\
                        "\t\tbackground-image: none;\n"\
                        "\t\tborder: 1px solid transparent;\n"\
                        "\t\tborder-radius: 4px\n"\
                        "\t}\n"\
                        "\t.btn-primary{\n"\
                        "\t\tcolor:#fff;\n"\
                        "\t\tbackground-color:#337ab7;\n"\
                        "\t\tborder-color:#2e6da4\n"\
                        "\t}\n"\
                        "\t.btn-primary:hover{\n"\
                        "\t\tcolor:#fff;\n"\
                        "\t\tbackground-color:#286090;\n"\
                        "\t\tborder-color:#204d74\n"\
                        "\t}\n"\
                        "\t</style>\n"\
                        "</head>\n"\
                        "<body>\n"\
                        "\t<div class=\"box\">\n"\
                        "\t\t<form action=\"ota\" method=\"post\">\n"\
                        "\t\t\t<div class=\"form-group\">\n"\
                        "\t\t\t\t<h1 class=\"text-center\">ESP OTA 升级</h1>\n"\
                        "\t\t\t\t<input type=\"text\" class=\"form-control\" id=\"url\" name=\"otaurl\" placeholder=\"http://yourdomain.com:9001/ota/\">\n"\
                        "\t\t\t</div>\n"\
                        "\t\t\t<br>\n"\
                        "\t\t\t<button type=\"submit\" name=\"submitOK\" class=\"btn btn-primary\" style=\"width: 300px;\">开始升级</button>\n"\
                        "\t\t</form>\t<p align=center><font size=\"1\" color=\"#333333\">HoGC</font></p>\n"\
                        "\t</div>\n"\
                        "</body>\n"\
                        "</html>"

#define WIFI_HTML_INDEX     "<!DOCTYPE html>\n"\
                            "<html>\n"\
                            "<head>\n"\
                            "\t<meta charset=\"UTF-8\">\n"\
                            "\t<title>OTA</title>\n"\
                            "\t<style type=\"text/css\">\n"\
                            "\t* {\n"\
                            "\t\tbox-sizing: border-box;\n"\
                            "\t}\n"\
                            "\tbody {\n"\
                            "\t\tline-height: 1;\n"\
                            "\t}\n"\
                            "\ta {\n"\
                            "\t\ttext-decoration: none;\n"\
                            "\t}\n"\
                            "\tp {\n"\
                            "\t\tmargin: 0 0 10px;\n"\
                            "\t}\n"\
                            "\th1 {\n"\
                            "\t\tdisplay: block;\n"\
                            "\t\tfont-size: 36px;\n"\
                            "\t\tmargin-block-start: 0.67em;\n"\
                            "\t\tmargin-block-end: 0.67em;\n"\
                            "\t\tmargin-inline-start: 0px;\n"\
                            "\t\tmargin-inline-end: 0px;\n"\
                            "\t\tfont-family: inherit;\n"\
                            "\t\tfont-weight: 500;\n"\
                            "\t\tline-height: 1.1;\n"\
                            "\t\tcolor: inherit;\n"\
                            "\t\tmargin-top: 20px;\n"\
                            "\t\tmargin-bottom: 20px\n"\
                            "\t}\n"\
                            "\t.box {\n"\
                            "\t\tposition: absolute;\n"\
                            "\t\ttop: 50%;\n"\
                            "\t\tleft: 50%;\t\tmargin: -150px 0 0 -150px;\n"\
                            "\t\twidth: 300px;\n"\
                            "\t\theight: 300px\n"\
                            "\t}\n"\
                            "\t.text-center {\n"\
                            "\t\ttext-align: center;\n"\
                            "\t}\n"\
                            "\t.form-control {\n"\
                            "\t\tdisplay: block;\n"\
                            "\t\twidth: 100%;\n"\
                            "\t\theight: 34px;\n"\
                            "\t\tpadding: 6px 12px;\n"\
                            "\t\tfont-size: 14px;\n"\
                            "\t\tline-height: 1.42857143;\n"\
                            "\t\tcolor: #555;\n"\
                            "\t\tbackground-color: #fff;\n"\
                            "\t\tbackground-image: none;\n"\
                            "\t\tborder: 1px solid #ccc;\n"\
                            "\t\tborder-radius: 4px;\n"\
                            "\t\tbox-shadow: inset 0 1px 1px rgba(0,0,0,.075);\n"\
                            "\t\ttransition: border-color ease-in-out .15s,box-shadow ease-in-out .15s\n"\
                            "\t}\n"\
                            "\t.btn {\n"\
                            "\t\tdisplay: inline-block;\n"\
                            "\t\tpadding: 6px 12px;\n"\
                            "\t\tmargin-bottom: 0;\n"\
                            "\t\tfont-size: 14px;\n"\
                            "\t\tfont-weight: 400;\n"\
                            "\t\tline-height: 1.42857143;\n"\
                            "\t\ttext-align: center;\n"\
                            "\t\twhite-space: nowrap;\n"\
                            "\t\tvertical-align: middle;\n"\
                            "\t\ttouch-action: manipulation;\n"\
                            "\t\tcursor: pointer;\n"\
                            "\t\tuser-select: none;\n"\
                            "\t\tbackground-image: none;\n"\
                            "\t\tborder: 1px solid transparent;\n"\
                            "\t\tborder-radius: 4px\n"\
                            "\t}\n"\
                            "\t.btn-success{\n"\
                            "\t\tcolor:#fff;\n"\
                            "\t\tbackground-color:#5cb85c;\n"\
                            "\t\tborder-color:#4cae4c\n"\
                            "\t}\n"\
                            "\t.btn-success:hover {\n"\
                            "\t\tcolor: #fff;\n"\
                            "\t\tbackground-color: #449d44;\n"\
                            "\t\tborder-color: #398439;\n"\
                            "\t}\n"\
                            "\t</style>\n"\
                            "</head>\n"\
                            "<body>\n"\
                            "\t<div class=\"box\">\n"\
                            "\t\t<form action=\"wifi\" method=\"post\">\n"\
                            "\t\t\t<div class=\"form-group\">\n"\
                            "\t\t\t<br>\n"\
                            "\t\t\t\t<h1 class=\"text-center\">WIFI配置</h1>\n"\
                            "\t\t\t\t<input type=\"text\" class=\"form-control\" id=\"ssid\" name=\"ssid\" placeholder=\"WIFI名称\">\n"\
                            "\t\t\t<br>\n"\
                            "\t\t\t\t<input type=\"text\" class=\"form-control\" id=\"password\" name=\"password\" placeholder=\"WIFI密码\">\n"\
                            "\t\t\t</div>\n"\
                            "\t\t\t<br>\n"\
                            "\t\t\t<button type=\"submit\" name=\"submitOK\" class=\"btn btn-success\" style=\"width: 300px;\">确定</button>\n"\
                            "\t\t</form>\t<p align=center><font size=\"1\" color=\"#333333\">HoGC</font></p>\n"\
                            "\t</div>\n"\
                            "</body>\n"\
                            "</html>"

#define SCCESS_HTML_INDEX   "<!DOCTYPE html>\n"\
                            "<html>\n"\
                            "<head>\n"\
                            "\t<meta charset=\"UTF-8\">\n"\
                            "\t<title>OTA</title>\n"\
                            "\t<style type=\"text/css\">\n"\
                            "\t* {\n"\
                            "\t\tbox-sizing: border-box;\n"\
                            "\t}\n"\
                            "\tbody {\n"\
                            "\t\tline-height: 1;\n"\
                            "\t}\n"\
                            "\ta {\n"\
                            "\t\ttext-decoration: none;\n"\
                            "\t}\n"\
                            "\tp {\n"\
                            "\t\tmargin: 0 0 10px;\n"\
                            "\t}\n"\
                            "\th1 {\n"\
                            "\t\tdisplay: block;\n"\
                            "\t\tfont-size: 36px;\n"\
                            "\t\tmargin-block-start: 0.67em;\n"\
                            "\t\tmargin-block-end: 0.67em;\n"\
                            "\t\tmargin-inline-start: 0px;\n"\
                            "\t\tmargin-inline-end: 0px;\n"\
                            "\t\tfont-family: inherit;\n"\
                            "\t\tfont-weight: 500;\n"\
                            "\t\tline-height: 1.1;\n"\
                            "\t\tcolor: inherit;\n"\
                            "\t\tmargin-top: 20px;\n"\
                            "\t\tmargin-bottom: 20px\n"\
                            "\t}\n"\
                            "\t.box {\n"\
                            "\t\tposition: absolute;\n"\
                            "\t\ttop: 50%;\n"\
                            "\t\tleft: 50%;\t\tmargin: -150px 0 0 -150px;\n"\
                            "\t\twidth: 300px;\n"\
                            "\t\theight: 300px\n"\
                            "\t}\n"\
                            "\t.text-center {\n"\
                            "\t\ttext-align: center;\n"\
                            "\t}\n"\
                            "\t.btn {\n"\
                            "\t\tdisplay: inline-block;\n"\
                            "\t\tpadding: 6px 12px;\n"\
                            "\t\tmargin-bottom: 0;\n"\
                            "\t\tfont-size: 14px;\n"\
                            "\t\tfont-weight: 400;\n"\
                            "\t\tline-height: 1.42857143;\n"\
                            "\t\ttext-align: center;\n"\
                            "\t\twhite-space: nowrap;\n"\
                            "\t\tvertical-align: middle;\n"\
                            "\t\ttouch-action: manipulation;\n"\
                            "\t\tcursor: pointer;\n"\
                            "\t\tuser-select: none;\n"\
                            "\t\tbackground-image: none;\n"\
                            "\t\tborder: 1px solid transparent;\n"\
                            "\t\tborder-radius: 4px\n"\
                            "\t}\n"\
                            "\t.btn-primary{\n"\
                            "\t\tcolor:#fff;\n"\
                            "\t\tbackground-color:#337ab7;\n"\
                            "\t\tborder-color:#2e6da4\n"\
                            "\t}\n"\
                            "\t.btn-primary:hover{\n"\
                            "\t\tcolor:#fff;\n"\
                            "\t\tbackground-color:#286090;\n"\
                            "\t\tborder-color:#204d74\n"\
                            "\t}\n"\
                            "\t</style>\n"\
                            "</head>\n"\
                            "<body>\n"\
                            "\t<div class=\"box\">\n"\
                            "\t\t<h1 class=\"text-center\">提交成功</h1>\n"\
                            "\t\t<br>\n"\
                            "\t\t<button class=\"btn btn-primary\" style=\"width: 300px;\">确定</button>\n"\
                            "\t\t<p align=center><font size=\"1\" color=\"#333333\">HuangoGC</font></p>\n"\
                            "\t</div>\n"\
                            "</body>\n"\
                            "</html>"

#endif
