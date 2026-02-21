# DCTelnet
### A classic *𝔸miga* Telnet/BBS client with Zmodem

**DCTelnet** is a Telnet client for Commodore **Amiga** computers.
It is **easy to use**, **lightweight**, and **reliable**.

**DCTelnet** is ideal for accessing **Telnet-enabled BBS systems** on classic
**Amiga hardware**, and supports **file transfers via ZModem**.

![DCTelnet screenshot](assets/screenshot.png)

---

> **Note:** This repository provides recompiled versions of DCTelnet,
> including a **68000-compatible binary** for classic Amiga systems
> (A500, A600, A2000), as well as an **optimized 68020 build**.
>
> **DCTelnet** was originally written by **Zed**. As of January 2026, the
> DCTelnet 1.5/1.6 releases available on Aminet target 68020+ CPUs only.
>
> This new release also offers **simplified installation**, **improved
> documentation** and **bug fixes**.
>
> 🎨 **Help wanted: UI icons**
>
> To give DCTelnet a more modern look, I’m looking for help to design a new set of toolbar icons.
> If you have pixel-art or icon design skills, your help would be greatly appreciated.
> See [issue #6](https://github.com/bruno-frederic/dctelnet/issues/6) for details.
>
>
> 💬 Contact & Discussion
>
> Want to chat, give feedback, or share your ideas about DCTelnet?
> Join the conversation on the [EAB forum thread](https://eab.abime.net/showthread.php?t=121939)


---

## ✨ Features

- **68000** compatible (no 68020 required)
- **ZModem** upload & download using **XPR libraries**
- Fast and efficient design : DCTelnet access the bsdsocket API directly
- Address book with username & password entries
- Additional terminal emulations via **XEM libraries**
- User-definable screen modes & fonts
– Application can be iconified while running
- **multiple simultaneous Telnet connections**
- **No MUI, ClassAct or ReAction required**
- **No `telser.device` required**

---

## ⚙️ Requirements

- Kickstart / Workbench **v2.00 (V36)** or higher
- A TCP/IP stack: **AmiTCP**, **Miami**, UAE bsdsocket.library or compatible

---

## 📦 Installation

1. Download the DCTelnet package either from Aminet:
   [comm/tcp/DCTelnet](https://aminet.net/package/comm/tcp/DCTelnet)
   or from the [GitHub releases section](https://github.com/bruno-frederic/dctelnet/releases).

2. The DCTelnet archive includes an **Installer** script.
   When available, using the *Installer* tool is the recommended installation method.

---

### Manual installation

On older AmigaOS versions where the *Installer* tool is not present (for example, pre-3.1.4 systems), DCTelnet can be installed **manually**. In this case, copy the required files to their respective system directories:

- `DCTelnet/Devs/ibmcon.device` -> `DEVS:`
- Libraries from `DCTelnet/Libs` -> `LIBS:`
- Fonts from `DCTelnet/Fonts` -> `FONTS:`

On AmigaOS / Kickstart **2.00 & 2.02**, the default versions of
reqtools.library and xprzmodem.library fail to load.
Replace them with the following compatible versions:
- `DCTelnet/Libs-r36/reqtools.library` -> `LIBS:`
- `DCTelnet/Libs-r36/xprzmodem.library` -> `LIBS:`

---

## 📖 How to use

Please refer to the included **DCTelnet.guide** documentation for detailed
usage instructions. The guide is in **AmigaGuide** format and can be opened
with **MultiView** on AmigaOS 3.*.
On Amiga OS 2.* you’ll need the AmigaGuide tool (available on Aminet: [text/hyper/aguide34](https://aminet.net/package/text/hyper/aguide34)) and to replace the "Default Tool" on DCTelnet.Guide icon to: SYS:Utilities/Amigaguide


## 📖 How to use

Please refer to the included **DCTelnet.guide** file for detailed usage
instructions.

The documentation is provided in **AmigaGuide** format:
- On **AmigaOS 3.x**, it can be opened directly with **MultiView**.
- On **AmigaOS 2.x**, the **AmigaGuide** tool is required (available on Aminet:
  [text/hyper/aguide34](https://aminet.net/package/text/hyper/aguide34)).
  In this case, update the *Default Tool* of the `DCTelnet.guide` icon to `SYS:Utilities/AmigaGuide`.

---

## 🐞 Known Bugs


### Character display issues and font selection

If, after connecting to a BBS, some characters appear incorrect or garbled, this is usually related to the selected terminal font. Make sure the font matches the character set expected by the remote system.

Many PC-based BBSes require an **ANSI-compatible font** to properly display line art and ANSI graphics. In such cases, select the bundled **HyperANSI** font.

On **Amiga-based BBSes**, graphics are often designed for the classic **Topaz** font. This is the case, for example, on **Retro 32 BBS**.

**Important:**
After switching the terminal font, **quit DCTelnet and restart it** before reconnecting. This avoids the display issue described **below** and ensures correct rendering.


### Connection stuck after changing display settings (fonts, XEM, screen mode) or iconifying the application (fixed in v1.8)


After iconifying the application or changing certain display-related settings (such as the **terminal font**, disabling the toolbar, enabling the **XEM library**, or switching the **screen mode**), the current connection and any future connections may appear successful, but **no output is displayed**. The status remains *Connected*, while the terminal window no longer shows any data. [(issue #3)](https://github.com/bruno-frederic/dctelnet/issues/3)

This issue has existed since **DCTelnet 1.1** and occurs when used with certain
versions of "bsdsocket.library", notably UAE bsdsocket.library 4.1 (WinUAE)
and Amiberry's built-in library.

The problem happens because "ibmcon.device" incorrectly releases an internal
system signal (bit 31).

Since v1.8, DCTelnet prevents this signal from being used, avoiding the
connection freeze.

**Workaround:**
After modifying any of these settings, **quit DCTelnet and restart it**.

Greetings to **Marius** (aka Firestone on EAB forum) for his help reporting
this bug and testing DCTelnet across different configurations.

### Address Book Profile may not be saved correctly (fixed in v1.8)

This bug has been present at least since the original version 1.1 release (December 1997).

Before v1.8, changes made in the Address Book Profile were not saved
correctly when navigating between fields **with the mouse**. [(issue #4)](https://github.com/bruno-frederic/dctelnet/issues/4)

**Workaround before v1.8:** When editing Address Book Profile:
- Press **[Enter]** after typing text in each field.
- Or move to the next field using **[Tab]**, then press **[Enter]** before leaving the profile window.


### On AmigaOS / Kickstart 2.00 & 2.02 (fixed in v1.8)

The `xprzmodem.library` bundled with DCTelnet since v1.1 fails to load on these OS versions. [(issue #5)](https://github.com/bruno-frederic/dctelnet/issues/5)

Starting with v1.8, a compatible version of the library is included in the `Libs-r36/` directory.

---

## 🛠️ Build environment

**As of March 2026:**

- **IDE**: VS Code (on Windows)
- **Compiler**: **VBCC** and **GNU Make** on Windows, using my custom toolchain :
  [vbcc-bin](https://github.com/bruno-frederic/vbcc-bin/tree/Bruno_toolchain)

- The project can also be built with SAS/C v6.58 under AmigaOS 3.2

### Source code

All source code referenced below, including **DCTelnet 1.5 / 1.6** and its
required third-party libraries, is publicly available on **Aminet**.

- Based on the **DCTelnet 1.5 / 1.6** sources:
  [comm/tcp/dct16_src](https://aminet.net/package/comm/tcp/dct16_src)

The required third-party source code dependencies are also included:

- **ReqTools 2.9** from [util/libs/ReqToolsDev](https://aminet.net/package/util/libs/ReqToolsDev)
- **XPRZModem 3.1** library and sources from "comm/misc/xprz31.lha" package on Aminet CD 2
- **Xem 2.0** from [comm/term/XEM2_0](https://aminet.net/package/comm/term/XEM2_0)

---

## 🙏 Credits

- **DCTelnet** was originally written by **Zed**.
- new 68000-compatible build by **Bruno FREDERIC**
- ReqTools is Copyright © Nico François and Magnus Holmgren

---

### ⚖️ License

The original **DCTelnet** code by Zed was released without an explicit license and remains *all rights reserved*. See [LICENSE file](LICENSE) for details.

This repository provides the source for preservation and maintenance only.
