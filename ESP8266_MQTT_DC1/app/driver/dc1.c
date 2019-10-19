/*
 * dc1 driver
 * Author: HoGC 
 */
#include "os_type.h"
#include "driver/dc1.h"
#include "driver/i2c_master.h"

//#define DC1_DEBUG_I2C

#if defined(DC1_DEBUG_I2C)
#define I2C_INFO(format, ...) os_printf(format, ##__VA_ARGS__)
#else
#define I2C_INFO( format, ... )
#endif

//#define DC1_DEBUG_KEY

#if defined(DC1_DEBUG_KEY)
#define INFO( format, ... ) os_printf( format, ## __VA_ARGS__ )
#else
#define INFO( format, ... )
#endif

uint32 keyCountTime = 0;

static uint16 electric_data[3] = {0};

static os_timer_t OS_DC1_key;

dc1_lkey_function key0LongPress = NULL;
dc1_lkey_function key1LongPress = NULL;
dc1_lkey_function key2LongPress = NULL;
dc1_lkey_function key3LongPress = NULL;
dc1_skey_function key0ShortPress = NULL;
dc1_skey_function key1ShortPress = NULL;
dc1_skey_function key2ShortPress = NULL;
dc1_skey_function key3ShortPress = NULL;


//向某寄存器中写入命令（数据），置位寄存器实现特定功能
bool ICACHE_FLASH_ATTR dc1_write_reg(uint8_t reg_addr,uint8_t val)
{
    //启动iic通信
    i2c_master_start();

    //写设备的地址
    i2c_master_writeByte(DC1_ADDRESS);
    //如未收到回应信号则停止iic通信，返回失败标识
    if(!i2c_master_checkAck()) {
	    i2c_master_stop();
        I2C_INFO("ERROR_i2c_master_checkAck: Write address -2001\r\n");
        return 0;
    }

    //写寄存器地址
    i2c_master_writeByte(reg_addr);
    //如未收到回应信号则停止iic通信，返回失败标识
    if(!i2c_master_checkAck()) {
	    i2c_master_stop();
        I2C_INFO("ERROR: Writr reg -2002 \r\n");
        return 0;
    }

    //写数据，更新各位的值
    i2c_master_writeByte(val);
    //如未收到回应信号则停止iic通信，返回失败标识
    if(!i2c_master_checkAck()) {
	    i2c_master_stop();
        I2C_INFO("ERROR_i2c_master_checkAck: Write data -2003 \r\n");
        return 0;
    }

    //停止iic通信
    i2c_master_stop();
    return 1;
}


u8 ICACHE_FLASH_ATTR dc1_read_reg(uint8_t reg_addr)
{
    u8 temp;
    //启动iic通信
    i2c_master_start();

    //写DC1作为从设备的地址
    i2c_master_writeByte(DC1_ADDRESS);
    //如未收到回应信号则停止iic通信，返回失败标识
    if(!i2c_master_checkAck()) {
	    i2c_master_stop();
        I2C_INFO("ERROR_i2c_master_checkAck: Write address -2101 \r\n");
        return 0;
    }

    //写寄存器地址
    i2c_master_writeByte(reg_addr);
    //如未收到回应信号则停止iic通信，返回失败标识
    if(!i2c_master_checkAck()) {
	    i2c_master_stop();
        I2C_INFO("ERROR_i2c_master_checkAck: Writr reg -2102 \r\n");
        return 0;
    }

    //停止iic通信
    i2c_master_stop();
    i2c_master_wait(20);
    i2c_master_start();

    //写DC1作为从设备的地址
    i2c_master_writeByte(0x41);
    //如未收到回应信号则停止iic通信，返回失败标识
    if(!i2c_master_checkAck()) {
	    i2c_master_stop();
        I2C_INFO("ERROR_i2c_master_checkAck: Writr reg -2103 \r\n");
        return 0;
    }

    temp = i2c_master_readByte();
    i2c_master_send_nack();
    i2c_master_stop();
    return temp;
}

void ICACHE_FLASH_ATTR gpio16InputConf(void) {
	WRITE_PERI_REG(PAD_XPD_DCDC_CONF,
			(READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32)0x1); // mux configuration for XPD_DCDC and rtc_gpio0 connection

	WRITE_PERI_REG(RTC_GPIO_CONF,
			(READ_PERI_REG(RTC_GPIO_CONF) & (uint32)0xfffffffe) | (uint32)0x0);	//mux configuration for out enable

	WRITE_PERI_REG(RTC_GPIO_ENABLE,
			READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32)0xfffffffe);//out disable
}

uint8 ICACHE_FLASH_ATTR gpio16InputGet(void) {
	return (uint8) (READ_PERI_REG(RTC_GPIO_IN_DATA) & 1);
}

uint8_t ICACHE_FLASH_ATTR dc1_read_gpio(){
    uint8_t read_count = 10;
    uint8_t gpio_ret=0;
    while (read_count--)
    {
        gpio_ret = dc1_read_reg(0x00);
        if(gpio_ret!=0x00){
            if(gpio_ret == dc1_read_reg(0x00)){
                return gpio_ret;
            }
        }
    }
    return 0xF8;
}

bool ICACHE_FLASH_ATTR dc1_write_gpio(uint8_t gpio_data){
    uint8_t write_count = 10;
    uint8_t gpio_ret;

    os_timer_disarm(&OS_DC1_key);

    gpio_data = gpio_data & 0xF0;
    while(write_count--){
        dc1_write_reg(0x01,gpio_data);
        gpio_ret = dc1_read_gpio();
        if(gpio_ret != 0xF8){
            if((gpio_ret & 0xF0) == gpio_data)
            {
                os_timer_arm(&OS_DC1_key, 10, 1);
                return 1;
            }
        }
    }
    os_timer_arm(&OS_DC1_key, 10, 1);
    return 0;
}

void ICACHE_FLASH_ATTR wifi_led_switch(bool bit_value){
    GPIO_OUTPUT_SET(GPIO_ID_PIN(0), ~bit_value);
}

void ICACHE_FLASH_ATTR logo_led_switch(bool bit_value){
    GPIO_OUTPUT_SET(GPIO_ID_PIN(14), ~bit_value);
}

void ICACHE_FLASH_ATTR reverse_switch(uint16_t switch_ret,uint8_t num){

    uint16_t switch_bit=0x10;
    os_printf("switch_ret:%02x",switch_ret);
    if(num == 0){
        if(dc1_write_gpio((switch_ret ^ 0x80) & 0x80)){
            if((switch_ret & 0x80) == 0x80){
                logo_led_switch(0);
            }else{
                logo_led_switch(1);
            }
        }
    }else{
        switch_bit=switch_bit<<(3-num);
        switch_ret = switch_ret ^ switch_bit;
        if(dc1_write_gpio(switch_ret & 0xF0 | 0x80)){
            logo_led_switch(1);
        }
    } 
}

u8 ICACHE_FLASH_ATTR set_switch(uint8_t num, bool bit_value){

    uint8_t set_count = 2;
    uint8_t switch_ret=0;
    uint16_t gpio_send=0;
    uint16_t switch_bit=0x10;

    os_timer_disarm(&OS_DC1_key);

    switch_ret = dc1_read_gpio();
    if(switch_ret == 0xF8){
        os_timer_arm(&OS_DC1_key, 10, 1);
        return 0;
    }
    switch_ret &= 0xf0;
    os_printf("switch_ret:%02x",switch_ret);
    if(num == 0){
        if(bit_value == 1){
            gpio_send = switch_ret | 0x80;
        }else{
            gpio_send = 0x00;
        }
    }else{
        switch_bit=switch_bit<<(3-num);
        if(bit_value == 1){
            switch_ret = switch_ret | switch_bit;
            os_printf("switch_bit:%02x",switch_ret);
            gpio_send = switch_ret & 0xf0 | 0x80;
        }else{
            switch_bit=~switch_bit;
            switch_ret = switch_ret & switch_bit;
            gpio_send = switch_ret & 0xf0 | 0x80;
        }
    }
    while (set_count--)
    {
        if(dc1_write_gpio(gpio_send)){
            if((gpio_send & 0x80) == 0x80){
                logo_led_switch(1); 
            }else{
                if(num == 0)
                    logo_led_switch(0); 
            }
            os_timer_arm(&OS_DC1_key, 10, 1);
            return 1;
        }
    }
    os_timer_arm(&OS_DC1_key, 10, 1);
    return 0;
}



void ICACHE_FLASH_ATTR DC1KeyHandle() {
    uint8_t read_count = 0;
	static uint8_t Key_Check = 0;
	static uint8_t Key_State = 0;
	static uint16_t Key_LongCheck = 0;
	uint16_t Key_press = 0;
	static uint16_t Key_Prev = 0;
    uint8_t gpio_ret=0;
    uint8_t gpio16_ret;

    gpio_ret = dc1_read_gpio();
    gpio16_ret = gpio16InputGet();
    if (gpio_ret == 0xF8 || gpio_ret == 0x07)
    {
        if(gpio16_ret){
            return;
        }
    }
    if(gpio16_ret == 0){
        gpio_ret = gpio_ret & 0xF7;
    }else{
        gpio_ret = gpio_ret | 0x08;
    }
    Key_press = gpio_ret & 0x0f;

    switch (Key_State) {
    case 0:
        if (Key_press != 0x0f) {
            Key_Prev = Key_press;
            Key_State = 1;
        }
        break;
    case 1:
        if (Key_press == Key_Prev) {
            Key_State = 2;
            INFO("KEY_DOWN\n");
            return;
        } else {
            Key_State = 0;
        }
        break;
    case 2:
        if (Key_press != Key_Prev) {
            Key_State = 0;
            Key_LongCheck = 0;
            INFO("KEY_SHORT_OR_UP\n");

            switch (Key_Prev & 0x0f)
            {
                case 0x07:
                    INFO("key0ShortPress\n");
                    reverse_switch(gpio_ret,0);
                    if(key0ShortPress!=NULL)
                        key0ShortPress(!(gpio_ret & 0x80));
                    break;
                case 0x0E:
                    INFO("key1ShortPress\n");
                    reverse_switch(gpio_ret,1);
                    if(key1ShortPress!=NULL)
                        key1ShortPress(!(gpio_ret & 0x40));
                    break;
                case 0x0D:
                    INFO("key2ShortPress\n");
                    reverse_switch(gpio_ret,2);
                    if(key2ShortPress!=NULL)
                        key2ShortPress(!(gpio_ret & 0x20));
                    break;
                case 0x0B:
                    INFO("key3ShortPress\n");
                    reverse_switch(gpio_ret,3);
                    if(key3ShortPress!=NULL)
                        key3ShortPress(!(gpio_ret & 0x10));
                    break;
            }
            return;
        }
        if (Key_press == Key_Prev) {
            Key_LongCheck++;
            if (Key_LongCheck >= (PRESS_LONG_TIME / DEBOUNCE_TIME)){
                Key_LongCheck = 0;
                Key_State = 3;
                INFO("KEY_LONG\n");
                
                switch (Key_Prev & 0x0f)
                {
                    case 0x07:
                        INFO("key0LongPress\n");
                        if(key0LongPress!=NULL)
                            key0LongPress();
                        break;
                    case 0x0E:
                        INFO("key1LongPress\n");
                        if(key1LongPress!=NULL)
                            key1LongPress();
                        break;
                    case 0x0D:
                        INFO("key2LongPress\n");
                        if(key2LongPress!=NULL)
                            key2LongPress();
                        break;
                    case 0x0B:
                        INFO("key3LongPress\n");
                        if(key3LongPress!=NULL)
                            key3LongPress();
                        break;
                }
                return;
            }
        }
        break;
    case 3:
        if (Key_press != Key_Prev) {
            Key_State = 0;
            INFO("KEY_LONG_UP\n");
            return;
        }
        break;
    }
	return;
}

/**
 * 串口回调
 */
void dc1_uart_data_handler(u8* data,u16 data_len){
	u8 i = 0;
	u32 coefficient = 0;
	u32 period = 0;
	u16 checkpack = 0;
	if (data[1] == 0x5A)
	{
		for(i=2;i<23;i++)
			checkpack += data[i];
		checkpack &= 0xFF;
		if (checkpack == data[23])
		{
			for (i = 0; i < 3; i++)
			{
				coefficient = data[2+6*i];
				coefficient <<= 8;
				coefficient |= data[3+6*i];
				coefficient <<= 8;
				coefficient |= data[4+6*i];
				period = data[5+6*i];
				period <<= 8;
				period |= data[6]+6*i;
				period <<= 8;
				period |= data[7+6*i];
				electric_data[i] = (coefficient*10)/period;
			}
		}
	}
}

void ICACHE_FLASH_ATTR get_electric_data(uint16_t *recv_data){
    uint8_t i = 0;
    for(i = 0; i < 3; i++)
        *(recv_data+i) = electric_data[i];
}


void ICACHE_FLASH_ATTR dc1_init(dc1_skey_function k0shortpress, dc1_lkey_function k0longpress,
                                dc1_skey_function k1shortpress, dc1_lkey_function k1longpress, 
                                dc1_skey_function k2shortpress, dc1_lkey_function k2longpress, 
                                dc1_skey_function k3shortpress, dc1_lkey_function k3longpress){
    key0ShortPress = k0shortpress;                                
    key0LongPress = k0longpress;

    key1ShortPress = k1shortpress;                                
    key1LongPress = k1longpress;

    key2ShortPress = k2shortpress;
    key2LongPress = k2longpress;

    key3ShortPress = k3shortpress;
    key3LongPress = k3longpress;

    gpio16InputConf();

    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(0), 1);

    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(14), 1);

    i2c_master_gpio_init();

    set_switch(0,0);

    os_timer_disarm(&OS_DC1_key);
    os_timer_setfn(&OS_DC1_key, (os_timer_func_t *)DC1KeyHandle, NULL);
    os_timer_arm(&OS_DC1_key, 30, 1);
}
