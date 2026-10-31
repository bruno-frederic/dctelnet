# tests/Makefile -- host-native build for DCTelnet's platform-independent
# PETSCII logic. Separate from src/Makefile (vbcc/Amiga target).
CC := cc
CFLAGS := -std=c99 -Wall -Wextra -Werror -I../src

TESTS := test_petscii_modules test_petscii_dispatch test_prefs_file test_site_prefs test_palette test_screenfont test_glyphraster test_progdir test_listsel test_textedit test_dsr test_keys test_clip test_ansiscan test_ticks test_rexx test_rlogin test_ansimusic test_booklist test_crypto test_ssh test_knownhosts test_term_engine test_renderer test_vtpersonality test_vtpens test_vtinput

.PHONY: all clean check-encoding check-fonts check-debug-stack check-currenttime-null
all: $(TESTS) check-encoding check-fonts check-debug-stack check-currenttime-null
	@for t in $(TESTS); do echo "--- $$t ---"; ./$$t || exit 1; done

# Check the file are standard printable ASCII or ISO-8859
# Forbid ISO-8859 chars \xC2-\xF4 that might start a UTF-8 multibyte sequence
# Forbid bytes \x80-\x9F that are not ISO 8859 but windows codepage 125x
# Allow only these control chars: \t \r \n and CSI 0x9B
check-encoding:
	@cd .. && if LC_ALL=C grep --recursive --line-number --text --perl-regexp \
								'[^\x20-\x7E\xA0-\xC1\xF5-\xFF\t\r\n\x9B]' \
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
	; then \
		echo "ERROR: Disallowed chars found in the files above; they may have been saved as UTF." ; \
		exit 1 ; \
	fi

# The debug Amiga build must not use vbcc's -stack-check. Its check compares
# the stack pointer with ONE global limit, the main task's stack bottom, but
# DCTelnet runs C code on other processes too (CreateNewProc: the Connecting
# window task). When that process's stack lies below the main one, every
# checked call "overflows": the check moves SP onto the main task's stack and
# calls exit() from the wrong process, and the machine resets (debug build,
# `DCTelnet <host> <port>`, rig 3, 2026-10-30).
check-debug-stack:
	@cd ../src && if LC_ALL=C grep --quiet --text 'CreateNewProc' *.c \
		&& LC_ALL=C grep --text 'DEBUG_CFLAGS' Makefile | LC_ALL=C grep --quiet -- '[[:space:]]-stack-check'; then \
		echo "ERROR: src/Makefile debug flags use vbcc -stack-check, which breaks code run by a second process (CreateNewProc)."; \
		exit 1; \
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

test_renderer: test_renderer.c ../src/renderer.c ../src/renderer.h ../src/prefs.h
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

clean:
	rm -f $(TESTS)

CRYPTO_SRC := ../src/crypto/sha256.c ../src/crypto/aes.c ../src/crypto/bignum.c ../src/crypto/rsa.c ../src/crypto/x25519.c ../src/crypto/random.c
test_crypto: test_crypto.c crypto_vectors.h bignum_vectors.h $(CRYPTO_SRC) $(wildcard ../src/crypto/*.h)
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

test_ssh: test_ssh.c ../src/ssh.c ../src/ssh.h $(CRYPTO_SRC) $(wildcard ../src/crypto/*.h) $(wildcard ssh_sessions/*.bin)
	$(CC) $(CFLAGS) -D_DEFAULT_SOURCE -Ishim -o $@ $(filter %.c,$^)

test_knownhosts: test_knownhosts.c ../src/knownhosts.c ../src/knownhosts.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

# The engine is written for the Amiga: its static helpers are not all used here.
VTCON := ../src/third_party/vtcon
VTCONI := -I$(VTCON)/engine
test_vtpersonality: test_vtpersonality.c ../src/vtpersonality.c ../src/vtpersonality.h
	$(CC) $(CFLAGS) -Wno-unused-parameter -Ishim $(VTCONI) -I$(VTCON)/render -o $@ $(filter %.c,$^)

test_vtinput: test_vtinput.c ../src/vtinput.c ../src/vtinput.h $(VTCON)/engine/vtengine.c
	$(CC) $(CFLAGS) -Wno-unused-function -Wno-unused-parameter -Ishim $(VTCONI) -o $@ test_vtinput.c ../src/vtinput.c $(VTCON)/engine/vtengine.c

test_vtpens: test_vtpens.c ../src/vtpens.c ../src/vtpens.h $(VTCON)/engine/vtengine.c
	$(CC) $(CFLAGS) -Wno-unused-function -Wno-unused-parameter -Ishim $(VTCONI) -o $@ test_vtpens.c ../src/vtpens.c $(VTCON)/engine/vtengine.c

test_glyphraster: test_glyphraster.c ../src/glyphraster.c ../src/glyphraster.h $(VTCON)/render/glyphmap.c
	$(CC) $(CFLAGS) -Wno-unused-function -Wno-unused-parameter -Ishim -I$(VTCON)/engine -I$(VTCON)/render -o $@ test_glyphraster.c ../src/glyphraster.c $(VTCON)/render/glyphmap.c

test_term_engine: test_term_engine.c ../src/third_party/retro32-term/term-engine.c ../src/vtbuiltin.c ../src/vtpersonality.c ../src/glyphraster.c ../src/glyphraster.h ../src/screenfont.c ../src/vtpens.c ../src/vtinput.c ../src/vtinput.h \
                  $(VTCON)/engine/vtengine.c $(VTCON)/render/glyphmap.c
	$(CC) $(CFLAGS) -Wno-unused-function -Wno-unused-parameter -Ishim -I$(VTCON)/engine -I$(VTCON)/render \
	      -o $@ test_term_engine.c ../src/glyphraster.c ../src/screenfont.c ../src/vtpersonality.c ../src/vtpens.c ../src/vtinput.c $(VTCON)/engine/vtengine.c $(VTCON)/render/glyphmap.c
