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

#include <assert.h>
#include <string.h>

#include "sysinit/sysinit.h"
#include "os/os.h"
#include "bsp/bsp.h"
#include "hal/hal_gpio.h"
#include "hal/hal_nvreg.h"
#include <hal/hal_system.h>
#include <console/console.h>

#include <tusb.h>
#include <device/dcd.h>

/* For LED toggling */
int g_led_pin;

/* The timer callout */
static struct os_callout blinky_callout;

/* peripherals with 8 bit or 16 bit register access allow only 8 bit or 16 bit bit band access, so cast to 8 bit always */
#define BITBAND_PERI(x, b)  (*((__IO  uint8_t *) (PERIPH_BB_BASE + (((uint32_t)(uint32_t *)&x) - PERIPH_BASE)*32 + b*4)))
#define REG_SET_BIT(reg, b) BITBAND_PERI(reg, b) = 1
#define REG_CLR_BIT(reg, b) BITBAND_PERI(reg, b) = 0

enum {
    HOST_RESET,
    HOST_RUNNING,
    HOST_SUSPENDED,
} host_state = HOST_RESET;

const int blink_time[] = {
    [HOST_RESET] = OS_TICKS_PER_SEC * 5,
    [HOST_RUNNING] = OS_TICKS_PER_SEC,
    [HOST_SUSPENDED] = OS_TICKS_PER_SEC * 3,
};

/*
 * Event callback function for timer events. It toggles the led pin.
 */
static void
timer_ev_cb(struct os_event *ev)
{
    assert(ev != NULL);

    hal_gpio_toggle(g_led_pin);

//    os_callout_reset(&blinky_callout, blink_time[host_state]);
    os_callout_reset(&blinky_callout, OS_TICKS_PER_SEC);
}

static void
init_timer(void)
{
    /*
     * Initialize the callout for a timer event.
     */
    os_callout_init(&blinky_callout, os_eventq_dflt_get(),
                    timer_ev_cb, NULL);

    os_callout_reset(&blinky_callout, blink_time[host_state]);
}

struct os_callout hid_send_report;

#define HID_USAGE_CONSUMER_FAST_FORWARD 0xB3
#define HID_USAGE_CONSUMER_REWIND       0xB4
#define HID_USAGE_CONSUMER_AC_FULL_SCREEN_VIEW 230
#define HID_USAGE_CONSUMER_AC_CLOSE     0x203
#define HID_USAGE_CONSUMER_AC_EXIT      0x204

void reboot_to_dfu(void)
{
    hal_nvreg_write(2, 0xDF00AE01);
    hal_system_reset();
}

void tud_mount_cb()
{
    host_state = HOST_RUNNING;
}

void tud_umount_cb(void)
{
    host_state = HOST_RESET;
}

void tud_suspend_cb(bool remote_wakeup_en)
{
    (void) remote_wakeup_en;
    host_state = HOST_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
    host_state = HOST_RUNNING;
}

static uint8_t pins_config[8];

/*
 * Invoked when received GET_REPORT control request
 * Application must fill buffer report's content and return its length.
 * Return zero will cause the stack to STALL request
 */
uint16_t
tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    (void)itf;
    (void)report_id;

    if (report_id == 5 && reqlen == 8) {
        memcpy(buffer, pins_config, 8);
    }

    return 0;
}

static int pins[] = {
    MYNEWT_VAL(HID_GPIO_PIN0),
    MYNEWT_VAL(HID_GPIO_PIN1),
    MYNEWT_VAL(HID_GPIO_PIN2),
    MYNEWT_VAL(HID_GPIO_PIN3),
    MYNEWT_VAL(HID_GPIO_PIN4),
    MYNEWT_VAL(HID_GPIO_PIN5),
    MYNEWT_VAL(HID_GPIO_PIN6),
    MYNEWT_VAL(HID_GPIO_PIN7),
};

/*
 * Invoked when received SET_REPORT control request or
 * received data on OUT endpoint ( Report ID = 0, Type = 0 )
 */
void
tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, const uint8_t *buffer, uint16_t bufsize)
{
    (void)itf;
    (void)bufsize;

    // Report 1, Keyboard LEDs
    if (report_id == 1) {
        // Bit 0 - Num-lock
        if (buffer[0] & 1) {
            hal_gpio_write(LED_BLINK_PIN, 1);
        } else {
            hal_gpio_write(LED_BLINK_PIN, 0);
        }
        // Num-Lock + Scroll-Lock + Caps-Lock -> Reset to bootloader
        if ((buffer[0] & 7) == 7) {
            hal_nvreg_write(2, 100);
            hal_system_reset();
        }
    } else if (report_id == 5 && report_type == HID_REPORT_TYPE_FEATURE) {
        // Feature report 5, each bit activates GPIO defined in syscfg HID_GPIO_PINx
        memcpy(pins_config, buffer, 8);
        for (int i = 0; i < 8; ++i) {
            switch (pins_config[i]) {
            case '0':
                hal_gpio_init_out(pins[i], 0);
                break;
            case '1':
                hal_gpio_init_out(pins[i], 1);
                break;
            case 'i':
                hal_gpio_deinit(pins[i]);
            default:
                break;
            }
        }
    }
    console_printf("set_report %d, %d\n", report_id, report_type);
}

void
mynewt_main(int argc, char **argv)
{
    sysinit();

    console_printf("HID_GPIO started\n");
    console_printf("Reset reason %s\n", hal_reset_cause_str());

    init_timer();
    while (1) {
        os_eventq_run(os_eventq_dflt_get());
    }
    assert(0);
}

