/*
 * dc1 driver
 * Author: HoGC 
 */
#ifndef _DC1_H_
#define	_DC1_H_

#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"

#define LONG_PRESS_TIME_OUT 200

#define KEY_TIMER_MS                            10                          ///< Defines the timing period required for the keypad module
#define DEBOUNCE_TIME                           30
#define PRESS_LONG_TIME                         3000

typedef void (*dc1_lkey_function)(void);
typedef void (*dc1_skey_function)(bool status);


//定义DC1作为从设备地址，
#define DC1_ADDRESS	0x40  

void ICACHE_FLASH_ATTR dc1_init(dc1_skey_function k0shortpress, dc1_lkey_function k0longpress,
                                dc1_skey_function k1shortpress, dc1_lkey_function k1longpress, 
                                dc1_skey_function k2shortpress, dc1_lkey_function k2longpress, 
                                dc1_skey_function k3shortpress, dc1_lkey_function k3longpress);

void ICACHE_FLASH_ATTR reverse_switch(uint16_t switch_ret,uint8_t num);
u8 ICACHE_FLASH_ATTR set_switch(uint8_t num, bool bit_value);

void dc1_uart_data_handler(u8* data,u16 data_len);
void ICACHE_FLASH_ATTR get_electric_data(uint16_t *recv_data);

void ICACHE_FLASH_ATTR wifi_led_switch(bool bit_value);
void ICACHE_FLASH_ATTR logo_led_switch(bool bit_value);

#endif