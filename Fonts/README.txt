Petscii.font / PetsciiLower.font
================================

Amiga bitmap fonts with the Commodore 64 character set, for DCTelnet's
PETSCII Mode. They are a free redraw of the C64 font.

  Petscii.font/8       upper case and graphics (the C64's default set)
  PetsciiLower.font/8  lower and upper case (the C64's shifted set)

Each is 16x8: the C64's 8x8 cells drawn twice as wide, so 40 columns fill
an Amiga hires line the way they fill a C64 screen. The glyphs are indexed
by the raw PETSCII byte.

Install: copy both .font files and their drawers to FONTS:, or leave them
in DCTelnet's own Fonts drawer -- DCTelnet looks there too. Without them
PETSCII Mode still works, with CP437 lookalike glyphs.

Check: python3 tools/check_amiga_font.py Fonts/Petscii.font parses a font
the way diskfont.library loads it (also run by `make` in test/).
