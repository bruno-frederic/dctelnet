#!/usr/bin/env python3
"""Reference model of ANSI music and generator of the shared test vectors.

ANSI music is the BASIC PLAY language sent by a BBS between an intro
(ESC[M, ESC[N or ESC[|) and the terminator 0x0E. DCTelnet, UP-Term (vtcon),
DEViLBOX and later the AmiExpress web BBS all test against one vector file:

    /Users/spot/Code/DEViLBOX/src/lib/import/__tests__/fixtures/ansi-music/vectors.json

DEViLBOX owns and commits that file. DCTelnet keeps a byte-identical copy in
tests/ansimusic-vectors.json (tests/Makefile compares the two when both exist).

The rules (agreed 2026-11-01 between the DCTelnet, vtcon and DEViLBOX
sessions; reference = GW-BASIC PLAY, GWSTS.ASM, plus SyncTERM cterm.txt):

  Pitch      GW-BASIC NOTTAB: the octave-6 table in whole Hz, shifted right by
             (6 - octave), plus the last bit shifted out ("ADC BX,0"; that is
             round half up). O2 A = 440, O3 C = 523, O4 C = 1047.
  N n        0-84. N0 is a rest. N1 = C in O0; octave = (n-1) div 12. N34 = 440.
  Dots       Each dot multiplies the length by 1.5 (C.. = 2.25 x).
  Gate       MN 7/8 of the length, MS 3/4, ML all of it; the rest is silent.
  Limits     L 1-64, T 32-255, O 0-6 (clamped). Defaults O4 L4 T120 MN.
  Intro      ESC[N and ESC[| always start music. ESC[M starts music only when
             the next byte that is not a space starts music: B F L S (MB MF ML
             MS), N not followed by a digit (MN), a note A-G, N + digit, O P T
             < > or 0x0E. Anything else: ESC[M is Delete Line and stays text.
  End        0x0E ends the string. ESC, CAN (0x18) or SUB (0x1A) end it early:
             the command being read is played, CAN and SUB are swallowed, ESC
             starts the next escape sequence.
  Bytes      Other bytes inside a string (CR, LF, '"', unknown letters) are
             skipped; the string keeps playing.
  SMILE      A string that contains ';' is a SMILE sound effect
             (MF freq;dur;cycles;delay;variation): no notes, no text.

Durations are in milliseconds: a note of length L at tempo T lasts
240000 / (T * L) ms. Values are rounded to 3 decimals; hosts compare with a
tolerance of 1 ms (DCTelnet truncates to whole ms).

Usage:  tools/ansimusic_reference.py <fixtures dir> <out.json>
"""
import json
import sys
from pathlib import Path

NOTTAB = [4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902]
ESC, CAN, SUB, SO = 0x1B, 0x18, 0x1A, 0x0E
GATE = {"N": (7, 8), "S": (3, 4), "L": (1, 1)}
SEMITONE = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}


def hz(n):
    """Whole Hz of note number 1-84 as GW-BASIC computes it."""
    octave, semitone = divmod(n - 1, 12)
    v, shift = NOTTAB[semitone], 6 - octave
    return v if shift == 0 else (v >> shift) + ((v >> (shift - 1)) & 1)


def clamp(v, lo, hi):
    return max(lo, min(hi, v))


class Player:
    def __init__(self):
        self.tempo, self.length, self.octave, self.style = 120, 4, 4, "N"
        self.events = []

    def note(self, freq, length, dots):
        ms = 240000.0 / (self.tempo * length) * (1.5 ** dots)
        if freq == 0:
            self.events.append({"hz": 0, "ms": round(ms, 3), "gate": 0, "rest": True})
            return
        num, den = GATE[self.style]
        self.events.append({"hz": freq, "ms": round(ms, 3),
                            "gate": round(ms * num / den, 3), "rest": False})

    def run(self, cmd, num, sharp, dots):
        """One complete command: letter, its number (None: none), sharp -1/0/1, dots."""
        if cmd in SEMITONE:
            n = self.octave * 12 + SEMITONE[cmd] + sharp + 1
            length = clamp(num, 1, 64) if num else self.length
            self.note(hz(n) if 1 <= n <= 84 else 0, length, dots)
        elif cmd == "N" and num is not None and num <= 84:
            self.note(hz(num) if num else 0, self.length, dots)
        elif cmd == "P":
            self.note(0, clamp(num, 1, 64) if num else self.length, dots)
        elif cmd == "T" and num is not None:
            self.tempo = clamp(num, 32, 255)
        elif cmd == "L" and num is not None:
            self.length = clamp(num, 1, 64)
        elif cmd == "O" and num is not None:
            self.octave = clamp(num, 0, 6)


def play_string(player, body):
    """Plays one music string (the bytes between intro and terminator)."""
    if b";" in body:
        return                                  # SMILE sound effect
    cmd, num, sharp, dots = None, None, 0, 0
    after_m = False

    def finish():
        nonlocal cmd, num, sharp, dots
        if cmd:
            player.run(cmd, num, sharp, dots)
        cmd, num, sharp, dots = None, None, 0, 0

    for i, b in enumerate(body):
        c = chr(b).upper()
        if after_m:
            after_m = False
            digit_next = i + 1 < len(body) and chr(body[i + 1]).isdigit()
            if c in "LS" or (c == "N" and not digit_next):
                player.style = c
                continue
            if c in "FB":
                continue                        # foreground / background: no sound effect
        if c.isdigit() and cmd:
            num = (num or 0) * 10 + int(c)
        elif c in "#+" and cmd in SEMITONE:
            sharp = 1
        elif c == "-" and cmd in SEMITONE:
            sharp = -1
        elif c == "." and cmd:
            dots += 1
        elif c in "<>":
            finish()
            player.octave = clamp(player.octave + (1 if c == ">" else -1), 0, 6)
        elif c in "ABCDEFGNPTLO":
            finish()
            cmd = c
        elif c == "M":
            finish()
            after_m = True
        # anything else (space, CR, LF, '"', unknown letters) is skipped
    finish()


def starts_music_after_m(data, i):
    """After ESC[M (data[i] is the first byte after it): is this music?"""
    while i < len(data) and data[i] == 0x20:
        i += 1
    if i >= len(data):
        return False
    c = chr(data[i]).upper()
    if c == "N":
        return True                             # MN, or N + digit: music either way
    return c in "BFLSACDEGOPT<>" or data[i] == SO


def run(data):
    """Whole byte stream in: (text out, note events)."""
    player, out, i = Player(), bytearray(), 0
    while i < len(data):
        b = data[i]
        intro = None
        if b == ESC and data[i + 1:i + 2] == b"[" and i + 2 < len(data):
            k = data[i + 2]
            if k in b"N|":
                intro = i + 3
            elif k == ord("M") and starts_music_after_m(data, i + 3):
                intro = i + 3
                body_prefix = b"M"
        if intro is None:
            out.append(b)
            i += 1
            continue
        j = intro
        while j < len(data) and data[j] not in (SO, ESC, CAN, SUB):
            j += 1
        body = data[intro:j]
        if data[i + 2] == ord("M"):
            body = body_prefix + body           # ESC[M B... = MB...
        play_string(player, body)
        if j < len(data) and data[j] == ESC:
            i = j                               # ESC starts the next sequence
        else:
            i = j + 1                           # SO, CAN, SUB swallowed
    return bytes(out), player.events


def vector(name, data, note, **extra):
    text, events = run(data)
    v = {"name": name, "note": note, "input_hex": data.hex(), "text_hex": text.hex(),
         "events": events}
    v.update(extra)
    return v


def blocks(path):
    """The music strings of a real file: (start, end) of ESC[ .. SO."""
    d = path.read_bytes()
    found, i = [], 0
    while True:
        i = d.find(b"\x1b[M", i)
        if i < 0:
            return d, found
        j = d.find(b"\x0e", i)
        found.append((i, j + 1))
        i = j + 1


def main():
    fixtures, out = Path(sys.argv[1]), Path(sys.argv[2])
    V = []
    add = lambda *a, **k: V.append(vector(*a, **k))

    add("pitch-o2-a", b"\x1b[NO2A\x0e", "O2 A = 440 Hz (GW-BASIC); default L4 T120 MN: 500 ms, gate 437.5")
    add("pitch-o3-c", b"\x1b[NO3C\x0e", "O3 C = 523 Hz, not middle C")
    add("pitch-o4-c", b"\x1b[NO4C\x0e", "O4 C = 1047 Hz (1046.5 rounds half up)")
    add("pitch-octave-ends", b"\x1b[NO0CO6B\x0e", "O0 C = 65 Hz, O6 B = 7902 Hz")
    add("pitch-sharp-flat", b"\x1b[NO2A#B-C+\x0e", "# and + sharp, - flat")
    add("n-numbers", b"\x1b[NN34N46N1N84N0\x0e", "N34 = 440, N46 = 880, N1 = O0 C, N84 = O6 B, N0 rest")
    add("dots-compound", b"\x1b[NL4CC.C..\x0e", "dots x1.5 each: 500, 750, 1125 ms")
    add("rest-dots", b"\x1b[NP4.P8\x0e", "P with a dot: 750 ms; P8: 250 ms")
    add("articulation", b"\x1b[NMLCMSCMNC\x0e", "ML full, MS 3/4, MN 7/8")
    add("length-and-tempo", b"\x1b[NT240L8CC16\x0e", "T240 L8: 125 ms; C16: 62.5 ms")
    add("limits-clamp", b"\x1b[NT20CT300CO9CL99C\x0e", "T20 -> 32, T300 -> 255, O9 -> 6, L99 -> 64")
    add("octave-steps", b"\x1b[NO6>CO0<C\x0e", "> above O6 and < below O0 stay inside 0-6")
    add("lower-case", b"\x1b[No2a#b-\x0e", "commands in lower case (the intro letter stays upper case: ESC[n is not music)")
    add("intro-pipe", b"\x1b[|O2A\x0e", "ESC[| starts music")
    add("intro-m-music", b"\x1b[MB O3 T120 C\x0e", "ESC[M then B = MB (background), then music")
    add("intro-m-n-note", b"\x1b[MN34\x0e", "ESC[M then N + digit = note N34")
    add("intro-m-mn", b"\x1b[MNC\x0e", "ESC[M then N, no digit = MN")
    add("intro-m-ms", b"\x1b[MSC\x0e", "ESC[M then S = MS (staccato)")
    add("intro-m-space", b"\x1b[M T200C\x0e", "ESC[M, spaces, then music (!ANSIMUS.ANS form)")
    add("delete-line", b"\x1b[MHello\x1b[2Mx", "ESC[M then text = Delete Line; ESC[2M is Delete Lines")
    add("skip-cr-lf", b"\x1b[NO2A\r\nB\"C\x0e", "CR, LF and '\"' inside a string are skipped")
    add("smile-effect", b"\x1b[MF 7000;.12;200;15;-100\x0ex",
        "a string with ';' is a SMILE sound effect: no notes, no text")
    add("end-by-esc", b"\x1b[NO2AB\x1b[1mx", "ESC ends the string early: B still plays, ESC[1m stays")
    add("end-by-can", b"\x1b[NO2A\x18x", "CAN ends the string early and is swallowed")
    add("logo-ans", b"\x1b[MB O3 T120 L2CO4CO3L6BGABO4L2CO3L2CL2AL1GL2CFL4EL6CDEL4FL3DL6O2BO3CDL4EL2C\x0e",
        "Terminate ANSMUSIC.DOC LOGO.ANS: first note 523 Hz, 1000 ms")

    def real(file, index, note, **extra):
        d, found = blocks(fixtures / file)
        s, e = found[index]
        add(f"{file}#{index}", d[s:e], note, source=f"{file} bytes {s}-{e}", **extra)

    real("MUSIC_pavt199.ANS", 0, "LOGO.ANS tune written with < and >")
    real("!ANSIMUS.ANS", 0, "ESC[M space T200: intro with a space")
    real("RELIGIOU.MUS", 1, "CR LF inside the string: skipped, keeps playing")
    real("RELIGIOU.MUS", 8, "CR LF and '\"' inside the string")
    real("ANSIPL20_INTRO.ANS", 0, "MF MS T150: music, no ';'")
    real("ANSIPL20_INTRO.ANS", 1, "SMILE sound effect with ';': no notes")
    real("ANSIPL20_INTRO.ANS", 2, "SMILE with CR LF before the numbers: no notes")

    doc = {
        "about": __doc__.split("Usage:")[0].strip(),
        "generator": "DCTelnet tools/ansimusic_reference.py",
        "tolerance_ms": 1,
        "event_fields": "hz (whole Hz, 0 for a rest), ms (whole length), gate (ms of sound), rest",
        "hosts_field": "per-host notes where a host is unconfirmed or differs on purpose",
        "vectors": V,
    }
    out.write_text(json.dumps(doc, indent=1) + "\n")
    print(f"{len(V)} vectors -> {out}")


if __name__ == "__main__":
    main()
