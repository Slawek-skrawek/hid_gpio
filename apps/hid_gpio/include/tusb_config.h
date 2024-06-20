/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------
// COMMON CONFIGURATION
//--------------------------------------------------------------------

#include <tusb_hw.h>

#define DCFG_TUSB_DEBUG 2

// defined by compiler flags for flexibility
#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU must be defined
#endif

#if CFG_TUSB_MCU == OPT_MCU_LPC43XX || CFG_TUSB_MCU == OPT_MCU_LPC18XX || CFG_TUSB_MCU == OPT_MCU_MIMXRT10XX
#define CFG_TUSB_RHPORT0_MODE       (OPT_MODE_DEVICE | OPT_MODE_HIGH_SPEED)
#else
#define CFG_TUSB_RHPORT0_MODE       OPT_MODE_DEVICE
#endif

#define CFG_TUSB_OS                 OPT_OS_MYNEWT

#ifndef CFG_TUSB_DEBUG
// Can be set during compilation i.e.: make LOG=<value for CFG_TUSB_DEBUG> BOARD=<bsp>
// Keep in mind that enabling logs when data is streaming can disrupt data flow.
// It can be very helpful though when audio unit requests are tested/debugged.
#define CFG_TUSB_DEBUG              0
#endif

/* USB DMA on some MCUs can only access a specific SRAM region with restriction on alignment.
 * Tinyusb use follows macros to declare transferring memory so that they can be put
 * into those specific section.
 * e.g
 * - CFG_TUSB_MEM SECTION : __attribute__ (( section(".usb_ram") ))
 * - CFG_TUSB_MEM_ALIGN   : __attribute__ ((aligned(4)))
 */
#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN          __attribute__ ((aligned(4)))
#endif

//--------------------------------------------------------------------
// DEVICE CONFIGURATION
//--------------------------------------------------------------------

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE    64
#endif

//------------- CLASS -------------//
#define CFG_TUD_CDC               0
#define CFG_TUD_MSC               0
#define CFG_TUD_HID               1
#define CFG_TUD_MIDI              0
#define CFG_TUD_AUDIO             0
#define CFG_TUD_VENDOR            0

/* Minimal number for alternative interfaces that is recognized by Windows as Bluetooth radio controller */
#define CFG_TUD_BTH_ISO_ALT_COUNT 2

#if 0
//--------------------------------------------------------------------
// AUDIO CLASS DRIVER CONFIGURATION
//--------------------------------------------------------------------

#ifndef AUDIO_SAMPLE_RATE
#define AUDIO_SAMPLE_RATE                   192000
#endif

#define CFG_TUD_AUDIO_IN_PATH               (CFG_TUD_AUDIO)
#define CFG_TUD_AUDIO_OUT_PATH              (CFG_TUD_AUDIO)

// Audio format type
#define CFG_TUD_AUDIO_FORMAT_TYPE_TX        AUDIO_FORMAT_TYPE_I
#define CFG_TUD_AUDIO_FORMAT_TYPE_RX        AUDIO_FORMAT_TYPE_I

// Audio format type I specifications
#define CFG_TUD_AUDIO_FORMAT_TYPE_I_TX      AUDIO_DATA_FORMAT_TYPE_I_PCM
#define CFG_TUD_AUDIO_FORMAT_TYPE_I_RX      AUDIO_DATA_FORMAT_TYPE_I_PCM
#define CFG_TUD_AUDIO_N_CHANNELS_TX         1
#define CFG_TUD_AUDIO_N_BYTES_PER_SAMPLE_TX 2
#define CFG_TUD_AUDIO_N_CHANNELS_RX         1
#define CFG_TUD_AUDIO_N_BYTES_PER_SAMPLE_RX 2
#define CFG_TUD_AUDIO_RX_ITEMSIZE           2
#define CFG_TUD_AUDIO_ENABLE_FEEDBACK_EP    0

// EP and buffer size - for isochronous EP´s, the buffer and EP size are equal (different sizes would not make sense)
#define CFG_TUD_AUDIO_EPSIZE_IN           (CFG_TUD_AUDIO_IN_PATH * (192 + 1) * (CFG_TUD_AUDIO_N_BYTES_PER_SAMPLE_TX) * (CFG_TUD_AUDIO_N_CHANNELS_TX)) // 48 Samples (48 kHz) x 2 Bytes/Sample x n Channels
#define CFG_TUD_AUDIO_TX_FIFO_COUNT       (CFG_TUD_AUDIO_IN_PATH * 1)
#define CFG_TUD_AUDIO_TX_FIFO_SIZE        (CFG_TUD_AUDIO_IN_PATH ? ((CFG_TUD_AUDIO_EPSIZE_IN)) : 0)

// EP and buffer size - for isochronous EP´s, the buffer and EP size are equal (different sizes would not make sense)
#define CFG_TUD_AUDIO_EPSIZE_OUT          (CFG_TUD_AUDIO_OUT_PATH * ((48 + CFG_TUD_AUDIO_ENABLE_FEEDBACK_EP) * (CFG_TUD_AUDIO_N_BYTES_PER_SAMPLE_RX) * (CFG_TUD_AUDIO_N_CHANNELS_RX))) // N Samples (N kHz) x 2 Bytes/Sample x n Channels
#define CFG_TUD_AUDIO_RX_FIFO_COUNT       (CFG_TUD_AUDIO_OUT_PATH * 1)
#define CFG_TUD_AUDIO_RX_FIFO_SIZE        (CFG_TUD_AUDIO_OUT_PATH ? (3 * (CFG_TUD_AUDIO_EPSIZE_OUT / CFG_TUD_AUDIO_RX_FIFO_COUNT)) : 0)

// Number of Standard AS Interface Descriptors (4.9.1) defined per audio function - this is required to be able to remember the current alternate settings of these interfaces - We restrict us here to have a constant number for all audio functions (which means this has to be the maximum number of AS interfaces an audio function has and a second audio function with less AS interfaces just wastes a few bytes)
#define CFG_TUD_AUDIO_N_AS_INT 			   		1

// Size of control request buffer
#define CFG_TUD_AUDIO_CTRL_BUF_SIZE 				64
#endif

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_CONFIG_H_ */
