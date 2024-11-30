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

#ifndef __usbdesc_h_included__
#define __usbdesc_h_included__

/* Use defines for the switch statement so that we can choose between an
 * if()else if() and a switch/case based implementation. switch() is more
 * efficient for a LARGE set of sequential choices, if() is better in all other
 * cases.
 */
#if USB_CFG_USE_SWITCH_STATEMENT
#   define SWITCH_START(cmd)       switch(cmd){{
#   define SWITCH_CASE(value)      }break; case (value):{
#   define SWITCH_CASE2(v1,v2)     }break; case (v1): case(v2):{
#   define SWITCH_CASE3(v1,v2,v3)  }break; case (v1): case(v2): case(v3):{
#   define SWITCH_DEFAULT          }break; default:{
#   define SWITCH_END              }}
#else
#   define SWITCH_START(cmd)       {uchar _cmd = cmd; if(0){
#   define SWITCH_CASE(value)      }else if(_cmd == (value)){
#   define SWITCH_CASE2(v1,v2)     }else if(_cmd == (v1) || _cmd == (v2)){
#   define SWITCH_CASE3(v1,v2,v3)  }else if(_cmd == (v1) || _cmd == (v2) || (_cmd == (v3))){
#   define SWITCH_DEFAULT          }else{
#   define SWITCH_END              }}
#endif

/* ------------------------- Assign String Indexes ------------------------- */

#define USB_STRING_INDEX_0              1

#define HAS_USB_STRING_INDEX_VENDOR	(USB_CFG_DESCR_PROPS_STRING_VENDOR != 0 || USB_CFG_VENDOR_NAME_LEN)
#define USB_STRING_INDEX_VENDOR        USB_STRING_INDEX_0*HAS_USB_STRING_INDEX_VENDOR
#define USB_STRING_INDEX_1             (USB_STRING_INDEX_0+HAS_USB_STRING_INDEX_PRODUCT)

#define HAS_USB_STRING_INDEX_PRODUCT	(USB_CFG_DESCR_PROPS_STRING_PRODUCT != 0 || USB_CFG_DEVICE_NAME_LEN)
#define USB_STRING_INDEX_PRODUCT        USB_STRING_INDEX_1*HAS_USB_STRING_INDEX_PRODUCT
#define USB_STRING_INDEX_2              (USB_STRING_INDEX_1+HAS_USB_STRING_INDEX_PRODUCT)

#ifndef USB_CFG_SERIAL_NUMBER_LEN
#define USB_CFG_SERIAL_NUMBER_LEN 0
#endif
#define HAS_USB_STRING_INDEX_SERIAL_NUMBER (USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER != 0 || USB_CFG_SERIAL_NUMBER_LEN)
#define USB_STRING_INDEX_SERIAL_NUMBER  USB_STRING_INDEX_2*HAS_USB_STRING_INDEX_SERIAL_NUMBER

#if USB_CFG_DESCR_PROPS_HID_REPORT != 0 && USB_CFG_DESCR_PROPS_HID == 0
#undef USB_CFG_DESCR_PROPS_HID
#define USB_CFG_DESCR_PROPS_HID     9   /* length of HID descriptor in config descriptor below */
#endif

#ifdef USB_CFG_DESCR_PROPS_HID
#define USB_CFG_IS_DYNAMIC_HID_FLAGS \
       (USB_CFG_DESCR_PROPS_HID | \
       USB_CFG_DESCR_PROPS_HID_REPORT)
#else
#define USB_CFG_IS_DNYNAMIC_HID_FLAGS (0)
#endif

/* We use if() instead of #if in the macro below because #if can't be used
 * in macros and the compiler optimizes constant conditions anyway.
 * This may cause problems with undefined symbols if compiled without
 * optimizing!
 */
#define GET_DESCRIPTOR(cfgProp, staticName)              \
    if(cfgProp){                                         \
        if((cfgProp) & USB_PROP_IS_RAM)                  \
            flags = 0;                                   \
        if((cfgProp) & USB_PROP_IS_DYNAMIC){             \
            len = usbFunctionDescriptor(rq);             \
        }else{                                           \
            len = USB_PROP_LENGTH(cfgProp);              \
            usbMsgPtr = (usbMsgPtr_t)(staticName);       \
        }                                                \
    }

static inline uchar usbDriverDynamicDescriptor(usbRequest_t *rq)
{
usbMsgLen_t len = 0;
uchar       flags = USB_FLG_MSGPTR_IS_ROM;

    SWITCH_START(rq->wValue.bytes[1])
    SWITCH_CASE(USBDESCR_DEVICE)    /* 1 */
        GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_DEVICE, usbDescriptorDevice)
    SWITCH_CASE(USBDESCR_CONFIG)    /* 2 */
        GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_CONFIGURATION, usbDescriptorConfiguration)
    SWITCH_CASE(USBDESCR_STRING)    /* 3 */
#if USB_CFG_DESCR_PROPS_STRINGS & USB_PROP_IS_DYNAMIC
        if(USB_CFG_DESCR_PROPS_STRINGS & USB_PROP_IS_RAM)
            flags = 0;
        len = usbFunctionDescriptor(rq);
#else   /* USB_CFG_DESCR_PROPS_STRINGS & USB_PROP_IS_DYNAMIC */
        SWITCH_START(rq->wValue.bytes[0])
        SWITCH_CASE(0)
            GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_STRING_0, usbDescriptorString0)
        SWITCH_CASE(USB_STRING_INDEX_VENDOR)
            GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_STRING_VENDOR, usbDescriptorStringVendor)
        SWITCH_CASE(USB_STRING_INDEX_PRODUCT)
            GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_STRING_PRODUCT, usbDescriptorStringDevice)
        SWITCH_CASE(USB_STRING_INDEX_SERIAL_NUMBER)
            GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER, usbDescriptorStringSerialNumber)
        SWITCH_DEFAULT
            if(USB_CFG_DESCR_PROPS_UNKNOWN & USB_PROP_IS_DYNAMIC){
                if(USB_CFG_DESCR_PROPS_UNKNOWN & USB_PROP_IS_RAM){
                    flags = 0;
                }
                len = usbFunctionDescriptor(rq);
            }
        SWITCH_END
#endif  /* USB_CFG_DESCR_PROPS_STRINGS & USB_PROP_IS_DYNAMIC */
#if USB_CFG_DESCR_PROPS_HID_REPORT  /* only support HID descriptors if enabled */
    SWITCH_CASE(USBDESCR_HID)       /* 0x21 */
        if (USB_CFG_DESCR_PROPS_CONFIGURATION)
            GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_HID, usbDescriptorConfiguration + 18)
    SWITCH_CASE(USBDESCR_HID_REPORT)/* 0x22 */
        GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_HID_REPORT, usbDescriptorHidReport)
#endif
    SWITCH_DEFAULT
        if(USB_CFG_DESCR_PROPS_UNKNOWN & USB_PROP_IS_DYNAMIC){
            if(USB_CFG_DESCR_PROPS_UNKNOWN & USB_PROP_IS_RAM){
                flags = 0;
            }
            len = usbFunctionDescriptor(rq);
        }
    SWITCH_END
    usbMsgFlags = flags;
    return len;
}

#endif /* __usbdesc_h_included__ */
