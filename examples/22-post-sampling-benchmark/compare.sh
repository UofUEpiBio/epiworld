#!/bin/sh
# Times two builds of this benchmark side by side.
#
#   ./compare.sh BASE NEW [ROUNDS] [-- benchmark options]
#
# The two binaries run interleaved (BASE, NEW, BASE, NEW, ...) so that a
# change in machine load hits both alike. Each round is a full benchmark run
# (the options after `--`, `--reps 3` by default) and reports CPU time. For
# every scenario the script prints the median of the per-round medians, the
# ratio NEW / BASE, whether the checksums agree (bit-identical runs), and flags
# a slowdown above 5%.
#
# Example:
#   c++ -std=c++17 -O2 -DNDEBUG -Depiworld_double=double main.cpp -o base   # on master
#   c++ -std=c++17 -O2 -DNDEBUG -Depiworld_double=double main.cpp -o new    # on the branch
#   ./compare.sh ./base ./new 9 -- --sizes 20000,100000

set -eu

if [ "$#" -lt 2 ]; then
    echo "usage: $0 BASE NEW [ROUNDS] [-- benchmark options]" >&2
    exit 1
fi

base=$1
new=$2
shift 2

rounds=7
if [ "$#" -gt 0 ] && [ "$1" != "--" ]; then
    rounds=$1
    shift
fi
if [ "$#" -gt 0 ] && [ "$1" = "--" ]; then
    shift
fi
if [ "$#" -eq 0 ]; then
    set -- --reps 3
fi

out=$(mktemp)
trap 'rm -f "$out"' EXIT

i=1
while [ "$i" -le "$rounds" ]; do
    for which in base new; do
        if [ "$which" = base ]; then exe=$base; else exe=$new; fi
        "$exe" "$@" | awk -v w="$which" -v r="$i" \
            'NR > 2 { print w, r, $1, $2, $3, $7 }' >> "$out"
    done
    i=$((i + 1))
done

# Columns: which round scenario n median_ms checksum
awk '
function median(v, n,    i, j, t) {
    for (i = 2; i <= n; i++) {
        t = v[i]
        for (j = i - 1; j >= 1 && v[j] > t; j--) v[j + 1] = v[j]
        v[j + 1] = t
    }
    return (n % 2) ? v[(n + 1) / 2] : (v[n / 2] + v[n / 2 + 1]) / 2
}
{
    key = $3 " " $4
    if (!(key in seen)) { seen[key] = 1; order[++nk] = key }
    c[$1, key]++
    v[$1, key, c[$1, key]] = $5
    sum[$1, key] = $6
}
END {
    printf "%-9s %8s %11s %11s %8s %-9s\n", "scenario", "n", "base_ms", "new_ms", "ratio", "checksum"
    bad = 0
    for (k = 1; k <= nk; k++) {
        key = order[k]
        for (w = 1; w <= 2; w++) {
            name = (w == 1) ? "base" : "new"
            m = c[name, key]
            for (i = 1; i <= m; i++) tmp[i] = v[name, key, i]
            med[name] = median(tmp, m)
        }
        ratio = med["new"] / med["base"]
        same = (sum["base", key] == sum["new", key]) ? "same" : "DIFFERENT"
        flag = (ratio > 1.05) ? "  <-- slower than +5%" : ""
        if (ratio > 1.05) bad = 1
        split(key, kk, " ")
        printf "%-9s %8s %11.3f %11.3f %8.3f %-9s%s\n", kk[1], kk[2], med["base"], med["new"], ratio, same, flag
    }
    exit bad
}
' "$out"
