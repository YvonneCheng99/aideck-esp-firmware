#include "com.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

#include "router.h"

// TODO krri Break up transport from functionality


#define ESP_WIFI_CTRL_QUEUE_LENGTH (2)
#define ESP_WIFI_CTRL_QUEUE_SIZE (sizeof(esp_routable_packet_t))
#define ESP_PM_QUEUE_LENGTH (2)
#define ESP_PM_QUEUE_SIZE (sizeof(esp_routable_packet_t))
#define ESP_APP_QUEUE_LENGTH (2)
#define ESP_APP_QUEUE_SIZE (sizeof(esp_routable_packet_t))
#define ESP_TEST_QUEUE_LENGTH (2)
#define ESP_TEST_QUEUE_SIZE (sizeof(esp_routable_packet_t))

static xQueueHandle espWiFiCTRLQueue;
static xQueueHandle espPMQueue;
static xQueueHandle espAPPQueue;
static xQueueHandle espTESTQueue;

static esp_routable_packet_t rxp;

// This is probably too big, but let's keep things simple....
#define ESP_ROUTER_TX_QUEUE_LENGTH (4)
#define ESP_ROUTER_RX_QUEUE_LENGTH (4)
#define ESP_ROUTER_QUEUE_SIZE (sizeof(esp_routable_packet_t))

static xQueueHandle espRxQueue;
static xQueueHandle espTxQueue;

static xSemaphoreHandle transportSendLock;
static StaticSemaphore_t transportSendLockBuffer;

static const int START_UP_RX_TASK = BIT0;
static EventGroupHandle_t startUpEventGroup;

static void com_rx(void* _param) {
  xEventGroupSetBits(startUpEventGroup, START_UP_RX_TASK);
  while (1) {
    ESP_LOGD("COM", "Waiting for packet");
    xQueueReceive(espRxQueue, &rxp, (TickType_t) portMAX_DELAY);
    ESP_LOGD("COM", "Received packet for 0x%02X", rxp.route.destination);
    ESP_LOG_BUFFER_HEX_LEVEL("COM", &rxp, 10, ESP_LOG_DEBUG);
    switch (rxp.route.function) {
      case TEST:
        xQueueSend(espTESTQueue, &rxp, (TickType_t) portMAX_DELAY);
        break;
      case WIFI_CTRL:
        xQueueSend(espWiFiCTRLQueue, &rxp, (TickType_t) portMAX_DELAY);
        break;
      default:
        ESP_LOGW("COM", "Cannot handle 0x%02X", rxp.route.function);
    }
  }
}

void com_init() {
  espWiFiCTRLQueue = xQueueCreate(ESP_WIFI_CTRL_QUEUE_LENGTH, ESP_WIFI_CTRL_QUEUE_SIZE);
  espPMQueue = xQueueCreate(ESP_PM_QUEUE_LENGTH, ESP_PM_QUEUE_SIZE);
  espAPPQueue = xQueueCreate(ESP_APP_QUEUE_LENGTH, ESP_APP_QUEUE_SIZE);
  espTESTQueue = xQueueCreate(ESP_TEST_QUEUE_LENGTH, ESP_TEST_QUEUE_SIZE);

  espRxQueue = xQueueCreate(ESP_ROUTER_RX_QUEUE_LENGTH, ESP_ROUTER_QUEUE_SIZE);
  espTxQueue = xQueueCreate(ESP_ROUTER_TX_QUEUE_LENGTH, ESP_ROUTER_QUEUE_SIZE);

  transportSendLock = xSemaphoreCreateMutexStatic(&transportSendLockBuffer);
  configASSERT(transportSendLock);

  startUpEventGroup = xEventGroupCreate();
  xEventGroupClearBits(startUpEventGroup, START_UP_RX_TASK);
  xTaskCreate(com_rx, "COM RX", 10000, NULL, 1, NULL);
  xEventGroupWaitBits(startUpEventGroup,
                      START_UP_RX_TASK,
                      pdTRUE, // Clear bits before returning
                      pdTRUE, // Wait for all bits
                      portMAX_DELAY);

  ESP_LOGI("COM", "Initialized");
}

void com_receive_test_blocking(esp_routable_packet_t * packet) {
  xQueueReceive(espTESTQueue, packet, (TickType_t) portMAX_DELAY);
}

void com_receive_wifi_ctrl_blocking(esp_routable_packet_t * packet) {
  xQueueReceive(espWiFiCTRLQueue, packet, (TickType_t) portMAX_DELAY);
}

void com_send_blocking(esp_routable_packet_t * packet) {
  xQueueSend(espTxQueue, packet, (TickType_t) portMAX_DELAY);
}

void com_router_post_packet(const uint8_t* data, const uint16_t dataLen) {
  static esp_routable_packet_t txBuffer;

  assert(dataLen <= ESP_PACKET_SIZE);

  xSemaphoreTake(transportSendLock, portMAX_DELAY);

  CPXRoutablePacket_t* rxp = (CPXRoutablePacket_t*)data;

  txBuffer.length = dataLen - CPX_ROUTING_INFO_SIZE;
  // TOOD krri unpack? Both esp_routable_packet_t and CPXRoutablePacket_t are packed so this is fine for now. esp_routable_packet_t should be changed to not packed though.
  txBuffer.route = rxp->route;
  memcpy(txBuffer.data, rxp->data, txBuffer.length);
  xSemaphoreGive(transportSendLock);

  xQueueSend(espRxQueue, &txBuffer, portMAX_DELAY);
}

uint16_t com_router_get_packet(uint8_t* data) {
  static esp_routable_packet_t rxBuffer;

  xQueueReceive(espTxQueue, &rxBuffer, portMAX_DELAY);
  // TODO krri pack
  CPXRoutablePacket_t* txp = (CPXRoutablePacket_t*)data;

  txp->route = rxBuffer.route;
  memcpy(txp->data, rxBuffer.data, rxBuffer.length);

  return rxBuffer.length + CPX_ROUTING_INFO_SIZE;
}
