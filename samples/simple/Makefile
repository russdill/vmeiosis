MEIOSIS_PATH = ../..
VUSB_INC = $(MEIOSIS_PATH)/stub/usbdrv
VPATH = $(VUSB_INC)

PROJECT = simple

CFLAGS =

CROSS_COMPILE=avr-
CC = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)cpp
NM = $(CROSS_COMPILE)nm
OBJCOPY = $(CROSS_COMPILE)objcopy
VMEDUDE = $(MEIOSIS_PATH)/scripts/vmedude.py

cpp_var = $(shell echo $1 | $(CPP) -I$(VUSB_INC) -I. $2 -include usbdrv.h -P 2>/dev/null | tail -n 1)
DEVICE := $(call cpp_var,DEVICE)
F_CPU := $(call cpp_var,F_CPU)
USB_PUBLIC := $(call cpp_var,USB_PUBLIC)

# Options:
CFLAGS +=  -g2 -Os -Wall -Werror
CFLAGS += -mmcu=$(DEVICE) -DF_CPU=$(F_CPU)
CFLAGS += -I$(VUSB_INC) -I.
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -fno-inline-small-functions -fno-move-loop-invariants -fno-tree-scev-cprop
CFLAGS += -fpack-struct

LDFLAGS += -Wl,--relax

# Common rules
DEPDIR := .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td
.SUFFIXES:
CLEAN += $(DEPDIR)/*.d $(DEPDIR)/*.Td

all: targets

ALL_DEP = Makefile

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

# Build output
$(PROJECT)_OBJECTS = main.o usbdrvasm.o
ifeq ($(USB_PUBLIC),static)
main.o_CFLAGS += -include usbdrv.c
else
$(PROJECT)_OBJECTS += usbdrv.o
endif
TARGETS += $(PROJECT).hex
CLEAN += $(PROJECT).hex $(PROJECT).elf $($(PROJECT)_OBJECTS)

flash: $(PROJECT).hex
	$(VMEDUDE) --enter --product Simple || true
	$(VMEDUDE) -U $< --run

# Common end file targets
targets: $(TARGETS)

clean: FORCE
	@rm -f $(CLEAN)
	@rmdir --ignore-fail-on-non-empty $(DEPDIR)

.PHONY: $(PHONY)
FORCE:

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

include $(wildcard $(DEPDIR)/*.d)

