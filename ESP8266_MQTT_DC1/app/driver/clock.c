#include "driver/clock.h"

#include "osapi.h"
#include "mem.h"
#include "sntp.h"
#include "stdlib.h"
#include "user_interface.h"

// #define CLOCK_DEBUG_ON

#if defined(CLOCK_DEBUG_ON)
#define INFO( format, ... ) os_printf( format, ## __VA_ARGS__ )
#else
#define INFO( format, ... )
#endif


static day_time_t day_time_data;
static user_timer_t user_timer[USER_TIMER_MIX];

clock_handler_cd_t clock_handler;

os_timer_t OS_Timer_Clock;

void ICACHE_FLASH_ATTR calculation_time_count(clock_time_t *time){
	time->time_count = ((time->hour*60*60) + (time->minute*60) + time->seconds);
}

uint8_t ICACHE_FLASH_ATTR verify_time_info(clock_time_t *time){

	if((time->hour < 24 ) && (time->minute < 60 ) && (time->seconds < 60 )){
		calculation_time_count(time);
		return 1;
	}else{
		return 0;
	}	
}

void ICACHE_FLASH_ATTR parse_time_count(clock_time_t *time) {
	time->hour = (time->time_count / 3600); 
	time->minute = ((time->time_count % 3600) / 60); 
	time->seconds = (time->time_count % 3600); 
}

void ICACHE_FLASH_ATTR clock_flash_write_timer(void){
	spi_flash_erase_sector(120);
    spi_flash_write(120 * 4096, (uint32 *) &user_timer, sizeof(user_timer_t)*USER_TIMER_MIX);
}

void ICACHE_FLASH_ATTR clock_flash_read_timer(void){
	uint8_t i;
	user_timer_t flash_timer[USER_TIMER_MIX];

	spi_flash_read(120 * 4096, (uint32 *) &flash_timer, sizeof(user_timer_t) * USER_TIMER_MIX);
	for (i = 0; i < USER_TIMER_MIX; i++)
	{
		if(flash_timer[i].id != (-1)){
			if((flash_timer[i].status <= 1) && (flash_timer[i].res <= 1) && (flash_timer[i].week_bit != 0x00) && (flash_timer[i].week_bit != 0xFF)){
				uint32_t time_count;
				time_count = ((flash_timer[i].time.hour*60*60) + (flash_timer[i].time.minute*60) + flash_timer[i].time.seconds);
				if(time_count == flash_timer[i].time.time_count){
					user_timer[i] = flash_timer[i];
					INFO("user_timer%d: %02d:%02d:%02d\n",i, flash_timer[i].time.hour, flash_timer[i].time.minute, flash_timer[i].time.seconds);
				}else{
					user_timer[i].id = (-1);
				}
			}else{
				user_timer[i].id = (-1);
			}
		}else{
			user_timer[i].id = (-1);
		}
		INFO("clock_flash_write_timer%d   id: %d", i, user_timer[i].id);
	}
}

//时间服务器初始化
void ICACHE_FLASH_ATTR clock_sntp_init(void){

	ip_addr_t * addr = (ip_addr_t *) os_zalloc(sizeof(ip_addr_t));

	sntp_setservername(0, "cn.ntp.org.cn");
	sntp_setservername(1, "edu.ntp.org.cn");
	ipaddr_aton("120.25.108.11", addr);
	sntp_setserver(2, addr);
	os_free(addr);
	sntp_init();
}

void ICACHE_FLASH_ATTR clock_time_parse(uint32_t timestamp, day_time_t *day_time) {

	u8 i=0;
	char *time_str;
	char str_buf[5];
 	char month_str_buf[4];
	char week_str[7][4]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
	char month_str[12][4]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

	time_str = sntp_get_real_time(timestamp);
	// INFO("time: %s",time_str);

	day_time->timestamp = timestamp;

	os_strncpy(str_buf,time_str,3);
	str_buf[3]='\0';
	for(i=0;i<7;i++){
		if(strcmp(str_buf,week_str[i])==0){
			day_time->week = i;
		}
	}

	os_strncpy(str_buf,time_str+4,3);
	str_buf[3]='\0';
	for(i=0;i<12;i++){
		if(strcmp(str_buf,month_str[i])==0){
			day_time->month = i+1;
		}
	}

	os_strncpy(str_buf,time_str+8,2);
	str_buf[2]='\0';
	day_time->day = atoi(str_buf);

	os_strncpy(str_buf,time_str+20,4);
	str_buf[4]='\0';
	day_time->year = atoi(str_buf);

	os_strncpy(str_buf,time_str+11,2);
	str_buf[2]='\0';
	day_time->time.hour = atoi(str_buf);

	os_strncpy(str_buf,time_str+14,2);
	str_buf[2]='\0';
	day_time->time.minute = atoi(str_buf);

	os_strncpy(str_buf,time_str+17,2);
	str_buf[2]='\0';
	day_time->time.seconds = atoi(str_buf);

	calculation_time_count(&(day_time->time));
}


//更新数据
uint8_t ICACHE_FLASH_ATTR clock_update(void){

	uint32_t timestamp = sntp_get_current_timestamp();
	INFO("clock_update_timestamp: %d\n",timestamp);
	if(timestamp){
		clock_time_parse(timestamp,&day_time_data);
		return 1;
	}
	return 0;
}
void ICACHE_FLASH_ATTR clock_str_to_week_bit(char *week_str, uint8_t *week_bit){
	INFO("clock_str_to_week_bit: %s\n",week_str);
	int i = 0;
	uint8_t bit = 0;
	uint8_t sun_bit = 0;
	uint8_t week_bit_buf = 0;
	uint32_t week_int;

	if(os_strcmp("today",week_str) == 0){
		*week_bit = WEEK_TODAT; 
		return;
	}
	
	week_int = atoi(week_str);
	if(week_int > 0){
		//sunday
		if(week_int%10 == 1){
			sun_bit = 0x01;
		}
		week_int = week_int/10;
		for ( i = 0; i < 6; i++)
		{
			uint8_t bit = 0;
			bit = week_int%10;
			if(bit == 1){
				week_bit_buf = week_bit_buf | (0x01 << i);
			}
			week_int = week_int/10;
		}
		*week_bit = week_bit_buf | sun_bit;
	}else{
		*week_bit = 0;
	}
}

void ICACHE_FLASH_ATTR clock_week_bit_to_str(uint8_t week_bit, char *week_str){
	INFO("clock_week_bit: %02x",week_bit);
	int i = 0;
	int bit = 0;
	int sun_bit = 0;
	uint32_t week_int;
	char week_str_buf1[15] = "%d%s";
	char week_str_buf2[15] = "";

	if(week_bit & WEEK_TODAT){
		os_strcpy(week_str, "today");
		return;
	}

	for ( i = 5; i >= 0; i--)
	{
		if(week_bit & (1<<i)){
			os_sprintf(week_str_buf2, week_str_buf1, 1, "%d%s");
		}else{
			os_sprintf(week_str_buf2, week_str_buf1, 0, "%d%s");
		}
		os_strcpy(week_str_buf1, week_str_buf2);
	}
	if(week_bit & (1<<6)){
		os_sprintf(week_str, week_str_buf1, 1, "\0");
	}else{
		os_sprintf(week_str, week_str_buf1, 0, "\0");
	}
}

//"12:00:00"
void ICACHE_FLASH_ATTR clock_str_to_time(char *time_str, clock_time_t *time){

	char str_buf[3];
	char *PA;
	char *PB;
	uint8_t len = 0;

	PA = (char *) os_strstr(time_str, ":");
	if(PA != NULL){
		len = os_strlen(time_str) - os_strlen(PA);
		os_strncpy(str_buf,time_str,len);
		str_buf[len]='\0';
		time->hour = atoi(str_buf);

		PA = PA + 1;
		PB = (char *) os_strstr(PA, ":");
		if(PB != NULL){
			len = os_strlen(PA) - os_strlen(PB);
			os_strncpy(str_buf,PA,len);
			str_buf[len]='\0';
			time->minute = atoi(str_buf);

			PB = PB + 1;
			len = os_strlen(PB);
			os_strncpy(str_buf,PB,len);
			str_buf[len]='\0';
			time->seconds = atoi(str_buf);
		}else{
			len = os_strlen(PA);
			os_strncpy(str_buf,PA,len);
			str_buf[len]='\0';
			time->minute = atoi(str_buf);
			time->seconds = 0;
		}
		INFO("time: %02d:%02d:%02d \n", time->hour, time->minute, time->seconds);
	}
}

//"12:00:00"
void ICACHE_FLASH_ATTR clock_time_to_str(clock_time_t time, char *time_str){

	os_sprintf(time_str, "%02d:%02d:%02d", time.hour, time.minute, time.seconds);
}

int ICACHE_FLASH_ATTR clock_add_today_timer(clock_time_t time, int task_id){

	return clock_add_timer(time, WEEK_TODAT, task_id);
}

int ICACHE_FLASH_ATTR clock_add_everyday_timer(clock_time_t time, int task_id){
	
	return clock_add_timer(time, WEEK_EVERY, task_id);
}

int ICACHE_FLASH_ATTR clock_get_timer(int id, user_timer_t *timer){
	INFO("clock_get_timer_to_id: %d\n", id);

	int timer_id;
	timer_id = id - TIMER_STATIC_ID;
	if((timer_id < USER_TIMER_MIX) || (user_timer[timer_id].id == id)){
		INFO("clock_get_timer_succeed\n");
		os_memcpy(timer, &user_timer[timer_id], sizeof(user_timer_t));
		return TIMER_SUCCEED;
	}else{
		INFO("clock_get_timer_nonentity\n");
		return TIMER_NONENTITY;
	}
}

int ICACHE_FLASH_ATTR clock_set_time_timer(int id, clock_time_t time){
	INFO("set_timer%d: %02d:%02d:%02d",id, time.hour, time.minute, time.seconds);
	
	if(verify_time_info(&time)){
		uint8_t i;
		int timer_id;
		timer_id = id - TIMER_STATIC_ID;
		if((timer_id < USER_TIMER_MIX) || (user_timer[timer_id].id == id)){
			for (i = 0; i < USER_TIMER_MIX; i++)
			{
				if(user_timer[i].time.time_count == time.time_count){
					if(user_timer[i].task_id == user_timer[timer_id].task_id){
						return TIMER_REPEAT;
					}
				}
			}
			user_timer[timer_id].res = 1;
			user_timer[timer_id].time = time;
			clock_flash_write_timer();
			return TIMER_SUCCEED;
		}else{
			return TIMER_NONENTITY;
		}
	}else{
		return TIMER_INFO_ERROR;
	}
	
}

int ICACHE_FLASH_ATTR clock_set_timer(int id, clock_time_t time, uint8_t week_bit){
	INFO("set_timer%d: %02d:%02d:%02d\n",id, time.hour, time.minute, time.seconds);

	if(verify_time_info(&time)){
		uint8_t i;
		int timer_id;
		timer_id = id - TIMER_STATIC_ID;
		if((timer_id < USER_TIMER_MIX) || (user_timer[timer_id].id == id)){
			for (i = 0; i < USER_TIMER_MIX; i++)
			{
				if(user_timer[i].time.time_count = time.time_count){
					if(user_timer[i].task_id == user_timer[timer_id].task_id){
						return TIMER_REPEAT;
					}
				}
			}
			user_timer[timer_id].res = 1;
			user_timer[timer_id].week_bit = week_bit;
			calculation_time_count(&time);
			user_timer[timer_id].time = time;
			clock_flash_write_timer();
			return TIMER_SUCCEED;
		}else{
			return TIMER_NONENTITY;
		}
	}else{
		return TIMER_INFO_ERROR;
	}
}

int ICACHE_FLASH_ATTR clock_add_timer(clock_time_t time, uint8_t week_bit, int task_id){

	uint8_t i;
	if(verify_time_info(&time) && (week_bit != 0)){
		for(i = 0; i < USER_TIMER_MIX; i++){
			if((user_timer[i].time.time_count == time.time_count) && (user_timer[i].task_id == task_id)){
				if(user_timer[i].week_bit == week_bit){
					INFO("TIMER_REPEAT");
					return TIMER_REPEAT;
				}
				user_timer[i].week_bit = week_bit;
				INFO("TIMER_MODIFY");
				return TIMER_MODIFY;
			}
		}
		for(i = 0; i < USER_TIMER_MIX; i++){
			if(user_timer[i].id == (-1)){
				INFO("add_timer%d: %02d:%02d:%02d\n",i, time.hour, time.minute, time.seconds);
				user_timer[i].id = TIMER_STATIC_ID + i;
				user_timer[i].status = 1;
				user_timer[i].res = 1;
				user_timer[i].week_bit = week_bit;
				user_timer[i].task_id = task_id;
				user_timer[i].time = time;

				clock_flash_write_timer();
				return TIMER_STATIC_ID + i;
			}
		}
		INFO("TIMER_FULL");
		return TIMER_FULL;
	}else{
		INFO("TIMER_INFO_ERROR");
		return TIMER_INFO_ERROR;
	}
}

int ICACHE_FLASH_ATTR clock_close_timer(int id){
	INFO("close_timer%d\n",id);

	int timer_id;
	timer_id = id - TIMER_STATIC_ID;
	if((timer_id < USER_TIMER_MIX) || (user_timer[timer_id].id == id)){
		user_timer[timer_id].status = 0;
		clock_flash_write_timer();
		return TIMER_SUCCEED;
	}else{
		return TIMER_NONENTITY;
	}
	

}

int ICACHE_FLASH_ATTR clock_open_timer(int id){
	INFO("open_timer%d\n",id);

	int timer_id;
	timer_id = id - TIMER_STATIC_ID;
	if((timer_id < USER_TIMER_MIX) || (user_timer[timer_id].id == id)){
		user_timer[timer_id].status = 1;
		user_timer[timer_id].res = 1;
		clock_flash_write_timer();
		return TIMER_SUCCEED;
	}else{
		return TIMER_NONENTITY;
	}
}

int ICACHE_FLASH_ATTR clock_delete_timer(int id){
	INFO("delete_timer%d\n",id);

	int timer_id;
	timer_id = id - TIMER_STATIC_ID;
	if((timer_id < USER_TIMER_MIX) && (user_timer[timer_id].id == id)){
		INFO("delete_timer%d\n",timer_id);
		user_timer[timer_id].id = -1;
		user_timer[timer_id].status = 0;
		user_timer[timer_id].res = 0;
		user_timer[timer_id].week_bit = 0;
		clock_flash_write_timer();
		INFO("TIMER_SUCCEED\n");
		return TIMER_SUCCEED;
	}else{
		INFO("TIMER_NONENTITY\n");
		return TIMER_NONENTITY;
	}
}

void ICACHE_FLASH_ATTR clock_timer_cb(void){
	
	static uint32_t update_count = 0;
	static uint8_t update_flag = 1;

	uint8_t i;
	uint8_t ret;
	uint32_t timestamp_time_count=0;

	day_time_data.timestamp++;
	day_time_data.time.time_count++;

	timestamp_time_count = day_time_data.timestamp % 86400;

	if(timestamp_time_count == 0){
		update_flag = 1;
		for(i = 0; i < USER_TIMER_MIX; i++){
			user_timer[i].res = 1;
			if(user_timer[i].week_bit == WEEK_TODAT){
				user_timer[i].status = 0;
			}
		}
		clock_flash_write_timer();
	}

	if(update_flag){
		ret = clock_update();
		if(ret){
			update_flag = 0;
		}
	}else if(update_count > TIME_UPDATE_COUNT){
		update_flag = 1;
		update_count = 0;
	}else{
		update_count++;
	}

	for(i = 0; i < USER_TIMER_MIX; i++)
	{
		if(user_timer[i].id != (-1)){
			if((user_timer[i].status) && (user_timer[i].res)){
				INFO("user_timer%d: %02d:%02d:%02d\n",i,user_timer[i].time.hour,user_timer[i].time.minute,user_timer[i].time.seconds);
				if((timestamp_time_count >= user_timer[i].time.time_count) && (timestamp_time_count <= (user_timer[i].time.time_count + 60))){
					clock_time_parse(day_time_data.timestamp, &day_time_data);
					if((user_timer[i].week_bit) & (1 << day_time_data.week) || (user_timer[i].week_bit == WEEK_TODAT)){
						if(clock_handler != NULL){
							user_timer[i].res = 0;
							if(user_timer[i].week_bit == WEEK_TODAT){
								user_timer[i].status = 0;
							}
							clock_handler(user_timer[i]);
							if(user_timer[i].week_bit == WEEK_TODAT){
								clock_delete_timer(user_timer[i].id);
							}
							clock_flash_write_timer();
						}
					}else{
						user_timer[i].res = 0;
						clock_flash_write_timer();
					}
				}
			}
		}
	}
}

void ICACHE_FLASH_ATTR clock_get_day_time(day_time_t *day_time){
	clock_time_parse(day_time_data.timestamp, day_time);
}

void ICACHE_FLASH_ATTR clock_get_oled_time(char *oled_str){

	static uint8_t week_flag = 7;
	clock_time_t time;
	char week_str[7][4]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

	time.time_count = day_time_data.time.time_count;

	parse_time_count(&time);
	os_sprintf(oled_str,"%02d:%02d    %s    %02d.%02d",
		time.hour, time.minute, week_str[day_time_data.week], day_time_data.month, day_time_data.day);
}

int ICACHE_FLASH_ATTR clock_get_timer_task_num(void){
	int i = 0;
	int timer_num = 0;
	for(i = 0; i < USER_TIMER_MIX; i++)
	{
		if(user_timer[i].id != (-1)){
			timer_num++;
		}
	}
	return timer_num;
}

int ICACHE_FLASH_ATTR clock_get_timer_task_info(user_timer_t *timer_info){
	int i = 0;
	int timer_num;
	for(i = 0; i < USER_TIMER_MIX; i++)
	{
		if(user_timer[i].id != (-1)){
			timer_info[timer_num] = user_timer[i];
		}else{
			timer_info[timer_num].id = (-1);
		}
	}
	return timer_num;
}

void ICACHE_FLASH_ATTR clock_get_timer_task_item_info_josn(int id, char *json_str){

	user_timer_t timer;
	char week_str[10] = "";
	char timer_itme_buf[200]= "{\n"			 	    	\
						"\t\"id\": %d,\n" 	 		   	 	\
						"\t\"status\": %d,\n" 	 			\
						"\t\"time\": \"%02d:%02d:%02d\",\n" 	\
						"\t\"week\": \"%s\",\n" 	 	 		\
						"\t\"taskId\": %d\n" 	 			\
						"}";

	clock_get_timer(id, &timer);
	clock_week_bit_to_str(timer.week_bit, week_str);
	os_sprintf(json_str, timer_itme_buf, timer.id,timer.status, timer.time.hour, timer.time.minute, timer.time.seconds, week_str, timer.task_id);
	// INFO("timer_itme: %s",json_str);
}

void ICACHE_FLASH_ATTR clock_get_timer_task_info_json(char *json_str){
	int i = 0;
	int num = 0;
	int timer_num;
	char str_buf1[1024] = "{\n"							\
							"\t\"timerNum\": %d,\n"		\
							"\t\"timerInfo\": [%s]\n"	\
							"}\n";
	char str_buf2[1024] = "";

	timer_num = clock_get_timer_task_num();
	INFO("timer_num: %d\n",timer_num);
	if(timer_num > 0){
		os_sprintf(str_buf2, str_buf1, timer_num, "%s%s");
		os_strcpy(str_buf1, str_buf2);
		for(i = 0; i < USER_TIMER_MIX; i++)
		{
			if(user_timer[i].id != (-1)){
				num++;
				char timer_itme[200];
				clock_get_timer_task_item_info_josn(user_timer[i].id, timer_itme);
				if(num < timer_num){
					os_sprintf(str_buf2, str_buf1, timer_itme, ",\n%s%s");
				}else{
					os_sprintf(str_buf2, str_buf1, timer_itme, "\0");
				}
				os_strcpy(str_buf1, str_buf2);
			}
		}
		strcpy(json_str, str_buf1);
	}else{
		os_sprintf(json_str, str_buf1, 0, "\0");
	}
	INFO("timer_info: \n%s",json_str);
}

void ICACHE_FLASH_ATTR clock_init(clock_handler_cd_t user_handler_cd){

	clock_handler = user_handler_cd;

	clock_sntp_init();

	os_memset(user_timer, 0, sizeof(user_timer_t)*USER_TIMER_MIX);
	clock_flash_read_timer();

	os_timer_disarm(&OS_Timer_Clock);
	os_timer_setfn(&OS_Timer_Clock, (os_timer_func_t *) clock_timer_cb,NULL);
	os_timer_arm(&OS_Timer_Clock, 1000, 1); 
}