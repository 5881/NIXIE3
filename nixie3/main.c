#include "ets_sys.h"
#include "user_interface.h"
#include "osapi.h"
#include "gpio.h"
#include "driver/uart.h"
//#include "driver/uart.h"
//#include "driver/spi.h"
//#include "driver/spi_overlap.h"

#include "espconn.h"
#include "mem.h"
#include "sntp.h"
#include "inc/spilib.h"
#include "inc/user_config.h"

#define CS_UP()				gpio_output_set((1 << 5), 0, (1 << 5), 0);
#define CS_DOWN()			gpio_output_set(0, (1 << 5), (1 << 5), 0);

//#define spi_send(data)	spi_mast_byte_write(SPI, data)

//#define http_port 8010
//#define ip_addr "192.168.1.10" // Alien

uint32_t time_cor=0; //поправка системного времени
uint32_t	ntp_stamp;

static struct station_config wifi_config;
struct ip_info ipConfig;

os_timer_t	sntp_timer, rtc_timer, led_timer, efect_timer;

void ICACHE_FLASH_ATTR print_connect_status();
void wifi_handle_event_cb(System_Event_t *evt);
void ICACHE_FLASH_ATTR printf_local_time(void *arg);
void ICACHE_FLASH_ATTR efect();
void ICACHE_FLASH_ATTR indicate(char *data);
void ICACHE_FLASH_ATTR user_check_sntp_stamp(void *arg){
	
	ntp_stamp = sntp_get_current_timestamp();

	if(ntp_stamp == 0){
		os_timer_arm(&sntp_timer, 1000, 0);
	} else{
		os_timer_disarm(&sntp_timer);
		os_printf("sntp: %d, %s",ntp_stamp, sntp_get_real_time(ntp_stamp));
		//time_cor=system_get_time()/1e6;
		//time format Sat Feb 15 22:16:45 2020
		//set_ind(sntp_get_real_time(ntp_stamp));
		os_printf("system time: %d\r\n",time_cor);
		
		os_timer_disarm(&sntp_timer);
        os_timer_setfn(&rtc_timer, (os_timer_func_t *)printf_local_time, NULL);
        os_timer_arm(&rtc_timer, 1000, 1);
		wifi_station_disconnect(); //disconnect
		//включаем индикацию
		//os_timer_setfn(&led_timer, (os_timer_func_t *) indicate, NULL);
		//os_timer_arm(&led_timer, 1, 1);
		
	}
}

void ICACHE_FLASH_ATTR printf_local_time(void *arg){
char real_time_str[25];
ntp_stamp++;
os_memcpy(real_time_str,sntp_get_real_time(ntp_stamp),24);
real_time_str[24]=0;
if(!(ntp_stamp%60)) efect(); 
indicate(real_time_str+11);//часы.минуты

os_printf("LOCAL: %d, %s\r\n",ntp_stamp, real_time_str);
os_printf("SYSTIME: %dus\r\n",system_get_time());
}

void ICACHE_FLASH_ATTR efect(){
	char str[5]="00:00";
	for(uint8_t i=0;i<10;i++){
		os_memset(str, 0x30+i, 5);
		indicate(str);
		os_delay_us(50000);
		}
	}

void ICACHE_FLASH_ATTR indicate(char *data){
	uint64_t temp=0;
	temp|=1<<(data[0]-0x30);
	temp<<=10;
	temp|=1<<(data[1]-0x30);
	temp<<=10;
	temp|=1<<(data[3]-0x30);
	temp<<=10;
	temp|=1<<(data[4]-0x30);
		
	CS_DOWN();
	spi_tx32(HSPI,(uint32_t)(temp>>8));
	spi_tx8(HSPI,(uint8_t)(temp&0xff));
	while(spi_busy(HSPI));
	CS_UP();
}



/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR user_init(void)
{
	// UART config
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	os_printf("SDK version:%s\r\n", system_get_sdk_version());
	os_printf("System started!!\r\n");
	//os_printf("System started!!\r\n");
    spi_init(HSPI);
    //настроим ручной cs
    gpio_init();
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
    gpio_output_set(0, 0, (1 << 5), 0);
    indicate("00:00");
    
    // wifi connect
    wifi_set_event_handler_cb(wifi_handle_event_cb);
    //os_printf("WIFI_0\r\n");
    if (wifi_set_opmode(STATION_MODE)) {
		//os_printf("WIFI_0\r\n");
	   
	    wifi_station_disconnect();
	    os_memcpy(wifi_config.ssid, SSID, sizeof(SSID));
    	os_memcpy(wifi_config.password, PASSWORD, sizeof(PASSWORD));
	    wifi_config.bssid_set=0;

    	wifi_station_set_config(&wifi_config);
    	
    	os_printf("WIFI\r\n");
	} else
		uart0_sendStr("ERROR: setting the station mode has failed.\r\n");
	

}

void user_rf_pre_init(void)
{
}

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}


void ICACHE_FLASH_ATTR print_connect_status() {
    uint8_t status=wifi_station_get_connect_status();
    switch (status) {
    case STATION_IDLE :
        uart0_sendStr("\nCurrent status is: Idle\r\n");
        break;
    case STATION_CONNECTING :
        uart0_sendStr("\nCurrent status is: Connecting\r\n");
        break;
    case STATION_WRONG_PASSWORD :
        uart0_sendStr("\nCurrent status is: Wrong password\r\n");
        break;
    case STATION_NO_AP_FOUND :
        uart0_sendStr("\nCurrent status is: No AP found\r\n");
        break;
    case STATION_CONNECT_FAIL :
        uart0_sendStr("\nCurrent status is: Connect Fail\r\n");
        break;
    case STATION_GOT_IP :
        uart0_sendStr("\nCurrent status is: Got IP\r\n");
        break;
     default:
        os_printf("Current status is: %d\r\n",status);
    }

    if ((status == STATION_GOT_IP) &&  wifi_get_ip_info(STATION_IF, &ipConfig)) {
        os_printf("ip: " IPSTR, IP2STR(&ipConfig.ip.addr));
    }
}

void wifi_handle_event_cb(System_Event_t *evt) {
    os_printf("event %x: ", evt->event);
    
    switch  (evt->event)    {
    case    EVENT_STAMODE_CONNECTED:
		uart0_sendStr("EVENT_STAMODE_CONNECTED\r\n");
        os_printf("connect to ssid %s, channel %d\r\n",
            evt->event_info.connected.ssid,
            evt->event_info.connected.channel);
        break;
    case    EVENT_STAMODE_DISCONNECTED:
		uart0_sendStr("EVENT_STAMODE_DISCONNECTED\r\n");
        os_printf("disconnect from ssid %s, reason %d\r\n",
            evt->event_info.disconnected.ssid,
            evt->event_info.disconnected.reason);

		//system_deep_sleep_instant(60000*1000);		// 60 sec
		//system_deep_sleep_set_option(2);

        break;
    case    EVENT_STAMODE_AUTHMODE_CHANGE:
		uart0_sendStr("EVENT_STAMODE_AUTHMODE_CHANGE\r\n");
        os_printf("mode: %d -> %d\r\n",
            evt->event_info.auth_change.old_mode,
            evt->event_info.auth_change.new_mode);
        break;
    case EVENT_STAMODE_GOT_IP:
		uart0_sendStr("EVENT_STAMODE_GOT_IP\r\n");
        os_printf("ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR,
            IP2STR(&evt->event_info.got_ip.ip),
            IP2STR(&evt->event_info.got_ip.mask),
            IP2STR(&evt->event_info.got_ip.gw));
        os_printf("\r\n");

        // SNTP enable
        ip_addr_t   *addr   =   (ip_addr_t  *)os_zalloc(sizeof(ip_addr_t));
        sntp_setservername(0,   "0.ru.pool.ntp.org");   //  set server  0   by  domain  name
        sntp_setservername(1,   "1.ru.pool.ntp.org");   //  set server  1   by  domain  name
        sntp_set_timezone(+3);   // set SAMT time zone
        sntp_init();
        os_free(addr);

        // Set a timer to check SNTP timestamp
        os_timer_disarm(&sntp_timer);
        os_timer_setfn(&sntp_timer, (os_timer_func_t *)user_check_sntp_stamp, NULL);
        os_timer_arm(&sntp_timer, 1000, 0);

        break;
    case EVENT_SOFTAPMODE_STACONNECTED:
		uart0_sendStr("EVENT_SOFTAPMODE_STACONNECTED\r\n");
        os_printf("station: " MACSTR "join, AID = %d\r\n",
            MAC2STR(evt->event_info.sta_connected.mac),
            evt->event_info.sta_connected.aid);
        break;
    case EVENT_SOFTAPMODE_STADISCONNECTED:
		uart0_sendStr("EVENT_SOFTAPMODE_STADISCONNECTED\r\n");
        os_printf("station: " MACSTR "leave, AID = %d\r\n",
            MAC2STR(evt->event_info.sta_disconnected.mac),
            evt->event_info.sta_disconnected.aid);
        break;
	default:
        break;
     }
     print_connect_status();
}
