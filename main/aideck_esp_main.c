/**
 * ,---------,       ____  _ __
 * |  ,-^-,  |      / __ )(_) /_______________ _____  ___
 * | (  O  ) |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * | / ,--´  |    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *    +------`   /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * ESP deck firmware
 *
 * Copyright (C) 2022 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_event.h"
#include "discovery.h"

#include "esp_transport.h"
#include "esp_wifi.h"
#include "spi_transport.h"
#include "uart_transport.h"
#include "router.h"
#include "com.h"
#include "test.h"
#include "wifi.h"
#include "system.h"
#include "cpx.h"

/* The LED is connected on GPIO */
#define BLINK_GPIO 4
#define DEFAULT_SCAN_LIST_SIZE CONFIG_EXAMPLE_SCAN_LIST_SIZE
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;
static const char *TAG = "scan";

static esp_routable_packet_t txp;
int cpx_and_uart_vprintf(const char * fmt, va_list ap) {
    int len = vprintf(fmt, ap);

    cpxInitRoute(CPX_T_ESP32, CPX_T_STM32, CPX_F_CONSOLE, &txp.route);
    txp.dataLength = vsprintf((char*)txp.data, fmt, ap) + 1;
    espAppSendToRouterBlocking(&txp);

    return len;
}

esp_err_t event_handler(void *ctx,system_event_t *event)
{
  switch (event->event_id)
  {
    case SYSTEM_EVENT_STA_START:
        // esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip:%s",
                  ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG,"_Disconnected from access point_");
        break;
    default:
    break;
  }
    return ESP_OK;
}

#define DEBUG_TXD_PIN (GPIO_NUM_0) // Nina 27 /SYSBOOT) => 0

int a = 1;

static void initialise_wifi(void)
{
  wifi_event_group = xEventGroupCreate();
  ESP_LOGI("CYYTEST", "in initialise wifi");
  ESP_LOGI(TAG, "in initialise wifi");
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler,NULL));
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  wifi_config_t wifi_config = {
    .sta = {
        .ssid = CONFIG_EXAMPLE_SSID,
        .password = CONFIG_EXAMPLE_PASSWORD,
    },
  };
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_LOGI(TAG,"wifi sta init");
}

void m_wifi_scan(void)
{
    esp_err_t ret = 0;

    // ESP_LOGI("CYYTEST", "enter wifi_scan");
    ESP_ERROR_CHECK(esp_netif_init());
    // ESP_LOGI(TAG, "000");
    
    ret = esp_event_loop_create_default();
    //ESP_LOGI(TAG, "000%d", ret);
    //ESP_ERROR_CHECK(esp_event_loop_create_default());
    //esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    //assert(sta_netif);
    // ESP_LOGI(TAG, "1111\n");

    // wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    // ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // ESP_LOGI(TAG, "222\n");

    uint16_t number = 10;
    wifi_ap_record_t ap_info[10];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));
    // ESP_LOGI(TAG, "333");

    //ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    //ESP_ERROR_CHECK(esp_wifi_start());
    // ret = esp_wifi_start();
    // if (ret==ESP_OK) ESP_LOGI(TAG, "esp_wifi_start-ESP_OK");
    // else if (ret==ESP_ERR_WIFI_NOT_INIT) ESP_LOGI(TAG, "esp_wifi_start-ESP_ERR_WIFI_NOT_INIT");
    // else if (ret==ESP_ERR_INVALID_ARG) ESP_LOGI(TAG, "esp_wifi_start-ESP_ERR_INVALID_ARG");
    // else if (ret==ESP_ERR_NO_MEM) ESP_LOGI(TAG, "esp_wifi_start-ESP_ERR_NO_MEM");
    // else if (ret==ESP_ERR_WIFI_CONN) ESP_LOGI(TAG, "esp_wifi_start-ESP_ERR_WIFI_CONN");
    // else if (ret==ESP_FAIL) ESP_LOGI(TAG, "esp_wifi_start-ESP_FAIL");
    // else ESP_LOGI(TAG, "esp_wifi_start-other error");
    esp_wifi_scan_start(NULL, true);
    
    // ESP_LOGI(TAG, "Max AP number ap_info can hold = %u", number);

    ret = esp_wifi_scan_get_ap_records(&number, ap_info);
    // if (ret==ESP_OK) ESP_LOGI(TAG, "esp_wifi_scan_get_ap_records-ESP_OK");
    // else if (ret==ESP_ERR_WIFI_NOT_INIT) ESP_LOGI(TAG, "esp_wifi_scan_get_ap_records-ESP_ERR_WIFI_NOT_INIT");
    // else if (ret==ESP_ERR_WIFI_NOT_STARTED) ESP_LOGI(TAG, "esp_wifi_scan_get_ap_records-ESP_ERR_WIFI_NOT_STARTED");
    // else if (ret==ESP_ERR_INVALID_ARG) ESP_LOGI(TAG, "esp_wifi_scan_get_ap_records-ESP_ERR_INVALID_ARG");
    // else if (ret==ESP_ERR_NO_MEM) ESP_LOGI(TAG, "esp_wifi_scan_get_ap_records-ESP_ERR_NO_MEM");
    // else ESP_LOGI(TAG, "esp_wifi_scan_get_ap_records-other error");

    ret = esp_wifi_scan_get_ap_num(&ap_count);
    // if (ret==ESP_OK) ESP_LOGI(TAG, "esp_wifi_scan_get_ap_num-ESP_OK");
    // else if (ret==ESP_ERR_WIFI_NOT_INIT) ESP_LOGI(TAG, "esp_wifi_scan_get_ap_num-ESP_ERR_WIFI_NOT_INIT");
    // else if (ret==ESP_ERR_WIFI_NOT_STARTED) ESP_LOGI(TAG, "esp_wifi_scan_get_ap_num-ESP_ERR_WIFI_NOT_STARTED");
    // else ESP_LOGI(TAG, "esp_wifi_scan_get_ap_num-other error");

    ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, number);
    for (int i = 0; i < ap_count; i++) {
        // if (ap_info[i].ssid == "WiFi streaming example")
        char str[] = "WiFi streaming example";
        if (0 == memcmp(str,ap_info[i].ssid, sizeof(str)))
          ESP_LOGI(TAG, "SSID: %s,RSSI: %d", ap_info[i].ssid, ap_info[i].rssi);
        // ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
        // ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
        // print_auth_mode(ap_info[i].authmode);
        // if (ap_info[i].authmode != WIFI_AUTH_WEP) {
        //     print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
        // }
        // ESP_LOGI(TAG, "Channel \t\t%d", ap_info[i].primary);
    }
    return;
}

static void scan_task(void *pvParameters)
{
  while(1)
  {
    ESP_LOGI(TAG,"enter scan_task 1");
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    m_wifi_scan();
    // xEventGroupWaitBits(wifi_event_group,WIFI_CONNECTED_BIT,0,1,portMAX_DELAY);
    // ESP_LOGI(TAG,"WIFI CONNECT DONE");
    // xEventGroupClearBits(wifi_event_group,WIFI_CONNECTED_BIT);
    // wifi_ap_record_t ap_info;
    // esp_wifi_sta_get_ap_info(&ap_info);
    // ESP_LOGI(TAG,"SSID: %s,RSSI: %d",ap_info.ssid,ap_info.rssi);
    ESP_ERROR_CHECK(esp_wifi_stop());
    vTaskDelay(2000/portTICK_PERIOD_MS);  //2s扫描一次
    ESP_ERROR_CHECK(esp_wifi_start());
  }
}

void app_main(void)
{
    // Menuconfig option set to DEBUG
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("SPI", ESP_LOG_INFO);
    esp_log_level_set("UART", ESP_LOG_INFO);
    esp_log_level_set("SYS", ESP_LOG_INFO);
    esp_log_level_set("ROUTER", ESP_LOG_INFO);
    esp_log_level_set("COM", ESP_LOG_INFO);
    esp_log_level_set("TEST", ESP_LOG_INFO);
    esp_log_level_set("WIFI", ESP_LOG_INFO);
    esp_log_level_set("CYYTEST", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_log_level_set("DISCOVERY", ESP_LOG_INFO);
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BLINK_GPIO, 1);

    // Intalling GPIO ISR service so that other parts of the code can
    // setup individual GPIO interrupt routines
    gpio_install_isr_service(ESP_INTR_FLAG_EDGE);

    spi_transport_init();

    const uart_config_t uart_config = {
      .baud_rate = 576000,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, 1000, 1000, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, 0, 25, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    ESP_LOGI("SYS", "\n\n -- Starting up --\n");
    ESP_LOGI("SYS", "Minimum free heap size: %d bytes", esp_get_minimum_free_heap_size());

    espTransportInit();
    uart_transport_init();
    com_init();

    // TODO krri remove test
    test_init();
    wifi_init();

    esp_log_set_vprintf(cpx_and_uart_vprintf);
    ESP_LOGI("CYYTEST", "after system_init 000");
    router_init();

    system_init();

    discovery_init();
    //m_wifi_scan();
    ESP_LOGI("CYYTEST", "after system_init");

    // cpxPrintToConsole(LOG_TO_CRTP, "Test start with CPX !\n");
  
    //get RSSI
    //nvs_flash_init();
    //tcpip_adapter_init();
    initialise_wifi();
    xTaskCreate(&scan_task,"scan_task",4096,NULL,1,NULL);
    ESP_LOGI("CYYTEST", "after scan_task");

    while(1) {
        vTaskDelay(10);
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(10);
        gpio_set_level(BLINK_GPIO, 0);
    }
    esp_restart();
}
