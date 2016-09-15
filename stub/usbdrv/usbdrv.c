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

#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <avr/signature.h>

#include <usbdrv/usbdrv.h>
#include <usbdrv/archived/usbdesc.c>

#ifdef USB_RESET_HOOK
static void __usbResetHook(uchar isReset)
{
	USB_RESET_HOOK(isReset);
}
#endif

#ifdef USB_SET_ADDRESS_HOOK
static void __usbSetAddressHook(void)
{
	USB_SET_ADDRESS_HOOK();
}
#endif

#ifdef USB_RX_USER_HOOK
static void __usbRxUserHook(uchar *data, uchar len)
{
	USB_RX_USER_HOOK(data, len);
}
#endif

uchar usbDriverDescriptor(usbRequest_t *rq)
{
	return usbDriverDynamicDescriptor(rq);
}

#if !USB_CFG_SUPPRESS_INTR_CODE
#if USB_CFG_HAVE_INTRIN_ENDPOINT
#if !USB_HAS_CFG_HAVE_INTRIN_ENDPOINT
#error USB_CFG_HAVE_INTRIN_ENDPOINT not supported by bootloader
#endif
#if USB_CFG_HAVE_INTRIN_ENDPOINT3
USB_PUBLIC void usbSetInterrupt(uchar *data, uchar len)
{
    usbGenericSetInterrupt(data, len, &usbTxStatus1);
}
#endif
#endif

#if USB_CFG_HAVE_INTRIN_ENDPOINT3
#if !USB_HAS_CFG_HAVE_INTRIN_ENDPOINT3
#error USB_CFG_HAVE_INTRIN_ENDPOINT3 not supported by bootloader
#endif
USB_PUBLIC void usbSetInterrupt3(uchar *data, uchar len)
{
    usbGenericSetInterrupt(data, len, &usbTxStatus3);
}
#endif
#endif /* USB_CFG_SUPPRESS_INTR_CODE */

/* Just here to silence the compiler on some unused function prototypes */
USB_PUBLIC void __attribute__((unused,naked)) __boot_usbPoll(void) {}
USB_PUBLIC void __attribute__((unused,naked)) __boot_usbInit(void) {}
USB_PUBLIC usbMsgLen_t __attribute__((unused,naked,noreturn)) __boot_usbFunctionSetup(uchar data[8]) { }
USB_PUBLIC void __attribute__((unused,naked)) __boot_usbFunctionWriteOut(uchar *data, uchar len) {}
USB_PUBLIC uchar __attribute__((unused,naked,noreturn)) __boot_usbFunctionWrite(uchar *data, uchar len) {}
USB_PUBLIC uchar __attribute__((unused,naked,noreturn)) __boot_usbFunctionRead(uchar *data, uchar len) {}
USB_PUBLIC __attribute__((unused,naked,noreturn)) usbMsgLen_t usbCustomDriverDescriptor(struct usbRequest *rq) { }
