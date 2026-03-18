#!/usr/bin/env bash

set -euo pipefail

BINARY="${BINARY:-./perf_runner}"
OUTFILE="${1:-perf_results.csv}"
EVENTS="cycles,instructions,cache-references,cache-misses,L1-dcache-load-misses,dTLB-load-misses"

# ── 前置檢查 ──────────────────────────────────────────────────────
if ! command -v perf &>/dev/null; then
    echo "[ERROR] perf 未找到。請先安裝："
    echo "        sudo apt install linux-tools-common linux-tools-\$(uname -r)"
    exit 1
fi

if [ ! -x "$BINARY" ]; then
    echo "[ERROR] 找不到可執行檔 $BINARY，請先執行 make"
    exit 1
fi

# ── 解析 perf stat 輸出中指定 metric 的計數值 ─────────────────────
# 若不支援則回傳 0
extract_metric() {
    local output="$1"
    local metric="$2"
    echo "$output" | awk -v m="$metric" '
    {
        line = $0
        sub(/ *#.*$/, "", line)
        gsub(/^[ \t]+|[ \t]+$/, "", line)
        n = split(line, a, /[ \t]+/)
        if (n >= 2 && a[n] == m) {
            val = a[1]
            gsub(",", "", val)
            if (val ~ /^[0-9]+$/) { print val; exit }
            else                  { print 0;     exit }
        }
    }'
}


# ── 寫入 CSV 標頭 ─────────────────────────────────────────────────
echo "exp,N,mode,mode_label,reps,cycles,instructions,cache_references,cache_misses,L1_dcache_load_misses,dTLB_load_misses" \
    > "$OUTFILE"

# ── reps 設定 ──────────────────────────────────────────────────
get_reps() {
    local exp=$1 n=$2 mode=$3
    if   [ "$exp" -eq 1 ] && [ "$n" -eq 10000 ];       then echo 500
    elif [ "$exp" -eq 1 ] && [ "$n" -eq 100000 ];      then echo 100
    elif [ "$exp" -eq 1 ] && [ "$n" -eq 1000000 ];     then echo 20
    elif [ "$exp" -eq 1 ] && [ "$n" -eq 10000000 ];    then echo 5
    elif [ "$exp" -eq 1 ] && [ "$n" -eq 100000000 ];   then echo 3
    elif [ "$exp" -eq 2 ] && [ "$n" -eq 10000 ];       then echo 2000
    elif [ "$exp" -eq 2 ] && [ "$n" -eq 100000 ];      then echo 500
    elif [ "$exp" -eq 2 ] && [ "$n" -eq 1000000 ];     then echo 50
    elif [ "$exp" -eq 2 ] && [ "$n" -eq 10000000 ];    then echo 10
    elif [ "$exp" -eq 2 ] && [ "$n" -eq 100000000 ] && [ "$mode" -eq 0 ]; then echo 5
    elif [ "$exp" -eq 2 ] && [ "$n" -eq 100000000 ] && [ "$mode" -eq 1 ]; then echo 1
    else echo 10
    fi
}
get_mode_label() {
    local exp=$1 mode=$2
    if [ "$exp" -eq 1 ]; then
        [ "$mode" -eq 0 ] && echo "per_node_malloc" || echo "memory_pool"
    else
        [ "$mode" -eq 0 ] && echo "sequential_scan" || echo "shuffled_scan"
    fi
}

# ── 主迴圈 ────────────────────────────────────────────────────────
TOTAL=20
COUNT=0

for exp in 1 2; do
    for n in 10000 100000 1000000 10000000 100000000; do
        for mode in 0 1; do
            COUNT=$((COUNT + 1))
            reps=$(get_reps "$exp" "$n" "$mode")
            label=$(get_mode_label "$exp" "$mode")

            printf "[%2d/%d] exp=%d  N=%-12d  mode=%d (%s)  reps=%d  " \
                   "$COUNT" "$TOTAL" "$exp" "$n" "$mode" "$label" "$reps"

            START_TS=$(date +%s)

            perf_out=$(
                perf stat \
                    -e "$EVENTS" \
                    -- "$BINARY" "$exp" "$n" "$mode" "$reps" \
                    2>&1 >/dev/null
            )

            ELAPSED=$(( $(date +%s) - START_TS ))
            printf "(%ds)\n" "$ELAPSED"
            cyc=$(  extract_metric "$perf_out" "cycles")
            instr=$(extract_metric "$perf_out" "instructions")
            cref=$( extract_metric "$perf_out" "cache-references")
            cmiss=$(extract_metric "$perf_out" "cache-misses")
            l1=$(   extract_metric "$perf_out" "L1-dcache-load-misses")
            tlb=$(  extract_metric "$perf_out" "dTLB-load-misses")

            printf "        cyc=%-14d instr=%-14d cref=%-12d cmiss=%-10d l1=%-10d tlb=%d\n" \
                   "$cyc" "$instr" "$cref" "$cmiss" "$l1" "$tlb"

            echo "$exp,$n,$mode,$label,$reps,$cyc,$instr,$cref,$cmiss,$l1,$tlb" >> "$OUTFILE"
        done
    done
done

echo ""
echo "[完成] 結果已寫入 $OUTFILE"