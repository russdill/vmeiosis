# Config
CONFIG ?= digispark

VME_MAJOR = 2
VME_MINOR = 0

CFLAGS =
CONFIGPATH = configs/$(CONFIG)

CROSS_COMPILE=avr-
CC = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)cpp
NM = $(CROSS_COMPILE)nm
OBJCOPY = $(CROSS_COMPILE)objcopy
AVRDUDE = avrdude $(AVRDUDE_OPTS) -p $(DEVICE)
VMEDUDE = ./scripts/vmedude.py

cpp_var = $(shell echo $1 | $(CPP) -Iv-usb/usbdrv -I$(CONFIGPATH) $2 -include usbdrv.h -P 2>/dev/null | tail -n 1)
DEVICE := $(call cpp_var,DEVICE)
F_CPU := $(call cpp_var,F_CPU)
USB_PUBLIC := $(call cpp_var,USB_PUBLIC)
FUSEOPTS := $(call cpp_var,FUSEOPTS,-mmcu=$(DEVICE))
FLASHEND := $(call cpp_var,FLASHEND,-mmcu=$(DEVICE))

# We choose targets based on available flash size. This may need massaging
# if you optimize or bloat v-usb
FLASH_HAS_1K := $(shell bash -c "[[ $(FLASHEND) -ge 0x3ff ]]" && echo true)
FLASH_HAS_2K := $(shell bash -c "[[ $(FLASHEND) -ge 0x7ff ]]" && echo true)
FLASH_HAS_4K := $(shell bash -c "[[ $(FLASHEND) -ge 0xfff ]]" && echo true)

ifeq ($(FLASH_HAS_1K),)
$(error Device too small to support vmeiosis)
endif

CFLAGS += -ggdb -Os -Wall -Werror
CFLAGS += -I$(CONFIGPATH) -Iv-usb/usbdrv
CFLAGS += -mmcu=$(DEVICE) -DF_CPU=$(F_CPU)
CFLAGS += -DVME_MINOR=$(VME_MINOR) -DVME_MAJOR=$(VME_MAJOR)
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -fno-inline-small-functions -fno-move-loop-invariants -fno-tree-scev-cprop
CFLAGS += -fpack-struct

LDFLAGS += -Wl,--relax

vpath usbdrvasm.S v-usb/usbdrv

# Common rules
DEPDIR := .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td
.SUFFIXES:
CLEAN += $(DEPDIR)/*.d $(DEPDIR)/*.Td

all: targets

CURR_CONFIG := $(shell cat config.stamp 2>/dev/null)

config.stamp:
	@echo -n $(CONFIG) > $@
ifneq ($(CURR_CONFIG),$(CONFIG))
PHONY += config.stamp
endif
CLEAN += config.stamp

ALL_DEP = Makefile config.stamp

.SECONDEXPANSION:
.SECONDARY:

CFLAGS += $(DEPFLAGS) $($@_CFLAGS)
%.o: %.c
%.o: %.c $(DEPDIR)/%.d $(ALL_DEP)
	$(CC) $(CFLAGS) -c $< -o $@
	@mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

%.o: %.S
%.o: %.S $(DEPDIR)/%.d $(ALL_DEP)
	$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@
	@mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

LDFLAGS += $($@_LDFLAGS)
%.elf: $(ALL_DEP) $$($$*_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(LDFLAGS)

OBJCOPY_FLAGS += $($@_OBJCOPY_FLAGS)
%.hex: %.elf
%.hex: %.elf $(ALL_DEP)
	$(OBJCOPY) $(OBJCOPY_FLAGS) -j .text -j .data -O ihex $< $@
	@echo Size of sections:
	@avr-size $<
	@echo Size of binary hexfile
	@avr-size $@

%.bin: %.elf
%.bin: %.elf $(ALL_DEP)
	$(OBJCOPY) $(OBJCOPY_FLAGS) -j .text -j .data -O binary $< $@

%.bin.o: %.bin
	$(OBJCOPY) $(OBJCOPY_FLAGS) --rename-section .data=.text -I binary -O elf32-avr $< $@ 

# Build output

# main is our primary bootloader object file
main_OBJECTS = vectors.o main.o usbdrvasm.o
main_reloc_OBJECTS = $(main_OBJECTS) # Pass one output to determine size
main_reloc.elf_LDFLAGS += -Wl,--defsym=__bl_num_pages=0,--defsym=FLASHEND=$(FLASHEND)

OSCCAL_DEFAULT=0

main.elf: LDFLAGS += -Wl,--section-start=.vme_vectors=0
main.elf: LDFLAGS += -nostartfiles -nostdlib
main.elf: LDFLAGS += -Wl,--defsym=osccal_default=$(OSCCAL_DEFAULT)
main.elf_LDFLAGS += -Wl,-Ttext=$$($(NM) main_reloc.elf | ./scripts/calc.sh start)
main.elf_LDFLAGS += -Wl,--section-start=.end_vectors=$$($(NM) main_reloc.elf | ./scripts/calc.sh end)
main.elf_LDFLAGS += -Wl,--defsym=__bl_num_pages=0x$$($(NM) main_reloc.elf | ./scripts/calc.sh __bl_num_pages)
main.elf: main_reloc.elf scripts/calc.sh $(ALL_DEP)
CLEAN += main.elf $(main_OBJECTS) main_reloc.elf

# Does *not* contain osccal (used for reflash)
main.hex_OBJCOPY_FLAGS += -j .end_vectors
main.bin_OBJCOPY_FLAGS += -j .end_vectors
CLEAN += main.bin main.hex

main_%.hex: main.hex $(ALL_DEP)
	srec_cat $(patsubst %,% -Intel,$(filter %.hex,$^)) -Output $@ -Intel
	@echo Size of binary hexfile
	@avr-size $@

foo:
	$(OBJCOPY) $(OBJCOPY_FLAGS) -j .end_vectors -j .text -j .data -O ihex $< $@
	@echo Size of sections:
	@avr-size $<
	@echo Size of binary hexfile
	@avr-size $@

# osccal object, embedded within initial image for finding OSCCAL value
osccal_usb_to_baked_OBJECTS = osccal_avr054.o osccal_measure_usb.o osccal_program_baked.o
osccal_usb_to_baked.elf_LDFLAGS += -nostartfiles -nostdlib
CLEAN += osccal_usb_to_baked.elf osccal_usb_to_baked.hex $(osccal_usb_to_baked_OBJECTS)
TARGETS_4K += osccal_usb_to_baked.hex

# Combined hex file that includes osccal from USB binary to baked
main_auto_osccal.hex: osccal_usb_to_baked.hex
CLEAN += main_auto_osccal.hex
TARGETS_4K += main_auto_osccal.hex

# osccal object, for finding an osccal value from USB and storing it in eeprom
osccal_program_eeprom.o_CFLAGS += -DEEPROM_OSCCAL=0
osccal_usb_to_eeprom_OBJECTS = osccal_avr054.o osccal_measure_usb.o osccal_program_eeprom.o
osccal_usb_to_eeprom.elf: LDFLAGS += -nostartfiles
CLEAN += osccal_usb_to_eeprom.elf osccal_usb_to_eeprom.hex $(osccal_usb_to_eeprom_OBJECTS)
TARGETS += osccal_usb_to_eeprom.hex

# Take the eeprom byte from EEPROM
osccal_from_eeprom.o_CFLAGS += -DEEPROM_OSCCAL=0
osccal_eeprom_to_baked_OBJECTS = osccal_from_eeprom.o osccal_program_baked.o
osccal_eeprom_to_baked.elf: LDFLAGS += -nostartfiles
CLEAN += osccal_eeprom_to_baked.elf osccal_eeprom_to_baked.hex $(osccal_eeprom_to_baked_OBJECTS)
TARGETS += osccal_eeprom_to_baked.hex

# Combined hex file that pulls the osccal value from the eeprom and bakes it
main_eeprom_osccal.hex: osccal_eeprom_to_baked.hex
CLEAN += main_eeprom_osccal.hex
TARGETS += main_eeprom_osccal.hex

# Reflash object, used for updating vmeiosis over USB
reflash_OBJECTS = reflash.o main.bin.o
reflash.elf: LDFLAGS += -nostartfiles -nostdlib
CLEAN += reflash.elf reflash.hex $(reflash_OBJECTS)
TARGETS_4K += reflash.hex

stub/usbdrv/generated:
	mkdir -p $@

stub/usbdrv/archived:
	mkdir -p $@

# Symbol table for linkage from user-program to bootloader
stub/usbdrv/generated/boot-syms.c: main.elf scripts/build_syms.py | stub/usbdrv/generated
	$(NM) -S $< | ./scripts/build_syms.py > .tmp_syms
	cp .tmp_syms $@
CLEAN += stub/usbdrv/generated/boot-syms.c .tmp_syms
TARGETS += stub/usbdrv/generated/boot-syms.c

# The configuration options the vmeiosis was built with
stub-usbconfig.h: configuration.h $(ALL_DEP)
	$(CPP) -nostdinc -P -fdirectives-only -include configuration.h $(CFLAGS) /dev/null -o $@
CLEAN += stub-usbconfig.h

# To filter out avr-gcc pre-baked macros
stub-common.h: $(ALL_DEP)
	 $(CPP) /dev/null -dM $(CFLAGS) -o $@
CLEAN += stub-common.h

# Generate bootloader config for user program, names are transformed
stub/usbdrv/generated/boot-usbconfig.h: $(ALL_DEP) stub-common.h stub-usbconfig.h scripts/stub-xform.sed | stub/usbdrv/generated
	(echo '#ifndef __BOOT_USBCONFIG_H__'; \
		echo '#define __BOOT_USBCONFIG_H__'; \
		grep -v -x -f stub-common.h stub-usbconfig.h; \
		echo "#define F_CPU $(F_CPU)"; \
		echo '#endif') | \
		sed -r -f scripts/stub-xform.sed > $@
CLEAN += stub/usbdrv/generated/boot-usbconfig.h
TARGETS += stub/usbdrv/generated/boot-usbconfig.h

# Copies of files from V-USB and locally for user program
VUSB_ARCHIVE_FILES = usbdrv.h usbportability.h oddebug.h
stub/usbdrv/archived/boot-%: v-usb/usbdrv/% | stub/usbdrv/archived
	cp $< $@
CLEAN += $(patsubst %,stub/usbdrv/archived/boot-%,$(VUSB_ARCHIVE_FILES))
TARGETS += $(patsubst %,stub/usbdrv/archived/boot-%,$(VUSB_ARCHIVE_FILES))

ARCHIVE_FILES = usbdesc.c usbdesc.h
stub/usbdrv/archived/%: % | stub/usbdrv/archived
	cp $< $@
CLEAN += $(patsubst %,stub/usbdrv/archived/%,$(ARCHIVE_FILES))
TARGETS += $(patsubst %,stub/usbdrv/archived/%,$(ARCHIVE_FILES))

# Useful commands

bl_enter:
	$(VMEDUDE) -E

ifeq ($(FLASH_HAS_4K),true)
flash: flash_auto_osccal
else ifeq ($(FLASH_HAS_2K),true)
flash: flash_eeprom_osscal
else
flash: flash_baked_osscal
endif

# Flash combined firmware and osccal calibration from USB to baked
# This requires the largest flash size but is fastest
flash_auto_osccal: main_auto_osccal.hex
	$(AVRDUDE) -U flash:w:$^:i

# Generates a calibration value then loads a firmware that bakes itself
# from the EEPROM value.
# This requires that the calibration routines and bootloader fit within flash,
# this is modestly smaller than the full auto image.
flash_eeprom_osscal: main_eeprom_osccal.hex osccal_usb_to_eeprom.hex
	@echo Clearing EEPROM calibration entry...
	$(AVRDUDE) -U eeprom:w:0xff:m
	@echo Flashing USB calibration firmware...
	$(AVRDUDE) -U flash:w:osccal_usb_to_eeprom.hex:i
	@echo Allowing calibration to run...
	@sleep 5
	@echo Reading back EEPROM...
	$(AVRDUDE) -U eeprom:r:-:h | cut -f 1 -d ',' > .osccal_byte.txt
	@[ "$$(cat .osccal_byte.txt)" != 0xff ] || \
		(echo OSCCAL calibration failed, is the device USB cable plugged in?; exit 1)
	@echo Programming firmware...
	$(AVRDUDE) -U flash:w:main_eeprom_osccal.hex:i

# Generates a calibration value, reads it back, bakes it into a firmware and
# then programs the pre-baked firmware.
# This just requires that the bootloader fit within flash.
# This currently isn't much use as it would require cutting V-USB down to
# below 1KB to support 1KB devices.
flash_baked_osccal: main.hex osccal_usb_to_eeprom.hex
	@echo Clearing EEPROM calibration entry...
	$(AVRDUDE) -U eeprom:w:0xff:m
	@echo Flashing USB calibration firmware...
	$(AVRDUDE) -U flash:w:osccal_usb_to_eeprom.hex:i
	@echo Allowing calibration to run...
	@sleep 5
	@echo Reading back EEPROM...
	$(AVRDUDE) -U eeprom:r:-:h | cut -f 1 -d ',' > .osccal_byte.txt
	@[ "$$(cat .osccal_byte.txt)" != 0xff ] || \
		(echo OSCCAL calibration failed, is the device USB cable plugged in?; exit 1)
	echo ldi r16, $$(cat .osccal_byte.txt) > .tmp_pgm.S
	$(CC) $(CFLAGS) .tmp_pgm.S -c -o .tmp_pgm.o
	$(CC) -Wl,--oformat=ihex .tmp_pgm.o -o .tmp_pgm.hex -Wl,-Ttext=$$($(NM) main_reloc.elf | ./scripts/calc.sh start)
	srec_cat main.hex -Intel .tmp_pgm.hex -Intel -contradictory-bytes=ignore -Output .tmp_firmware.hex -Intel
	@echo Programming firmware with baked in calibration value of $$(cat .osccal_byte.txt)
	$(AVRDUDE) -U flash:w:.tmp_firmware.hex:i
CLEAN += .osccal_byte.txt .tmp_pgm.S .tmp_pgm.o .tmp_pgm.hex .tmp_firmware.hex

# Reflash on device running vmeiosis
reflash: reflash.hex
	$(VMEDUDE) -U $< --run

# Read back flash contents for debug
# avr-objdump -j .sec1 -D -m avr25 read.hex
read.hex:
	$(AVRDUDE) -U flash:r:$@:i
PHONY += read.hex
CLEAN += read.hex

# Read back flash contents for debug
## avr-objdump -b binary -D -m avr25 read.bin
bl_read.elf:
	$(VMEDUDE) -U flash:r:$@:e
PHONY += bl_read.elf
CLEAN += bl_read.elf

# Program fuses
fuse:
	$(AVRDUDE) $(FUSEOPT)

bl_fuse:
	$(VMEDUDE) $(FUSEOPT)

# Check fuses
fuse_read:
	$(AVRDUDE) -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h

bl_fuse_read:
	$(VMEDUDE) -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h

# Common end file targets
ifeq ($(FLASH_HAS_4K),true)
TARGETS += $(TARGETS_4K)
endif
targets: $(TARGETS)

clean: FORCE
	@rm -f $(CLEAN)
	@rmdir --ignore-fail-on-non-empty $(DEPDIR)

.PHONY: $(PHONY)
FORCE:

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

include $(wildcard $(DEPDIR)/*.d)
