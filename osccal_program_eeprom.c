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
#include <avr/eeprom.h>

__attribute__((naked,section(".vectors"),noreturn)) void vectors(void)
{
        asm("rjmp main\n");
}

void osccal_program(unsigned char osccal)
{
	eeprom_write_byte(EEPROM_OSCCAL, osccal);
	for(;;);
	__builtin_unreachable();
}
