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

#ifndef OSC_VER_H
#define OSC_VER_H

#include <avr/io.h>

#if defined(OSCCAL)
#define OSCCAL_REG OSCCAL
#elif defined(OSCCAL0)
#define OSCCAL_REG OSCCAL0
#else
#error "Unknown/unsupported oscillator calibration register"
#endif

#if defined(__AVR_ATtiny11__) \
|| defined(__AVR_ATtiny12__) \
|| defined(__AVR_ATtiny28__) \
|| defined(__AVR_ATtiny15__)

#define OSC_VER 1

#elif defined(__AVR_ATmega163__) \
|| defined(__AVR_ATmega323__)

#define OSC_VER 2

#elif defined(__AVR_ATmega128__) \
|| defined(__AVR_ATmega128A__) \
|| defined(__AVR_ATmega128RFA1__) \
|| defined(__AVR_ATmega128RFR2__) \
|| defined(__AVR_ATmega16__) \
|| defined(__AVR_ATmega16A__) \
|| defined(__AVR_ATmega16HVA__) \
|| defined(__AVR_ATmega16HVA2__) \
|| defined(__AVR_ATmega16HVB__) \
|| defined(__AVR_ATmega16HVBREVB__) \
|| defined(__AVR_ATmega16M1__) \
|| defined(__AVR_ATmega16U2__) \
|| defined(__AVR_ATmega16U4__) \
|| defined(__AVR_ATmega32__) \
|| defined(__AVR_ATmega32A__) \
|| defined(__AVR_ATmega32C1__) \
|| defined(__AVR_ATmega32HVB__) \
|| defined(__AVR_ATmega32HVBREVB__) \
|| defined(__AVR_ATmega32M1__) \
|| defined(__AVR_ATmega32U2__) \
|| defined(__AVR_ATmega32U4__) \
|| defined(__AVR_ATmega32U6__) \
|| defined(__AVR_ATmega64__) \
|| defined(__AVR_ATmega64A__) \
|| defined(__AVR_ATmega64HVE__) \
|| defined(__AVR_ATmega64HVE2__) \
|| defined(__AVR_ATmega64C1__) \
|| defined(__AVR_ATmega64M1__) \
|| defined(__AVR_ATmega64RFR2__) \
|| defined(__AVR_ATmega8__) \
|| defined(__AVR_ATmega8A__) \
|| defined(__AVR_ATmega8HVA__) \
|| defined(__AVR_ATmega8U2__) \
|| defined(__AVR_ATmega8515__) \
|| defined(__AVR_ATmega8535__) \
|| defined(__AVR_ATtiny26__)

#define OSC_VER 3

#elif defined(__AVR_AT90CAN128__) \
|| defined(__AVR_AT90CAN32__) \
|| defined(__AVR_AT90CAN64__) \
|| defined(__AVR_ATmega162__) \
|| defined(__AVR_ATmega165__) \
|| defined(__AVR_ATmega165A__) \
|| defined(__AVR_ATmega165P__) \
|| defined(__AVR_ATmega165PA__) \
|| defined(__AVR_ATmega169__) \
|| defined(__AVR_ATmega169A__) \
|| defined(__AVR_ATtiny13__) \
|| defined(__AVR_ATtiny13A__) \
|| defined(__AVR_ATtiny2313__) \
|| defined(__AVR_ATtiny2313A__) \
|| defined(__AVR_ATtiny4313__)

#define OSC_VER 4

#elif defined(__AVR_AT90PWM2__) \
|| defined(__AVR_AT90PWM2B__) \
|| defined(__AVR_AT90PWM3__) \
|| defined(__AVR_AT90PWM3B__) \
|| defined(__AVR_AT90PWM216__) \
|| defined(__AVR_AT90PWM316__) \
|| defined(__AVR_ATmega1280__) \
|| defined(__AVR_ATmega1281__) \
|| defined(__AVR_ATmega164__) \
|| defined(__AVR_ATmega164A__) \
|| defined(__AVR_ATmega164P__) \
|| defined(__AVR_ATmega164PA__) \
|| defined(__AVR_ATmega168__) \
|| defined(__AVR_ATmega168A__) \
|| defined(__AVR_ATmega168P__) \
|| defined(__AVR_ATmega168PA__) \
|| defined(__AVR_ATmega168PB__) \
|| defined(__AVR_ATmega169P__) \
|| defined(__AVR_ATmega169P__) \
|| defined(__AVR_ATmega169PA__) \
|| defined(__AVR_ATmega256RFR2__) \
|| defined(__AVR_ATmega2560__) \
|| defined(__AVR_ATmega2561__) \
|| defined(__AVR_ATmega2564RFR2__) \
|| defined(__AVR_ATmega324__) \
|| defined(__AVR_ATmega324A__) \
|| defined(__AVR_ATmega324P__) \
|| defined(__AVR_ATmega324PA__) \
|| defined(__AVR_ATmega325__) \
|| defined(__AVR_ATmega325A__) \
|| defined(__AVR_ATmega325P__) \
|| defined(__AVR_ATmega325PA) \
|| defined(__AVR_ATmega328__) \
|| defined(__AVR_ATmega328P__) \
|| defined(__AVR_ATmega3250__) \
|| defined(__AVR_ATmega3250A__) \
|| defined(__AVR_ATmega3250P__) \
|| defined(__AVR_ATmega3250PA__) \
|| defined(__AVR_ATmega329__) \
|| defined(__AVR_ATmega329A__) \
|| defined(__AVR_ATmega329P__) \
|| defined(__AVR_ATmega329PA__) \
|| defined(__AVR_ATmega3290__) \
|| defined(__AVR_ATmega3290A__) \
|| defined(__AVR_ATmega3290PA__) \
|| defined(__AVR_ATmega3290P__) \
|| defined(__AVR_ATmega406__) \
|| defined(__AVR_ATmega48__) \
|| defined(__AVR_ATmega48A__) \
|| defined(__AVR_ATmega48P__) \
|| defined(__AVR_ATmega48PA__) \
|| defined(__AVR_ATmega48PB__) \
|| defined(__AVR_ATmega640__) \
|| defined(__AVR_ATmega644__) \
|| defined(__AVR_ATmega644A__) \
|| defined(__AVR_ATmega644P__) \
|| defined(__AVR_ATmega644PA__) \
|| defined(__AVR_ATmega644RFR2__) \
|| defined(__AVR_ATmega1284__) \
|| defined(__AVR_ATmega1284P__) \
|| defined(__AVR_ATmega1284RFR2__) \
|| defined(__AVR_ATmega645__) \
|| defined(__AVR_ATmega645A__) \
|| defined(__AVR_ATmega645P__) \
|| defined(__AVR_ATmega6450__) \
|| defined(__AVR_ATmega6450A__) \
|| defined(__AVR_ATmega6450P__) \
|| defined(__AVR_ATmega649__) \
|| defined(__AVR_ATmega649A__) \
|| defined(__AVR_ATmega649P__) \
|| defined(__AVR_ATmega6490__) \
|| defined(__AVR_ATmega6490A__) \
|| defined(__AVR_ATmega6490P__) \
|| defined(__AVR_ATmega88__) \
|| defined(__AVR_ATmega88A__) \
|| defined(__AVR_ATmega88P__) \
|| defined(__AVR_ATmega88PA__) \
|| defined(__AVR_ATmega88PB__) \
|| defined(__AVR_ATtiny24__) \
|| defined(__AVR_ATtiny24A__) \
|| defined(__AVR_ATtiny25__) \
|| defined(__AVR_ATtiny261__) \
|| defined(__AVR_ATtiny261A__) \
|| defined(__AVR_ATtiny43U__) \
|| defined(__AVR_ATtiny44__) \
|| defined(__AVR_ATtiny44A__) \
|| defined(__AVR_ATtiny45__) \
|| defined(__AVR_ATtiny461__) \
|| defined(__AVR_ATtiny461A__) \
|| defined(__AVR_ATtiny84__) \
|| defined(__AVR_ATtiny84A__) \
|| defined(__AVR_ATtiny85__) \
|| defined(__AVR_ATtiny85__) \
|| defined(__AVR_ATtiny87__) \
|| defined(__AVR_ATtiny167__) \
|| defined(__AVR_ATtiny861__) \
|| defined(__AVR_ATtiny861A__)

#define OSC_VER 5

#elif defined(__AVR_ATtiny4__) \
|| defined(__AVR_ATtiny5__) \
|| defined(__AVR_ATtiny9__) \
|| defined(__AVR_ATtiny10__) \
|| defined(__AVR_ATtiny20__) \
|| defined(__AVR_ATtiny40__) \
|| defined(__AVR_ATtiny48__) \
|| defined(__AVR_ATtiny88__)

/* Does not fit within AVR053 version scheme */
#define OSC_VER 1

#elif defined(__AVR_ATtiny441__) \
|| defined(__AVR_ATtiny841__) \
|| defined(__AVR_ATtiny828__) \
|| defined(__AVR_ATtiny1634__)

/* Has additional temperature calibration registers */
#define OSC_VER 1

#else

#error "osc_ver.h missing OSC_VER table entry for this platform"

#endif

#endif
