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

#include <syscfg/syscfg.h>
#include <bsp/bsp.h>
#include <string.h>
#include <tusb.h>
#include <device/usbd.h>
#include <os/util.h>
#include <console/console.h>
#include <hal/hal_gpio.h>

#define USBD_PRODUCT_RELEASE_NUMBER MYNEWT_VAL(USBD_PRODUCT_RELEASE_NUMBER)

#ifndef CONFIG_NUM
#define CONFIG_NUM 1
#endif

#define CDC_IF_STR_IX (MYNEWT_VAL(USBD_CDC_DESCRIPTOR_STRING) == NULL ? 0 : 4)
#define MSC_IF_STR_IX (MYNEWT_VAL(USBD_MSC_DESCRIPTOR_STRING) == NULL ? 0 : 5)
#define HID_IF_STR_IX (MYNEWT_VAL(USBD_HID_DESCRIPTOR_STRING) == NULL ? 0 : 6)
#define BTH_IF_STR_IX (MYNEWT_VAL(USBD_BTH_DESCRIPTOR_STRING) == NULL ? 0 : 7)


#if CFG_TUD_HID

#define HID_REPORT_DESC() \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     )                    ,\
  HID_USAGE      ( HID_USAGE_DESKTOP_KEYBOARD )                    ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION )                    ,\
    HID_REPORT_ID(1)\
    /* Output 5-bit LED Indicator Kana | Compose | ScrollLock | CapsLock | NumLock */ \
    HID_USAGE_PAGE  ( HID_USAGE_PAGE_LED                   )       ,\
      HID_USAGE_MIN    ( 1                                       ) ,\
      HID_USAGE_MAX    ( 5                                       ) ,\
      HID_REPORT_COUNT ( 5                                       ) ,\
      HID_REPORT_SIZE  ( 1                                       ) ,\
      HID_OUTPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE  ) ,\
      /* led padding */ \
      HID_REPORT_COUNT ( 1                                       ) ,\
      HID_REPORT_SIZE  ( 3                                       ) ,\
      HID_OUTPUT       ( HID_CONSTANT                            ) ,\
  HID_COLLECTION_END, \
  HID_USAGE_PAGE_N ( HID_USAGE_PAGE_VENDOR, 2)                     ,\
  HID_USAGE      ( 0 )                                             ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION )                    ,\
    /* Report ID if any */\
    HID_REPORT_ID(5)\
      HID_USAGE_MIN    ( 0                                   )     ,\
      HID_USAGE_MAX    ( 0xFF                                )     ,\
      HID_LOGICAL_MIN  ( 0                                   )     ,\
      HID_LOGICAL_MAX  ( 0xFF                                )     ,\
      HID_REPORT_COUNT ( 8                                   )     ,\
      HID_REPORT_SIZE  ( 8                                   )     ,\
      HID_FEATURE      ( HID_DATA | HID_ARRAY | HID_ABSOLUTE )     ,\
  HID_COLLECTION_END \

const uint8_t desc_hid_report[] = {
    HID_REPORT_DESC(),
};

/*
 * Invoked when received GET HID REPORT DESCRIPTOR
 * Application return pointer to descriptor
 * Descriptor contents must exist long enough for transfer to complete
 */
const uint8_t *
tud_hid_descriptor_report_cb(uint8_t itf)
{
    (void)itf;

    return desc_hid_report;
}

#endif /* CFG_TUD_HID */

const tusb_desc_device_t desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,

#if CFG_TUD_BTH
    .bDeviceClass       = TUD_BT_APP_CLASS,
    .bDeviceSubClass    = TUD_BT_APP_SUBCLASS,
    .bDeviceProtocol    = TUD_BT_PROTOCOL_PRIMARY_CONTROLLER,
#elif CFG_TUD_CDC
    /*
     * Use Interface Association Descriptor (IAD) for CDC
     * As required by USB Specs IAD's subclass must be common class (2) and protocol must be IAD (1)
     */
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
#else
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
#endif

    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = MYNEWT_VAL(USBD_VID),
    .idProduct          = MYNEWT_VAL(USBD_PID),
    .bcdDevice          = USBD_PRODUCT_RELEASE_NUMBER,

    .iManufacturer      = 0x02,
    .iProduct           = 0x03,
    .iSerialNumber      = 0x01,

    .bNumConfigurations = 0x01
};

/*
 * Invoked when received GET DEVICE DESCRIPTOR
 * Application return pointer to descriptor
 */
const uint8_t *
tud_descriptor_device_cb(void)
{
    return (const uint8_t *)&desc_device;
}

/*
 * Configuration Descriptor
 */

enum {
#if CFG_TUD_BTH
    ITF_NUM_BTH,
#if CFG_TUD_BTH_ISO_ALT_COUNT > 0
    ITF_NUM_BTH_VOICE,
#endif
#endif

#if CFG_TUD_CDC
    ITF_NUM_CDC,
    ITF_NUM_CDC_DATA,
#endif

#if CFG_TUD_MSC
    ITF_NUM_MSC,
#endif

#if CFG_TUD_HID
    ITF_NUM_HID,
#endif

    ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + \
                             CFG_TUD_CDC * TUD_CDC_DESC_LEN + \
                             CFG_TUD_MSC * TUD_MSC_DESC_LEN + \
                             (CFG_TUD_HID ? 1 : 0) * TUD_HID_DESC_LEN + \
                             CFG_TUD_BTH * TUD_BTH_DESC_LEN + \
                             0)

const uint8_t desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(CONFIG_NUM, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,
                          MYNEWT_VAL(USBD_CONFIGURATION_MAX_POWER)),

#if CFG_TUD_BTH
    TUD_BTH_DESCRIPTOR(ITF_NUM_BTH, BTH_IF_STR_IX, USBD_BTH_EVENT_EP, USBD_BTH_EVENT_EP_SIZE,
                       USBD_BTH_EVENT_EP_INTERVAL, USBD_BTH_DATA_IN_EP, USBD_BTH_DATA_OUT_EP, USBD_BTH_DATA_EP_SIZE,
                       0, 9, 17, 25, 33, 49),
#endif

#if CFG_TUD_CDC
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, CDC_IF_STR_IX, USBD_CDC_NOTIFY_EP, USBD_CDC_NOTIFY_EP_SIZE,
                       USBD_CDC_DATA_OUT_EP, USBD_CDC_DATA_IN_EP, USBD_CDC_DATA_EP_SIZE),
#endif

#if CFG_TUD_MSC
    /* TODO: MSC not handled yet */
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, MSC_IF_STR_IX, EPNUM_MSC_OUT, EPNUM_MSC_IN,
                       (CFG_TUSB_RHPORT0_MODE & OPT_MODE_HIGH_SPEED) ? 512 : 64),
#endif

#if CFG_TUD_HID
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, HID_IF_STR_IX, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report),
                       USBD_HID_REPORT_EP, USBD_HID_REPORT_EP_SIZE, USBD_HID_REPORT_EP_INTERVAL),
#endif
};

/**
 * Invoked when received GET CONFIGURATION DESCRIPTOR
 * Application return pointer to descriptor
 * Descriptor contents must exist long enough for transfer to complete
 */
const uint8_t *
tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;

    return desc_configuration;
}

const char *string_desc_arr[] = {
    MYNEWT_VAL(USBD_VENDOR_STRING),
    MYNEWT_VAL(USBD_PRODUCT_STRING),
    NULL,
    NULL,
    MYNEWT_VAL(USBD_HID_DESCRIPTOR_STRING),
    NULL,
};

static uint16_t desc_string[MYNEWT_VAL(USBD_STRING_DESCRIPTOR_MAX_LENGTH) + 1];

uint16_t *desc_string_from_ascii(uint16_t *uchars, size_t uchar_count, const char *astr)
{
    if (astr == NULL) {
        uchars = NULL;
    } else {
        size_t len = strlen(astr);
        if (len > uchar_count - 1) {
            len = uchar_count - 1;
        }
        for (size_t i = 0; i < len; ++i) {
            uchars[i + 1] = astr[i];
        }
        if (len > 0) {
            uchars[0] = (TUSB_DESC_STRING << 8) | (2 * len + 2);
        } else {
            uchars = NULL;
        }
    }

    return uchars;
}

/*
 * Invoked when received GET STRING DESCRIPTOR request
 * Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
 */
const uint16_t *
tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    uint16_t *desc_str = desc_string;

    if (index == 0) {
        desc_string[1] = MYNEWT_VAL(USBD_LANGID);
        desc_string[0] = (TUSB_DESC_STRING << 8) | (2 * 1 + 2);
    } else if (index == 1) {
        desc_str = desc_string_from_ascii(desc_string, ARRAY_SIZE(desc_string), "2");
    } else if (index - 2 < ARRAY_SIZE(string_desc_arr)) {
        desc_str = desc_string_from_ascii(desc_string, ARRAY_SIZE(desc_string), string_desc_arr[index - 2]);
    } else if (index == 32) {
        desc_str = desc_string_from_ascii(desc_string, ARRAY_SIZE(desc_string), "HASH:hid_gpio:"MYNEWT_VAL(REPO_HASH_HID_GPIO));
    } else if (index == 33) {
        desc_str = desc_string_from_ascii(desc_string, ARRAY_SIZE(desc_string), "HASH:apache-mynewt_core:"MYNEWT_VAL(REPO_HASH_APACHE_MYNEWT_CORE));
    }

    return desc_str;
}
