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
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <string.h>

#include "usbdrv.h"

unsigned char force_wdr;

uint8_t usbFunctionSetup(uint8_t data[8])
{
	usbRequest_t *rq = (void *) data;

	if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_VENDOR) {
		wdt_enable(0);
	 	force_wdr = 1;
	}

	return 0;
}

int main(void)
{
	systemInit();
	sei();

	for (;;) {
		if (!force_wdr)
			wdt_reset();
		usbPoll();
	}
}
