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

TESTS := test_petscii_modules test_petscii_dispatch test_prefs_file test_site_prefs test_palette test_screenfont test_progdir test_listsel test_textedit test_dsr test_keys test_clip test_ansiscan test_ticks test_renderer test_rexx test_rlogin test_ansimusic test_booklist test_crypto test_ssh test_knownhosts test_term_engine test_connectaddr test_vtpersonality test_petscii_font

# The test programs are host executables, but make only compares
# timestamps. Docker "make ci" (Linux) and "make host-ci" (macOS) share this
# directory, so a binary built by one would be run by the other ("Exec
# format error"). Remove them whenever the host changes.
HOST_ID := $(shell uname -s -m)
ifneq ($(HOST_ID),$(shell cat .host-id 2>/dev/null))
$(shell rm -f $(TESTS) ansimusic_dump; echo '$(HOST_ID)' > .host-id)
endif

.PHONY: all clean check-encoding check-fonts check-currenttime-null check-stack check-statics check-petscii-reopen check-debug-stack check-ssh-terminal check-petscii-termtype check-petscii-font-builtin
all: $(TESTS) check-encoding check-fonts check-currenttime-null check-stack check-statics check-petscii-reopen check-debug-stack check-ssh-terminal check-petscii-termtype
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

# SSH terminal wiring (host tests cannot link DCTelnet.c): the pty TERM comes from
# Vt_SshTermName (terminfo is case-sensitive, the telnet "VT102" is unknown), the
# pty echoes so local echo goes off right after SshConn_Start, and Backspace is
# decided by Vt_BackspaceByte.
check-ssh-terminal:
	@cd ../src && if ! LC_ALL=C grep --text --quiet 'Vt_SshTermName(&prefs)' DCTelnet.c \
		|| LC_ALL=C grep --text --after-context=12 'SshConn_Start(' DCTelnet.c | LC_ALL=C grep --text --quiet 'TelnetTermType' \
		|| ! LC_ALL=C grep --text --after-context=12 'SshConn_Start(' DCTelnet.c | LC_ALL=C grep --text --quiet 'SetLocalEchoBack(FALSE)' \
		|| ! LC_ALL=C grep --text --after-context=6 'static void OutKey' DCTelnet.c | LC_ALL=C grep --text --quiet 'Vt_BackspaceByte'; then \
		echo "ERROR: DCTelnet.c must pass Vt_SshTermName to SshConn_Start, call SetLocalEchoBack(FALSE) after it, and use Vt_BackspaceByte in OutKey."; \
		exit 1; \
	fi

# PETSCII is a Terminal Type. The old State bit (APP_OLD_PETSCII_MODE) is read
# only by the migration in prefs_file.c; every other test of "is this PETSCII"
# goes through Prefs_IsPetscii, the View menu checks the PETSCII item from it,
# and the old menu item and settings checkbox are gone.
check-petscii-termtype:
	@cd ../src && if LC_ALL=C grep -aqE 'APP_PETSCII_MODE|MENU_PETSCII_MODE|SG_TERM_PETSCII' *.c *.h \
		|| LC_ALL=C grep -alE 'APP_OLD_PETSCII_MODE' *.c *.h | LC_ALL=C grep -qvxE 'prefs\.h|prefs_file\.[ch]' \
		|| ! LC_ALL=C grep -a 'MENU_TERMTYPE_PETSCII' DCTelnet.c | LC_ALL=C grep -aq 'Prefs_IsPetscii(&prefs)' \
		|| ! LC_ALL=C grep -a 'define PETSCII_SESSION' DCTelnet.c | LC_ALL=C grep -aq 'Prefs_IsPetscii'; then \
		echo "ERROR: PETSCII must be read through Prefs_IsPetscii; APP_OLD_PETSCII_MODE only in prefs.h, prefs_file.c and prefs_file.h; the old menu item and checkbox must stay gone."; \
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

test_petscii_font: test_petscii_font.c ../src/petscii_font.c ../src/petscii_glyphs.c ../src/petscii_font.h \
                   ../build/package/DCTelnet/Fonts/Petscii/8 ../build/package/DCTelnet/Fonts/PetsciiLower/8
	$(CC) $(CFLAGS) -Ishim -I../src -o $@ $(filter %.c,$^)

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

# C64 font is drawn from the program, without Petscii.font on disk: a PETSCII
# session on ibmcon, console.device or the built-in renderer showed CP437
# lookalikes when the font files were not installed. OpenPetsciiFonts() builds
# both fonts from the compiled-in glyphs (test_petscii_font holds that build
# to the shipped fonts) and must not go back to a disk lookup.
check-petscii-font-builtin:
	@cd ../src && body=$$(LC_ALL=C sed -n '/^static void OpenPetsciiFonts(void)/,/^}/p' DCTelnet.c); \
	if ! printf '%s\n' "$$body" | LC_ALL=C grep --text --quiet 'BuildPetsciiFont(&petsciiFontUpperStore'; then \
		echo "ERROR: DCTelnet.c OpenPetsciiFonts() must build the C64 fonts from the compiled-in glyphs (BuildPetsciiFont)."; exit 1; fi; \
	if printf '%s\n' "$$body" | LC_ALL=C grep --text --quiet 'OpenBundledFont\|OpenDiskFont\|OpenFont'; then \
		echo "ERROR: DCTelnet.c OpenPetsciiFonts() opens a font from disk: without Petscii.font installed the session shows CP437 lookalikes."; exit 1; fi
