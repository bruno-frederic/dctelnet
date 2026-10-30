# src/sshlib/Makefile -- ssh.library, the SSH engine as an AmigaOS shared library.
#
#   make              ->  build/68000/ssh.library and build/68020/ssh.library
#   make 68020        ->  one CPU
#   make test         ->  run test/ssh_lib_test under vamos against the built library
#   make clean
#
# Needs vbcc (vc, vasmm68k_mot, vlink; VBCC below) and the NDK 3.2 headers
# (NDK; Homebrew's NDK3.2R4 directory is empty, hence its own config: the
# template aos68k.cfg.in gets the NDK path). CPU variants come from CPUS and
# the per-CPU settings in the "cpu-<n>" block: add 68040 / 68060 there (the
# 68060 also needs the bn_muladd.s without 64-bit mulu.l, -DBN_060=1).
VBCC ?= /opt/homebrew/opt/vbcc
NDK  ?= /Users/spot/Code/UP-Term/vtcon/vendor/ndk-3.2r4-Include_H
VAMOS ?= vamos
export VBCC

CPUS := 68000 68020
SRC  := ..
# One Makefile variable per CPU: what vc and vasm get besides -cpu.
DIV_68000 := build/68000/ssh_div.o
ASFLAGS_68000 := -m68000
ASFLAGS_68020 := -m68020
ASFLAGS_68040 := -m68040
ASFLAGS_68060 := -m68060 -DBN_060=1

# -DSSH_LIBRARY: random.c locks its pool. -DBN_ASM: bignum.c uses bn_muladd.s.
# The stack check and small data are off: the library runs on its caller's stack.
WARN   := -warn=-1 "-dontwarn=163,166,167,168,170,306,307" -warnings-as-errors
CFLAGS := +build/aos68k.cfg $(WARN) -O3 -speed -DSSH_LIBRARY -DBN_ASM \
          -Iinclude -I$(SRC) -I$(SRC)/crypto
# vbcc stops at 30 optimizer passes; the engine needs more at -speed (src/Makefile).
ENGINE_CFLAGS := -maxoptpasses=100

ENGINE_C := $(SRC)/ssh.c $(wildcard $(SRC)/crypto/*.c)

.PHONY: all clean test $(CPUS)
all: $(CPUS)

build/aos68k.cfg: aos68k.cfg.in
	@mkdir -p build
	sed 's|@NDK@|$(NDK)|' $< > $@

define CPU_RULES
$(1): build/$(1)/ssh.library

build/$(1)/ssh.library: build/$(1)/ssh_romtag.o build/$(1)/ssh_lib.o build/$(1)/ssh_libc.o \
		$(patsubst $(SRC)/%.c,build/$(1)/engine/%.o,$(ENGINE_C)) build/$(1)/bn_muladd.o $$(DIV_$(1))
	vlink -bamigahunk -x -Bstatic -Cvbcc -nostdlib -o $$@ $$^
	@echo "[OK] $$@ ($$$$(wc -c < $$@) bytes)"

# The ROM tag first: no startup code, the first hunk starts with it.
build/$(1)/ssh_romtag.o: ssh_romtag.asm
	@mkdir -p build/$(1)
	vasmm68k_mot -quiet -Fhunk $$(ASFLAGS_$(1)) -o $$@ $$<

build/$(1)/ssh_div.o: ssh_div.asm
	@mkdir -p build/$(1)
	vasmm68k_mot -quiet -Fhunk $$(ASFLAGS_$(1)) -o $$@ $$<

build/$(1)/bn_muladd.o: $(SRC)/crypto/bn_muladd.s
	@mkdir -p build/$(1)
	vasmm68k_mot -quiet -Fhunk -nowarn=62 $$(ASFLAGS_$(1)) -o $$@ $$<

build/$(1)/ssh_%.o: ssh_%.c build/aos68k.cfg include/libraries/ssh.h $(SRC)/ssh.h
	@mkdir -p build/$(1)
	vc $$(CFLAGS) -cpu=$(1) -c -o $$@ $$<

build/$(1)/engine/%.o: $(SRC)/%.c build/aos68k.cfg include/libraries/ssh.h
	@mkdir -p $$(dir $$@)
	vc $$(CFLAGS) $$(ENGINE_CFLAGS) -cpu=$(1) -c -o $$@ $$<
endef
$(foreach c,$(CPUS),$(eval $(call CPU_RULES,$(c))))

# Under vamos on a LIBS: assign of its own (a copy of the built 68000 library).
TEST_CPU ?= 68000
test: build/$(TEST_CPU)/ssh.library build/aos68k.cfg
	@mkdir -p build/test/LIBS
	cp build/$(TEST_CPU)/ssh.library build/test/LIBS/ssh.library
	vc $(CFLAGS) -cpu=$(TEST_CPU) -Iinclude -o build/test/ssh_lib_test test/ssh_lib_test.c
	$(VAMOS) -C $(TEST_CPU) -V sshtest:$(CURDIR)/build/test -V sshrec:$(abspath $(CURDIR)/../../tests/ssh_sessions) -a LIBS:sshtest:LIBS \
		sshtest:ssh_lib_test sshrec:curve25519_password_rekey.bin

clean:
	rm -rf build
