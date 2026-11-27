#!/bin/sh
## @file  bench-responsiveness.sh
## @brief Simulate navigating a BBS-style indexed message reader using dialog on Linux.
##
## Allows interactively testing DCTelnet's responsiveness (response time) when navigating the menus.
## This is a Telnet client use case that particularly stresses the latency.
##
## @author Bruno FREDERIC
## @date 2026

if ! command -v dialog >/dev/null 2>&1; then
    echo "Error: dialog is required" >&2
    exit 1
fi

# Use the PC ANSI terminal type so dialog displays colors.
export TERM=pcansi

# Match the 14 visible entries in Mystic BBS.
# Do not use --no-tags: dialog misinterprets the \Z color sequences.
exec dialog \
    --colors \
    --backtitle "MESSAGES" \
    --title "Message Base Listings" \
    --default-item "01" \
    --menu "n  aREA dESCRIPTION                       tOTAL     nEW yOURS    lAST pOST" \
    21 78 14 \
    "01" "\Zn\Zb\Z5localz\Zn\Zb\Z0.\Zn\Zb\Z3e\Zn\Zb\Z0-\Zn\Zb\Z3mail\Zn                       4,282       0     0  04 Sep 2026" \
    "02" "\Zn\Zb\Z3netMail \Zn\Zb(\Zn\Z3GLoBAL\Zn\Zb)\Zn                       81       0     0  11 Aug 2026" \
    "03" "\Zn\Zb\Z5n\Zn\Z5EW \Zn\Zb\Z5localz\Zn\Zb\Z0.\Zn\Zbgeneral\Zn\Zb\Z5z\Zn\ZbZ\Zn\Zb\Z5z\Zn               2,983       2     0  05 Sep 2026" \
    "04" "\Zn\Zb\Z5localz\Zn\Zb\Z0.\Zn\Zb\Z62oFB \Zn\Z6nEWZ \Zn\Zb\Z6& \Zn\Z6iNF0Z\Zn              601       0     0  21 Aug 2026" \
    "05" "\Zn\Zb\Z5localz\Zn\Zb\Z0.\Zn\ZbASCii \Znartz\Zn                      64       0     0  01 May 2026" \
    "06" "\Zn\Zb\Z5localz\Zn\Zb\Z0.\Zn\Z6door \Zn\Zb\Z5games C\Zn\Z3i\Zn\Zb\Z5TY\Zn                432       0     0  07 Aug 2026" \
    "07" "\Zn\Zb\Z5localz\Zn\Zb\Z0.\ZnANS\Zn\Zbi \Zn\Zb\Z3scene\Zn\Zb!!\Zn                   121       0     0  01 Oct 2025" \
    "08" "\Zn\Zb\Z5localz\Zn\Zb\Z0.\Zn\Zb\Z4bbses\Zn\Zb\Z1+\Zn\Zb\Z4bbs softwarez\Zn            527       0     0  14 Aug 2026" \
    "09" "\Zn\Zb\Z5localz\Zn\Zb\Z0.\Zn\Zb\Z1DOS \Zn\Z1days\Zn\Zb\Z0..\Zn1990\Zn\Zb\Z1dir/w\Zn            366       0     0  01 Sep 2026" \
    "10" "\Zn\Zb\Z5localz\Zn\Zb\Z0.\Zn\Z3linux \Zn\Zb\Z1z\Zn\Z10\Zn\Zb\Z1ne\Zn                     397       0     0  03 Aug 2026" \
    "11" "\Zn\Zb\Z5localz\Zn\Zb\Z0.FOSS\Zn\Z5/\Zn\Z3Github\Zn\Z5/\ZnOpen-Source\Zn        128       0     0  27 Aug 2026" \
    "12" "\Zn\Zb\Z5n\Zn\Z5EW \Zn\Zb\Z5localz\Zn\Zb\Z0.\Zncomputer \Zn\Zb\Z1hard\Zn\Z1WARE\Zn          137       1     0  04 Sep 2026" \
    "13" "\Zn\Zb\Z5localz\Zn\Zb\Z0.\Zn\Z1raspberry \Zn\Zb\Z1pi \Zn\Zb\Z3#TaKEoVER\Zn         226       0     0  23 Aug 2026" \
    "14" "\Zn\Zb\Z5localz\Zn\Zb\Z0.\Zn\Z6Single \Zn\Zb\Z4Boards \Zn\ZbForever\Zn!\Zn         105       0     0  22 Jun 2026" \
    "15" "\Zn\Zb\Z5n\Zn\Z5EW \Zn\Zb\Z5localz\Zn\Zb\Z0.retro computers \Zn+\Zn\Z3antiques\Zn  559       4     0  04 Sep 2026" \
    "16" "\Zn\Zb\Z5localz\Zn\Zb\Z0.\Zn\ZbhACKiNG\Zn/\Zn\Zb\Z0cODING\Zn                 397       0     0  03 Jun 2026" \
    "17" "\Zn\Zb\Z5localz\Zn\Zb\Z0.\Zn\Zb\Z0Social\ZnZzZ \Zn\ZbhACKiNG\Zn\Zn               55       0     0  25 Jul 2026" \
    "18" "\Zn\Zb\Z5localz\Zn\Zb\Z0.Technology\Zn\Z3FUTURE\Zn               449       0     0  23 May 2026" \
    "19" "\Zn\Zb\Z5n\Zn\Z5EW \Zn\Zb\Z5localz\Zn\Zb\Z0.\Zn\Zb\Z3muzik \Zn\Z3/ \Zn\Zbsoundz \Zn\Z3/ \Zn\Zb\Z3zines\Zn      87       2     0  04 Sep 2026" \
    "20" "\Zn\Zb\Z5localz\Zn\Zb\Z0.\Zn\Z5retro \Zn\Zb\Z1gaming\Zn\Zb\Z4!!\Zn                 320       0     0  01 Aug 2026" \
    "21" "\Zn\Zb\Z5localz\Zn\Zb\Z0.computer \Zn\Z3talk\Zn                  253       0     0  07 Jun 2026" \
    "22" "localz.2oFB File Requests              44       0     0  24 Jan 2026"
