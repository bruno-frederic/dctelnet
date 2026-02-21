# Makefile for VBCC (tested on Windows with VBCC and GNU Make for Windows).
#
# make must be run in "src" folder under Powershell.
#
# make                    ->  build 68000 & 68020 release binaries
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
	-Ithird_party/Xem/INCLUDE \
	-Ithird_party/Xpr/include

# Do not use spaces or '.' VC passes '.' to the linker, which causes problems.
COMPILER_STRING=VBCC_0_9h_patch_3
CC=vc

TARGET     := DCTelnet
CPU        ?= 68000
CFLAGS     := +aos68k -cpu=$(CPU) -DCOMPILER_STRING=$(COMPILER_STRING) $(INCLUDES)
SRCS       := DCTelnet.c connect.c guis.c Xfer.c
DEPS       := abook.h connect.h DCTelnet.h DCTelnet-debug.h edit.h fkey.h guis.h Xfer.h
INSTALLDIR := ../build/installed/DCTelnet

# OBJ subdir depends on DEBUG flag (use DEBUG=yes to enable)
OBJ_SUFFIX ?= release
ifeq ($(DEBUG),yes)
	CFLAGS += -g -D_DEBUG
	OBJ_SUFFIX := debug
else
	CFLAGS += -O3 -size
	OBJ_SUFFIX := release
endif

# final OBJDIR depends on CPU and OBJ_SUFFIX
OBJDIR     := ../build/vbcc-$(CPU)-$(OBJ_SUFFIX)
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
