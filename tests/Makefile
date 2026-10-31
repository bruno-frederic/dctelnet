# tests/Makefile -- host-native build for DCTelnet's platform-independent
# PETSCII logic. Separate from src/Makefile (vbcc/Amiga target).
CC := cc
CFLAGS := -std=c99 -Wall -Wextra -Werror -I../src

TESTS := test_petscii_modules test_petscii_dispatch test_prefs_file test_site_prefs test_palette test_screenfont test_progdir test_listsel test_textedit test_dsr test_keys test_clip test_ansiscan test_ticks test_rexx test_rlogin test_ansimusic test_booklist test_iconpens

.PHONY: all clean check-encoding check-fonts check-icons
all: $(TESTS) check-encoding check-fonts check-icons
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

# Parse the shipped PETSCII fonts the way diskfont.library loads them. The
# first build put the DiskFontHeader 4 bytes late and every OpenDiskFont()
# failed silently.
check-fonts:
	python3 ../tools/check_amiga_font.py ../build/package/DCTelnet/Fonts/Petscii.font ../build/package/DCTelnet/Fonts/PetsciiLower.font ../build/package/DCTelnet/Fonts/TopazPro.font

# The tool bar icons carry their colours (DCTELNET_PALETTE) and DCTelnet maps
# them to the screen's nearest pens. The 1.6 icons were 8-colour MagicWB art
# for fixed pens and drew pink and cyan on a standard palette.
check-icons:
	python3 ../tools/toolbar_icons.py check

MODULE_SRC := ../src/petscii_screencode.c ../src/petscii_fallback.c ../src/petscii_keymap.c
MODULE_HDR := ../src/petscii_screencode.h ../src/petscii_fallback.h ../src/petscii_keymap.h

test_petscii_modules: test_petscii_modules.c $(MODULE_SRC) $(MODULE_HDR)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

test_petscii_dispatch: test_petscii_dispatch.c ../src/petscii_dispatch.c ../src/petscii_dispatch.h \
                       ../src/petscii_screencode.c ../src/petscii_fallback.c $(MODULE_HDR)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

test_prefs_file: test_prefs_file.c ../src/prefs_file.c ../src/prefs_file.h ../src/prefs.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

test_site_prefs: test_site_prefs.c ../src/site_prefs.c ../src/site_prefs.h ../src/prefs.h ../src/prefs_file.c ../src/prefs_file.h
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

test_iconpens: test_iconpens.c ../src/iconpens.c ../src/iconpens.h
	$(CC) $(CFLAGS) -Ishim -o $@ $(filter %.c,$^)

clean:
	rm -f $(TESTS)
