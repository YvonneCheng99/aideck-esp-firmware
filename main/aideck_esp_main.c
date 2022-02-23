/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_event.h"

#include "spi_transport.h"
#include "uart_transport.h"
#include "router.h"
#include "com.h"
#include "test.h"
#include "wifi.h"

/* The LED is connected on GPIO */
#define BLINK_GPIO 4

// TODO krri remove?
// void test_echo(int count) {
//     static spi_transport_packet_t packet;

//     printf("Testing %d max-size pings ...\n", count);

//     packet.length = ESP_PACKET_SIZE;
//     packet.data[0] = 0x01;
//     spi_transport_send(&packet);

//     int start  = xTaskGetTickCount();
//     for (int i=0; i<100; i++) {
//         packet.length = 64;
//         packet.data[0] = 0x01;
//         spi_transport_send(&packet);
//         spi_transport_receive(&packet);
//     }

//     spi_transport_receive(&packet);

//     int stop = xTaskGetTickCount();

//     float runtime = (float)(stop - start) / (float)xPortGetTickRateHz();
//     float ping_per_seconds = count / runtime;
//     printf("Done in %f ms, %f ping/s\n", runtime * 1000, ping_per_seconds);
// }

// void test_source() {
//     static spi_transport_packet_t packet;

//     printf("Testing sourcing 100 packets ...\n");

//     packet.length = 10;
//     packet.data[0] = 0x02;
//     packet.data[1] = 100;
//     packet.data[2] = 62;

//     spi_transport_send(&packet);

//     for (int i=0; i<100; i++) {
//         spi_transport_receive(&packet);
//     }
//     printf("Done!\n");
// }

// void test_sink(int count) {
//     static spi_transport_packet_t packet;

//     printf("Testing %d packet TX\n", count);

//     int start  = xTaskGetTickCount();
//     for (int i=0; i<count; i++) {
//         packet.length = ESP_PACKET_SIZE;
//         packet.data[0] = 0x00;
//         spi_transport_send(&packet);
//     }
//     int stop = xTaskGetTickCount();

//     float runtime = (float)(stop - start) / (float)xPortGetTickRateHz();
//     float pk_per_seconds = count / runtime;
//     printf("Done in %f ms, %f pk/s, %f B/s\n", runtime * 1000, pk_per_seconds, pk_per_seconds * ESP_PACKET_SIZE);
// }

int my_vprintf(const char * fmt, va_list ap) {
    int len = vprintf("Hello: ", ap);
    len += vprintf(fmt, ap);
    return len;
}



#define DEBUG_TXD_PIN (GPIO_NUM_0) // Nina 27 /SYSBOOT) => 0

int a = 1;

void app_main(void)
{
    // Menuconfig option set to DEBUG
    esp_log_level_set("*", ESP_LOG_ERROR);
    esp_log_level_set("SPI", ESP_LOG_INFO);
    esp_log_level_set("UART", ESP_LOG_INFO);
    esp_log_level_set("SYS", ESP_LOG_INFO);
    esp_log_level_set("ROUTER", ESP_LOG_DEBUG);
    esp_log_level_set("COM", ESP_LOG_INFO);
    esp_log_level_set("TEST", ESP_LOG_INFO);
    esp_log_level_set("WIFI", ESP_LOG_DEBUG);
    //esp_log_set_vprintf(my_vprintf);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BLINK_GPIO, 1);

    // Intalling GPIO ISR service so that other parts of the code can
    // setup individual GPIO interrupt routines
    gpio_install_isr_service(ESP_INTR_FLAG_EDGE);

    spi_transport_init();

    const uart_config_t uart_config = {
      .baud_rate = 115200,
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

    uart_transport_init();
    com_init();

    // TODO krri remove test
    test_init();

    wifi_init();
    router_init();

    vTaskDelay(200);

    while(1) {
        vTaskDelay(10);
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(10);
        gpio_set_level(BLINK_GPIO, 0);
    }
    esp_restart();
}
