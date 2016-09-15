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

#ifndef __vmeconfig_h_included__
#define __vmeconfig_h_included__

#define VME_CFG_CRC                     1
/* Define this to 1 to peform CRC check on flash write/erase commands before
 * executing them. This costs 12 bytes. Not including this risks bricking the
 * device if a CRC error occurs. */
#define VME_MODE_GPIOR_IDX		USB_MODE_IRQ_GPIOR_IDX
/* Assign a GPIORn register and bit (in VME_MODE_GPIOR_BIT) to indicate that
 * the system is currently in bootloader mode. This is needed so that the user
 * callbacks can be called while running in user mode.
 */
#define VME_MODE_GPIOR_BIT		USB_MODE_IRQ_GPIOR_BIT
/* The bit within the selected GPIORn register as mentioned above. Note that
 * if the above two settings match the USB_MODE_IRQ_GPIOR_IDX/
 * USB_MODE_IRQ_GPIOR_BIT settings (and both IRQ and interrupt-free mode are
 * enabled) then the config bit is shared and while in bootloader mode V-USB
 * runs interrupt free, and while in user mode it runs with IRQs.
 */
#define DEVICE attiny85
/* Target device */
#define F_CPU 16500000
/* Target frequency of device. This is used for both oscillator calibration
 * and to select a V-USB backend. */
#define FUSEOPT -U lfuse:w:0xe1:m -U hfuse:w:0xd5:m -U efuse:w:0xfe:m
/* Fuse configuration. Note that self programming must be enabled */

/*
 * ATtiny85
 *
 * Fuse extended byte:
 * 0xFE = - - - -   - 1 1 0
 *                        ^
 *                        |
 *                        +---- SELFPRGEN (enable self programming flash)
 *
 * Fuse high byte:
 * 0xd5 = 1 1 0 1   0 1 0 1
 *        ^ ^ ^ ^   ^ \-+-/
 *        | | | |   |   +------ BODLEVEL 2..0 (brownout trigger level -> 2.7V)
 *        | | | |   +---------- EESAVE (preserve EEPROM on Chip Erase -> preserved)
 *        | | | +-------------- WDTON (watchdog timer always on -> disable)
 *        | | +---------------- SPIEN (enable serial programming -> enabled)
 *        | +------------------ DWEN (debug wire enable)
 *        +-------------------- RSTDISBL (disable external reset -> enabled)
 *
 * Fuse low byte:
 * 0xe1 = 1 1 1 0   0 0 0 1
 *        ^ ^ \+/   \--+--/
 *        | |  |       +------- CKSEL 3..0 (clock selection -> HF PLL)
 *        | |  +--------------- SUT 1..0 (BOD enabled, fast rising power)
 *        | +------------------ CKOUT (clock output on CKOUT pin -> disabled)
 *        +-------------------- CKDIV8 (divide clock by 8 -> don't divide)
 */

#endif /* __vmeconfig_h_included__ */
