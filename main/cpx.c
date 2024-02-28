#include "cpx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

typedef struct {
    uint16_t wireLength;
    CPXRoutingPacked_t route;
    uint8_t data[MTU - CPX_HEADER_SIZE];
} __attribute__((packed)) CPXPacketPacked_t;

SemaphoreHandle_t xSemaphore = NULL;
static xQueueHandle queues[CPX_F_LAST];
static CPXPacketPacked_t txpPacked;

static uint32_t start;
static EventGroupHandle_t evGroup;
static QueueHandle_t txq = NULL;
#define TX_QUEUE_BIT (1 << 1)

void com_write(packet_t *p)
{
  start = xTaskGetTickCount();
  //printf("Will queue up packet\n");
  xQueueSend(txq, p, (TickType_t)portMAX_DELAY);
  //printf("Have queued up packet!\n");
  xEventGroupSetBits(evGroup, TX_QUEUE_BIT);
}

void cpxInitRoute(const CPXTarget_t source, const CPXTarget_t destination, const CPXFunction_t function, CPXRouting_t* route) {
    route->source = source;
    route->destination = destination;
    route->function = function;
    route->version = CPX_VERSION;
}

void cpxRouteToPacked(const CPXRouting_t* route, CPXRoutingPacked_t* packed) {
    packed->source = route->source;
    packed->destination = route->destination;
    packed->function = route->function;
    packed->version = route->version;
    packed->lastPacket = route->lastPacket;
}

void cpxPackedToRoute(const CPXRoutingPacked_t* packed, CPXRouting_t* route) {
    if(CPX_VERSION == packed->version)
    {
        route->version = packed->version;
        route->source = packed->source;
        route->destination = packed->destination;
        route->function = packed->function;
        route->lastPacket = packed->lastPacket;
    }
}

void cpxReceivePacketBlocking(CPXFunction_t function, CPXPacket_t * packet) {
  xQueueReceive(queues[function], packet, (TickType_t)portMAX_DELAY);
}

void cpxSendPacketBlocking(const CPXPacket_t * packet) {
  txpPacked.wireLength = packet->dataLength + CPX_HEADER_SIZE;
  txpPacked.route.destination = packet->route.destination;
  txpPacked.route.source = packet->route.source;
  txpPacked.route.function = packet->route.function;
  txpPacked.route.version = packet->route.version;
  txpPacked.route.lastPacket = packet->route.lastPacket;
  memcpy(txpPacked.data, packet->data, packet->dataLength);
  com_write((packet_t*)&txpPacked);
}

static CPXPacket_t consoleTx;
void cpxPrintToConsole(CPXConsoleTarget_t target, const char * fmt, ...) {
  if( xSemaphoreTake( xSemaphore, ( TickType_t )portMAX_DELAY) == pdTRUE )
  {
    va_list ap;
    int len;

    va_start(ap, fmt);
    len = vsnprintf((char*)consoleTx.data, sizeof(consoleTx.data), fmt, ap);
    va_end(ap);

    consoleTx.route.destination = target;
    consoleTx.route.source = CPX_T_GAP8;
    consoleTx.route.function = CPX_F_CONSOLE;
    consoleTx.dataLength = len + 1;

    cpxSendPacketBlocking(&consoleTx);
    xSemaphoreGive( xSemaphore );
  }
}
