# ESP8266_MQTT_DC1
## 将斐讯DC1排插接入MQTT服务器

斐讯DC1智能排插，由于某些原因斐讯公司的服务器已经关闭，官方app已经无法再使用。排插失去远程控制的功能，所以价格也沦为普通排插的价格。但是可以通过自主搭建MQTT服务器实现APP的远程控制。

2. APP远程控制
2. 低成本(40RMB)
3. 可在线升级（OTA）
4. 可查看电压、电流、功率
5. 改造现成插座，简单，美观
6. 间接有了智能台灯、智能风扇......


<img src="./image/DC1.png"> 

### 使用方法
1. 搭建MQTT服务器
3. clone 项目
3. 修改 mqtt_config.h文件
    - `MQTT_HOST`MQTT服务器域名或IP
    - `MQTT_PORT`MQTT服务器端口号
    - `MQTT_USER`MQTT用户名
    - `MQTT_PASS`MQTT密码
4. 修改 Makefile 文件
    * 芯片型号带B为8Mbit  `SPI_SIZE_MAP?=2`
    * 芯片型号不带B为16Mbit  `SPI_SIZE_MAP?=5`
    * <img src="./image/型号图.jpg"  height="210" width="200"> 
5. [拆解DC1排插](https://github.com/HoGC/phicomm_dc1-esphome/tree/master/cookbook)取出控制板
6. [编译下载](https://wiki.ai-thinker.com/esp8266)
    * 使用USB转TTL下载
    * 下载前请先拉低IO0(按下白色按键)
    * <img src="./image/下载链接图.png"  height="280" width="430"> 
7. 重新上电LED慢闪即固件刷入成功
8. 将控制板安装回插排中(请将后盖盖好再插上电源)
9. 长按总开关WIFI信号灯快闪进入配网模式
10. 进行配网（ESPTouch、Airkiss）

### 使用说明
1. 任意开关打开都会触发总开关
2. USB的开关随总开关的状态
3. LOGO灯即是总开关和USB的状态
6. WIFI信号灯慢闪即正在连接WIFI
7. WIFI信号灯快闪即进入配网模式
8. WIFI信号灯常亮即成功连接WIFI 
7. 长按总开关进入配网模式，用于重新配网
8. 每次配网自动保存，可共存两个WIFI信息



### 话题说明

1. 定义文件mqtt_config.h
2. `%s`及芯片的MAC
3. 状态话题：`/dc1/{mac}/status{0-3}`
4. 控制话题：`/dc1/{mac}/control{0-3}`
5. 功率信息话题：`/dc1/{mac}/power`
6. 电压信息话题：`/dc1/{mac}/voltage`
7. 电流信息话题：`/dc1/{mac}/current`
8. 遗嘱话题：`/dc1/lwt/{mac}`
9. 出生话题：`/dc1/birth/{mac}`
10. ota升级话题：`/dc1/ota/{mac}`



### OAT升级说明

* 使用ota升级话题发送升级文件获取地址
* 消息格式`{"url"="http://yourdomain.com:9001/ota/"}`