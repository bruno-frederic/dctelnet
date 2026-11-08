#!/usr/bin/env python3
"""Check an Amiga bitmap font the way diskfont.library loads it.

Usage: check_amiga_font.py <Name.font> [...]

For every size listed in the .font contents file, loads the hunk file,
applies its relocations, and reads the DiskFontHeader where diskfont
looks for it: 4 bytes into the first hunk (after the moveq/rts stub; the
segment's NextSegment link is the loader's, not the file's). Fails with a
message naming the first broken field. Also checks that glyph 'A' (0x41)
has pixels, so an empty or misaligned strike bitmap fails too.
"""
import os
import struct
import sys

FCH_ID = 0x0F00
DFH_ID = 0x0F80
FPF_DESIGNED = 0x40


def load_hunks(path):
    b = open(path, 'rb').read()
    p = 0

    def L():
        nonlocal p
        v = struct.unpack('>I', b[p:p + 4])[0]
        p += 4
        return v

    if L() != 0x3F3:
        raise ValueError('not a HUNK_HEADER file')
    while L():  # resident library names
        pass
    L()
    first, last = L(), L()
    # A linker may drop trailing zero bytes from the data and keep the full
    # size here; LoadSeg() allocates this size, so pad to it below.
    sizes = [(L() & 0x3FFFFFFF) * 4 for _ in range(last - first + 1)]
    hunks, relocs = [], []
    while p < len(b):
        t = L() & 0x3FFFFFFF
        if t in (0x3E9, 0x3EA):  # CODE, DATA
            n = L()
            data = bytearray(b[p:p + n * 4])
            data += bytes(max(0, sizes[len(hunks)] - len(data)))
            hunks.append(data)
            p += n * 4
        elif t == 0x3EB:  # BSS
            hunks.append(bytearray(L() * 4))
        elif t == 0x3EC:  # RELOC32
            while True:
                n = L()
                if n == 0:
                    break
                target = L()
                relocs.append((len(hunks) - 1, target, [L() for _ in range(n)]))
        elif t == 0x3F2:  # END
            pass
        else:
            raise ValueError(f'unexpected hunk type {t:#x}')
    return hunks, relocs


def check_size(path, ysize):
    hunks, relocs = load_hunks(path)
    if len(hunks) != 1:
        raise ValueError(f'{len(hunks)} hunks, expected 1')
    h = hunks[0]
    # Relocations are offsets inside hunk 0; the base address is 0 here,
    # so a relocated pointer is simply its offset into the hunk.
    reloc_offsets = {o for _, _, offs in relocs for o in offs}
    if h[0:2] != b'\x70\xff' and h[0:2] != b'\x70\x00':
        raise ValueError(f'hunk does not start with a moveq stub: {h[:4].hex()}')
    hdr = 4
    file_id = struct.unpack('>H', h[hdr + 14:hdr + 16])[0]
    if file_id != DFH_ID:
        raise ValueError(f'dfh_FileID at offset {hdr + 14} is {file_id:#06x}, expected {DFH_ID:#06x}')
    tf = hdr + 54
    (ys, _style, flags, xs, baseline, _smear, _acc, lo, hi,
     chardata, modulo, charloc, charspace, charkern) = struct.unpack(
        '>HBBHHHHBBIHIII', h[tf + 20:tf + 52])
    if ys != ysize:
        raise ValueError(f'tf_YSize {ys}, .font contents says {ysize}')
    if not flags & FPF_DESIGNED:
        raise ValueError(f'tf_Flags {flags:#x} lacks FPF_DESIGNED')
    if baseline >= ys:
        raise ValueError(f'tf_Baseline {baseline} >= tf_YSize {ys}')
    for name, off, val in (('tf_CharData', tf + 34, chardata), ('tf_CharLoc', tf + 40, charloc)):
        if off not in reloc_offsets:
            raise ValueError(f'{name} at offset {off} is not relocated')
        if not 0 < val < len(h):
            raise ValueError(f'{name} {val:#x} outside the hunk')
    for name, off, val in (('tf_CharSpace', tf + 44, charspace), ('tf_CharKern', tf + 48, charkern)):
        if val and off not in reloc_offsets:
            raise ValueError(f'{name} at offset {off} is not relocated')
    nglyphs = hi - lo + 2  # +1 for the default glyph
    if charloc + nglyphs * 4 > len(h) or chardata + modulo * ys > len(h):
        raise ValueError('CharLoc or CharData runs past the hunk')
    if not lo <= 0x41 <= hi:
        raise ValueError(f"glyph 'A' outside tf_LoChar..tf_HiChar ({lo}..{hi})")
    bit, width = struct.unpack('>HH', h[charloc + (0x41 - lo) * 4:charloc + (0x41 - lo) * 4 + 4])
    lit = 0
    for row in range(ys):
        base = chardata + row * modulo
        for x in range(bit, bit + width):
            lit += (h[base + x // 8] >> (7 - x % 8)) & 1
    if lit == 0:
        raise ValueError("glyph 'A' is blank")
    return f'{xs}x{ys}, chars {lo}..{hi}'


def check_font(font_path):
    b = open(font_path, 'rb').read()
    fid, n = struct.unpack('>HH', b[:4])
    if fid != FCH_ID:
        raise ValueError(f'fch_FileID {fid:#06x}, expected {FCH_ID:#06x}')
    base = os.path.dirname(font_path)
    out = []
    for i in range(n):
        entry = b[4 + i * 260:4 + (i + 1) * 260]
        name = entry[:256].split(b'\0')[0].decode('latin-1')
        ysize = struct.unpack('>H', entry[256:258])[0]
        out.append(f'{name}: ' + check_size(os.path.join(base, name), ysize))
    return out


def main(argv):
    failed = False
    for font in argv[1:]:
        try:
            for line in check_font(font):
                print(f'[OK] {font} {line}')
        except (ValueError, OSError, struct.error) as e:
            print(f'[ERROR] {font}: {e}')
            failed = True
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
