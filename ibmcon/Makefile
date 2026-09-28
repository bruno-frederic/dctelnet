# ibmcon.device -- build and the 68000 check.
# The source starts with MC68020 (a reassembler artifact); every instruction
# must still be plain 68000. `make check` assembles a copy with MC68000 and
# requires the two binaries to be identical.
VASM := vasmm68k_mot
FLAGS := -quiet -Fhunkexe -kick1hunks -nosym
SRC := ibmcon.device.bugfixed.asm

all: ibmcon.device check

ibmcon.device: $(SRC)
	$(VASM) $(FLAGS) -o $@ $(SRC)

check: ibmcon.device
	sed 's/^        MC68020 /        MC68000 /' $(SRC) > .check-68000.asm
	$(VASM) $(FLAGS) -o .check-68000.device .check-68000.asm
	cmp ibmcon.device .check-68000.device
	@rm -f .check-68000.asm .check-68000.device
	@echo "[OK] ibmcon.device is plain 68000"

.PHONY: all check
