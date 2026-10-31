#!/usr/bin/env python3
"""Builds and checks the tool bar icons (build/package/DCTelnet/ToolBar/Square and Wide).

The icons DCTelnet 1.6 shipped were drawn for fixed pens: 8 colours of the
MagicWB palette. On any other palette -- a standard Workbench, DCTelnet's own
256-colour screen -- those pens hold other colours and the icons came out pink,
cyan and blue.

These icons carry their colours instead: an 8-colour image plus the tool type
DCTELNET_PALETTE=RRGGBB,... (one entry per pen of the image). DCTelnet maps
each colour to the closest pen of the screen the tool bar opens on
(src/iconpens.c, OpenToolBarWindow in src/guis.c), so the colours suit every
screen, and the symbols can be antialiased. What differs between screens is
the shape of their pixels: Square holds 24x24 symbols, Wide the same twice as
wide for modes whose pixels are twice as tall (hires without interlace, such
as 640x256), where square art looked squeezed.

The symbols are Lucide icons (ISC, tools/toolbar_icons/LICENSE), rendered with
rsvg-convert: black on grey normally, white on blue pressed, with two blend
colours each for the antialiased edges. Every byte of the icon other than the
two images and the tool types is kept.

  toolbar_icons.py build [ICONDIR...]  render the SVGs into the icons (default: both drawers)
  toolbar_icons.py check [ICONDIR...]  verify every icon
  toolbar_icons.py preview OUT.png [ICONDIR]
"""
import os
import struct
import subprocess
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
PACKAGE = os.path.join(HERE, "..", "build", "package", "DCTelnet")
ICONDIRS = [os.path.join(PACKAGE, "ToolBar", "Square"), os.path.join(PACKAGE, "ToolBar", "Wide")]
# Symbol size (width, height) in pixels per set.
SYMBOL_SIZES = {"Square": (24, 24), "Wide": (48, 24)}
SVGDIR = os.path.join(HERE, "toolbar_icons")

# Icon file -> Lucide symbol.
SYMBOLS = {"connect": "plug-zap", "disconnect": "unplug", "addressbook": "book-user",
           "information": "info", "upload": "upload", "download": "download", "quit": "power"}

# Pen 0 is the background: DCTelnet draws it with the window's background pen,
# so an icon blends into any tool bar. Pens 0-3 are the standard Workbench
# colours; 4-7 the blends of the antialiased edges.
PALETTE = [(0xAA, 0xAA, 0xAA), (0x00, 0x00, 0x00), (0xFF, 0xFF, 0xFF), (0x66, 0x88, 0xBB),
           (0x71, 0x71, 0x71), (0x38, 0x38, 0x38), (0x99, 0xB0, 0xD2), (0xCC, 0xD7, 0xE8)]
NORMAL = (0, 4, 5, 1)    # background, 1/3, 2/3, full symbol
PRESSED = (3, 6, 7, 2)
DEPTH = 3
PALETTE_TOOLTYPE = "DCTELNET_PALETTE=" + ",".join("%02X%02X%02X" % c for c in PALETTE)


HEADER = 78          # struct DiskObject as stored
DRAWERDATA = 56      # struct OldDrawerData
IMAGE = 20           # struct Image as stored


class Image:
    def __init__(self, raw, offset):
        (self.left, self.top, self.width, self.height, self.depth, self.dataptr,
         self.pick, self.onoff, self.nextptr) = struct.unpack_from(">hhhhhIBBI", raw, offset)
        self.rowbytes = (self.width + 15) // 16 * 2
        planes = bin(self.pick & ((1 << self.depth) - 1)).count("1")
        self.size = IMAGE + planes * self.height * self.rowbytes
        if offset + self.size > len(raw):
            raise ValueError("image data runs past the end of the file")

    def encode(self, pixels, depth=DEPTH):
        """This image's header with `depth` planes holding `pixels` (rows of pens)."""
        out = bytearray(struct.pack(">hhhhhIBBI", self.left, self.top, self.width, self.height,
                                    depth, self.dataptr or 1, (1 << depth) - 1, 0, self.nextptr))
        for bit in range(depth):
            for row in pixels:
                data = bytearray(self.rowbytes)
                for x, pen in enumerate(row):
                    if pen >> bit & 1:
                        data[x >> 3] |= 0x80 >> (x & 7)
                out += data
        return bytes(out)


class Icon:
    """A DiskObject parsed to its last byte (OS 1.3-3.1 layout, no NewIcon/GlowIcon)."""

    def __init__(self, raw):
        self.raw = raw
        magic, version = struct.unpack_from(">HH", raw, 0)
        if magic != 0xE310 or version != 1:
            raise ValueError("not a Workbench icon")
        self.gadget_width, self.gadget_height = struct.unpack_from(">hh", raw, 4 + 8)
        render, select = struct.unpack_from(">II", raw, 4 + 18)
        default_tool, tool_types = struct.unpack_from(">II", raw, 0x32)
        drawer, tool_window = struct.unpack_from(">II", raw, 0x42)
        offset = HEADER + (DRAWERDATA if drawer else 0)
        self.image_start = offset
        self.images = []
        for ptr in (render, select):
            if ptr:
                self.images.append(Image(raw, offset))
                offset += self.images[-1].size
        self.image_end = offset
        if default_tool:
            offset = self._string(offset)
        self.tool_types = []
        self.tool_types_start = offset
        if tool_types:
            count = struct.unpack_from(">I", raw, offset)[0] // 4 - 1
            offset += 4
            for _ in range(count):
                end = self._string(offset)
                self.tool_types.append(raw[offset + 4:end].rstrip(b"\0").decode("latin-1"))
                offset = end
        self.tool_types_end = offset
        if tool_window:
            offset = self._string(offset)
        revision = struct.unpack_from(">I", raw, 4 + 40)[0] & 0xFF   # gg_UserData
        if drawer and revision == 1:
            offset += 6      # OS 2 NewDrawerData: dd_Flags, dd_ViewModes
        self.end = offset

    def _string(self, offset):
        length = struct.unpack_from(">I", self.raw, offset)[0]
        if offset + 4 + length > len(self.raw):
            raise ValueError("string runs past the end of the file")
        return offset + 4 + length

    def rebuilt(self, normal, pressed, tool_types):
        """The icon with new images and tool types; every other byte kept."""
        images = self.images[0].encode(normal) + self.images[1].encode(pressed)
        types = struct.pack(">I", (len(tool_types) + 1) * 4)
        for t in tool_types:
            data = t.encode("latin-1") + b"\0"
            types += struct.pack(">I", len(data)) + data
        return (self.raw[:self.image_start] + images
                + self.raw[self.image_end:self.tool_types_start] + types
                + self.raw[self.tool_types_end:])


def symbol_coverage(name, size):
    """The Lucide symbol as rows of coverage 0-3 (none, 1/3, 2/3, full), size = (w, h);
    rsvg-convert stretches it when w != h."""
    sw, sh = size
    png = subprocess.run(["rsvg-convert", "-w", str(sw), "-h", str(sh),
                          os.path.join(SVGDIR, name + ".svg")], check=True, capture_output=True).stdout
    alpha = subprocess.run(["magick", "png:-", "-alpha", "extract", "-depth", "8", "gray:-"],
                           input=png, check=True, capture_output=True).stdout
    return [[(alpha[y * sw + x] * 3 + 127) // 255 for x in range(sw)] for y in range(sh)]


def compose(cover, width, height, pens):
    sh, sw = len(cover), len(cover[0])
    left, top = (width - sw) // 2, (height - sh) // 2
    return [[pens[cover[y - top][x - left]] if 0 <= y - top < sh and 0 <= x - left < sw
             else pens[0] for x in range(width)] for y in range(height)]


def build(icondir):
    for icon_name, symbol in SYMBOLS.items():
        path = os.path.join(icondir, icon_name + ".info")
        icon = Icon(open(path, "rb").read())
        if len(icon.images) != 2:
            raise ValueError(path + ": needs a normal and a pressed image")
        w, h = icon.images[0].width, icon.images[0].height
        cover = symbol_coverage(symbol, SYMBOL_SIZES[os.path.basename(os.path.normpath(icondir))])
        label = icon.tool_types[:1]
        out = icon.rebuilt(compose(cover, w, h, NORMAL), compose(cover, w, h, PRESSED),
                           label + [PALETTE_TOOLTYPE])
        open(path, "wb").write(out)


def pixels(icon, raw, image):
    offset = icon.image_start + (0 if image is icon.images[0] else icon.images[0].size) + IMAGE
    rows = [[0] * image.width for _ in range(image.height)]
    for bit in range(image.depth):
        base = offset + bit * image.height * image.rowbytes
        for y in range(image.height):
            for x in range(image.width):
                if raw[base + y * image.rowbytes + (x >> 3)] & (0x80 >> (x & 7)):
                    rows[y][x] |= 1 << bit
    return rows


def check(icondir):
    """Every tool bar icon parses to its last byte and carries the palette of its pens."""
    errors = []
    for icon_name in SYMBOLS:
        path = os.path.join(icondir, icon_name + ".info")
        try:
            icon = Icon(open(path, "rb").read())
            if icon.end != len(icon.raw):
                errors.append("%s: %d bytes after the last field" % (path, len(icon.raw) - icon.end))
            if len(icon.images) != 2:
                errors.append("%s: %d images, want normal + pressed" % (path, len(icon.images)))
            for image in icon.images:
                if image.depth != DEPTH or image.pick != (1 << DEPTH) - 1:
                    errors.append("%s: depth %d PlanePick %d, want %d and %d"
                                  % (path, image.depth, image.pick, DEPTH, (1 << DEPTH) - 1))
            if not icon.tool_types or icon.tool_types[0].startswith("DCTELNET_"):
                errors.append(path + ": no label (the tool bar shows the first tool type)")
            if PALETTE_TOOLTYPE not in icon.tool_types:
                errors.append(path + ": no DCTELNET_PALETTE tool type: its pens would draw in"
                              " whatever colours the screen has there")
        except ValueError as e:
            errors.append("%s: %s" % (path, e))
    for e in errors:
        print(e)
    print("toolbar icons: %s" % ("FAIL" if errors else "ok"))
    return not errors


def write_png(path, rows):
    height, width = len(rows), len(rows[0])
    raw = b"".join(b"\0" + bytes(c for px in row for c in px) for row in rows)

    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data))
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))


def preview(out, icondir, scale=3):
    rows = []
    gap = (40, 40, 40)
    for which in (0, 1):
        band = None
        for icon_name in SYMBOLS:
            raw = open(os.path.join(icondir, icon_name + ".info"), "rb").read()
            icon = Icon(raw)
            img = pixels(icon, raw, icon.images[which])
            cells = [[PALETTE[p] for p in row for _ in range(scale)] + [gap] * 6 for row in img]
            band = cells if band is None else [a + b for a, b in zip(band, cells)]
        for row in band:
            rows += [row] * scale
        rows.append([gap] * len(rows[-1]))
    write_png(out, rows)


def main():
    if len(sys.argv) < 2 or sys.argv[1] not in ("build", "check", "preview"):
        sys.exit(__doc__)
    if sys.argv[1] == "preview":
        preview(sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else ICONDIRS[0])
        return
    icondirs = sys.argv[2:] or ICONDIRS
    ok = True
    for icondir in icondirs:
        if sys.argv[1] == "build":
            build(icondir)
        ok = check(icondir) and ok
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
