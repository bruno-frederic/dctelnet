#!/bin/sh
# Install a host copy of the DCTelnet builder toolchain so "make host-ci"
# can build without Docker.
#
# The compiler, assembler and linker are built from pinned sources whose
# SHA-256 sums are checked below:
#   vbcc 0.9hP3, vasm 2.0f, vlink 0.18a
# These are the sources the walkero/docker4amigavbcc v2.0.0 image builds, so
# the host compiler emits the same code as the Docker "make ci".
# The vbcc config and target files, the NDK 3.2 headers and the Roadshow
# netinclude are copied once from the reference builder image, so the host
# build uses the same headers and libraries as the Docker "make ci". After
# this script has run, the Docker image can be removed.
#
# Usage: sh build/host-toolchain.sh [destination]
# Default destination: ~/Library/Caches/dctelnet-toolchain (macOS) or
# ~/.cache/dctelnet-toolchain (other systems); it must match HOST_TOOLCHAIN
# in src/Makefile.

set -eu

IMAGE=walkero/docker4amigavbcc:m68k-amigaos-2.0.0

if [ "$(uname -s)" = Darwin ]; then
	DEFAULT_DEST="$HOME/Library/Caches/dctelnet-toolchain"
else
	DEFAULT_DEST="$HOME/.cache/dctelnet-toolchain"
fi
DEST=${1:-$DEFAULT_DEST}
OPT="$DEST/opt"

VBCC_URL=http://phoenix.owl.de/tags/vbcc0_9hP3.tar.gz
VBCC_SHA=cc5571f36ff63158f31b2757c5374b636e073451e147f05575307d56572d9029
VASM_URL=http://phoenix.owl.de/tags/vasm2_0f.tar.gz
VASM_SHA=c84b2de1cbb87831795fe64a85c5d9a7002a766e3a7c30b0a2d7d5e99d878f49
VLINK_URL=http://phoenix.owl.de/tags/vlink0_18a.tar.gz
VLINK_SHA=8d151cdd30a4feb575a364e68810c2bc300fe1a7c074dbbee6fd1175a6c5bfae

WORK=$(mktemp -d "${TMPDIR:-/tmp}/dctelnet-toolchain.XXXXXX")
trap 'rm -rf "$WORK"' EXIT

fetch() { # url sha256 file [tar exclude patterns...]
	curl -fsSL -o "$WORK/$3" "$1"
	got=$(shasum -a 256 "$WORK/$3" | cut -d' ' -f1)
	if [ "$got" != "$2" ]; then
		echo "Checksum mismatch for $1: got $got, expected $2" >&2
		exit 1
	fi
	dir="$WORK/${3%.tar.gz}"
	file="$WORK/$3"
	shift 3
	mkdir "$dir"
	tar -xzf "$file" -C "$dir" "$@"
}

# 1. Config, targets and SDK from the builder image (once).
if [ ! -d "$OPT/vbcc/targets/m68k-amigaos" ] || [ ! -d "$OPT/sdk/NDK3.2" ]; then
	command -v docker >/dev/null || {
		echo "Need Docker once to copy config, targets and SDK from $IMAGE" >&2
		exit 1
	}
	mkdir -p "$OPT/vbcc"
	cid=$(docker create "$IMAGE")
	docker cp "$cid:/opt/vbcc/config" "$OPT/vbcc/"
	docker cp "$cid:/opt/vbcc/targets" "$OPT/vbcc/"
	docker cp "$cid:/opt/sdk" "$OPT/"
	docker rm "$cid" >/dev/null
fi

# 2. vbcc 0.9hP3. dtgen asks which host C type holds each target type;
#    empty answers accept its defaults, as the builder image does. Other
#    answers change the generated code (vbcc 0.9i or other dtgen answers
#    give a different DCTelnet binary).
#    The tarball stores some files of other back ends as hardlinks to
#    themselves, which tar cannot extract; m68k does not use them.
fetch "$VBCC_URL" "$VBCC_SHA" vbcc.tar.gz \
	--exclude 'vbcc/machines/6502/*' --exclude 'vbcc/machines/m65/*' \
	--exclude 'vbcc/machines/alpha/*' --exclude 'vbcc/machines/ppc/*'
(
	cd "$WORK/vbcc/vbcc"
	mkdir -p bin
	yes '' | make TARGET=m68k CC="cc -std=c9x -g -DHAVE_AOS4" bin/vc bin/vbccm68k >/dev/null
)

# 3. vasm 2.0f (m68k, Motorola syntax) and vlink 0.18a.
fetch "$VASM_URL" "$VASM_SHA" vasm.tar.gz
(cd "$WORK/vasm/vasm" && make CPU=m68k SYNTAX=mot)
fetch "$VLINK_URL" "$VLINK_SHA" vlink.tar.gz
(cd "$WORK/vlink/vlink" && mkdir -p objects && make)

mkdir -p "$OPT/vbcc/bin"
cp "$WORK/vbcc/vbcc/bin/vc" "$WORK/vbcc/vbcc/bin/vbccm68k" \
	"$WORK/vasm/vasm/vasmm68k_mot" "$WORK/vlink/vlink/vlink" "$OPT/vbcc/bin/"

echo "Host toolchain installed in $DEST"
