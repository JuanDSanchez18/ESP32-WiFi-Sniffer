#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#define CHANNEL 1
#define CHANNEL_HOPPING true //if true it will scan on all channels
#define MAX_CHANNEL 13 //(only necessary if channelHopping is true)
#define HOP_INTERVAL 240//in ms (only necessary if channelHopping is true)

#define size 50
#define TTL 20
//
int ch = CHANNEL;

char maclist[size][13];
int countime[size];
int status[size];
int listcount = 0;



static esp_err_t event_handler(void *ctx, system_event_t *event);

static void Wifi_Sniffer(void);
static void sniffer(void *buff, wifi_promiscuous_pkt_type_t type);


const char *
wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type)
{
	switch(type) {
		case WIFI_PKT_MGMT: return "MGMT";
		case WIFI_PKT_DATA: return "DATA";
		default:
			case WIFI_PKT_MISC: return "MISC";
	}
}

//packet sniffer
void sniffer(void *buf, wifi_promiscuous_pkt_type_t type){
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  wifi_pkt_rx_ctrl_t rx_ctrl = (wifi_pkt_rx_ctrl_t)pkt->rx_ctrl; //Metadata header

  char Mac[13] = "";
  char aux[2] = "";
  char aux2[8] = "";


  itoa(pkt->payload[0],aux2,2);
  printf("mgmt subtype: %02X\n",pkt->payload[0]
 			  );

  /*for(int i = 10; i <= 15; i++){ //MAC Source Address

	  itoa(pkt->payload[i],aux,16);
	  strcat(Mac,aux);
  }
  strupr(Mac);

  //printf("LonigtudP %d, Longitud %d\n",strlen(phone),strlen(Mac));

  int count = 0;
  bool added = false;
  if (strcmp(Mac,phone) == 0){

	  if (listcount != 0 && listcount < size){
		  for(int j = 0; j < listcount; j++){ // checks if the MAC address has been added before
			  if(strcmp(Mac,maclist[j]) == 0){
				  added = true;
				  count = countime[j];
				  count++;
				  countime[j] = count;
				  //printf("%s, %d\n", Mac,count);
				  break;
			  }
		  }
	  }

	  if (!added){
		  strcpy(maclist[listcount],Mac);
		  countime[listcount] = 1;
		  count = countime[listcount];
		  listcount++;
	  }

	  if (listcount >= size){
		  //memset(maclist, 0, sizeof maclist);
		  //memset(countime, 0, sizeof countime);
		  listcount = 0;
	  }


	  printf("Type= %s, Channel= %02d, RSSI= %02d, Length= %d,"
			  " SMAC= %02X:%02X:%02X:%02X:%02X:%02X,"
			  " Time= %d\n",
			  wifi_sniffer_packet_type2str(type),
			  rx_ctrl.channel,rx_ctrl.rssi,rx_ctrl.sig_len,
			  pkt->payload[10],pkt->payload[11],pkt->payload[12],
			  pkt->payload[13],pkt->payload[14],pkt->payload[15],
			  count
			  );

   }*/
}

// Inicialización del wifi
static const wifi_country_t wifi_country = {//Set Colombia country
				.cc = "CO",
				.schan = 1,
				.nchan = 13,
				.policy = WIFI_COUNTRY_POLICY_AUTO
		};

const wifi_promiscuous_filter_t filt = {//Set promiscuous filter
					.filter_mask =  WIFI_PROMIS_FILTER_MASK_MGMT
		};

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

static void Wifi_Sniffer(void)
{
	nvs_flash_init();
	tcpip_adapter_init();
	ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	ESP_ERROR_CHECK( esp_wifi_set_country(&wifi_country)); /* set country for channel range [1, 13] */
	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_AP) );
	wifi_config_t ap_config = {
				.ap = {
					.ssid = "Esp32_Serial1",
					.ssid_len = 0,
					.password = "StuTm_Seial12020",
					.channel = 6,
					.authmode = WIFI_AUTH_WPA2_PSK,
					.ssid_hidden = 0,
					.max_connection = 4,
					.beacon_interval = 60000//100-60000
				}
			};
	ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
	ESP_ERROR_CHECK( esp_wifi_start() );
	ESP_ERROR_CHECK( esp_wifi_set_ps(WIFI_PS_NONE) );//Power mode


	//Promiscuous mode
	esp_wifi_set_promiscuous(true);
	esp_wifi_set_promiscuous_filter(&filt);
	esp_wifi_set_promiscuous_rx_cb(&sniffer);
	wifi_second_chan_t secondCh = (wifi_second_chan_t)NULL;
	esp_wifi_set_channel(ch,secondCh);
}


//Main
void app_main(void)
{

	Wifi_Sniffer();

    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    int level = 0;
    while (CHANNEL_HOPPING) {

        vTaskDelay(HOP_INTERVAL/ portTICK_PERIOD_MS);
		ch++;
		if(ch > MAX_CHANNEL){
			ch = 1;
			gpio_set_level(GPIO_NUM_2, level);
			level = !level;
		}
		wifi_second_chan_t secondCh = (wifi_second_chan_t)NULL;
		esp_wifi_set_channel(ch,secondCh);

	}
}

