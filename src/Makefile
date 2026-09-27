# Makefile for building with VBCC or GCC
#
# Tested on Windows with:
#   VBCC 0.9h patch 3
#   Bebbo's Amiga-GCC 6.5.0b
#   GNU Make 4.2.90 built for Windows32 & 4.4.1 built for x86_64-pc-msys
#
# Notes:
#  - Put the selected toolchain's bin directory first in PATH.
#    Otherwise, commands from the other toolchain may be picked up instead.
#  - On Windows, Bebbo's Amiga-GCC 6.5.0b replaces GNU Make with a patched executable that is
#    unreliable with current versions of GNU Make. Restore the original executable from the GCC
#    toolchain before building:
#      copy amiga-gcc\bin\_make.exe amiga-gcc\bin\make.exe
#
#
#
# make must be run in "src" folder under Powershell.
#
# make                    ->  build 68000 & 68020 release binaries with VBCC
# make TOOLCHAIN=gcc      ->  build 68000 & 68020 release binaries with GCC
# make 68000              ->  build only 68000 release binary
# make debug              ->  build 68000 & 68020 debug binaries
# make DEBUG=yes 68000    ->  build only 68000 debug binary
# make DEBUG=yes clean    ->  clean debug binaries
# make clean              ->  clean release binaries

# Include path configuration:
#   - On AmigaOS, add the following paths to vincludeos3:
#       -I<PATH_TO_NDK3.2R4>/Include_H
#       -Ithird_party/netinclude
#       -Ithird_party/ReqTools/include
#
#   - On other OS, edit vbcc/config/aos68k and append the same include paths to the end of
#     the -cc= and -ccv= lines, AFTER: -I%%VBCC%%/targets/m68k-amigaos/include

# These includes can be placed here:
INCLUDES := \
	-Ithird_party/retro32-term \
	-Ithird_party/Xem/INCLUDE \
	-Ithird_party/Xpr/include

# vbcc or gcc
TOOLCHAIN ?= vbcc
CPU       ?= 68000

ifeq ($(TOOLCHAIN),vbcc)
	# Do not use spaces or '.' VC passes '.' to the linker, which causes problems.
	COMPILER_STRING := VBCC_0_9h_patch_3
	CC              := vc
	WARN            := -warn=-1 "-dontwarn=163,166,167,168,170,307" -warnings-as-errors
	CFLAGS          := +aos68k $(WARN) -cpu=$(CPU)
		DEBUG_CFLAGS    := -g -D_DEBUG -stack-check
		RELEASE_CFLAGS  := -O3 -speed
else ifeq ($(TOOLCHAIN),gcc)
	COMPILER_STRING := GCC $(shell m68k-amigaos-gcc -dumpversion)
	CC              := m68k-amigaos-gcc
	# TODO: analyse all these GCC warning then enables all:
	WARN            := -Wno-incompatible-pointer-types -Wno-int-conversion -Wno-overflow -Wno-discarded-qualifiers
	#WARN           := -Wall -Wextra -Werror
	CFLAGS          := -noixemul -fbaserel $(WARN) -m$(CPU) -D__AMIGADATE__=__DATE__
		DEBUG_CFLAGS    := -g -D_DEBUG -fstack-check
		RELEASE_CFLAGS  := -O3
else
	$(error Invalid TOOLCHAIN value '$(TOOLCHAIN)'; choose 'vbcc' or 'gcc')
endif

BUILD_HASH := $(shell git rev-parse --short HEAD)

CFLAGS += -D"COMPILER_STRING=$(COMPILER_STRING)" -DBUILD_HASH=$(BUILD_HASH) $(INCLUDES)


TARGET     := DCTelnet
SRCS       := DCTelnet.c connect.c guis.c Xfer.c Xem_wrapper.c requesters.c utils.c prefs.c \
              petscii_screencode.c petscii_fallback.c petscii_keymap.c petscii_dispatch.c
DEPS       := abook.h connect.h DCTelnet.h DCTelnet-debug.h DCTelnet-protocol.h third_party/retro32-term/term-engine.c edit.h fkey.h guis.h Xfer.h Xem_wrapper.h requesters.h utils.h prefs.h
INSTALLDIR := ../build/installed/DCTelnet

# OBJ subdir depends on TOOLCHAIN and DEBUG flag (use DEBUG=yes to enable)
OBJ_SUFFIX ?= release
ifeq ($(DEBUG),yes)
	CFLAGS += $(DEBUG_CFLAGS)
	OBJ_SUFFIX := debug
else
	CFLAGS += $(RELEASE_CFLAGS)
	OBJ_SUFFIX := release
endif

# final OBJDIR depends on TOOLCHAIN, CPU and OBJ_SUFFIX
OBJDIR     := ../build/$(TOOLCHAIN)-$(CPU)-$(OBJ_SUFFIX)
OBJS       := $(SRCS:%.c=$(OBJDIR)/%.o)
TARGET_EXE := $(OBJDIR)/$(TARGET).$(CPU)
MAKEFILE_UPTODATE=$(OBJDIR)/Makefile.uptodate

ifeq ($(OS),Windows_NT)
#    Windows-specific: PowerShell accepts '/' as path separator which eases cross-platform Makefile,
#    but disallows backslash line continuation. Thus, all commands MUST fit on a SINGLE line.
#    Helper functions below operate on multiple paths in one call
	SHELL       := powershell.exe
	.SHELLFLAGS := -NoProfile -NoLogo -NonInteractive -ExecutionPolicy bypass -Command
	RM    := function rmps{ foreach ($$f in $$args){rm -Literal $$f -EA Silent} } ; rmps
	TOUCH := $$null = New-Item -ItemType File -Force -Path
	MKDIR := function mdps{ foreach ($$f in $$args){$$null = mkdir -Force -Path  $$f} } ; mdps
else
	RM:=rm -f
	TOUCH:=touch
	MKDIR := mkdir -p
endif


# make treats a phony target as always out-of-date and runs its recipe when requested.
.PHONY: all clean debug release

all: $(MAKEFILE_UPTODATE) 68000 68020

68000:
	$(MAKE) CPU=68000 build

68020:
	$(MAKE) CPU=68020 build

build: $(TARGET_EXE)

debug:
	$(MAKE) DEBUG=yes all

release:
	$(MAKE) all


LIBS := -lamiga     # needed for NewObject() in guis.c. It is a varargs stub for NewObjectA().

$(TARGET_EXE): $(OBJS)
	@echo "------------- Link $@ --------------------"
# $^   -> all dependants files (= all OBJ .o files)
	$(CC) $(CFLAGS) -o $@  $^ $(LIBS)


$(OBJDIR)/%.o: %.c $(DEPS) | $(OBJDIR)
	@echo "------------- $@ --------------------"
# '|' makes $(OBJDIR) an order-only prerequisite: ensures exists but ignores its timestamp.
# $<   -> first dependant file (=%c)
	$(CC) $(CFLAGS) -o $@  -c $<


$(OBJDIR):
	@echo "------------- mkdir $@ --------------------"
	$(MKDIR) $@


$(INSTALLDIR):
	@echo "------------- mkdir $@ --------------------"
	$(MKDIR) $@


install: $(TARGET_EXE) | $(INSTALLDIR)
	@echo "------------- $@ $^ --------------------"
	cp $(TARGET_EXE) $(INSTALLDIR)


clean: 68000_clean 68020_clean

68000_clean:
	$(MAKE) CPU=68000 clean_cpu

68020_clean:
	$(MAKE) CPU=68020 clean_cpu

clean_cpu:
	@echo "----------------- $@ $(CPU) ------------------------"
#	"-" sign -> make will ignore errors of the commands
	-$(RM) $(TARGET_EXE) $(OBJS) $(MAKEFILE_UPTODATE)

# Force clean when Makefile is updated :
$(MAKEFILE_UPTODATE): Makefile | $(OBJDIR)
	make clean
	$(TOUCH) $@
