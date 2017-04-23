#
# Makefile
#
# Circle - A C++ bare metal environment for Raspberry Pi
# Copyright (C) 2014-2017  R. Stange <rsta2@o2online.de>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

CIRCLEHOME = ../../circle

SRCS := ../build_raspi/raspi_main.c ../build_raspi/raspi_framebuffer.c \
  main.cpp kernel.cpp koptions.cpp syscalls.c emmc.c platform.c \
  fatfs/diskio.c fatfs/ff.c fatfs/option/cc932.c

RASPPI = 1

ifeq ($(strip $(CIRCLEHOME)),)
CIRCLEHOME = ..
endif

PREFIX = arm-none-eabi-

CC	= $(PREFIX)gcc
CPP	= $(PREFIX)g++
AS	= $(CC)
#LD	= $(PREFIX)ld
LD	= $(PREFIX)gcc
AR	= $(PREFIX)ar

ifeq ($(strip $(RASPPI)),1)
ARCH	?= -march=armv6j -mtune=arm1176jzf-s -mfloat-abi=softfp 
TARGET	?= kernel
LIBPATH ?= pi1
else ifeq ($(strip $(RASPPI)),2)
ARCH	?= -march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=softfp
TARGET	?= kernel7
LIBPATH ?= pi2
else
ARCH	?= -march=armv8-a -mtune=cortex-a53 -mfpu=neon-fp-armv8 -mfloat-abi=softfp
TARGET	?= kernel8-32
LIBPATH ?= pi3
endif

LIBS	= $(CIRCLEHOME)/$(LIBPATH)/libusb.a \
	  $(CIRCLEHOME)/$(LIBPATH)/libinput.a \
	  $(CIRCLEHOME)/$(LIBPATH)/libfs.a \
	  $(CIRCLEHOME)/$(LIBPATH)/libcircle.a

DARUMA = $(TARGET).img

OPTIMIZE ?= -O2

INCLUDE	+= -I $(CIRCLEHOME)/include -I $(CIRCLEHOME)/addon -I $(CIRCLEHOME)/app/lib

OTHER_DIRS = build/build_raspi build/build_raspi_bm/fatfs build/build_raspi_bm/fatfs/option

AFLAGS	+= $(ARCH) -DRASPPI=$(RASPPI) $(INCLUDE)
CFLAGS	+= $(ARCH) -Wall -Wno-psabi -fsigned-char -fno-builtin \
	   -D__circle__ -D__RASPBERRY_PI__=1 -D__BAREMETAL__=1 -DRASPPI=$(RASPPI) \
     $(INCLUDE) $(OPTIMIZE) -g #-DNDEBUG
CPPFLAGS+= $(CFLAGS) -fno-exceptions -fno-rtti -std=c++14

%.o: %.S
	$(AS) $(AFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CPP) $(CPPFLAGS) -c -o $@ $<

-include ../Makefile.inc

build/common/fontdata.o: ../common/fontdata.bin
	$(PREFIX)objcopy -I binary -O elf32-littlearm -B arm ../common/fontdata.bin build/common/fontdata.o

build/build_raspi_bm/%.o: %.S
	$(AS) $(AFLAGS) -c -o $@ $<

A_OBJS += build/common/fontdata.o build/build_raspi_bm/libstub.o
CFLAGS += -DEMBED_FONTDATA

$(TARGET).img: $(A_OBJS) $(LIBS) $(CIRCLEHOME)/$(LIBPATH)/startup.o $(CIRCLEHOME)/circle.ld
	$(LD) -o $(TARGET).elf -nostartfiles -Wl,-Map,$(TARGET).map -T circle.ld $(CIRCLEHOME)/$(LIBPATH)/startup.o $(A_OBJS) $(LIBS) -lm
	$(PREFIX)objdump -d $(TARGET).elf | $(PREFIX)c++filt > $(TARGET).lst
	$(PREFIX)objcopy $(TARGET).elf -O binary $(TARGET).img
	wc -c $(TARGET).img

build/build_raspi:
	mkdir -p $@

build/build_raspi_bm/fatfs:
	mkdir -p $@

build/build_raspi_bm/fatfs/option:
	mkdir -p $@
