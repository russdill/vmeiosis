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
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>

/* Don't change by more than this value at once */
#define OSCCAL_MAXD	32

/* Taken as same frequency value in both ranges by inspection */
#define RANGE1_SWITCH 96
#define RANGE2_SWITCH 128

#include "osc_ver.h"

extern short osccal_loop(void);
extern void osccal_program(unsigned char osccal) __attribute__((noreturn));

static void adjust_osccal(unsigned char target)
{
	unsigned char curr = OSCCAL_REG;

#if OSC_VER == 5
	unsigned char orig_target = target;
	if (curr < 0x80 && target >= 0x80)
		/* Get to range 1 switchover point first */
		target = RANGE1_SWITCH;
	if (curr >= 0x80 && target < 0x80)
		/* Get to range 2 switchover point first */
		target = RANGE2_SWITCH;
#endif

	/* Don't adjust OSCCAL too much at once */
	while (curr != target) {
		if (curr < target) {
			if (target - curr > OSCCAL_MAXD)
				curr += OSCCAL_MAXD;
			else
				curr = target;
		} else {
			if (curr - target > OSCCAL_MAXD)
				curr -= OSCCAL_MAXD;
			else
				curr = target;
		}
#if OSC_VER == 5
		if (orig_target != target && curr == target) {
			/* Switching to desired range */
			curr = RANGE1_SWITCH + RANGE2_SWITCH - target;
			target = orig_target;
		}
#endif
		OSCCAL_REG = curr;
		/* Wait for new setting to become stable (AVR054 2.5) */
		_delay_us(5);
	}
}

static unsigned short calibrate_range(unsigned char osccal_min, unsigned char osccal_max)
{
	unsigned char step;
	unsigned char osccal;
	short delta;
	unsigned short deviation;
	unsigned short min_deviation;
	unsigned char best;

	/* Binary search for ideal OSCCAL (AVR054 4.1.1)  */
	step = (osccal_max - osccal_min + 1) / 2;
	osccal = osccal_min + step;

	for (;;) {
		adjust_osccal(osccal);

		delta = osccal_loop();

		if (!delta)
			return 0;

		step /= 2;

		if (!step)
			break;

		if (delta > 0)
			osccal += step;
		else
			osccal -= step;
	}

	/* Neighbor search (AVR054 4.1.2) */

	/* Try two below, two above (clip to osccal_min/max) */
	if (osccal > osccal_min + 2)
		osccal_min = osccal - 2;
	if (osccal < osccal_max - 2)
		osccal_max = osccal + 2;

	best = osccal;
	min_deviation = 0xffff;
	adjust_osccal(osccal_min);
	for (osccal = osccal_min; osccal <= osccal_max; osccal++) {
		adjust_osccal(osccal);
		delta = osccal_loop();
		if (!delta) {
			return 0;
		}
		if (delta < 0)
			deviation = -delta;
		else
			deviation = delta;
		if (deviation <= min_deviation) {
			min_deviation = deviation;
			best = osccal;
		}
		if (osccal == 0xff)
			break;
	}

	adjust_osccal(best);
	return min_deviation;
}

int main(void)
{
	asm("clr __zero_reg__");
	/*
	 * It may be necessary to clip the ranges here as the AVR054
	 * specifies:
	 *
	 *     it is important to notice that it is not recommended to tune
	 *     the oscillator more than 10% off the base frequency specified
	 *     in the datasheet.
	 *
	 * However, it's not clear how to best obtain accurate values for
	 * those limits. This is mostly a potential problem for dual range
	 * devices. An an example, the middle of range 2 for an ATmega169P
	 * is approximately 10MHz and the base frequency is 8MHz (+25%).
	 */
#if (OSC_VER == 1) || (OSC_VER == 2) || (OSC_VER == 3)
	calibrate_range(0, 0xff);
#elif OSC_VER == 4
	calibrate_range(0, 0x7f);
#elif OSC_VER == 5
	unsigned short min_deviation = min_deviation;
	unsigned char osccal = osccal;
	unsigned char i = 0;
	unsigned short deviation;
again:
	deviation = calibrate_range(i, i + 0x7f);
	if (i == 0) {
		i = 0x80;
		osccal = OSCCAL_REG;
		min_deviation = deviation;
		goto again;
	}
	if (min_deviation < deviation)
		/* Switch back */
		adjust_osccal(osccal);
#else
#error "Invalid OSC_VER"
#endif

	/* Calibration success, store new value */
	osccal_program(OSCCAL_REG);
	__builtin_unreachable();
}
