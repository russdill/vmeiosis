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

	.macro bl_vector name
	.global __bl_\name
__bl_\name:
	rjmp \name
	.endm

	.macro vme_vector name
	.global \name
\name:
	.skip 2
	.endm

#include "usbdrv.h"
#include "pagesize.h"
#include "configuration.h"

	.global __pagesize
	__pagesize = PAGESIZE

/*
 * Vectors into shared v-usb functions that are part of the bootloader.
 * Located at head of bootloader.
 */
	.section .vectors, "ax", @progbits
/*
 * Vectors at the very end of (user) flash space. These get written last so
 * that if flashing fails, they will be blank (sbrs r31, 7). If bit 7 in r31
 * is set, it will skip the user_vector word and go straight to vectors. If
 * it is not set, it will run the user_vector word and then the vectors.
 * The command line tool fills in user_reset from the app's reset vector and
 * saves osccal from it's current value.
 */
	.global user_reset
	user_reset = . - 4

	.global user_vector
	user_vector = . - 2

	.section .end_vectors, "ax", @progbits

.global __end_vectors
__end_vectors:

	/* At start because it is optional */
#if !USB_CFG_SUPPRESS_INTR_CODE
#if USB_CFG_HAVE_INTRIN_ENDPOINT || USB_CFG_HAVE_INTRIN_ENDPOINT3
#if USB_CFG_HAVE_INTRIN_ENDPOINT3
	bl_vector	usbGenericSetInterrupt
#else
	bl_vector	usbSetInterrupt
#endif
#endif
#endif
	bl_vector	USB_INTR_VECTOR /* Used by flashing tool */
	bl_vector	usbInit
	bl_vector	usbPoll

	.word		USB_CFG_WORD_0 + __bl_num_pages
	.word		USB_CFG_WORD_1
.global __end_vectors_end
__end_vectors_end:


/*
 * Symbols pointing to v-usb callbacks within user app. These are located
 * just after the AVR interrupt vectors
 */
	.section .vme_vectors, "ax", @nobits
	.skip _VECTORS_SIZE
	vme_vector	user_usbFunctionSetup
	vme_vector	user_usbDriverDescriptor
	vme_vector	user_usbDescriptors
#if USB_CFG_IMPLEMENT_FN_WRITE
	vme_vector	user_usbFunctionWrite
#endif
#if USB_CFG_IMPLEMENT_FN_READ
	vme_vector	user_usbFunctionRead
#endif
#if USB_CFG_IMPLEMENT_FN_WRITEOUT
	vme_vector	user_usbFunctionWriteOut
#endif
#if USB_CFG_RESET_HOOK
	vme_vector	user_usbResetHook
#endif
#if USB_CFG_SET_ADDRESS_HOOK
	vme_vector	user_usbSetAddressHook
#endif
#if USB_CFG_RX_USER_HOOK
	vme_vector	user_usbRxUserHook
#endif
