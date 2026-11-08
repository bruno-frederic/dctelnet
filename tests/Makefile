# tests/Makefile -- host-native build for DCTelnet's platform-independent
# PETSCII logic. Separate from src/Makefile (vbcc/Amiga target).
CC := cc
CFLAGS := -std=c99 -Wall -Wextra -Werror -I../src

# The SSH engine runs on the caller's stack, 4096 bytes by default on
# Workbench (issue #58). No frame of ssh.c or crypto/ may hold the big
# temporaries again: they live in struct Ssh. gcc only (clang has no
# -Wstack-usage, so the check is skipped there). -O0 gives the largest frames.
STACK_MAX := 400
STACKCHK := $(shell $(CC) -Werror -Wstack-usage=$(STACK_MAX) -c -x c /dev/null -o /dev/null 2>/dev/null && echo -Wstack-usage=$(STACK_MAX))

TESTS := test_petscii_modules test_petscii_dispatch test_prefs_file test_site_prefs test_palette test_screenfont test_progdir test_listsel test_textedit test_dsr test_keys test_clip test_ansiscan test_ticks test_rexx test_rlogin test_ansimusic test_booklist test_crypto test_ssh test_knownhosts test_term_engine test_connectaddr test_recvgate

# The test programs are host executables, but make only compares
# timestamps. Docker "make ci" (Linux) and "make host-ci" (macOS) share this
# directory, so a binary built by one would be run by the other ("Exec
# format error"). Remove them whenever the host changes.
HOST_ID := $(shell uname -s -m)
ifneq ($(HOST_ID),$(shell cat .host-id 2>/dev/null))
$(shell rm -f $(TESTS) ansimusic_dump; echo '$(HOST_ID)' > .host-id)
endif

.PHONY: all clean check-encoding check-fonts check-currenttime-null check-stack check-statics check-petscii-reopen
all: $(TESTS) check-encoding check-fonts check-currenttime-null check-stack check-statics check-petscii-reopen
	@for t in $(TESTS); do echo "--- $$t ---"; ./$$t || exit 1; done

# Check the file are standard printable ASCII or ISO-8859
# Forbid ISO-8859 chars \xC2-\xF4 that might start a UTF-8 multibyte sequence
# Forbid bytes \x80-\x9F that are not ISO 8859 but windows codepage 125x
# Allow only these control chars: \t \r \n and CSI 0x9B
check-encoding:
	@cd .. && pat="$$(printf '[^\040-\176\240-\301\365-\377\t\r\233]')"; \
	LC_ALL=C grep --recursive --line-number --text --extended-regexp "$$pat" \
								--exclude-dir=third_party \
								--exclude-dir=installed \
								--include='*.c' \
								--include='*.h' \
								--include='*.asm' \
								--include='smakefile' \
								--include='Makefile' \
								--include='Install_DCTelnet' \
								--include='*.script' \
								--include='*.guide' \
								--include='*.readme' \
								--include='*.DIZ' \
								. ; \
	rc=$$?; \
	if [ $$rc -eq 0 ]; then \
		echo "ERROR: Disallowed chars found in the files above; they may have been saved as UTF." ; \
		exit 1 ; \
	elif [ $$rc -ne 1 ]; then \
		echo "ERROR: check-encoding: grep failed (exit $$rc), so the files were not checked." ; \
		exit 1 ; \
	fi

# Kickstart 3.1 CurrentTime() writes both out-parameters, so NULL for either one is a write to
# address 0 (MuForce LONG WRITE). Host tests cannot link intuition.library, hence a source check.
check-currenttime-null:
	@cd ../src && if LC_ALL=C grep --text --line-number --extended-regexp 'CurrentTime[[:space:]]*\([^;]*NULL' *.c; then \
		echo "ERROR: CurrentTime() called with a NULL argument; pass a dummy ULONG."; \
		exit 1; \
	fi

# Parse the shipped PETSCII fonts the way diskfont.library loads them. The
# first build put the DiskFontHeader 4 bytes late and every OpenDiskFont()
# failed silently.
check-fonts:
	python3 ../tools/check_amiga_font.py ../build/package/DCTelnet/Fonts/Petscii.font ../build/package/DCTelnet/Fonts/PetsciiLower.font ../build/package/DCTelnet/Fonts/TopazPro.font

check-stack:
	@if [ -n "$(STACKCHK)" ]; then for f in ../src/ssh.c ../src/crypto/*.c; do \
		$(CC) $(CFLAGS) $(STACKCHK) -D_DEFAULT_SOURCE -Ishim -c $$f -o /dev/null || exit 1; done; \
	else echo "check-stack: $(CC) has no -Wstack-usage, skipped"; fi

# Several tasks will open ssh.library at once and share its one data segment,
# so no function may keep mutable state in a static: it lives in struct Ssh
# (see Big temporaries there). Fail on any writable data or bss symbol of the
# engine's objects (nm types b B d D, which also catches gcc's name.N function
# statics) except the entropy pool, shared by design; it needs Forbid() or a
# semaphore once it is in the library (see random.c).
STATICS_OK := pool counter added
check-statics:
	@d=$$(mktemp -d); bad=""; for f in ../src/ssh.c ../src/crypto/*.c; do \
		o=$$d/$$(basename $$f .c).o; \
		$(CC) $(CFLAGS) -O0 -fno-common -D_DEFAULT_SOURCE -Ishim -c $$f -o $$o || exit 1; \
		for sym in $$(nm -P $$o | awk '$$2 ~ /^[bBdD]$$/ { sub(/^_/, "", $$1); if ($$1 !~ /^l?tmp[0-9]*$$/) print $$1 }'); do \
			case " $(STATICS_OK) " in *" $$sym "*) [ "$$(basename $$f)" = random.c ] && continue;; esac; \
			bad="$$bad $$(basename $$f):$$sym"; done; done; rm -rf $$d; \
		if [ -n "$$bad" ]; then echo "check-statics: writable static data in the engine:$$bad"; exit 1; fi; \
		echo "check-statics: ok"

MODULE_SRC := ../src/petscii_screencode.c ../src/petscii_fallback.c ../src/petscii_keymap.c
MODULE_HDR := ../src/petscii_screencode.h ../src/petscii_fallback.h ../src/petscii_keymap.h

test_petscii_modules: test_petscii_modules.c $(MODULE_SRC) $(MODULE_HDR)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

test_petscii_dispatch: test_petscii_dispatch.c ../src/petscii_dispatch.c ../src/petscii_dispatch.h \
                       ../src/petscii_screencode.c ../src/petscii_fallback.c $(MODULE_HDR)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

test_prefs_file: test_prefs_file.c ../src/prefs_file.c ../src/prefs_file.h ../src/prefs.h ../src/palette.c ../src/palette.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

test_site_prefs: test_site_prefs.c ../src/site_prefs.c ../src/site_prefs.h ../src/prefs.h ../src/prefs_file.c ../src/prefs_file.h ../src/palette.c ../src/palette.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

test_palette: test_palette.c ../src/palette.c ../src/palette.h ../src/prefs.h ../src/prefs_file.c ../src/prefs_file.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

test_screenfont: test_screenfont.c ../src/screenfont.c ../src/screenfont.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

test_progdir: test_progdir.c ../src/progdir.c ../src/progdir.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

test_listsel: test_listsel.c ../src/listsel.c ../src/listsel.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

test_textedit: test_textedit.c ../src/textedit.c ../src/textedit.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

test_dsr: test_dsr.c ../src/dsr.c ../src/dsr.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

test_keys: test_keys.c ../src/keys.c ../src/keys.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

test_clip: test_clip.c ../src/clip.c ../src/clip.h ../src/charset.c ../src/charset.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

test_ansiscan: test_ansiscan.c ../src/ansiscan.c ../src/ansiscan.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

test_recvgate: test_recvgate.c ../src/recvgate.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

test_ticks: test_ticks.c ../src/ticks.c ../src/ticks.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

test_rexx: test_rexx.c ../src/rexxcmd.c ../src/rexxcmd.h ../src/waitfor.c ../src/waitfor.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

test_rlogin: test_rlogin.c ../src/rlogin.c ../src/rlogin.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

test_ansimusic: test_ansimusic.c ../src/ansimusic.c ../src/ansimusic.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

test_booklist: test_booklist.c ../src/booklist.c ../src/booklist.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

test_connectaddr: test_connectaddr.c ../src/connectaddr.c ../src/connectaddr.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

clean:
	rm -f $(TESTS) ansimusic_dump .host-id

CRYPTO_SRC := ../src/crypto/sha256.c ../src/crypto/aes.c ../src/crypto/bignum.c ../src/crypto/rsa.c ../src/crypto/x25519.c ../src/crypto/random.c
test_crypto: test_crypto.c crypto_vectors.h bignum_vectors.h $(CRYPTO_SRC) $(wildcard ../src/crypto/*.h)
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

test_ssh: test_ssh.c ../src/ssh.c ../src/ssh.h $(CRYPTO_SRC) $(wildcard ../src/crypto/*.h) $(wildcard ssh_sessions/*.bin)
	$(CC) $(CFLAGS) -D_DEFAULT_SOURCE -Ishim -o $@ $(filter %.c,$^)

test_knownhosts: test_knownhosts.c ../src/knownhosts.c ../src/knownhosts.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

# The engine is written for the Amiga: its static helpers are not all used here.
test_term_engine: test_term_engine.c ../src/third_party/retro32-term/term-engine.c
	$(CC) $(CFLAGS) -Wno-unused-function -Ishim -o $@ test_term_engine.c

# A PETSCII mode change (an Address Book entry starts or ends) must take the
# full reopen on every renderer: OpenDisplay() drops the old mode's kept text
# (VtClose, Shadow_Close) and writes the welcome screen. ReopenTerminal()'s
# in-place console path replayed C64 text into the ANSI console and the
# reverse (host tests cannot link DCTelnet.c).
check-petscii-reopen:
	@cd ../src && if ! LC_ALL=C grep --text --after-context=12 '^static void ReopenTerminal(void)' DCTelnet.c \
		| LC_ALL=C grep --text --after-context=3 '(PETSCII_SESSION() != 0) != displayIsPetscii)' \
		| LC_ALL=C grep --text --quiet 'shouldRestart = TRUE;'; then \
		echo "ERROR: DCTelnet.c ReopenTerminal() must send a PETSCII mode change to the full reopen (shouldRestart), not replay the old mode's text in place."; \
		exit 1; \
	fi
