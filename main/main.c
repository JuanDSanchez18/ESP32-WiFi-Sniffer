/*ESP32 captador de paquestes IEEE802.11, filtrado para obtener unicamente paquetes Probe Request con una cobertura
de 10 metros, también está configurado como punto de accesso. */

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

#define CHANNEL 6
#define CHANNEL_HOPPING true //if true it will scan on all channels
#define MAX_CHANNEL 13 //(only necessary if channelHopping is true)
#define HOP_INTERVAL 200//in ms (only necessary if channelHopping is true)

int ch = CHANNEL;

const char *probe_request = "40"; // Subtipo paquete en frame
int minrssi = -60; // Filtro al RSSI, Cobertura de XX metros

static esp_err_t event_handler(void *ctx, system_event_t *event);

static void Wifi_Sniffer(void);
static void sniffer(void *buff, wifi_promiscuous_pkt_type_t type);

// Recibe todas las capturas y aplica los filtros
void sniffer(void *buf, wifi_promiscuous_pkt_type_t type){
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  wifi_pkt_rx_ctrl_t rx_ctrl = (wifi_pkt_rx_ctrl_t)pkt->rx_ctrl; //Metadata header

  char subtype[2] = "";

  itoa(pkt->payload[0],subtype,16);

  strupr(subtype);

  //printf("mgmt subtype: %01x, aux: %s\n",pkt->payload[0],subtype);
  //printf ("% \n",pkt->payload[0]);

  if ((strcmp(subtype,probe_request) == 0) && (rx_ctrl.rssi > minrssi)){

	  printf("SMAC= %02X%02X%02X%02X%02X%02X,"
			  " RSSI= %02d" " \n",
			  pkt->payload[10],pkt->payload[11],pkt->payload[12],
			  pkt->payload[13],pkt->payload[14],pkt->payload[15],
			  rx_ctrl.rssi
			  );

   }
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

// Configuración ESP32
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
					.ssid = "Esp32_EStacion2",
					.ssid_len = 0,
					.password = "STUTM_Estacion2",
					.channel = 4,
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

