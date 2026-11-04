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
# the per-CPU settings below. Every build carries both multiply loops in
# bn_muladd.s; bignum.c picks the 16-bit one at run time on the 68000 and
# the 68060 (Bn_HasMul64), so no variant needs its own assembler source.
VBCC ?= /opt/homebrew/opt/vbcc
NDK  ?= /Users/spot/Code/UP-Term/vtcon/vendor/ndk-3.2r4-Include_H
VAMOS ?= vamos
export VBCC

CPUS := 68000 68020 68040 68060
SRC  := ..
# One Makefile variable per CPU: what vc and vasm get besides -cpu.
DIV_68000 := build/68000/ssh_div.o
ASFLAGS_68000 := -m68000
ASFLAGS_68020 := -m68020
ASFLAGS_68040 := -m68040
ASFLAGS_68060 := -m68060

# -DSSH_LIBRARY: random.c locks its pool. -DBN_ASM: bignum.c uses bn_muladd.s.
# The stack check and small data are off: the library runs on its caller's stack.
WARN   := -warn=-1 "-dontwarn=163,166,167,168,170,306,307" -warnings-as-errors
CFLAGS := +build/aos68k.cfg $(WARN) -O3 -speed -DSSH_LIBRARY -DBN_ASM \
          -Iinclude -I$(SRC) -I$(SRC)/crypto
# vbcc stops at 30 optimizer passes; the engine needs more at -speed (src/Makefile).
ENGINE_CFLAGS := -maxoptpasses=100

ENGINE_C := $(SRC)/ssh.c $(wildcard $(SRC)/crypto/*.c)

.PHONY: all clean test test-conn $(CPUS)
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
# vamos has no 68060 model; the 68040 one runs the 68060 build. It then
# reports a 68040, so the 060 multiply pick (Bn_HasMul64) is tested on the
# host (tests/test_crypto.c), not here.
VAMOS_CPU := $(if $(filter 68060,$(TEST_CPU)),68040,$(TEST_CPU))
test: build/$(TEST_CPU)/ssh.library build/aos68k.cfg
	@mkdir -p build/test/LIBS
	cp build/$(TEST_CPU)/ssh.library build/test/LIBS/ssh.library
	vc $(CFLAGS) -cpu=$(TEST_CPU) -Iinclude -o build/test/ssh_lib_test test/ssh_lib_test.c
	$(VAMOS) -C $(VAMOS_CPU) -V sshtest:$(CURDIR)/build/test -V sshrec:$(abspath $(CURDIR)/../../tests/ssh_sessions) -a LIBS:sshtest:LIBS \
		sshtest:ssh_lib_test sshrec:curve25519_password_rekey.bin

# The application's side: ../sshconn.c itself (DCTelnet.h and all) with the
# requesters, console and sockets replaced (test/sshconn_test.c), run once
# with ssh.library in LIBS: and once with an empty LIBS: (a directory of its
# own: vamos finds the library in a sibling LIBS directory otherwise).
APP_INC := -I$(SRC) -I$(SRC)/third_party/retro32-term -I$(SRC)/third_party/Xem/INCLUDE \
           -I$(SRC)/third_party/Xpr/include -I$(SRC)/third_party/netinclude \
           -I$(SRC)/third_party/ReqTools/include
test-conn: build/$(TEST_CPU)/ssh.library build/aos68k.cfg
	@mkdir -p build/test/LIBS build/test-nolib/LIBS
	cp build/$(TEST_CPU)/ssh.library build/test/LIBS/ssh.library
	vc +build/aos68k.cfg $(WARN) -O3 -cpu=$(TEST_CPU) -DBUILD_HASH=test -DCOMPILER_STRING=test $(APP_INC) -Iinclude \
		-o build/test/sshconn_test test/sshconn_test.c $(SRC)/sshconn.c $(SRC)/knownhosts.c
	$(VAMOS) -C $(VAMOS_CPU) -V sshtest:$(CURDIR)/build/test -a LIBS:sshtest:LIBS sshtest:sshconn_test present
	cp build/test/sshconn_test build/test-nolib/sshconn_test
	$(VAMOS) -C $(VAMOS_CPU) -V sshtest:$(CURDIR)/build/test-nolib -a LIBS:sshtest:LIBS sshtest:sshconn_test missing

clean:
	rm -rf build
