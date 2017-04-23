#  Makefile for BareMetal PiBasic
#  2016.3.13. Toshi Nagata
#  Copyright 2016 Toshi Nagata. All rights reserved.

export BAREMETAL = 1

SUBDIR := $(shell basename $(PWD))
TARGET := kernel.img
SRCS := ../build_raspi/raspi_main.c ../build_raspi/raspi_framebuffer.c baremetal.c usb.c syscalls.c

TOOLCHAIN := /usr/local/cross-pi/bin/arm-none-eabi-
 
CC := $(TOOLCHAIN)gcc
CXX := $(TOOLCHAIN)g++
LD := $(TOOLCHAIN)ld
AS := $(TOOLCHAIN)as
AR := $(TOOLCHAIN)ar
OBJCOPY := $(TOOLCHAIN)objcopy

#  The maccasoft Raspberry-Pi template is installed here
#  https://github.com/maccasoft/raspberry-pi
#  (By default, it is installed in /opt/raspberry-pi;
#   it is a single directory and can be simply mv'ed to the directory below)
KERNEL_DIR := /usr/local/cross-pi/maccasoft

USPI := 1

ASFLAGS = --warn -mcpu=arm1176jzf-s -mfpu=vfp -mfloat-abi=softfp
CFLAGS += -Wall -O2 -ffreestanding -marm -mcpu=arm1176jzf-s -mfpu=vfp -mfloat-abi=softfp -fsigned-char -I$(KERNEL_DIR)/include -D__RASPBERRY_PI__=1 -D__BAREMETAL__=1 -D__FILES__  -std=c99
CPPFLAGS = $(CFLAGS) -fno-exceptions -fno-unwind-tables -fno-rtti
LDFLAGS = -T $(KERNEL_DIR)/raspberry.ld -nostartfiles -fno-exceptions -fno-unwind-tables -fno-rtti

ifeq ($(USPI),1)
CFLAGS += -DHAVE_USPI
endif

LDFLAGS += -L$(KERNEL_DIR)/lib
LIBS += -lkernel
LIBS_DEP += $(TARGET_DIR)/libkernel.a

OTHER_DIRS = build/build_raspi

-include ../Makefile.inc

kernel.img: $(A_OBJS) $(SYS_OBJS)
	$(CXX) $(LDFLAGS) -Wl,-Map=build/$(SUBDIR)/kernel.map -o build/$(SUBDIR)/kernel.elf $(A_OBJS) $(SYS_OBJS) $(LIBS)
	$(OBJCOPY) build/$(SUBDIR)/kernel.elf -O binary kernel.img
	mkdir boot; cp _boot/* boot; cp kernel.img boot; cp -R ../basic boot; cp ../common/fontdata.bin boot; zip -r daruma_raspi_bm.zip boot -x \*.DS_Store; rm -rf boot; mv -f daruma_raspi_bm.zip ../binaries

build/build_raspi:
	mkdir -p build/build_raspi

