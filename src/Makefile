# Makefile for building with VBCC or GCC
#
# Official build (also used by GitHub CI workflow):
#   Run "make ci" from "src" folder.
#   Requires GNU Make and Docker (plus WSL on Windows), but no locally installed C compiler.
#   Outputs binaries under build/vbcc-680x0-release.
#
# Local builds with compiler toolchain installed:
#   Run from "src" with the selected compiler toolchain installed and its bin directory first in
#   PATH. Use PowerShell on Windows.
# make                    ->  build 68000 & 68020 release binaries with VBCC
# make TOOLCHAIN=gcc      ->  build 68000 & 68020 release binaries with GCC
# make 68000              ->  build only 68000 release binary
# make debug              ->  build 68000 & 68020 debug binaries
# make ci-debug           ->  build only 68000 debug binary
# make DEBUG=yes 68000    ->  build only 68000 debug binary
# make clean              ->  clean debug & release output
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
# Include path configuration with VBCC:
#  - For local builds without Docker under Windows/Linux, set NDK32_INC and NET_INC environment variables.
#      NDK32_INC : /path/to/NDK3.2R4/Include_H
#      NET_INC   : /path/to/Roadshow-SDK/netinclude
#
#  - The Docker builder used by "make ci" defines NDK32_INC and NET_INC automatically.
#
#  - On Windows, if either variable is unset, Makefile falls back to its corresponding legacy default:
#      NDK32_INC : $(VBCC)/NDK3.2R4/Include_H
#      NET_INC   : third_party/netinclude
#
#  - TODO: On MacOS
#
#  - On AmigaOS, add the following paths to the "vincludeos3:" assign in this order:
#      assign >NIL: vincludeos3:  <path/to/NDK3.2R4>/Include_H       ADD
#      assign >NIL: vincludeos3:  <path/to/Roadshow-SDK>/netinclude  ADD
#      assign >NIL: vincludeos3:  third_party/ReqTools/include       ADD
$(info =========================== make $(MAKECMDGOALS) ==================================)

BUILDER_IMAGE := walkero/docker4amigavbcc:m68k-amigaos-2.0.0
# vbcc or gcc
TOOLCHAIN ?= vbcc
CPU       ?= 68000

ifeq ($(OS),Windows_NT)
	NDK32_INC ?= $(VBCC)/NDK3.2R4/Include_H
	NET_INC   ?= third_party/netinclude
endif

INCLUDES := \
	-I$(NDK32_INC) \
	-I$(NET_INC) \
	-Ithird_party/ReqTools/include \
	-Ithird_party/retro32-term \
	-Ithird_party/Xem/INCLUDE \
	-Ithird_party/Xpr/include

ifeq ($(MAKECMDGOALS),build)
  ifeq ($(strip $(NDK32_INC)),)
    $(error NDK32_INC environment variable is not set. Set it to your AmgiaOS 3.2 NDK Include_H directory.)
  endif
  ifeq ($(strip $(NET_INC)),)
    $(error NET_INC environment variable is not set. Set it to your Roadshow SDK netinclude directory.)
  endif
endif

# Submodules must be checked out before anything else can be built.
ifeq (,$(wildcard third_party/retro32-term/term-engine.c))
  $(error third_party/retro32-term/term-engine.c is missing. Run 'git submodule update --init --recursive')
endif

ifeq ($(OS),Windows_NT)
#    Windows-specific: PowerShell accepts '/' as path separator which eases cross-platform Makefile,
#    but disallows backslash line continuation. Thus, all commands MUST fit on a SINGLE line.
#    Helper functions below operate on multiple paths in one call
	SHELL       := powershell.exe
	.SHELLFLAGS := -NoProfile -NoLogo -NonInteractive -ExecutionPolicy bypass -Command
	RM    := function rmps{ foreach ($$f in $$args){rm -Literal $$f -EA Silent} } ; rmps
	TOUCH := $$null = New-Item -ItemType File -Force -Path
	MKDIR := function mdps{ foreach ($$f in $$args){$$null = mkdir -Force -Path  $$f} } ; mdps
#   PODMAN_USERNS=keep-id needed when podman is used instead of docker:
	DOCKER := wsl -- env PODMAN_USERNS=keep-id  docker
	STAT   := wsl stat
	NULL   := $$null
else
	RM:=rm -f
	TOUCH:=touch
	MKDIR := mkdir -p
	DOCKER := env PODMAN_USERNS=keep-id  docker
	STAT   := stat
	NULL   := /dev/null
endif

ifeq ($(TOOLCHAIN),vbcc)
  ifeq ($(MAKECMDGOALS),build)
    ifeq ($(strip $(VBCC)),)
      $(error VBCC environment variable is not set. Set it to your VBCC installation directory)
    endif
  endif
	CC              := vc
#	Do not use spaces, which causes problems:
	COMPILER_STRING := VBCC-$(word 2,$(shell vbccm68k 2>$(NULL)))
#	VBCC's customized headers MUST take priority over the NDK, ReqTools, and project headers:
	INCLUDES        := -I$(VBCC)/targets/m68k-amigaos/include  $(INCLUDES)
	WARN            := -warn=-1 "-dontwarn=163,166,167,168,170,307,65" -warnings-as-errors	# TODO re-enable warning #65
	CFLAGS          := +aos68k $(WARN) -cpu=$(CPU)
		DEBUG_CFLAGS    := -g -D_DEBUG -stack-check
		RELEASE_CFLAGS  := -O3 -speed
	# The SSH bignum multiply in 68000/68020 assembly (vasm syntax); other
	# toolchains build the C loop of crypto/bignum.c.
	CFLAGS          += -DBN_ASM
	ASRCS           := crypto/bn_muladd.s
else ifeq ($(TOOLCHAIN),gcc)
	CC              := m68k-amigaos-gcc
	COMPILER_STRING := GCC-v$(shell m68k-amigaos-gcc -dumpversion)
#	TODO: analyse all these GCC warning then re-enable -Wall:
	WARN            := -Wno-incompatible-pointer-types -Wno-int-conversion -Wno-overflow -Wno-discarded-qualifiers
#	WARN           := -Wall -Wextra -Werror
	CFLAGS          := -noixemul -fbaserel $(WARN) -m$(CPU) -D__AMIGADATE__=__DATE__
		DEBUG_CFLAGS    := -g -D_DEBUG -fstack-check
		RELEASE_CFLAGS  := -O3
	ASRCS           :=
else
	$(error Invalid TOOLCHAIN value '$(TOOLCHAIN)'; choose 'vbcc' or 'gcc')
endif

BUILD_HASH := $(shell git rev-parse --short HEAD)

CFLAGS += -D"COMPILER_STRING=$(COMPILER_STRING)" -DBUILD_HASH=$(BUILD_HASH) $(INCLUDES)


TARGET     := DCTelnet
SRCS       := DCTelnet.c connect.c guis.c Xfer.c Xem_wrapper.c requesters.c utils.c prefs.c prefs_file.c \
              petscii_screencode.c petscii_fallback.c petscii_keymap.c petscii_dispatch.c site_prefs.c palette.c \
              screenfont.c progdir.c shipped.c listsel.c textedit.c dsr.c iconpens.c keys.c clip.c charset.c ansiscan.c ticks.c waitfor.c rexxcmd.c rlogin.c ansimusic.c sound.c booklist.c ssh.c sshconn.c knownhosts.c crypto/sha256.c crypto/aes.c crypto/bignum.c crypto/rsa.c crypto/x25519.c crypto/random.c
DEPS       := abook.h connect.h DCTelnet.h DCTelnet-debug.h DCTelnet-protocol.h third_party/retro32-term/term-engine.c edit.h fkey.h guis.h Xfer.h Xem_wrapper.h requesters.h utils.h prefs.h prefs_file.h site_prefs.h palette.h screenfont.h progdir.h shipped.h listsel.h textedit.h dsr.h iconpens.h keys.h clip.h charset.h ansiscan.h ticks.h waitfor.h rexxcmd.h rlogin.h ansimusic.h sound.h booklist.h ssh.h sshconn.h knownhosts.h crypto/sha256.h crypto/aes.h crypto/bignum.h crypto/rsa.h crypto/x25519.h crypto/random.h
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
OBJS       := $(SRCS:%.c=$(OBJDIR)/%.o) $(ASRCS:%.s=$(OBJDIR)/%.o)
TARGET_EXE := $(OBJDIR)/$(TARGET).$(CPU)
MAKEFILE_UPTODATE=$(OBJDIR)/Makefile.uptodate


# make treats a phony target as always out-of-date and runs its recipe when requested.
.PHONY: all clean debug release ci ci-debug

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

# Setting properly the user avoids git warnings about safe.directory:
DOCKER_USER := $(shell $(STAT) --format '%u:%g'  Makefile)

# Recursive assignment with "=" so target-specific INTERACTIVE is expanded when the recipe runs:
DOCKER_RUN = $(DOCKER) run $(INTERACTIVE) --user $(DOCKER_USER) -q --rm --volume "./..:/opt/code" --workdir /opt/code/src "$(BUILDER_IMAGE)"
ci:
	$(DOCKER_RUN) make

ci-debug:
	$(DOCKER_RUN) make DEBUG=yes 68000

ibmcon:
	$(DOCKER_RUN) make -C ../ibmcon

tests:
	$(DOCKER_RUN) make -C ../tests


# for debugging builder container
enter-builder: INTERACTIVE := --interactive --tty
enter-builder:
	$(DOCKER_RUN) bash

# Add "show_env" to target "build:" prerequisites for debugging runners and containers
show_env:
ifeq ($(OS),Windows_NT)
	-@echo Windows_NT
	-@whoami
else
	-@grep PRETTY_NAME= /etc/*release
	-@id
	-@env | sort
	-pwd
	-ls -ln       | head
	-ls -ln ..    | head
	-ls -ln ../.. | head
endif
	@echo ---

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


$(OBJDIR)/%.o: %.s | $(OBJDIR)
	@echo "------------- $@ --------------------"
	$(CC) $(CFLAGS) -o $@  -c $<


$(OBJDIR):
	@echo "------------- mkdir $@ --------------------"
	$(MKDIR) $@

# The SSH crypto sources build into a subdirectory of their own.
$(OBJS): | $(OBJDIR)/crypto

# vbcc stops at 30 optimizer passes (warning 172, an error here); the SSH
# code needs more at -speed, and AES runs for every byte of a session.
ifeq ($(TOOLCHAIN),vbcc)
$(OBJDIR)/ssh.o $(ASRCS:%.s=$(OBJDIR)/%.o) $(patsubst %.c,$(OBJDIR)/%.o,$(filter crypto/%,$(SRCS))): CFLAGS += -maxoptpasses=100
endif

$(OBJDIR)/crypto: | $(OBJDIR)
	$(MKDIR) $@


$(INSTALLDIR):
	@echo "------------- mkdir $@ --------------------"
	$(MKDIR) $@


install: $(TARGET_EXE) ibmcon | $(INSTALLDIR)
	@echo "------------- $@ $^ --------------------"
	cp $(TARGET_EXE) $(INSTALLDIR)
	cp ../ibmcon/ibmcon.device $(INSTALLDIR)/Devs


clean: 68000_clean 68020_clean

68000_clean:
	$(MAKE) CPU=68000 DEBUG=no clean_cpu
	$(MAKE) CPU=68000 DEBUG=yes clean_cpu

68020_clean:
	$(MAKE) CPU=68020 DEBUG=no clean_cpu
	$(MAKE) CPU=68020 DEBUG=yes clean_cpu

clean_cpu:
	@echo "----------------- $@ $(CPU) ------------------------"
#	"-" sign -> make will ignore errors of the commands
	-$(RM) $(TARGET_EXE) $(OBJS) $(MAKEFILE_UPTODATE)

# Force clean when Makefile is updated :
$(MAKEFILE_UPTODATE): Makefile | $(OBJDIR)
	make clean
	$(TOUCH) $@
