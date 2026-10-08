# tests/Makefile -- host-native build for DCTelnet's platform-independent
# PETSCII logic. Separate from src/Makefile (vbcc/Amiga target).
CC := cc
CFLAGS := -std=c99 -Wall -Wextra -Werror -I../src

# v2.0 implements many tests:
#TESTS := test_petscii_modules test_petscii_dispatch test_prefs_file test_site_prefs test_palette test_screenfont test_progdir test_listsel test_textedit test_dsr

.PHONY: all clean check-encoding check-fonts
all: $(TESTS) check-encoding check-fonts
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
