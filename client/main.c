/* Прототип клиентской части для esp v 0.9
 */

#include "ets_sys.h"
#include "user_interface.h"
#include "osapi.h"
#include "gpio.h"
#include "driver/uart.h"
#include "espconn.h"
#include "mem.h"
//#include "sntp.h"
#include "inc/spilib.h"
#include "inc/user_config.h"

#define spi_tx40(data)      spi_transaction(HSPI, 0, 0, 0, 0, 32,(uint32)data, 0, 0)
#define CS_UP()				gpio_output_set((1 << 5), 0, (1 << 5), 0);
#define CS_DOWN()			gpio_output_set(0, (1 << 5), (1 << 5), 0);

#define PORT 7890
#define SERVER "192.168.0.170"

char buffer[512];
uuint32_t	ntp_stamp=1;
struct espconn *con_str_adr;
static struct station_config wifi_config;
struct ip_info ipConfig;
os_timer_t	 rtc_timer, temp_tim;
void ICACHE_FLASH_ATTR print_connect_status();
void ICACHE_FLASH_ATTR wifi_handle_event_cb(System_Event_t *evt);
void ICACHE_FLASH_ATTR indicate2(char *data);
void ICACHE_FLASH_ATTR tcp_connected(void *arg);
void ICACHE_FLASH_ATTR data_received_cb(void *arg, char *pdata, unsigned short len);
void ICACHE_FLASH_ATTR tcp_disconnected_cb(void *arg);

/***********************************************************
 * Секция tcp функций
 ***********************************************************/ 


void ICACHE_FLASH_ATTR send_data(){
     espconn_connect(con_str_adr);
	}

void ICACHE_FLASH_ATTR tcp_connected(void *arg){
    static uint32_t count=0;
    struct espconn *conn = arg;
    //os_printf("%s\n", __FUNCTION__ );
    os_sprintf(buffer, "MSG FROM ESP %d\n",count++);
    os_printf("Sending: %s", buffer);
    espconn_sent(conn, buffer, os_strlen(buffer));
}

void ICACHE_FLASH_ATTR  data_received_cb(void *arg, char *pdata, unsigned short len )
{
    struct espconn *conn = (struct espconn *)arg;
    os_printf( "recive val: %s", pdata);
    espconn_disconnect(conn);
    indicate2(pdata);
}

void ICACHE_FLASH_ATTR tcp_disconnected_cb(void *arg){
    //struct espconn *conn = (struct espconn *)arg;
    //os_printf( "%s\n", __FUNCTION__ );
    //os_printf("esp8266 disconnected\r\n");
    //wifi_station_disconnect();
}

/***********************************************************
 * Конец секции  tcp функций
 ***********************************************************/ 




void ICACHE_FLASH_ATTR indicate2(char *data){
	uint64_t temp=0;
	temp|=1<<(data[0]-0x30);
	temp<<=10;
	temp|=1<<(data[1]-0x30);
	temp<<=10;
	temp|=1<<(data[2]-0x30);
	temp<<=10;
	temp|=1<<(data[3]-0x30);
		
	CS_DOWN();
	spi_tx32(HSPI,(uint32_t)(temp>>8));
	spi_tx8(HSPI,(uint8_t)(temp&0xff));
	while(spi_busy(HSPI));
	CS_UP();
}



void ICACHE_FLASH_ATTR indicate(char *data){
	uint64_t temp=0;
	temp|=1<<(data[4]-0x30);
	temp<<=10;
	temp|=1<<(data[3]-0x30);
	temp<<=10;
	temp|=1<<(data[1]-0x30);
	temp<<=10;
	temp|=1<<(data[0]-0x30);
		
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
	int sockfd, connfd,n=0; 
    uint32_t ip=0;
    	
	// UART config
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	os_printf("SDK version:%s\r\n", system_get_sdk_version());
	os_printf("System started!!\r\n");
	spi_init(HSPI);
    //настроим ручной cs
    gpio_init();
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
    gpio_output_set(0, 0, (1 << 5), 0);
    //wifi connect
    wifi_set_event_handler_cb(wifi_handle_event_cb);
    if (wifi_set_opmode(STATION_MODE)) {
		wifi_station_disconnect();
	    os_memcpy(wifi_config.ssid, SSID, sizeof(SSID));
    	os_memcpy(wifi_config.password, PASSWORD, sizeof(PASSWORD));
	    wifi_config.bssid_set=0;
    	wifi_station_set_config(&wifi_config);
    	os_printf("WIFI\r\n");
	} else
		os_printf("ERROR: setting the station mode has failed.\r\n");
	       
    struct espconn *conn = (struct espconn *)os_zalloc(sizeof(struct espconn));
    if (conn != NULL) {
        conn->type = ESPCONN_TCP;
        conn->state = ESPCONN_NONE;
        conn->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
        conn->proto.tcp->local_port = espconn_port();
        conn->proto.tcp->remote_port = PORT;
        ip = ipaddr_addr(SERVER);
        os_memcpy(conn->proto.tcp->remote_ip,&ip,sizeof(ip));
        espconn_regist_connectcb(conn, tcp_connected);
        espconn_regist_disconcb(conn, tcp_disconnected_cb);
        espconn_regist_recvcb(conn, data_received_cb);
        } else os_printf("TCP connect failed!\r\n");
	con_str_adr=conn;	
			
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

void ICACHE_FLASH_ATTR wifi_handle_event_cb(System_Event_t *evt){
	if(evt->event==EVENT_STAMODE_GOT_IP){
		//send_data();
		os_printf("EVENT_STAMODE_GOT_IP\r\n");
        os_printf("ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR"\r\n",
            IP2STR(&evt->event_info.got_ip.ip),
            IP2STR(&evt->event_info.got_ip.mask),
            IP2STR(&evt->event_info.got_ip.gw));
        //запускаем таймер опроса температуры
        os_timer_disarm(&temp_tim);
        os_timer_setfn(&temp_tim, (os_timer_func_t *)send_data, NULL);
        os_timer_arm(&temp_tim, 1000, 1);
            
	};
	
}

