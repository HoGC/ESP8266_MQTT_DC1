
#ifndef __CLOCK_H
#define	__CLOCK_H

#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"


#define USER_TIMER_MIX 			10          //秒

#define TIME_UPDATE_COUNT 		60*60       //秒

#define WEEK_SUN                           0x01
#define WEEK_MON                           0x02
#define WEEK_TUE     		               0x04
#define WEEK_WED                           0x08
#define WEEK_THU                           0x10
#define WEEK_FRI                           0x20
#define WEEK_SAT                           0x40
#define WEEK_TODAT                         0x80

#define WEEK_EVERY						   0x7F

#define TIMER_SUCCEED                      501		//成功
#define TIMER_REPEAT                       502		//已存在
#define TIMER_FULL                   	   503		//满
#define TIMER_NONENTITY                    504		//不存在
#define TIMER_INFO_ERROR                   505		//输入信息有误
#define TIMER_MODIFY                   	   506		//做了修改

#define TIMER_STATIC_ID 				   1000

typedef struct{
	int hour;
	int minute;
	int seconds;
	uint32_t time_count;
}clock_time_t;

typedef struct{
	int year;
	int month;
	int day;
	int week;
	clock_time_t time;
	uint32_t timestamp;
}day_time_t;

typedef struct
{
	int id;
	int status;
	int res;
	int week_bit;
	int task_id;
    clock_time_t time;
}user_timer_t; 

typedef void (*clock_handler_cd_t)(user_timer_t timer);

//初始化
void ICACHE_FLASH_ATTR clock_init(clock_handler_cd_t user_handler_cd);

//联网校准时间 已自动校准 无需调用 校准周期请修改 TIME_UPDATE_COUNT
uint8_t ICACHE_FLASH_ATTR clock_update(void);
//获取当前日期和时间
void ICACHE_FLASH_ATTR clock_get_day_time(day_time_t *day_time);
//解析时间字符串
void ICACHE_FLASH_ATTR clock_str_to_time(char *time_str, clock_time_t *time);
//解析一周重复周期   "1111100" -> 周一到周五       "today" -> 今天  
void ICACHE_FLASH_ATTR clock_str_to_week_bit(char *week_str, uint8_t *week_bit);

//关闭定时任务
int ICACHE_FLASH_ATTR clock_close_timer(int id);
 //开启定时任务
int ICACHE_FLASH_ATTR clock_open_timer(int id);
//删除定时任务
int ICACHE_FLASH_ATTR clock_delete_timer(int id);

//获取定时任务
int ICACHE_FLASH_ATTR clock_get_timer(int id, user_timer_t *user_timer);
//修改定时任务
int ICACHE_FLASH_ATTR clock_set_time_timer(int id, clock_time_t time);
//修改定时任务(带修改重复日期)
int ICACHE_FLASH_ATTR clock_set_timer(int id, clock_time_t time, uint8_t week_bit);

//添加定时任务 
int ICACHE_FLASH_ATTR clock_add_today_timer(clock_time_t time, int task_id);
//添加当天的定时
int ICACHE_FLASH_ATTR clock_add_everyday_timer(clock_time_t time, int task_id);
//添加每天的定时
int ICACHE_FLASH_ATTR clock_add_timer(clock_time_t time, uint8_t week_bit, int task_id);

//获取定时任务的信息
void ICACHE_FLASH_ATTR clock_get_timer_task_item_info_josn(int id, char *json_str);
//获取全部定时任务的信息
int ICACHE_FLASH_ATTR clock_get_timer_task_info(user_timer_t *timer_info);
//获取全部定时任务的信息
void ICACHE_FLASH_ATTR clock_get_timer_task_info_json(char *json_str);

//获取用于oled显示的字符串  "13.11    Tue    02.04"
void ICACHE_FLASH_ATTR clock_get_oled_time(char *oled_str);

#endif
