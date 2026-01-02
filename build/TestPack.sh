; This script removes DCTelnet from SYS: and also the libraries
;
; For tests purpose
;
; Make this script executable :
; PROTECT  protect #?.sh +SRE
;
;

set InitialDir `cd`

SET LatestPackagePath      ${InitialDir}/DCTelnet.lha
SET InstallerPackagePath   WIN:/Installer-43_3.lha


echo "------------SYS:DCTelnet drawer:"
DELETE SYS:DCTelnet.info       SYS:DCTelnet       ALL

echo "------------device:"
DELETE DEVS:ibmcon.device

echo "------------fonts:"
DELETE FONTS:HyperANSI.font    FONTS:HyperANSI    ALL
DELETE FONTS:MicroKnight.font  FONTS:MicroKnight  ALL
DELETE FONTS:P0T-NOoDLE.font   FONTS:P0T-NOoDLE   ALL

echo "------------libraries:"
DELETE LIBS:xemvt340.library
DELETE LIBS:xprftp.library
DELETE LIBS:xprzmodem.library

echo "------------RAM:DCTelnet drawer:"
DELETE RAM:DCTelnet.info       RAM:DCTelnet       ALL

echo "------------Unarchive latest package:"
lha -Qa x "$LatestPackagePath"   RAM:

echo "------------ Installer:"
WHICH >NIL: Installer
IF WARN
  ECHO "------------Installer tool not found, uncompressing:"
  lha -T -Qa x "$InstallerPackagePath"   RAM:

  CD RAM:DCTelnet
  echo "------------Launching Install"
  RAM:Installer43_3/Installer  Install_DCTelnet APPNAME DCTelnet  MINUSER NOVICE

ELSE
  ECHO "Installer tool found"
  CD RAM:DCTelnet
  Installer Install_DCTelnet APPNAME DCTelnet MINUSER NOVICE
ENDIF

echo "FIXME: Launching Installer has no effect on AmigaOS 2.00 et 2.02 with Installer-43_3"
version >NIL: exec.library 37
If warn
    echo "Kickstart < 2.04 : Manual install of devices and library then you can run DCTelnet from RAM:"
    COPY RAM:DCTelnet        SYS:DCTelnet  CLONE ALL
    COPY RAM:DCTelnet/Devs   Devs:         CLONE ALL
    COPY RAM:DCTelnet/Fonts  Fonts:        CLONE ALL
    COPY RAM:DCTelnet/Libs   Libs:         CLONE ALL
Endif



; WIP below to ease ReqTools installation on blank system:

; Test Kickstart version (bug with KS 1.0)
; 31=1.1 ; 33=1.2 ; 34=1.3 ; 36=2.00/2.02 ; 37=2.04 ; 39=3.0 ; 40=3.1; 45/46=3.1.4; 47=3.2
version >NIL: exec.library 37
If warn
    echo "Kickstart < 2.04"
;    SET ReqToolsPackagePath    WIN:/ReqTools-2.2-user.lha
Else
    echo "Kickstart 2.04+"
;    SET ReqToolsPackagePath    WIN:/ReqToolsUsr-2.9a.lha
Endif

;echo "$ReqToolsPackagePath"
;ECHO "------------Uncompressing ReqTools library installer:"
;lha -T -Qa e "$ReqToolsPackagePath"    ReqTools/libs13/ReqTools.library   RAM:
; FIXME : On KS2.0
; - 9 files fails to extract because of european characters in filenames (Español, Français…)
; - The lhA <filespec> argument is ignored! -> full archive is uncompressed.


VERSION exec.library   FULL
VERSION version.library   FULL
VERSION icon.library   FULL
VERSION workbench.library   FULL
VERSION reqtools.library   FULL


CD SYS:DCTelnet
VERSION DCTelnet   FULL
VERSION Docs/DCTelnet.guide FULL
VERSION RAM:DCTelnet/Install_DCTelnet FULL

echo "------------Launching DCTelnet:"
DCTelnet telehack.com

CD $InitialDir
