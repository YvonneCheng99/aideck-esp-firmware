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

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define CPX_VERSION (0)
#define CPX_HEADER_SIZE (2)
#define MTU (1022)

// This enum is used to identify source and destination for CPX routing information
typedef enum {
  CPX_T_STM32 = 1, // The STM in the Crazyflie
  CPX_T_ESP32 = 2, // The ESP on the AI-deck
  CPX_T_WIFI_HOST = 3,  // A remote computer connected via Wifi
  CPX_T_GAP8 = 4   // The GAP8 on the AI-deck
} CPXTarget_t;

typedef enum {
  CPX_F_SYSTEM = 1,
  CPX_F_CONSOLE = 2,
  CPX_F_CRTP = 3,
  CPX_F_WIFI_CTRL = 4,
  CPX_F_APP = 5,
  CPX_F_TEST = 0x0E,
  CPX_F_BOOTLOADER = 0x0F,
  CPX_F_LAST // NEEDS TO BE LAST
} CPXFunction_t;

typedef struct {
  CPXTarget_t destination;
  CPXTarget_t source;
  bool lastPacket;
  CPXFunction_t function;
  uint8_t version;
} CPXRouting_t;

// This struct contains routing information in a packed format. This struct
// should mainly be used to serialize data when tranfering. Unpacked formats
// should be preferred in application code.
typedef struct {
  CPXTarget_t destination : 3;
  CPXTarget_t source : 3;
  bool lastPacket : 1;
  bool reserved : 1;
  CPXFunction_t function : 6;
  uint8_t version : 2;
} __attribute__((packed)) CPXRoutingPacked_t;

#define CPX_ROUTING_PACKED_SIZE (sizeof(CPXRoutingPacked_t))

// The maximum MTU of any link
#define CPX_MAX_PAYLOAD_SIZE 1022

typedef struct {
  CPXRouting_t route;
  uint16_t dataLength;
  uint8_t data[MTU-CPX_HEADER_SIZE];
} CPXPacket_t;

typedef enum {
  LOG_TO_WIFI = CPX_T_WIFI_HOST,
  LOG_TO_CRTP = CPX_T_STM32
} CPXConsoleTarget_t;

typedef struct {
  CPXRouting_t route;

  uint16_t dataLength;
  uint8_t data[CPX_MAX_PAYLOAD_SIZE - CPX_ROUTING_PACKED_SIZE];
} CPXRoutablePacket_t;

typedef struct
{
  uint16_t len; // Of data (max 1022)
  uint8_t data[MTU];
} __attribute__((packed)) packet_t;

void cpxInitRoute(const CPXTarget_t source, const CPXTarget_t destination, const CPXFunction_t function, CPXRouting_t* route);
void cpxRouteToPacked(const CPXRouting_t* route, CPXRoutingPacked_t* packed);
void cpxPackedToRoute(const CPXRoutingPacked_t* packed, CPXRouting_t* route);

/**
 * @brief Receive a CPX packet from the ESP32
 *
 * This function will block until a packet is availale from CPX. The
 * function will return all packets routed to the STM32.
 *
 * @param function function to receive packet on
 * @param packet received packet will be stored here
 */
void cpxReceivePacketBlocking(CPXFunction_t function, CPXPacket_t * packet);

/**
 * @brief Send a CPX packet to the ESP32
 *
 * This will send a packet to the ESP32 to be routed using CPX. This
 * will block until the packet can be queued up for sending.
 *
 * @param packet packet to be sent
 */
void cpxSendPacketBlocking(const CPXPacket_t * packet);

/**
 * @brief Print debug data though CPX
 *
 * This will print debug data though CPX to the Crazyflie client console.
 * The function doesn't add a newline, so this will have to be supplied.
 *
 * @param target The target where the printout should go
 * @param fmt Standard C format string
 * @param variable Data for format string
 */
void cpxPrintToConsole(CPXConsoleTarget_t target, const char * fmt, ...);

void com_write(packet_t * p);