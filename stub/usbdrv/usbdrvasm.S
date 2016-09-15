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

#define __SFR_OFFSET 0      /* used by avr-libc's register definitions */
#include <usbdrv/usbportability.h>
#include <usbdrv/usbdrv.h>         /* for common defs */

#ifndef EEWE
#define EEWE EEPE
#endif
#ifndef EEMWE
#define EEMWE EEMPE
#endif

	.section .user_signatures, "a", @progbits
	rjmp	reset_vector /* Reset */
	.zero	SPM_PAGESIZE - 2
	.word	USB_CFG_WORD_0
	.word	USB_CFG_WORD_1
reset_vector:
	sbrs	r24, 7	/* Only run if requested by the bootloader */
	rjmp	0

        /* Drop off USB bus for a while */
#ifdef USB_CFG_PULLUP_IOPORTNAME
	cbi	USB_PULLUP_DDR, USB_CFG_PULLUP_BIT
	cbi	USB_PULLUP_OUT, USB_CFG_PULLUP_BIT
#else
	sbi	USBDDR, USBMINUS
#endif
	clr	r26 /* eeprom write offset */
	clr 	r27
#if defined(__AVR_TINY__)
	ldi	r30, lo8(eeprom_program_end + __AVR_TINY_PM_BASE_ADDRESS__)
	ldi	r31, hi8(eeprom_program_end + __AVR_TINY_PM_BASE_ADDRESS__)
#else
	ldi	r30, lo8(eeprom_program_end) /* codes block pointer */
	ldi	r31, hi8(eeprom_program_end)
#endif

codes_loop:
#if defined(__AVR_HAVE_LPMX__)
	lpm	r16, z+ /* Read current skip value */
#elif defined(__AVR_TINY__)
	ld	r16, z+
#else
	lpm
	adiw	r30, 1
	mov	r16, r0
#endif
	cpi	r16, 0xff	/* 0xff is end */
	breq	0		/* Return to bootloader */
	add	r26, r16	/* Advance ee offset by skip */
	adc	r27, r1

#if defined(__AVR_HAVE_LPMX__)	/* Read in length byte */
	lpm	r17, z+
#elif defined(__AVR_TINY__)
	ld	r17, z+
#else
	lpm
	adiw	r30, 1
	mov	r17, r0
#endif
write_loop:
	out	EEARL, r26
	out	EEARH, r27
	adiw	r26, 1
	sbi	EECR, EERE	/* Read current value */
	in	r18, EEDR		/* Skip if byte is "correct"  */
#if defined(__AVR_HAVE_LPMX__)
	lpm	r0, z+
#elif defined(__AVR_TINY__)
	ld	r0, z+
#else
	lpm
	adiw	r30, 1
#endif
	cp	r18, r0
	breq	match
	out	EEDR, r0
	sbi	EECR, EEMWE /* Master program enable */
	sbi	EECR, EEWE /* Program enable */
1:	wdr
	sbic	EECR, EEWE /* Wait for operation to complete */
	rjmp	1b
match:
	dec	r17
	breq	codes_loop
	rjmp	write_loop
eeprom_program_end:


/* NOTE: .vectors section *must* be the correct length for this to append in
 * the right location */
	.section .vectors
	.subsection 8192 /* Place beyond any other .vectors code */
	rjmp usbFunctionSetup
	rjmp usbDriverDescriptor
	.word usbDescriptors
#if USB_HAS_CFG_IMPLEMENT_FN_WRITE
#if USB_CFG_IMPLEMENT_FN_WRITE
	rjmp usbFunctionWrite
#else
1:	rjmp 1b /* This being called is a user program bug */
#endif
#elif USB_CFG_IMPLEMENT_FN_WRITE
#error USB_CFG_IMPLEMENT_FN_WRITE not supported by bootloader
#endif
#if USB_HAS_CFG_IMPLEMENT_FN_READ
#if USB_CFG_IMPLEMENT_FN_READ
	rjmp usbFunctionRead
#else
1:	rjmp 1b /* This being called is a user program bug */
#endif
#elif USB_CFG_IMPLEMENT_FN_READ
#error USB_CFG_IMPLEMENT_FN_READ not supported by bootloader
#endif
#if USB_HAS_CFG_IMPLEMENT_FN_WRITEOUT
#if USB_CFG_IMPLEMENT_FN_WRITEOUT
	rjmp usbFunctionWriteOut
#else
	ret
#endif
#elif USB_CFG_IMPLEMENT_FN_WRITEOUT
#error USB_CFG_IMPLEMENT_FN_WRITEOUT not supported by bootloader
#endif
#if USB_HAS_CFG_RESET_HOOK
#if defined(USB_RESET_HOOK)
	rjmp __usbResetHook
#else
	ret
#endif
#elif defined(USB_RESET_HOOK)
#error USB_RESET_HOOK not supported by bootloader
#endif
#if USB_HAS_CFG_SET_ADDRESS_HOOK
#if defined(USB_SET_ADDRESS_HOOK)
	rjmp __usbSetAddressHook
#else
	ret
#endif
#elif defined(USB_SET_ADDRESS_HOOK)
#error USB_SET_ADDRESS_HOOK not supported by bootloader
#endif
#if USB_HAS_CFG_RX_USER_HOOK
#if defined(USB_RX_USER_HOOK)
	rjmp __usbRxUserHook
#else
	ret
#endif
#elif defined(USB_RX_USER_HOOK)
#error USB_RX_USER_HOOK not supported by bootloader
#endif
	.previous

#if !USB_CFG_SUPPRESS_INTR_CODE
#if USB_HAS_CFG_HAVE_INTRIN_ENDPOINT || USB_HAS_CFG_HAVE_INTRIN_ENDPOINT3
#if USB_HAS_CFG_HAVE_INTRIN_ENDPOINT3
.set usbGenericSetInterrupt, 0x1ff4 + __TEXT_REGION_ORIGIN__
.type usbGenericSetInterrupt, @function
.global usbGenericSetInterrupt
#else
.set usbSetInterrupt, 0x1ff4 + __TEXT_REGION_ORIGIN__
.type usbSetInterrupt, @function
.global usbSetInterrupt
#endif
#endif
#endif
.set systemInit, 0x1ff8 + __TEXT_REGION_ORIGIN__
.type systemInit, @function
.global systemInit
.set usbPoll, 0x1ffa + __TEXT_REGION_ORIGIN__
.type usbPoll, @function
.global usbPoll

