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

#ifndef __usbdrv_h_included__

#define USB_USER_PROGRAM 1

#ifdef __usbconfig_h_included__
#undef __usbconfig_h_included__
#include <usbdrv/generated/boot-usbconfig.h>
#else
#include <usbdrv/generated/boot-usbconfig.h>
#undef __usbconfig_h_included__
#endif


#define usbPoll __boot_usbPoll
#define usbInit __boot_usbInit
#define usbFunctionSetup __boot_usbFunctionSetup
#define usbFunctionWriteOut __boot_usbFunctionWriteOut
#define usbFunctionWrite __boot_usbFunctionWrite
#define usbFunctionRead __boot_usbFunctionRead
#define usbGenericSetInterrupt __boot_usbGenericSetInterrupt
#define usbSetInterrupt __boot_usbSetInterrupt
#define usbSetInterrupt3 __boot_usbSetInterrupt3

#include <usbdrv/archived/boot-usbdrv.h>

#undef usbPoll
#undef usbInit
#undef usbFunctionSetup
#undef usbFunctionWriteOut
#undef usbFunctionWrite
#undef usbFunctionRead
#undef usbGenericSetInterrupt
#undef usbSetInterrupt
#undef usbSetInterrupt3

#ifndef __ASSEMBLER__
/* These functions link to exported symbols */
extern void usbPoll(void);
extern void systemInit(void);
extern void usbGenericSetInterrupt(uchar *data, uchar len, usbTxStatus_t *txStatus);
extern void usbSetInterrupt(uchar *data, uchar len);
extern void usbSetInterrupt3(uchar *data, uchar len);
extern usbMsgLen_t usbFunctionSetup(uchar data[8]);
extern uchar usbFunctionWrite(uchar *data, uchar len);
extern uchar usbFunctionRead(uchar *data, uchar len);
extern void usbFunctionWriteOut(uchar *data, uchar len);

#include <usbdrv/generated/boot-syms.c>

#endif

#endif /* __usbdrv_h_included__ */
