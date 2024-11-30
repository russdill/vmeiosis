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

#include <stdbool.h>

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <avr/cpufunc.h>
#include <avr/interrupt.h>

#include <util/delay.h>

#define usbInit real_usbInit
#define usbPoll real_usbPoll
#define usbGenericSetInterrupt real_usbGenericSetInterrupt
#define usbSetInterrupt real_usbSetInterrupt

#include "osc_ver.h"
#include "usbconfig.h"
#include "vmeconfig.h"

#if !USB_CFG_USBINIT_CONNECT
#error vmeiosis requires USB_CFG_USBINIT_CONNECT
#endif

#if !USB_CFG_MODE_IRQLESS
#error vmeiosis requires USB_CFG_MODE_IRQLESS
#endif

/* command system schedules functions to run in the main loop */
enum {
	cmd_exit = 128,
};

#define isUserMode() !(USB_GPIOR(VME_MODE_GPIOR_IDX) & _BV(VME_MODE_GPIOR_BIT))

/* Wrap the hooks if so enabled */
#if USB_HAS_CFG_SET_ADDRESS_HOOK
#define USB_SET_ADDRESS_HOOK() do { \
	if (isUserMode()) user_usbSetAddressHook(); \
} while(0)
#endif

#if USB_HAS_CFG_RX_USER_HOOK
#define USB_RX_USER_HOOK(data, len) do { \
	if (isUserMode()) user_usbRxUserHook(data, len); \
} while(0)
#endif

#if USB_HAS_CFG_RESET_HOOK
#define USB_RESET_HOOK(isReset) do { \
	if (isUserMode()) user_usbResetHook(isReset); \
} while(0)
#endif


#include "usbdesc.c"
#include "usbdrv.c"
#undef usbInit
#undef usbPoll
#undef usbGenericSetInterrupt
#undef usbSetInterrupt

extern void user_usbFunctionWriteOut(uchar *data, uchar len);
extern uchar user_usbFunctionRead(uchar *data, uchar len);
extern uchar user_usbFunctionWrite(uchar *data, uchar len);
extern usbMsgLen_t user_usbDriverDescriptor(usbRequest_t *rq);

/* Wrap the callbacks if so enabled */
#if USB_CFG_IMPLEMENT_FN_WRITEOUT
USB_PUBLIC void usbFunctionWriteOut(uchar *data, uchar len)
{
	if (isUserMode())
		user_usbFunctionWriteOut(data, len);
}
#endif

#if USB_CFG_IMPLEMENT_FN_READ
USB_PUBLIC uchar usbFunctionRead(uchar *data, uchar len)
{
	if (isUserMode())
		return user_usbFunctionRead(data, len);
	return 0;
}
#endif

#if USB_CFG_IMPLEMENT_FN_WRITE
USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len)
{
	if (isUserMode())
		return user_usbFunctionWrite(data, len);
	return 0;
}
#endif

/* Silence some compiler warnings */
#if !USB_CFG_SUPPRESS_INTR_CODE
#if USB_CFG_HAVE_INTRIN_ENDPOINT
#if USB_CFG_HAVE_INTRIN_ENDPOINT3
void usbGenericSetInterrupt(uchar *data, uchar len, usbTxStatus_t *txStatus)
{
	real_usbGenericSetInterrupt(data, len, txStatus);
}
USB_PUBLIC void usbSetInterrupt(uchar *data, uchar len) __attribute__((unused));
#else
void usbSetInterrupt(uchar *data, uchar len)
{
	real_usbSetInterrupt(data, len);
}
#endif
#endif
#if USB_CFG_HAVE_INTRIN_ENDPOINT3
USB_PUBLIC void usbSetInterrupt3(uchar *data, uchar len) __attribute__((unused));
#endif
#endif

USB_PUBLIC usbMsgLen_t usbFunctionDescriptor(usbRequest_t *rq)
{
	return 0;
}

/* usbPoll and usbInit are declared static, so we wrap them here to get symbols
 * for exporting */
void usbInit(void)
{
	real_usbInit();
}

void usbPoll(void)
{
	real_usbPoll();
}

/* Just to save a couple of bytes */
__attribute__((used,naked)) static void store_usbMsgPtr(void)
{
	asm(
"	sts	usbMsgPtr, r26\n"
"	sts	usbMsgPtr+1, r27\n"
"	ret\n"
	);
}

/* usbDriverDescriptor() is similar to usbFunctionDescriptor(), but used
 * internally for all types of descriptors.
 *
 * Utilizes inline asm so it can get inlined into usbPoll.
 */
USB_PUBLIC usbMsgLen_t usbCustomDriverDescriptor(usbRequest_t *rq)
{
	register uchar ret asm("r24");
	asm (
"usbCustomDriverDescriptorStart:\n"
"	ldd	r20, Y+2\n" /* idx */
"	ldd	r19, Y+3\n" /* type */
"	ldi	r18, %[is_rom]\n"
"	mov	%[usbMsgFlags], r18\n"
/* Try the user descriptors if in user mode */
"	ldi	r30, lo8(user_usbDescriptors)\n"
"	ldi	r31, hi8(user_usbDescriptors)\n"
#if defined(__AVR_TINY__)
"	ldd	r26, z+\n"
"	ldd	r27, z\n"
#elif defined(__AVR_HAVE_LPMX__)
"	lpm	r26, z+\n"
"	lpm	r27, z\n"
#else
"	lpm\n"
"	mov	r26, __tmp_reg__\n"
"	adiw	r30, 1\n"
"	lpm\n"
"	mov	r27, __tmp_reg__\n"
#endif
"	adiw	r26, 0\n"
/* If we are in BL mode, go straight to loading BL descriptors */
"	sbis	%[gpior_bl_reg], %[gpior_bl_bit]\n"
/* For user mode, skip loading BL descriptors if user ones are nonzero */
"	brne	1f\n"
"again:\n"
"	clr	r18\n" /* r18 is used as "do again" indication */
"	ldi	r26, lo8(usbDescriptors)\n"
"	ldi	r27, hi8(usbDescriptors)\n"
"1:	clr	%[ret]\n"
"read_loop:\n"
"	add	r26, %[ret]\n" /* Advance read pointer by len */
"	adc	r27, __zero_reg__\n"
"	movw	r30, r26\n"
#if defined(__AVR_TINY__)
"	ld	%[ret], z+\n"
#elif defined(__AVR_HAVE_LPMX__)
"	lpm	%[ret], z+\n" /* Load new len value */
#else
"	lpm\n"
"	adiw	r30, 1\n"
"	mov	%[ret], __tmp_reg__\n"
#endif
"	tst	%[ret]\n" /* Be done if it is zero */
"	breq	done_scanning\n"
#if defined(__AVR_TINY__)	/* Get the type byte */
"	ldd	__tmp_reg__, z+\n"
#elif defined(__AVR_HAVE_LPMX__)
"	lpm	__tmp_reg__, z+\n"
#else
"	lpm\n"
"	adiw	r30, 1\n"
#endif
"	cp	__tmp_reg__, r19\n" /* Check that the entry type matches */
"	brne	read_loop\n"
"	dec	r20\n" /* check that the index matches (reached zero) */
"	brpl	read_loop\n"
"	cpi	r19, %[usbdescr_config]\n" /* Additional len byte */
"	brne	1f\n"
#if defined(__AVR_TINY__)
"	ldd	%[ret], z\n"
#elif defined(__AVR_HAVE_LPMX__)
"	lpm	%[ret], z\n"
#else
"	lpm\n"
"	mov	%[ret], __tmp_reg__\n"
#endif
"1:\n"
"	rcall	store_usbMsgPtr\n"
"	rjmp	usbCustomDriverDescriptorEnd\n"
"done_scanning:\n"
"	tst	r18\n" /* Try again with BL descriptors if "do again" is set */
"	brne	again\n"
"	sbic	%[gpior_bl_reg], %[gpior_bl_bit]\n"
"	rjmp	usbCustomDriverDescriptorEnd\n"
"	movw	r24, r28\n"
"	rcall	user_usbDriverDescriptor\n"
"usbCustomDriverDescriptorEnd:\n"
	:	[ret] "=r"(ret)
	:	[usbMsgFlags] "g"(usbMsgFlags),
		[gpior_bl_reg] "I" (_SFR_IO_ADDR(USB_GPIOR(VME_MODE_GPIOR_IDX))),
		[gpior_bl_bit] "M" (VME_MODE_GPIOR_BIT),
		[usbdescr_config] "M" (USBDESCR_CONFIG),
		[is_rom] "I" (USB_FLG_MSGPTR_IS_ROM),
		[rq] "y" (rq)
	:	"r18", "r19", "r20", "r21", "r22", "r23", "r25", "r26", "r27", "r30", "r31"
	
	);
	if (!ret)
		ret = usbDriverDynamicDescriptor(rq);
	return ret;
}

/* Utilizes inline asm so it can get inlined into usbPoll. */
USB_PUBLIC uint8_t usbFunctionSetup(uint8_t data[8])
{
	register uint8_t ret asm("r24");
	asm(
	"usbFunctionSetupStart:\n"
	"	movw	r24, r28\n"

	/* In user mode, just call the user_usbFunctionSetup */
	"	sbis	%[gpior_bl_reg], %[gpior_bl_bit]\n"
	"	rcall	user_usbFunctionSetup\n"
	"	sbis	%[gpior_bl_reg], %[gpior_bl_bit]\n"
	"	rjmp	usbFunctionSetupEnd\n"

	/* Clear T on Host-to-device, set T on Device-to-host */
	"	bst	%[bmRequestType], 7\n"
	"	brts	1f\n"
#if VME_CFG_CRC
	"	ldi	r22, 10\n"
	"	rcall	usbCrc16Append\n"
	"	subi	r24, 0xfe\n"
	"	sbci	r25, 0x4f\n"
	"	breq	1f\n"
	"	rjmp	__init\n" /* re-enumerate on CRC error */
#endif
	/* Load the values we'd use for a read */
	"1:	ldd	r26, Y + 4\n" /* wIndex */
	"	ldd	r27, Y + 5\n"
	"	rcall	store_usbMsgPtr\n"
#ifdef USB_MSGFLAGS_REG
	"	ldd	%[usbMsgFlags], Y + 1\n" /* bRequest */
#else
	"	ldd	r22, Y + 1\n" /* bRequest */
	"	sts	usbMsgFlags, r22\n"
#endif
	/* Read only occurs if ret is non-zero */
	"	ldd	%[ret], Y + 6\n" /* wLength */
	"usbFunctionSetupEnd:\n"
	:	[ret] "=r"(ret)
	:	[gpior_bl_reg] "I" (_SFR_IO_ADDR(USB_GPIOR(VME_MODE_GPIOR_IDX))),
		[gpior_bl_bit] "M" (VME_MODE_GPIOR_BIT),
		[is_rom] "M"(USB_FLG_MSGPTR_IS_ROM),
#ifdef USB_MSGFLAGS_REG
		[usbMsgFlags] "r"(usbMsgFlags),
#endif
		[bmRequestType] "r"(data[0]),
		"y" (data)
	: "memory",
	  "r18", "r19", "r20", "r21", "r22", "r23", "r25", "r26", "r27", "r30", "r31"
	);
	return ret;
}

/* Marked vectors to keep close to start of program for brne user_reset */
void __init(void) __attribute__((naked,noreturn,section(".vectors")));
void __init(void)
{
		asm(
"	rjmp	1f\n"		/* Space for loading osccal value to r16 */
"	out	%[osccal_reg], r16\n"
"1:	in	r24, %[mcusr]\n"	/* Save reset reason */
"	clr	__zero_reg__\n"
"	out	%[mcusr], __zero_reg__\n" /* Clear reset reason register */

/* Setup r1 and SP */
"bl_exit:\n"

/* Clear BSS (Requires BSS with size >0 <255) */
"	ldi	r30, lo8(__bss_start)\n"
"	ldi	r31, hi8(__bss_start)\n"
"1:	st	Z+, __zero_reg__\n"
"	cpi	r30, lo8(__bss_end)\n"
"	brne	1b\n"

#if USB_CFG_MODE_IRQ
"	out	%[usb_intr_enable], __zero_reg__\n"	/* "unconfigure V-USB */
#endif
"	out	%[usb_intr_cfg], __zero_reg__\n"
#if !USB_CFG_MODE_IRQ || VME_MODE_GPIOR_IDX == USB_MODE_IRQ_GPIOR_IDX
"	out	%[gpior_bl_reg], __zero_reg__\n"
#else
"	cbi	%[gpior_bl_reg], %[gpior_bl_bit]\n"	/* Clear user/irq mode */
#if USB_CFG_MODE_IRQ
"	cbi	%[gpior_irqless_reg], %[gpior_irqless_bit]\n"
#endif
#endif

/* Jump to user program if it's a power on, brown out reset or cmd_exit. Note
 * that the reason will be stored in r24 for the user program */
"	andi	r24, %[rst_mask]\n"
"	brne	__init - 4\n"

/* Set bootloader and irqless mode flags */
"	sbi	%[gpior_bl_reg], %[gpior_bl_bit]\n"
#if USB_CFG_MODE_IRQ && (VME_MODE_GPIOR_IDX != USB_MODE_IRQ_GPIOR_IDX || \
	VME_MODE_GPIOR_BIT != USB_MODE_IRQ_GPIOR_BIT)
"	sbi	%[gpior_irqless_reg], %[gpior_irqless_bit]\n"
#endif

/* Initialize V-USB */
"	rcall	usbInit\n"

/* Bootloader main loop */
"bl_main_loop:\n"
"	set\n"			/* Set T flag (no write message) */
"	rcall	usbPoll\n"
"	wdr\n"
"	brts	bl_main_loop\n" /* T flag is cleared, we have a write command */

/* Load the offset to the current command buffer */
"	ldi	r26, lo8(%[rx_buf])\n"
"	ldi	r27, hi8(%[rx_buf])\n"
"	lds	r24, usbInputBufOffset\n"
"	sub	r26, r24\n"
"	sbci	r27, 0\n"  /* almost certainly not needed */
"	ld	r24, X+\n" /* bRequest */
"	cpi	r24, %[cmd_exit]\n" /* check if it's exit */
"	breq	bl_exit\n"

/* Run given flash command */
"	ld	r0, X+\n" /* wValue */
"	ld	r1, X+\n"
"	ld	r30, X+\n" /* wIndex */
"	ld	r31, X+\n"
"	out	%[spm], r24\n"
"	spm\n"
#if defined(__AVR_ATmega161__) || defined(__AVR_ATmega163__) \
	|| defined(__AVR_ATmega323__)
"	.word	0xffff\n"
"	nop\n"
#endif
"1:	clr	__zero_reg__\n"
"	rjmp	bl_main_loop\n"
	:
	:	[osccal_reg] "I"(_SFR_IO_ADDR(OSCCAL_REG)),
		[spm] "I" (_SFR_IO_ADDR(__SPM_REG)),
		[mcusr] "I" (_SFR_IO_ADDR(MCUSR)),
		[cmd_exit] "M" (cmd_exit),
		[rx_buf] "i" (usbRxBuf + USB_BUFSIZE + 2),
		[usb_bufsize] "I" (USB_BUFSIZE),
		[gpior_bl_reg] "I" (_SFR_IO_ADDR(USB_GPIOR(VME_MODE_GPIOR_IDX))),
#if USB_CFG_MODE_IRQ
		[gpior_irqless_reg] "I" (_SFR_IO_ADDR(USB_MODE_IRQ_GPIOR)),
#endif
		[usb_intr_enable] "I" (_SFR_IO_ADDR(USB_INTR_ENABLE)),
		[usb_intr_cfg] "I" (_SFR_IO_ADDR(USB_INTR_CFG)),
		[gpior_bl_bit] "M" (VME_MODE_GPIOR_BIT),
		[gpior_irqless_bit] "M" (USB_MODE_IRQ_GPIOR_BIT),
		[ramendl] "M" (RAMEND & 0xff),
		[ramendh] "M" (RAMEND >> 8),
		[rst_mask] "M"(_BV(BORF) | _BV(PORF) | cmd_exit)
	);
}
