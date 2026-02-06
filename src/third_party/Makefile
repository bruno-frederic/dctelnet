# Makefile for proto/pragma headers with fd2pragma
# (tested on Windows with fd2pragma and GNU Make for Windows. Incompatible with SAS/C smake)
#
# GNU make must be run in "src/third_party" folder.
#

FD2PRAGMA=fd2pragma

ifeq ($(OS),Windows_NT)
#    Windows-specific: PowerShell accepts '/' as path separator which eases cross-platform Makefile,
#    but disallows backslash line continuation. Thus, all commands MUST fit on a SINGLE line.
#    Helper functions below operate on multiple paths in one call
	SHELL       := powershell.exe
	.SHELLFLAGS := -NoProfile -NoLogo -NonInteractive -ExecutionPolicy bypass -Command
	RM    := function rmps{ foreach ($$f in $$args){rm -Literal $$f -EA Silent} } ; rmps
	TOUCH := $$null = New-Item -ItemType File -Force -Path
	MKDIR := function mdps{ foreach ($$f in $$args){$$null = mkdir -Force -Path  $$f} } ; mdps
else
	RM:=rm -f
	TOUCH:=touch
	MKDIR := mkdir -p
endif

HEADERS_BUILT := Xpr/include/proto/xpr.h \
                 Xpr/include/inline/xpr_protos.h \
                 Xpr/include/pragma/xpr_lib.h \
                 Xem/include/proto/xem.h \
                 Xem/include/inline/xem_protos.h \
                 Xem/include/pragma/xem_lib.h

all: folders $(HEADERS_BUILT)

folders:
	$(MKDIR)       Xpr/include/proto        Xpr/include/inline        Xpr/include/pragma
	$(MKDIR)       Xem/include/proto        Xem/include/inline        Xem/include/pragma


FD_XEM=Xem/xem_lib.fd
CLIB_XEM=Xem/include/clib/xem_protos.h
# Proto header file for all compilers (#38):
Xem/include/proto/xem.h : $(FD_XEM)
	@echo "------------- $@ --------------------"
	$(FD2PRAGMA) --infile $(FD_XEM) --special 38 --to Xem/include/proto

# Header file with VBCC-specific assembler inline calls (#70):
Xem/include/inline/xem_protos.h : $(FD_XEM) $(CLIB_XEM)
	@echo "------------- $@ --------------------"
	$(FD2PRAGMA) -i $(FD_XEM) --clib $(CLIB_XEM) -s 70 --voidbase -t Xem/include/inline

# Create the correct pragma files for SAS/C (#6):
Xem/include/pragma/xem_lib.h : $(FD_XEM) $(CLIB_XEM)
	@echo "------------- $@ --------------------"
	$(FD2PRAGMA) -i $(FD_XEM) --clib $(CLIB_XEM) -s  6 --voidbase -t Xem/include/pragma


FD_XPR=Xpr/xpr_lib.fd
CLIB_XPR=Xpr/include/clib/xpr_protos.h

# Proto header file for all compilers (VBCC inline) (#38):
Xpr/include/proto/xpr.h : $(FD_XPR)
	@echo "------------- $@ --------------------"
	$(FD2PRAGMA) --infile $(FD_XPR) --special 38                  --to Xpr/include/proto

# Header file with VBCC-specific assembler inline calls (#70):
Xpr/include/inline/xpr_protos.h : $(FD_XPR) $(CLIB_XPR)
	@echo "------------- $@ --------------------"
	$(FD2PRAGMA) -i $(FD_XPR) --clib $(CLIB_XPR) -s 70 --voidbase --to Xpr/include/inline

# Create the correct pragma files for SAS/C (#6):
Xpr/include/pragma/xpr_lib.h : $(FD_XPR) $(CLIB_XPR)
	@echo "------------- $@ --------------------"
	$(FD2PRAGMA) -i $(FD_XPR) --clib $(CLIB_XPR) -s  6 --voidbase --to Xpr/include/pragma


clean:
	@echo "----------------- $@ ------------------------"
	-$(RM) $(HEADERS_BUILT)

