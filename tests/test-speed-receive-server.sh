#!/bin/bash

# Display test-speed.ans in blocks (simulates sending data via telnet)
# Usage: ./test-speed-receive-server.sh [buffer_size]
#   buffer_size: number of bytes per block (default: 10)

BUFFER_SIZE="${1:-10}"
FILE="test-speed.ans"

if [ ! -f "$FILE" ]; then
    echo "Error: $FILE not found" >&2
    exit 1
fi

if ! printf '%d' "$BUFFER_SIZE" 2>/dev/null | grep -q '^[0-9]*$' || [ "$BUFFER_SIZE" -le 0 ]; then
    echo "Error: buffer_size must be a positive integer" >&2
    exit 1
fi

offset=0
while true; do
    chunk=$(dd if="$FILE" bs=1 skip="$offset" count="$BUFFER_SIZE" 2>/dev/null)
    if [ -z "$chunk" ]; then
        break
    fi
    echo -n "$chunk"
    offset=$((offset + BUFFER_SIZE))
done
