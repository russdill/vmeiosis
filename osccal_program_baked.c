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
#include <avr/boot.h>
#include <avr/wdt.h>

#include "osc_ver.h"
#include "pagesize.h"

extern const char __data_load_end;

#define PAGE_BOUNDARY(n) (((n) + PAGESIZE - 1) & ~(PAGESIZE - 1))

#define OSCCAL_ADDR _SFR_IO_ADDR(OSCCAL_REG)

#define SCRATCH_PAGE 0

/* Initial vectors force osccal routine */
void vectors(void)  __attribute__((naked,section(".vectors"),noreturn));
void vectors(void)
{
        asm(
"	rjmp 1f\n"
"	.skip %0, 0xff\n"
"1:	rjmp main\n"
	: : "M"(PAGESIZE - 2)
	);
}

void osccal_program(unsigned char osccal)
{
	unsigned short offset;
	unsigned short i;

	/* Check to see if we've already programmed our buffer with the offset */
	offset = pgm_read_word(SCRATCH_PAGE + 2);
	if (offset == 0xffff) {
		/*
		 * No? Let's walk through empty flash until we find the
		 * bootloader.
		 */
		offset = PAGE_BOUNDARY((unsigned short) &__data_load_end);
		while (pgm_read_byte(offset) == 0xff)
			offset += PAGESIZE;
	}

	/* Clear the scratch area */
	wdt_reset();
	boot_page_erase(SCRATCH_PAGE);

	/* Include a rjmp to osccal in case we restart while updating page 0 */
	boot_page_fill(SCRATCH_PAGE, 0xc000 | (PAGESIZE - 2) / 2); /* rjmp */
	/*
	 * Include the offset to the old bootloader in case we lose power and
	 * need to find it again.
	 */
	boot_page_fill(SCRATCH_PAGE + 2, offset);

	/* Copy the bootloader page so we can rewrite it safely */
	for (i = 4; i < PAGESIZE; i += 2) {
		boot_page_fill(SCRATCH_PAGE + i, pgm_read_word(offset + i));
		if (!((SCRATCH_PAGE + i + 2) & (SPM_PAGESIZE - 1))) {
			wdt_reset();
			boot_page_write((SCRATCH_PAGE + i) & ~(SPM_PAGESIZE - 1));
		}
	}

	/*
	 * Erase the first page of the bootloader in preperation to update
	 * it's osccal programming sequence
	 */
	wdt_reset();
	boot_page_erase(offset);

	/* Store the baked-in OSCCAL value */
	boot_page_fill(offset + 0, 0xe000 | ((osccal & 0xf0) << 4) |
			(osccal & 0xf)); /* LDI r16, osccal */
	boot_page_fill(offset + 2, 0xb800 | (0x10 << 4) | (OSCCAL_ADDR & 0xf) |
			((OSCCAL_ADDR & 0x30) << 5)); /* OUT OSCCAL_REG, r16 */

	/* Fill in the original contents from our scratch page */
	for (i = 4; i < PAGESIZE; i += 2) {
		boot_page_fill(offset + i, pgm_read_word(SCRATCH_PAGE + i));
		if (!((offset + i + 2) & (SPM_PAGESIZE - 1))) {
			wdt_reset();
			boot_page_write((offset + i) & ~(SPM_PAGESIZE - 1));
		}
	}

	/*
	 * When osccal gets integrated as a combined flashing image, the reset
	 * vector points to osccal rather than the bootloader. Update it to
	 * point to the bootloader instead.
	 */
	offset = ((offset - 0x2000 - 2) / 2) & 0xfff;
	boot_page_fill(0, 0xc000 | offset); /* rjmp */
	wdt_reset();
	boot_page_erase(0);
	wdt_reset();
	boot_page_write(0);

	/*
	 * When osccal gets programmed by the flasher tool, the bootloader
	 * uses an rjmp set by the flasher in the last page to jump to the
	 * user program (osccal). Erase that page so re-running osccal isn't
	 * possible.
	 */
	wdt_reset();
	boot_page_erase(offset - PAGESIZE);

	/* Return to bootloader */
	asm("rjmp vectors");
	__builtin_unreachable();
}
