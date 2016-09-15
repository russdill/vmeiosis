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

#define __SFR_OFFSET 0

#include <avr/io.h>
#include "usbdrv/usbdrv.h"
#include "div.h"

/*
 * Idle pulse come at 1ms intervals. Each interval starts with an SEO that is
 * low for 2 clock cycles (2 / 1.5MHz).
 *
 * Typical calibration values vary from half of nominal to double.
 */
#define F_CPU_MIN 8000000
#define F_CPU_MAX 32000000

#define F_USB 1500000
#define IDLE_MS 1

#define IDLE_MIN ((IDLE_MS * F_CPU_MIN / 1000 - 2 * F_CPU_MIN / F_USB - 8) / 8)
#define IDLE_MAX DIV_ROUND_UP((IDLE_MS * F_CPU_MAX / 1000 - 2 * F_CPU_MAX / F_USB - 2), 8)
#define IDLE_MIN_REMAIN (IDLE_MAX - IDLE_MIN)
#define IDLE_TGT DIV_ROUND((IDLE_MS * F_CPU / 1000 - 2 * F_CPU / F_USB - 5), 8)
#define IDLE_TGT_REMAIN (IDLE_MAX - IDLE_TGT)

#define SE0_MIN ((2 * F_CPU_MIN / F_USB - 6) / 5)
#define SE0_MAX DIV_ROUND_UP((2 * F_CPU_MAX / F_USB - 2), 5)
#define SE0_MIN_REMAIN (SE0_MAX - SE0_MIN)

restart:
	in	r20, TIFR
	sbrs	r20, TOV1
	rjmp	skip_cycle
; Long timeout, host is not toggling keep alive, try to "re-enumerate"
	sbi	USBDDR, USBMINUS
	clr	r26
	clr	r27
	ldi	r21, 10
1:	sbiw	x, 1
	sbci	r21, 0
	wdr
	brne 1b
	cbi	USBDDR, USBMINUS

/* Count how many CPU cycles pass during FREQ_CYCLES of the stimulus clock. */
.global osccal_loop
osccal_loop:

	ldi	r21, 0xf	; Max pre-scaler
	out	TCCR1, r21
	out	TCNT1, r1	; Reset long timer
	ldi	r20, _BV(TOV1)
	out	TIFR, r20

skip_cycle:
	ldi	r26, lo8(IDLE_MAX)
	ldi	r27, hi8(IDLE_MAX)
	ldi	r18, hi8(IDLE_MIN_REMAIN)
	ldi	r21, SE0_MAX
	ldi	r19, SE0_MAX
	clr	r0

; Wait for K (USBMINUS low, USBPLUS high) for at least 256 * 7 cycles
1:	wdr
	sbis	USBIN, USBPLUS ; Reset timer if USBPLUS is high
	sbis	USBIN, USBMINUS ; Don't reset timer if USBMINUS is high
	clr	r0
	dec	r0
	brne	1b

; Wait for SEO0 (USBMINUS/USBPLUS low) (8 cycles per loop, exit time 4-8 cycles)
1:	wdr
	in	r20, TIFR
	sbrs	r20, TOV1
	sbic	USBIN, USBPLUS
	rjmp	restart
	sbic	USBIN, USBMINUS
	rjmp	1b


; Measure SEO0 state (USBMINUS/USBPLUS low) (7 cycles per loop, exit time 2-8 cycles)
1:	dec	r19
	breq	restart		; Too long
	sbic	USBIN, USBPLUS	; USBPLUS going high is always a restart
	rjmp	restart
	sbis	USBIN, USBMINUS	; Waiting for USBMINUS to go high
	rjmp	1b

	cpi	r19, SE0_MIN_REMAIN	; Too short (2 cycles)
	brcc	restart

; Measure K state (USBPLUS low, USBMINUS high) (8 cycles per loop, exit time 2-9 cycles)
1:	sbiw	x, 1
	breq	restart		; Too long
	sbic	USBIN, USBPLUS	; USBPLUS going high is always a restart
	rjmp	restart
	sbic	USBIN, USBMINUS	; Waiting for USBMINUS to go low
	rjmp	1b
	
; Measure SEO0 state (USBMINUS/USBPLUS low) (7 cycles per loop, exit time 2-8 cycles)
1:	dec	r21
	breq	restart		; Too long
	sbic	USBIN, USBPLUS	; USBPLUS going high is always a restart
	rjmp	restart
	sbis	USBIN, USBMINUS	; Waiting for USBMINUS to go high
	rjmp	1b

	cpi	r21, SE0_MIN_REMAIN	; Too short
	brcc	restart

	cpi	r26, lo8(IDLE_MIN_REMAIN)
	cpc	r27, r18
	brcc	restart		; Too short

	ldi	r19, hi8(IDLE_TGT_REMAIN)
	subi	r26, lo8(IDLE_TGT_REMAIN)
	sbc	r27, r19

	movw	r24, r26

; Make sure hardware is returned to reset state
	out	TIFR, r1
	out	TCCR1, r1
	out	TCNT1, r1
	ret
