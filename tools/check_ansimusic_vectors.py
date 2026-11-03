#!/usr/bin/env python3
"""Checks DCTelnet's ANSI music filter against the shared vectors.

Usage: tools/check_ansimusic_vectors.py <ansimusic_dump> <vectors.json>

Runs tests/ansimusic_dump twice: the whole input in one call, and one byte
per call (a tune split over network reads must play the same). DCTelnet
gives a note as its sound (hz, gate ms) and then a silent rest (0, the rest
of the length); a rest is one silent note. Times are whole ms (truncated),
so each may differ from the vector by the file's tolerance_ms.
"""
import json
import subprocess
import sys


def expected(events):
    want = []
    for e in events:
        ms = min(int(e["ms"]), 60000)
        if e["rest"]:
            if ms:
                want.append((0, ms))
            continue
        gate = min(int(e["gate"]), 60000)
        if gate:
            want.append((e["hz"], gate))
        if ms - gate > 0:
            want.append((0, ms - gate))
    return want


def same(got, want, tol):
    return len(got) == len(want) and all(
        g[0] == w[0] and abs(g[1] - w[1]) <= tol for g, w in zip(got, want))


def main():
    dump, path = sys.argv[1], sys.argv[2]
    doc = json.load(open(path))
    vectors, tol = doc["vectors"], doc["tolerance_ms"]
    feed = "".join(v["input_hex"] + "\n" for v in vectors)
    failed = 0
    for mode in ([], ["-1"]):
        lines = subprocess.run([dump] + mode, input=feed, capture_output=True,
                               text=True, check=True).stdout.splitlines()
        for v, line in zip(vectors, lines):
            parts = line.split(" ")
            text = "" if parts[0] == "-" else parts[0]
            got = [tuple(int(x) for x in p.split(":")) for p in parts[1:]]
            want = expected(v["events"])
            if text != v["text_hex"] or not same(got, want, tol):
                failed += 1
                how = "byte by byte" if mode else "whole"
                print(f"FAIL {v['name']} ({how}): {v['note']}")
                print(f"  text want {v['text_hex'] or '-'} got {text or '-'}")
                print(f"  notes want {want[:8]}")
                print(f"  notes got  {got[:8]}")
    print(f"ansimusic vectors: {len(vectors)} x 2 modes, {failed} failed")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
