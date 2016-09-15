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
#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include "usbconfig.h"
#include "vmeconfig.h"

#ifndef USB_CFG_LONG_TRANSEFRS
#define USB_CFG_LONG_TRANSEFRS 0
#endif
#ifndef USB_HAS_CFG_RX_USER_HOOK
#define USB_HAS_CFG_RX_USER_HOOK 0
#endif
#ifndef USB_HAS_CFG_RESET_HOOK
#define USB_HAS_CFG_RESET_HOOK 0
#endif
#ifndef USB_HAS_CFG_SET_ADDRESS_HOOK
#define USB_HAS_CFG_SET_ADDRESS_HOOK 0
#endif

#ifndef USB_CFG_HAVE_INTRIN_ENDPOINT
#define USB_CFG_HAVE_INTRIN_ENDPOINT 0
#endif
#ifndef USB_HAS_CFG_HAVE_INTRIN_ENDPOINT
#define USB_HAS_CFG_HAVE_INTRIN_ENDPOINT USB_CFG_HAVE_INTRIN_ENDPOINT
#endif
#ifndef USB_CFG_HAVE_INTRIN_ENDPOINT3
#define USB_CFG_HAVE_INTRIN_ENDPOINT3 0
#endif
#ifndef USB_HAS_CFG_HAVE_INTRIN_ENDPOINT3
#define USB_HAS_CFG_HAVE_INTRIN_ENDPOINT3 USB_CFG_HAVE_INTRIN_ENDPOINT3
#endif

#ifndef USB_CFG_IMPLEMENT_FN_WRITEOUT
#define USB_CFG_IMPLEMENT_FN_WRITEOUT 0
#endif
#ifndef USB_HAS_CFG_IMPLEMENT_FN_WRITEOUT
#define USB_HAS_CFG_IMPLEMENT_FN_WRITEOUT USB_CFG_IMPLEMENT_FN_WRITEOUT
#endif
#ifndef USB_CFG_IMPLEMENT_FN_WRITE
#define USB_CFG_IMPLEMENT_FN_WRITE 0
#endif
#ifndef USB_HAS_CFG_IMPLEMENT_FN_WRITE
#define USB_HAS_CFG_IMPLEMENT_FN_WRITE USB_CFG_IMPLEMENT_FN_WRITE
#endif
#ifndef USB_CFG_IMPLEMENT_FN_READ
#define USB_CFG_IMPLEMENT_FN_READ 0
#endif
#ifndef USB_HAS_CFG_IMPLEMENT_FN_READ
#define USB_HAS_CFG_IMPLEMENT_FN_READ USB_CFG_IMPLEMENT_FN_READ
#endif


/*
 * Device configuration reply, 6 bytes
 *   Byte 0-2: Flash layout
 *    Bit 2:0 - n=log2(pagesize/16) pagesize=16<<n
 *    Bit 3 - Device has 4-page erase (attiny1641/ttiny841/attiny441)
 *    Bit 15:4 - Bootloader start address / 16
 *   Byte 3-4: vmeiosis config (must match user application)
 *    Bit 1 - USB_COUNT_SOF
 *    Bit 2 - USB_CFG_CHECK_DATA_TOGGLING
 *    Bit 3 - USB_CFG_LONG_TRANSEFRS
 *    Bit 4 - USB_CFG_SUPPRESS_INTR_CODE
 *    Byt 5 - USB_CFG_HAVE_FLOWCONTROL
 *    Byt 6 - USB_HAS_CFG_HAVE_INTRIN_ENDPOINT || USB_HAS_CFG_HAVE_INTRIN_ENDPOINT3
 *    Bit 7 - USB_CFG_HAVE_INTRIN_ENDPOINT
 *    Bit 8 - USB_CFG_HAVE_INTRIN_ENDPOINT3
 *    Bit 9 - USB_CFG_IMPLEMENT_FN_WRITE
 *    Bit 10 - USB_CFG_IMPLEMENT_FN_READ
 *    Bit 11 - USB_CFG_IMPLEMENT_FN_WRITEOUT
 *    Bit 12 - USB_HAS_CFG_RX_USER_HOOK
 *    Bit 13 - USB_HAS_CFG_RESET_HOOK
 *    Bit 14 - USB_HAS_CFG_SET_ADDRESS_HOOK
 *    Bit 15 - USB_CFG_IMPLEMENT_REMOTE_WAKE
 *   Byte 4:  SIGNATURE_2
 *   Byte 5:  SIGNATURE_1
 */

#define CC(a, b) a ## b
#define _CC(a, b) CC(a, b)

#define __vector_0_num 0
#define __vector_1_num 1
#define __vector_2_num 2
#define __vector_3_num 3
#define __vector_4_num 4
#define __vector_5_num 5
#define __vector_6_num 6
#define __vector_7_num 7
#define __vector_8_num 8
#define __vector_9_num 9
#define __vector_10_num 10
#define __vector_11_num 11
#define __vector_12_num 12
#define __vector_13_num 13
#define __vector_14_num 14
#define __vector_15_num 15
#define __vector_16_num 16
#define __vector_17_num 17
#define __vector_18_num 18
#define __vector_19_num 19
#define __vector_20_num 20
#define __vector_21_num 21
#define __vector_22_num 22
#define __vector_23_num 23
#define __vector_24_num 24
#define __vector_25_num 25
#define __vector_26_num 26
#define __vector_27_num 27
#define __vector_28_num 28
#define __vector_29_num 29
#define __vector_30_num 30
#define __vector_31_num 31

#ifndef _VECTOR
#define _VECTOR(N) __vector_ ## N
#endif

#define USB_CFG_WORD_0 (_CC(USB_INTR_VECTOR, _num) << 8)

#define USB_CFG_WORD_1 \
	(USB_COUNT_SOF << 1) | \
	(USB_CFG_CHECK_DATA_TOGGLING << 2) | \
	(USB_CFG_LONG_TRANSEFRS << 3) | \
	(USB_CFG_SUPPRESS_INTR_CODE << 4) | \
	(USB_CFG_HAVE_FLOWCONTROL << 5) | \
	(USB_CFG_IMPLEMENT_REMOTE_WAKE << 6) | \
	(USB_HAS_CFG_HAVE_INTRIN_ENDPOINT << 7) | \
	(USB_HAS_CFG_HAVE_INTRIN_ENDPOINT3 << 8) | \
	(USB_HAS_CFG_IMPLEMENT_FN_WRITE << 9) | \
	(USB_HAS_CFG_IMPLEMENT_FN_READ << 10) | \
	(USB_HAS_CFG_IMPLEMENT_FN_WRITEOUT << 11) | \
	(USB_HAS_CFG_RX_USER_HOOK << 12) | \
	(USB_HAS_CFG_RESET_HOOK << 13) | \
	(USB_HAS_CFG_SET_ADDRESS_HOOK << 14)
#endif
