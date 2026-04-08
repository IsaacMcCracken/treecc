#!/usr/bin/env bash

# Directories to scan

DIRS=("src/sea" "src/front" "src/bin")

total=0

for dir in "${DIRS[@]}"; do
if [ -d "$dir" ]; then
echo "== $dir =="

```
    # Find .c and .h files and count lines
    lines=$(find "$dir" -type f \( -name "*.c" -o -name "*.h" \) -print0 \
        | xargs -0 cat 2>/dev/null \
        | wc -l)

    echo "Lines: $lines"
    total=$((total + lines))
else
    echo "== $dir (not found) =="
fi
echo
```

done

echo "Total lines: $total"
