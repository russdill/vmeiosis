# vmeiosis: V-USB Meiosis Bootloader

vmeiosis is a V-USB based bootloader for 8-bit AVR devices that allows for
shared use of V-USB code between the user program and bootloader. This allows
bootloader functionality to be added with minimal overhead.

 - V-USB: https://github.com/obdev/v-usb

Some of the features of vmeiosis:

* Interrupt free V-USB in bootloader, optional in user program.
* Chaining of V-USB interrupt to user program.
* Auto-calibration of oscillator performed at first programming.
* Reflashing of bootloader over USB (only for >= 4kB devices).
* Minimal changes to user program source necessary.
* Reading from EEPROM from within bootloader (optional).
* Ability to flash EEPROM when loading new user program.
* Resilience against powerloss during programming.
* Programming tool that understands avrdude commands.
* Operation for devices as small as 2KB (attiny25, etc).

For instance, the sizes of a sample HID supporting V-USB firmware:

* Original firmware: 2366 bytes
* vmeiosis: 1614 bytes
* Firmware built with vmeiosis: 1100 bytes
* Total bootloader overhead: 348 bytes

Special thanks to Tim Bo"scke <cpldcpu@gmail.com> for providing the initial
interrupt free V-USB implementation as well as the robust Micronucleus
bootloader.

# Usage

When the AVR resets, the bootloader initially runs and determines whether to
run the user program or continue into bootloader mode. It will run the user
program if:

* The reset reason is power on reset.
* The reset reason is brown out reset.

If will enter bootloader mode if:

* The last update of the user program was incomplete.
* The reset reason is watchdog.
* The reset reason is external reset.

Once in bootloader mode, vmeiosis will accepts USB control commands to erase
and program the flash. When complete, the bootloader can be instructed to
execute the user program.

To re-enter the bootloader mode, the user program can perform a watchdog reset
or it can be done manually via an external reset.

Because the vmeiosis manages the `MCUSR` register itself, vmeiosis passes the
reset reason to the user program via `r24`. If the value is `0x80` the user
program was executed by the bootloader after receiving an exit command via USB.

# Bootloader Configuration

Configuration of the bootloader and it's capabilities available to the user
program are performed via the `configs/` directory. A build configuration
is chosen with the `CONFIG` make variable.

## `configs/<CONFIG>/vmeconfig.h`

This contains configuration options for vmeiosis. See `vmeconfig-prototype.h`
for available options and documentation on each.

## `configs/<CONFIG>/usbconfig.h`

This contains things such as the vmeiosis USB descriptor definitions (vendor/
device ID, etc) as well as device specific V-USB configuration such as USB
D+/D- pairs and what additional V-USB features are available to the user
program. The sample configuration is contained in
`v-usb/usbdrv/usbconfig-prototype.h`.

Several options when enabled within the bootloader allow the user program to
utilize those options if so configured. When the headers for use by the user
program are generated a `USB_HAS_CFG` identifier is inserted. The user
program can then define the `USB_CFG` identifier to utilize the option.

| Config option                   | Recorded name                       |
|---------------------------------|------------------------------------ |
| `USB_CFG_HAVE_INTRIN_ENDPOINT`  | `USB_HAS_CFG_HAVE_INTRIN_ENDPOINT`  |
| `USB_CFG_HAVE_INTRIN_ENDPOINT3` | `USB_HAS_CFG_HAVE_INTRIN_ENDPOINT3` |
| `USB_CFG_IMPLEMENT_FN_WRITE`    | `USB_HAS_CFG_IMPLEMENT_FN_WRITE`    |
| `USB_CFG_IMPLEMENT_FN_READ`     | `USB_HAS_CFG_IMPLEMENT_FN_READ`     |
| `USB_CFG_IMPLEMENT_FN_WRITEOUT` | `USB_HAS_CFG_IMPLEMENT_FN_WRITEOUT` |

Additionally, in order to support the use of the `HOOK` macros additional
config options are added. If enabled the user program can define and use the
associated hook macro.

| Config option                  | Hook Macro
|--------------------------------|------------------------------------ 
| `USB_HAS_CFG_RX_USER_HOOK`     | `USB_RX_USER_HOOK`
| `USB_HAS_CFG_RESET_HOOK`       | `USB_RESET_HOOK`
| `USB_HAS_CFG_SET_ADDRESS_HOOK` | `USB_SET_ADDRESS_HOOK`

Other config options just need to be defined within the bootloader to enable
support within the user program:

* `USB_CFG_EP3_NUMBER`
* `USB_INITIAL_DATATOKEN`
* `USB_CFG_IMPLEMENT_HALT`
* `USB_CFG_SUPPRESS_INTR_CODE`
* `USB_CFG_IS_SELF_POWERED`
* `USB_CFG_HAVE_FLOWCONTROL`
* `USB_CFG_DRIVER_FLASH_PAGE`
* `USB_CFG_LONG_TRANSFERS`
* `USB_COUNT_SOF`
* `USB_CFG_CHECK_DATA_TOGGLING`
* `USB_USE_FAST_CRC`

And others can be configured by the user program:

* `USB_CFG_INTR_POLL_INTERVAL` (Returned in descriptors)
* `USB_CFG_MAX_BUS_POWER` (Returned in descriptors)

A few are not currently supported:

* `USB_SOF_HOOK`
* `USB_CFG_HAVE_MEASURE_FRAME_LENGTH`

Options that can be configured in the bootloader and the user program
differently are recorded in the `stub/usbdrv/generated/boot-usbconfig.h`
header. The mapping is as follows:

| Config option                              | Recorded name in `stub/usbdrv/usbdrv.h`         |
|--------------------------------------------|-------------------------------------------------|
| `USB_CFG_VENDOR_ID`                        | `USB_BOOT_CFG_VENDOR_ID`                        |
| `USB_CFG_VENDOR_NAME`                      | `USB_BOOT_CFG_VENDOR_NAME`                      |
| `USB_CFG_VENDOR_NAME_LEN`                  | `USB_BOOT_CFG_VENDOR_NAME_LEN`                  |
| `USB_CFG_VENDOR_ID`                        | `USB_BOOT_CFG_VENDOR_ID`                        |
| `USB_CFG_VENDOR_NAME`                      | `USB_BOOT_CFG_VENDOR_NAME`                      |
| `USB_CFG_VENDOR_NAME_LEN`                  | `USB_BOOT_CFG_VENDOR_NAME_LEN`                  |
| `USB_CFG_DEVICE_ID`                        | `USB_BOOT_CFG_DEVICE_ID`                        |
| `USB_CFG_DEVICE_NAME`                      | `USB_BOOT_CFG_DEVICE_NAME`                      |
| `USB_CFG_DEVICE_NAME_LEN`                  | `USB_BOOT_CFG_DEVICE_NAME_LEN`                  |
| `USB_CFG_DEVICE_VERSION`                   | `USB_BOOT_CFG_DEVICE_VERSION`                   |
| `USB_CFG_SERIAL_NUMBER`                    | `USB_BOOT_CFG_SERIAL_NUMBER`                    |
| `USB_CFG_SERIAL_NUMBER_LEN`                | `USB_BOOT_CFG_SERIAL_NUMBER_LEN`                |
| `USB_CFG_DEVICE_CLASS`                     | `USB_BOOT_CFG_DEVICE_CLASS`                     |
| `USB_CFG_DEVICE_SUBCLASS`                  | `USB_BOOT_CFG_DEVICE_SUBCLASS`                  |
| `USB_CFG_INTERFACE_CLASS`                  | `USB_BOOT_CFG_INTERFACE_CLASS`                  |
| `USB_CFG_INTERFACE_SUBCLASS`               | `USB_BOOT_CFG_INTERFACE_SUBCLASS`               |
| `USB_CFG_INTERFACE_PROTOCOL`               | `USB_BOOT_CFG_INTERFACE_PROTOCOL`               |
| `USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH`     | `USB_BOOT_CFG_HID_REPORT_DESCRIPTOR_LENGTH`     |
| `USB_CFG_HAVE_INTRIN_ENDPOINT`             | `USB_BOOT_CFG_HAVE_INTRIN_ENDPOINT`             |
| `USB_CFG_HAVE_INTRIN_ENDPOINT3`            | `USB_BOOT_CFG_HAVE_INTRIN_ENDPOINT3`            |
| `USB_CFG_SUPPRESS_INTRIN_ENDPOINTS`        | `USB_BOOT_CFG_SUPPRESS_INTRIN_ENDPOINTS`        |
| `USB_CFG_EP3_NUMBER`                       | `USB_BOOT_CFG_EP3_NUMBER`                       |
| `USB_CFG_DESCR_PROPS_DEVICE`               | `USB_BOOT_CFG_DESCR_PROPS_DEVICE`               |
| `USB_CFG_DESCR_PROPS_CONFIGURATION`        | `USB_BOOT_CFG_DESCR_PROPS_CONFIGURATION`        |
| `USB_CFG_DESCR_PROPS_STRINGS`              | `USB_BOOT_CFG_DESCR_PROPS_STRINGS`              |
| `USB_CFG_DESCR_PROPS_STRING_0`             | `USB_BOOT_CFG_DESCR_PROPS_STRING_0`             |
| `USB_CFG_DESCR_PROPS_STRING_VENDOR`        | `USB_BOOT_CFG_DESCR_PROPS_STRING_VENDOR`        |
| `USB_CFG_DESCR_PROPS_STRING_PRODUCT`       | `USB_BOOT_CFG_DESCR_PROPS_STRING_PRODUCT`       |
| `USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER` | `USB_BOOT_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER` |
| `USB_CFG_DESCR_PROPS_HID`                  | `USB_BOOT_CFG_DESCR_PROPS_HID`                  |
| `USB_CFG_DESCR_PROPS_HID_REPORT`           | `USB_BOOT_CFG_DESCR_PROPS_HID_REPORT`           |
| `USB_CFG_DESCR_PROPS_UNKNOWN`              | `USB_BOOT_CFG_DESCR_PROPS_UNKNOWN`              |
| `USB_CFG_INTR_POLL_INTERVAL`               | `USB_BOOT_CFG_INTR_POLL_INTERVAL`               |
| `USB_CFG_MAX_BUS_POWER`                    | `USB_BOOT_CFG_MAX_BUS_POWER`                    |

# User Program Configuration

Include the generated `usbdrv.h` as well as the user program's own
`usbconfig.h`. The user program should define the "Device Description" and
"Functional Range" sections. Note that a compile error will occur if a
functional range option is selected that is not supported by the bootloader
configuration.

# Building the Bootloader

Building is done via `make CONFIG=<CONFIG>`, eg `make CONFIG=digispark`. The
following outputs are produced:

## `main.hex`

This is the bootloader image. Note that this image does not contain an
oscillator calibration value and so will not enumerate onto the USB bus. The
calibration byte must be either programmed into `main.hex` before flashing
or `main.hex` must be combined with a user program that does so before
flashing.

## `main_auto_osccal.hex` 

Most complete hexfile for flashing via avrdude. This hexfile contains the
bootloader image (`main.hex`) as well as a user program for calibrating from
USB and baking the calibration value into the bootloader (
`osccal_usb_to_baked.hex`). Note that the use program will retry until it is
connected to a USB port and then once complete not run again.

The `flash_auto_osccal` performs this step. This image requires an 4k device.

## `main_eeprom_osccal.hex`

Similar to the above, but bakes in the calibration value stored at EEPROM
byte zero. This hexfile contains the bootloader image (`main.hex`) as well
as a user program for baking EEPROM byte zero into the bootloader (
`osccal_eeprom_to_baked.hex` This can be more convenient than baking the
calibration value into the bootloader and then programming the bootloader.
The EEPROM byte can be programmed manually (via `avrdude`) or via the
`osccal_usb_to_eeprom.hex` described below.

The `flash_eeprom_osscal` make rule performs these steps. This image
requires a 2k device.

## `osccal_usb_to_eeprom.hex`

This image determines an oscillator calibration value from the USB bus and
then programs it into the EEPROM. The `main_eeprom_osccal.hex` image can then
be flashed to the device. For smaller devices the value can be read back
from the device EEPROM via `avrdude`, baked into a firmware image, and then
the modified firmware image can be flashed.

The `flash_baked_osccal` make rule performs these steps.

## `reflash.hex`

This image allows the bootloader to be reflashed onto a system via USB. The
image contains a small reflashing program as well as the bootloader binary.
Because this fully duplicates the bootloader image, it requires a 4k device.

## `stub/`

The stub path contains a skeleton version of V-USB for "linking" user programs
to the bootloader. The first set wrap vmeiosis implementations of V-USB
functions for use by the user program. These have identical names to files
utilized when building programs for V-USB in order to minimize changes needed
when building for vmeiosis:

* `stub/usbdrv/usbdrv.h`
* `stub/usbdrv/usbdrv.c`
* `stub/usbdrv/usbdrvasm.S`

These simply include the associated file from the `archived` folder.

* `stub/usbdrv/usbportability.h`
* `stub/usbdrv/oddebug.h`

Some headers are copied directly from V-USB:

* `stub/usbdrv/archived/boot-usbdrv.h`
* `stub/usbdrv/archived/boot-usbportability.h`
* `stub/usbdrv/archived/boot-oddebug.h`

vmeiosis has its own way of compactly storing and returning USB descriptors,
the handlers for that in the `usbdesc.c`/`usbdesc.h` files and automatically
get incorporated into the user program when built.

* `stub/usbdrv/archived/usbdesc.h`
* `stub/usbdrv/archived/usbdesc.c`

These files are generated based on the current build configuration. While the
function calls defined within `boot-syms.c` link to a vector table and are
stable, the RAM locations may change based on changes within the V-USB source.
Care should be taken when providing a bootloader update that the RAM locations
remain stable.

* `stub/usbdrv/generated/boot-syms.c`
* `stub/usbdrv/generated/boot-usbconfig.h`

# `vmedude.py` tool

There is not yet vmeiosis integration into `avrdude`. The `vmedude.py` tool
stands in for this and accepts most of the same command line options as
`avrdude`. In addition it also accepts an `--enter` command for sending a
simple vendor command to enter the bootloader (for user programs that support
this) and a `--run` command to run the user program from the bootloader.

Before programming, `vmedude.py` will verify that the device signature bytes
of the bootloader match those of the firmware (if present). Additionally,
if the user signature row is present in the user program, `vmedude.py` will
verify that the configuration bytes match those of the bootloader.

The user signature row is automatically embedded as part of the build process.
A EEPROM flasher is also embedded within the user signature row allowing
`vmedude.py` to program new EEPROM values if an EEPROM section is included in
the user image. `vmedude.py` will perform two programming steps in this case.
The first to program the embedded EEPROM user program along with the new
EEPROM values, and the second to program the actual user program.
should just include the EEPROM values, and the second the actual user
program.

# User Program Build

The build of the user program follows the same process as building with V-USB
directly. The contents of the `stub/usbdrv` directory should be copied just
as `v-usb/usbdrv` directory would be. And similarly the `usbdrv.h` header
should be included and the `usbdrv.c` and `usbdrvasm.S` files should be built
and linked. The `usbdrv.c` file can be included directly rather than linked
and built separately when defining `USB_PUBLIC` as `static`.

Note that in order to reduce code duplication, rather than export `usbInit()`
vmeiosis exports a `systemInit()` function which itself calls `usbInit()`
followed by disconnecting from the USB bus, waiting 300ms, and then
reconnecting. Any user programs must be modified to call `systemInit()` rather
than `usbInit()`.

# Internals

vmeiosis sits at the end of flash allowing the user program to sit at the head
of flash and maintain it's own vector table. The first 4 bytes of the
bootloader are reserved for loading the `OSCCAL` register. This will generally
be an `LDI` instruction followed by an `OUT` instruction. The `LDI`
instruction is modified as part of the initial programming process or the
update process.

The user program will have a vector table inserted immediately after the
standard AVR vector table. This vector table allows the bootloader to call
callbacks within the user application such as `usbFunctionSetup`,
`usbFunctionWrite`, etc. The size of the vector table varies with the
bootloader configuration, which is captured within the configuration words.

The bootloader contains a vector table at the end of flash. This vector table
includes linkage to `systemInit`, `usbPoll`, and the bootloader interrupt
vector. It also contains a link to the V-USB `usbGenericSetInterrupt` if
interrupt endpoints are supported within the bootloader configuration. The
last two bytes of the bootloader vector table are configuration words which
store which configuration options the bootloader was configured with, how
large the bootloader is, and what interrupt vector it uses. These values are
used by the programming tool to verify the build of the bootloader matches
the build of the user program and also to properly patch the user program's
AVR vector table.

## USB Descriptor System

Because both the user program and the bootloader must return USB descriptors
and these descriptors are likely to be different, vmeiosis contains an
alternate mechanism for returning USB descriptors. Rather than the if/else
mechanism used by V-USB, vmeiosis contains code that walks through static
descriptors to find and return the matching descriptor. This code is 80 bytes
but shared between the user program and bootloader. This also allows the user
program to re-use any duplicate descriptors from the bootloader.

## Protocol

All bootloader commands are USB vendor messages to the control endpoint:

|`bmRequestType` | `bRequest` | `wValue` | `wIndex` | Description
|----------------|------------|----------|----------|------------
| `0xc0`         | `0x00`     | N/A      | Address  | Read SRAM
| `0xc0`         | `0x01`     | N/A      | Address  | Read flash
| `0xc0`         | `0x09`     | N/A      | Address  | Read fuse row
| `0xc0`         | `0x21`     | N/A      | Address  | Read signature row
| `0xc0`         | `0x40`     | N/A      | Address  | Read EEPROM
| `0x40`         | `0x01`     | Data     | Address  | Write data to temporary buffer
| `0x40`         | `0x03`     | N/A      | Address  | Erase flash page
| `0x40`         | `0x05`     | N/A      | Address  | Write temporary buffer to flash
| `0x40`         | `0x80`     | N/A      | N/A      | Exit to user program

Note that when writing, any `bRequest` values other than `0x80` are used to
build an `spm` command as follows:

* `bRequest`: Written to `SPMCR`/`SPMCSR` register.
* `wWalue`: `r0/r1`
* `wIndex`: `r30/r31`

To avoid accidental bricking, any write requests perform a CRC16 check. If the
check fails the bootloader will reset.

When reading the `bRequest` field is used as the `usbMsgFlags` value. If bit 0
is set it is used as a `SPMCR`/`SPMCSR` when performing an `lpm` command.

The protocol is extremely limited in order to simplify the device side
implementation as much as possible. The general steps to probe a device are:

* Read the signature row.
* Determine the flash size based on the device type.
* Read the last two words of flash memory.

The last configuration words at the end of flash memory contain the size of
the bootloader in pages as well as the vector it utilizes for interrupts (if
applicable).

When flashing, the vector table at the start of flash is patched so that the
reset vector points to the bootloader start page and the relevant interrupt
vector points to an entry in the bootloader vector table. A jump to the real
user reset vector and real user interrupt vector are stored at the end of
user flash. If these are erased, the bootloader will immediately re-execute
if it attempts to jump to user flash.

Reflashing the user application thus starts with erasing the last page,
erasing the remaining pages, programming the user program, and then
programming the two vectors directly before the start of the bootloader. The
bootloader can then be instructed to jump to exit.
