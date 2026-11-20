#!/bin/sh
# tools/bin2c.sh NAME FILE -- write FILE's bytes as a C array named NAME
# (const unsigned char NAME[]) on standard output. POSIX od and sed only.
#
# tools/gen_petscii_glyphs.sh makes src/petscii_glyphs.c with it.
# The build does not run it: the Windows build has no POSIX shell. The
# generated source is tracked instead, and tests/test_petscii_font fails
# when it no longer matches the assets.
set -e
[ $# -eq 2 ] || { echo "usage: $0 NAME FILE" >&2; exit 2; }
[ -r "$2" ] || { echo "$0: cannot read $2" >&2; exit 1; }
printf '/* Generated from %s by tools/bin2c.sh. Do not edit. */\n' "$2"
printf 'const unsigned char %s[] = {\n' "$1"
od -An -v -tu1 "$2" | sed -e 's/^ *//' -e '/^$/d' -e 's/  */,/g' -e 's/$/,/'
printf '};\n'
