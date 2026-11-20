# tests/check_label_gap.awk -- every gadget label in the SettingsGadgetDef
# tables of src/guis.c keeps clear of its neighbours on the design grid.
#
# The tables use the topaz 8 design grid that ComputeX/Y scale, so 8 pixels
# per label character. GadTools draws a PLACETEXT_LEFT label 4 pixels left
# of the gadget and a PLACETEXT_RIGHT label 4 pixels right of it (measured
# on the A1200: "Terminal Type" at x=516 starts at x=408). Two items whose
# rows overlap must be at least MINGAP pixels apart, label included.
#
# Usage: awk -f check_label_gap.awk ../src/guis.c

BEGIN { MINGAP = 4; intable = 0; bad = 0 }

/static const struct SettingsGadgetDef [A-Za-z_]+\[/ {
    intable = 1
    table = $0
    sub(/.*SettingsGadgetDef[ \t]+/, "", table)
    sub(/\[.*/, "", table)
    n = 0
    next
}

intable && /^};/ { check(); intable = 0; next }

intable && /^[ \t]*\{[ \t]*[0-9]/ {
    line = $0
    sub(/^[ \t]*\{/, "", line)
    split(line, f, ",")
    x = f[1] + 0; y = f[2] + 0; w = f[3] + 0; h = f[4] + 0
    label = ""
    if (match(line, /"[^"]*"/)) label = substr(line, RSTART + 1, RLENGTH - 2)
    place = "IN"
    if (line ~ /PLACETEXT_LEFT/)  place = "LEFT"
    if (line ~ /PLACETEXT_RIGHT/) place = "RIGHT"
    n++
    name[n] = (label == "" ? "(no label)" : label); ln[n] = FNR
    top[n] = y; bottom[n] = y + h
    left[n] = x; right[n] = x + w
    if (place == "LEFT")  left[n]  = x - length(label) * 8 - 4
    if (place == "RIGHT") right[n] = x + w + 4 + length(label) * 8
}

function check(   a, b, gap) {
    for (a = 1; a <= n; a++)
        for (b = 1; b <= n; b++) {
            if (a == b || left[a] > left[b]) continue
            if (top[a] >= bottom[b] || top[b] >= bottom[a]) continue
            gap = left[b] - right[a]
            if (gap < MINGAP) {
                printf "src/guis.c:%d: %s: \"%s\" ends at x=%d, \"%s\" (line %d) starts at x=%d: gap %d, need %d\n", \
                    ln[a], table, name[a], right[a], name[b], ln[b], left[b], gap, MINGAP
                bad = 1
            }
        }
}

END { exit bad }
