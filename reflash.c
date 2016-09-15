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

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/boot.h>

#include "pagesize.h"
#include "bitops.h"
#include "usbconfig.h"

/* From usbdrv.h */
#define USB_CONCAT(a, b)            a ## b
#define USB_CONCAT_EXPANDED(a, b)   USB_CONCAT(a, b)

#define USB_OUTPORT(name)           USB_CONCAT(PORT, name)
#define USB_DDRPORT(name)           USB_CONCAT(DDR, name)

#define USB_PULLUP_OUT  USB_OUTPORT(USB_CFG_PULLUP_IOPORTNAME)
#define USB_PULLUP_DDR  USB_DDRPORT(USB_CFG_PULLUP_IOPORTNAME)
#define USBDDR          USB_DDRPORT(USB_CFG_IOPORTNAME)

/* Start of the new bootloader code appended to the end of reflash */
extern unsigned char __ctors_end;
/* End of the new bootloader code */
extern unsigned char _etext;

__attribute__((naked,section(".vectors"),noreturn)) void reflash(void)
{
	uint16_t src;
	uint16_t dst;
	uint16_t rst;
	uint8_t b;

	/*
	 * Allocate a couple of pages at start.
	 * At start, page 0 jumps to the bootloader (reset vector modified by
	 * flasher). We begin by writing a jump direct to the reflash code
	 * in page 1 along with saving the osccal programming information
	 * from the current bootloader. When can then erase page 0. If the
	 * process gets interrupted on next boot reflash will immediately
	 * resume. When we are finished, we rewrite page 0 to jump to the
	 * new bootloader.
	 *
	 * Note because the jumps to reflash stored initially by the flasher
	 * in the last user page will be erased, reflash will not run if
	 * the bootloader exits, the bootloader will merely restart.
	 * */
	asm(
"	rjmp __start\n"
"	.zero %0\n"
"__start:"
	: : "M"(PAGESIZE * 2 - 2));

	/* Drop off USB bus for a while */
#ifdef USB_CFG_PULLUP_IOPORTNAME
	USB_PULLUP_DDR &= ~(1<<USB_CFG_PULLUP_BIT);
	USB_PULLUP_OUT &= ~(1<<USB_CFG_PULLUP_BIT);
#else
	USBDDR |= (1<<USB_CFG_DMINUS_BIT);
#endif

	/* Read the reset vector, see if it still contains a jump, or if we've
	 * erased it */
	if ((pgm_read_byte(1) & 0xf0) == 0xc0) {
		/* Calculate start of bootloader based on bootloader page count */
		b = pgm_read_byte(FLASHEND + 1 - 4);
	       	rst = FLASHEND + 1 - ((uint16_t) b << LOG2(PAGESIZE));

		/* Add a jump straight to reflash */
		boot_page_fill(PAGESIZE,
			0xc000 | ((((uint16_t) (PAGESIZE * 2 - PAGESIZE) >> 1) - 1) & 0xfff));
		/* Save the osccal programming sequence in the last page */
		boot_page_fill(PAGESIZE + 2, pgm_read_word(rst));
		boot_page_fill(PAGESIZE + 4, pgm_read_word(rst + 2));
		wdt_reset();
		boot_page_erase(PAGESIZE);
		wdt_reset();
		boot_page_write(PAGESIZE);

		/* kill the page containing the rewritten jump to bootloader */
		wdt_reset();
		boot_page_erase(0);
	}

	/*
	 * The configuration block at the end of flash gives the start address
	 * of the new bootloader.
	 */
	b = pgm_read_byte((uint16_t) &_etext - 4);
	rst = FLASHEND + 1 - ((uint16_t) b << LOG2(PAGESIZE));
	dst = rst;
	src = PAGESIZE + 2; /* Start from osccal words */

	/* Erase the bootloader and write the new one */
	do {
		unsigned char i;
		if (!(dst % PAGESIZE)) {
			wdt_reset();
			boot_page_erase(dst);
		}

		for (i = 0; i < SPM_PAGESIZE / 2; i++) {
			boot_page_fill(dst, pgm_read_word(src));
			src += 2;
			dst += 2;
			if (src == PAGESIZE + 6)
				src = (uint16_t) &__ctors_end + 4;
		}

		wdt_reset();
		boot_page_write(dst - SPM_PAGESIZE);
	} while (src < (uint16_t) &_etext);

	/*
	 * Make sure the page before the bootloader is empty. This ensures
	 * the new bootloader doesn't try to run reflash again.
	 */
	wdt_reset();
	boot_page_erase(rst - PAGESIZE);

	/* Update the vectors with a single rjmp to the bootloader */
	boot_page_fill(0, 0xc000 | (((rst >> 1) - 1) & 0xfff));
	wdt_reset();
	boot_page_write(0);

	/* Return to the bootloader */
	asm("rjmp 0");
	__builtin_unreachable();
}
