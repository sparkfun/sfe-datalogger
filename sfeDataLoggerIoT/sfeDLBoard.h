/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2024, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */

// Board specific things for the DataLogger..
#pragma once

#include <cstdint>
// Pins

// Das Boot button
const uint8_t kDLBoardBootButton = 0;

// 3v3 pin
const uint8_t kDLBoardEn3v3_SW = 32;

// LED Built in
const uint8_t kDLBoardLEDBuiltin = 25;

// RGB LED
const uint8_t kDLBoardLEDRGBBuiltin = 26;

// Define the GNSS PPS pin for the datalogger IoT board
const uint16_t kDLBoardGNSSPPSPins[] = {33, 36};

// External Serial pins on the board
const uint8_t kDLBoardExtSerialRXPin = 16;
const uint8_t kDLBoardExtSerialTXPin = 17;

// pins that can be used for interrupts.
const uint16_t kDLBoardInterruptPins[] = {33, 36};