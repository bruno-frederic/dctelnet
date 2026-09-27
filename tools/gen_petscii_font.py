#!/usr/bin/env python3
"""Generate the Petscii/PetsciiLower Amiga strike fonts from 256-glyph 8x8
bitmap blobs (glyph-major: 256 glyphs x 8 row-bytes each, indexed by raw
PETSCII byte), doubling each row horizontally to 16 pixels wide
(compensates Amiga hires' 2x horizontal pixel density vs C64's near-square
pixels). Writes Fonts/<Name>/8 via vasm + vlink."""
import os
import subprocess
import tempfile


def double_bits(b):
    """Double each bit of an 8-bit value into a 16-bit value (bit N -> bits 2N,2N+1)."""
    out = 0
    for i in range(8):
        bit = (b >> (7 - i)) & 1
        out <<= 2
        out |= (0b11 if bit else 0b00)
    return out


def build_font_asm(blob_path, out_path, font_display_name, label_prefix):
    data = open(blob_path, 'rb').read()
    assert len(data) == 2048, f"expected 2048 bytes, got {len(data)}"

    XSIZE = 16
    YSIZE = 8
    NGLYPHS = 256
    MODULO = (NGLYPHS * XSIZE) // 8  # bytes per row = 512

    # Repack glyph-major -> row-major strike bitmap, each row doubled per glyph.
    rows = [bytearray(MODULO) for _ in range(YSIZE)]
    for g in range(NGLYPHS):
        for r in range(YSIZE):
            wide = double_bits(data[g * 8 + r])
            byte_offset = (g * XSIZE) // 8
            rows[r][byte_offset] = (wide >> 8) & 0xFF
            rows[r][byte_offset + 1] = wide & 0xFF

    esc_name = font_display_name.replace("'", "''")
    name_bytes = font_display_name.encode('ascii') + b'\x00'
    pad = 32 - len(name_bytes)
    assert pad >= 0, "font name too long for MAXFONTNAME"

    lines = [
        f"; Auto-generated Amiga strike font: {font_display_name}",
        "; Glyph source: SyncTerm allfonts.c eight_by_eight Commodore table,",
        "; width-doubled 8x8 -> 16x8 for Amiga hires pixel-aspect compensation.",
        "    section text,code",
        "",
        # The hunk starts with the stub, NOT a NextSegment longword: LoadSeg()
        # puts the segment link in front of the hunk itself, and diskfont
        # reads the DiskFontHeader 4 bytes in. An extra dc.l here moved the
        # header 4 bytes late and diskfont rejected the font (FileID 0).
        f"{label_prefix}_start:",
        "    moveq #-1,d0        ; fail if ever Run by mistake",
        "    rts",
        f"{label_prefix}_dfh:",
        "    dc.l 0              ; dfh_DF.ln_Succ",
        "    dc.l 0              ; dfh_DF.ln_Pred",
        "    dc.b 12             ; dfh_DF.ln_Type = NT_FONT",
        "    dc.b 0              ; dfh_DF.ln_Pri",
        f"    dc.l {label_prefix}_name  ; dfh_DF.ln_Name",
        "    dc.w $0f80          ; dfh_FileID = DFH_ID",
        "    dc.w 0              ; dfh_Revision",
        "    dc.l 0              ; dfh_Segment",
        f"    dc.b '{esc_name}',0",
    ]
    if pad > 0:
        lines.append(f"    dcb.b {pad},0       ; pad dfh_Name to MAXFONTNAME (32)")
    lines += [
        "    ; --- TextFont dfh_TF ---",
        "    dc.l 0              ; tf_Message.mn_Node.ln_Succ",
        "    dc.l 0              ; tf_Message.mn_Node.ln_Pred",
        "    dc.b 0              ; tf_Message.mn_Node.ln_Type",
        "    dc.b 0              ; tf_Message.mn_Node.ln_Pri",
        f"    dc.l {label_prefix}_name  ; tf_Message.mn_Node.ln_Name",
        "    dc.l 0              ; tf_Message.mn_ReplyPort",
        "    dc.w 0              ; tf_Message.mn_Length",
        f"    dc.w {YSIZE}              ; tf_YSize",
        "    dc.b 0              ; tf_Style",
        "    dc.b $40            ; tf_Flags = FPF_DESIGNED",
        f"    dc.w {XSIZE}             ; tf_XSize",
        f"    dc.w {YSIZE - 1}              ; tf_Baseline",
        "    dc.w 1              ; tf_BoldSmear",
        "    dc.w 0              ; tf_Accessors",
        "    dc.b 0              ; tf_LoChar",
        "    dc.b 255            ; tf_HiChar",
        f"    dc.l {label_prefix}_chardata  ; tf_CharData",
        f"    dc.w {MODULO}             ; tf_Modulo",
        f"    dc.l {label_prefix}_charloc   ; tf_CharLoc",
        "    dc.l 0              ; tf_CharSpace (NULL = use tf_XSize for all)",
        "    dc.l 0              ; tf_CharKern  (NULL = no kerning)",
        "",
        f"{label_prefix}_name:",
        f"    dc.b '{esc_name}',0",
        "    even",
        "",
        f"{label_prefix}_chardata:",
    ]
    for r in range(YSIZE):
        row = rows[r]
        for i in range(0, MODULO, 16):
            lines.append("    dc.b " + ','.join(f"${b:02x}" for b in row[i:i + 16]))
    # CharLoc last: its final entry is non-zero, so the linker cannot trim
    # the tail of the hunk into load-time zero fill.
    lines.append("")
    lines.append(f"{label_prefix}_charloc:")
    for g in range(NGLYPHS):
        lines.append(f"    dc.w {g * XSIZE},{XSIZE}")
    # tf_CharLoc holds HiChar-LoChar+2 entries: the last one is the glyph
    # drawn for characters outside the range. Point it at the space.
    lines.append(f"    dc.w {0x20 * XSIZE},{XSIZE}  ; default glyph")

    open(out_path, 'w').write('\n'.join(lines) + '\n')


def build_font_file(blob_path, font_dir, font_name, label_prefix, work_dir):
    """Assemble and link one size file: <font_dir>/<Name>/8."""
    base = font_name[:-len('.font')]
    asm = os.path.join(work_dir, f'{base}.s')
    obj = os.path.join(work_dir, f'{base}.o')
    build_font_asm(blob_path, asm, font_name, label_prefix)
    subprocess.run(['vasmm68k_mot', '-quiet', '-Fhunk', '-o', obj, asm], check=True)
    os.makedirs(os.path.join(font_dir, base), exist_ok=True)
    subprocess.run(['vlink', '-bamigahunk', '-x', '-Bstatic', '-nostdlib', '-mrel',
                    obj, '-o', os.path.join(font_dir, base, '8')], check=True)


if __name__ == '__main__':
    here = os.path.dirname(os.path.abspath(__file__))
    fonts = os.path.join(here, '..', 'Fonts')
    with tempfile.TemporaryDirectory() as work:
        build_font_file(os.path.join(here, 'glyphs', 'c64_upper_8x8.bin'), fonts, 'Petscii.font', 'pu', work)
        build_font_file(os.path.join(here, 'glyphs', 'c64_lower_8x8.bin'), fonts, 'PetsciiLower.font', 'pl', work)
