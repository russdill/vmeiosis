/*
 * V-USB Meiosis Bootloader      (c) 2024 Russ Dill <russd@asu.edu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Bulk of code from:
 * Project: V-USB, virtual USB port for Atmel's(r) AVR(r) microcontrollers
 * Author: Christian Starkjohann
 * Creation Date: 2005-04-01
 * Tabsize: 4
 * Copyright: (c) 2005 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 */

#include <avr/pgmspace.h>

#include <usbdrv.h>
#include "usbdesc.h"

#if USB_CFG_SUPPRESS_INTRIN_ENDPOINTS
#define USB_CFG_HAVE_INTRIN_ENDPOINT_DESC 0
#define USB_CFG_HAVE_INTRIN_ENDPOINT3_DESC 0
#else
#define USB_CFG_HAVE_INTRIN_ENDPOINT_DESC USB_CFG_HAVE_INTRIN_ENDPOINT
#define USB_CFG_HAVE_INTRIN_ENDPOINT3_DESC USB_CFG_HAVE_INTRIN_ENDPOINT3
#endif

#if USB_CFG_DESCR_PROPS_HID_REPORT != 0 && USB_CFG_DESCR_PROPS_HID == 0
#undef USB_CFG_DESCR_PROPS_HID
#define USB_CFG_DESCR_PROPS_HID     9   /* length of HID descriptor in config descriptor below */
#endif

#define USBDESCR_CONFIG_LEN (18 + 7 * USB_CFG_HAVE_INTRIN_ENDPOINT_DESC + \
                             7 * USB_CFG_HAVE_INTRIN_ENDPOINT3_DESC + \
                             (USB_CFG_DESCR_PROPS_HID & 0xff))

#if USB_CFG_CUSTOM_DRIVER_DESCRIPTOR
PROGMEM const struct {
    char usbDesc[(USB_CFG_DESCR_PROPS_CONFIGURATION == 0) * USBDESCR_CONFIG_LEN +
                 (USB_CFG_DESCR_PROPS_DEVICE == 0) * 18];
    int usbString[];
} usbDescriptors = {
    {
/* ----------------------- Configuration Descriptor ------------------------ */

#if USB_CFG_DESCR_PROPS_CONFIGURATION == 0
    9,          /* sizeof(usbDescriptorConfiguration): length of descriptor in bytes */
    USBDESCR_CONFIG,    /* descriptor type */
    USBDESCR_CONFIG_LEN, 0,
                /* total length of data returned (including inlined descriptors) */
    1,          /* number of interfaces in this configuration */
    1,          /* index of this configuration */
    0,          /* configuration name string index */
#if USB_CFG_IS_SELF_POWERED
    (1 << 7) | USBATTR_SELFPOWER,       /* attributes */
#else
    (1 << 7) | USBATTR_REMOTEWAKE,      /* attributes */
#endif
    USB_CFG_MAX_BUS_POWER/2,            /* max USB current in 2mA units */
/* interface descriptor follows inline: */
    9,          /* sizeof(usbDescrInterface): length of descriptor in bytes */
    USBDESCR_INTERFACE, /* descriptor type */
    0,          /* index of this interface */
    0,          /* alternate setting for this interface */
    USB_CFG_HAVE_INTRIN_ENDPOINT_DESC + USB_CFG_HAVE_INTRIN_ENDPOINT3_DESC, /* endpoints excl 0: number of endpoint descriptors to follow */
    USB_CFG_INTERFACE_CLASS,
    USB_CFG_INTERFACE_SUBCLASS,
    USB_CFG_INTERFACE_PROTOCOL,
    0,          /* string index for interface */
#if (USB_CFG_DESCR_PROPS_HID & 0xff)    /* HID descriptor */
    9,          /* sizeof(usbDescrHID): length of descriptor in bytes */
    USBDESCR_HID,   /* descriptor type: HID */
    0x01, 0x01, /* BCD representation of HID version */
    0x00,       /* target country code */
    0x01,       /* number of HID Report (or other HID class) Descriptor infos to follow */
    0x22,       /* descriptor type: report */
    USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH, 0,  /* total length of report descriptor */
#endif
#if USB_CFG_HAVE_INTRIN_ENDPOINT_DESC    /* endpoint descriptor for endpoint 1 */
    7,          /* sizeof(usbDescrEndpoint) */
    USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
    (char)0x81, /* IN endpoint number 1 */
    0x03,       /* attrib: Interrupt endpoint */
    8, 0,       /* maximum packet size */
    USB_CFG_INTR_POLL_INTERVAL, /* in ms */
#endif
#if USB_CFG_HAVE_INTRIN_ENDPOINT3_DESC   /* endpoint descriptor for endpoint 3 */
    7,          /* sizeof(usbDescrEndpoint) */
    USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
    (char)(0x80 | USB_CFG_EP3_NUMBER), /* IN endpoint number 3 */
    0x03,       /* attrib: Interrupt endpoint */
    8, 0,       /* maximum packet size */
    USB_CFG_INTR_POLL_INTERVAL, /* in ms */
#endif
#endif

/* --------------------------- Device Descriptor --------------------------- */

#if USB_CFG_DESCR_PROPS_DEVICE == 0
    18,         /* sizeof(usbDescriptorDevice): length of descriptor in bytes */
    USBDESCR_DEVICE,        /* descriptor type */
    0x10, 0x01,             /* USB version supported */
    USB_CFG_DEVICE_CLASS,
    USB_CFG_DEVICE_SUBCLASS,
    0,                      /* protocol */
    8,                      /* max packet size */
    /* the following two casts affect the first byte of the constant only, but
     * that's sufficient to avoid a warning with the default values.
     */
    (char)USB_CFG_VENDOR_ID,/* 2 bytes */
    (char)USB_CFG_DEVICE_ID,/* 2 bytes */
    USB_CFG_DEVICE_VERSION, /* 2 bytes */
    USB_STRING_INDEX_VENDOR,         /* manufacturer string index */
    USB_STRING_INDEX_PRODUCT,        /* product string index */
    USB_STRING_INDEX_SERIAL_NUMBER,  /* serial number string index */
    1,          /* number of configurations */
#endif

    }, {

/* -------------------------- String Descriptors --------------------------- */

#if USB_CFG_DESCR_PROPS_STRINGS == 0

#if USB_CFG_DESCR_PROPS_STRING_0 == 0
    USB_STRING_DESCRIPTOR_HEADER(1),
    0x0409, /* language index (0x0409 = US-English) */
#endif

#if USB_CFG_DESCR_PROPS_STRING_VENDOR == 0 && USB_CFG_VENDOR_NAME_LEN
    USB_STRING_DESCRIPTOR_HEADER(USB_CFG_VENDOR_NAME_LEN),
    USB_CFG_VENDOR_NAME,
#endif

#if USB_CFG_DESCR_PROPS_STRING_PRODUCT == 0 && USB_CFG_DEVICE_NAME_LEN
    USB_STRING_DESCRIPTOR_HEADER(USB_CFG_DEVICE_NAME_LEN),
    USB_CFG_DEVICE_NAME,
#endif

#if USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER == 0 && USB_CFG_SERIAL_NUMBER_LEN
    USB_STRING_DESCRIPTOR_HEADER(USB_CFG_SERIAL_NUMBER_LEN),
    USB_CFG_SERIAL_NUMBER,
#endif
#endif  /* USB_CFG_DESCR_PROPS_STRINGS == 0 */
    0, /* Final length 0, indicates end */
    }
};
#else
PROGMEM const short usbDescriptors[] = {0};
#endif
