# test/Makefile -- host-native build for DCTelnet's platform-independent
# PETSCII logic. Separate from src/Makefile (vbcc/Amiga target).
CC := cc
CFLAGS := -std=c99 -Wall -Wextra -Werror -I../src

TESTS := test_petscii_modules test_petscii_dispatch

.PHONY: all clean check-encoding check-fonts
all: $(TESTS) check-encoding check-fonts
	@for t in $(TESTS); do echo "--- $$t ---"; ./$$t || exit 1; done

# DCTelnet's own sources carry legacy 8-bit bytes (Amiga CSI 0x9B, Latin-1
# accents) inside live strings. A UTF-8-only editor rewrites them as U+FFFD
# (EF BF BD) file-wide, and every LocalPrint() CSI becomes garbage. Fail if
# any source holds U+FFFD.
check-encoding:
	@if LC_ALL=C grep -l "$$(printf '\357\277\275')" ../src/*.c ../src/*.h; then \
		echo "U+FFFD found in the files above: 8-bit bytes were destroyed by a UTF-8 round-trip"; exit 1; fi

# Parse the shipped PETSCII fonts the way diskfont.library loads them. The
# first build put the DiskFontHeader 4 bytes late and every OpenDiskFont()
# failed silently.
check-fonts:
	python3 ../tools/check_amiga_font.py ../Fonts/Petscii.font ../Fonts/PetsciiLower.font

MODULE_SRC := ../src/petscii_screencode.c ../src/petscii_fallback.c ../src/petscii_keymap.c
MODULE_HDR := ../src/petscii_screencode.h ../src/petscii_fallback.h ../src/petscii_keymap.h

test_petscii_modules: test_petscii_modules.c $(MODULE_SRC) $(MODULE_HDR)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

test_petscii_dispatch: test_petscii_dispatch.c ../src/petscii_dispatch.c ../src/petscii_dispatch.h \
                       ../src/petscii_screencode.c ../src/petscii_fallback.c $(MODULE_HDR)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

clean:
	rm -f $(TESTS)
