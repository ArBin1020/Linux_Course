import sys
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np

# ── Shared style ─────────────────────────────────────────────────
plt.rcParams.update({
    "font.family":       "sans-serif",
    "axes.spines.top":   False,
    "axes.spines.right": False,
    "axes.grid":         True,
    "grid.linestyle":    "--",
    "grid.alpha":        0.4,
})

C = {
    "ll_unk_malloc": "#e74c3c",
    "ll_kno_malloc": "#e67e22",
    "ll_unk_pool":   "#9b59b6",
    "ll_kno_pool":   "#27ae60",
    "array":         "#2980b9",
    "frag_ll":       "#c0392b",
    "cont_ll":       "#8e44ad",
}


# ── EXP 1 — Tail Append ──────────────────────────────────────────
def plot_exp1():
    try:
        df = pd.read_csv("exp1_tail_append.csv")
    except FileNotFoundError:
        print("[SKIP] exp1_tail_append.csv not found")
        return

    fig, ax = plt.subplots(figsize=(10, 6))

    ax.plot(df["N"], df["LL_Unknown_Malloc"], "o-",  color=C["ll_unk_malloc"], lw=2,
            label="LL Unknown Tail + Malloc  [O(N)]")
    ax.plot(df["N"], df["LL_Unknown_Pool"],   "v-",  color=C["ll_unk_pool"],   lw=2,
            label="LL Unknown Tail + Pool    [O(N)]")
    ax.plot(df["N"], df["LL_Known_Malloc"],   "s--", color=C["ll_kno_malloc"], lw=2,
            label="LL Known Tail + Malloc    [O(1)]")
    ax.plot(df["N"], df["Array_Append"],      "D-",  color=C["array"],         lw=2,
            label="Array Append              [amortised O(1)]")
    ax.plot(df["N"], df["LL_Known_Pool"],     "^--", color=C["ll_kno_pool"],   lw=2,
            label="LL Known Tail + Pool      [O(1)]")

    ax.set_yscale("log")
    ax.set_title("EXP 1 — Tail Append: Traversal vs. malloc vs. Amortised Cost",
                 fontsize=13, pad=12)
    ax.set_xlabel("N (elements)", fontsize=11)
    ax.set_ylabel("Avg time per element (ns)  [log scale]", fontsize=11)
    ax.legend(fontsize=9, loc="upper left")

    plt.tight_layout()
    plt.savefig("exp1_tail_append.png", dpi=150)
    plt.close()
    print("[OK] exp1_tail_append.png")


# ── EXP 2 — Worst-case Insertion ─────────────────────────────────
def plot_exp2():
    try:
        df = pd.read_csv("exp2_worst_insert.csv")
    except FileNotFoundError:
        print("[SKIP] exp2_worst_insert.csv not found")
        return

    fig, ax = plt.subplots(figsize=(10, 6))

    ax.plot(df["N"], df["Array_Front_Insert"],    "o-",  color=C["array"],       lw=2,
            label="Array Front Insert  [O(N) memmove]")
    ax.plot(df["N"], df["LL_Head_Prepend_Pool"],  "s--", color=C["ll_kno_pool"], lw=2,
            label="LL Head Prepend + Pool  [O(1) pointer swap]")

    ax.set_yscale("log")
    ax.set_title("EXP 2 — Worst-case Insertion: memmove vs. Pointer Swap",
                 fontsize=13, pad=12)
    ax.set_xlabel("N (elements)", fontsize=11)
    ax.set_ylabel("Avg time per element (ns)  [log scale]", fontsize=11)
    ax.legend(fontsize=10)

    plt.tight_layout()
    plt.savefig("exp2_worst_insert.png", dpi=150)
    plt.close()
    print("[OK] exp2_worst_insert.png")


# ── EXP 3 — Tail Latency ─────────────────────────────────────────
def plot_exp3():
    try:
        stats  = pd.read_csv("exp3_latency_stats.csv")
        series = pd.read_csv("exp3_latency_series.csv")
    except FileNotFoundError as e:
        print(f"[SKIP] {e.filename} not found")
        return

    fig, axes = plt.subplots(1, 2, figsize=(14, 6))
    fig.suptitle("EXP 3 — Tail Latency & Real-time Stability  (N = 200 000)",
                 fontsize=13, y=1.01)

    # ── Left: percentile bar chart ────────────────────────────────
    ax = axes[0]
    labels = ["P50", "P90", "P95", "P99", "P99.9", "Max"]
    cols   = ["P50_ns", "P90_ns", "P95_ns", "P99_ns", "P999_ns", "Max_ns"]
    arr_row  = stats[stats["Method"] == "Array_Append"].iloc[0]
    pool_row = stats[stats["Method"] == "LL_Known_Pool"].iloc[0]

    x   = np.arange(len(labels))
    w   = 0.35
    ax.bar(x - w/2, [arr_row[c]  for c in cols], w, color=C["array"],       label="Array Append")
    ax.bar(x + w/2, [pool_row[c] for c in cols], w, color=C["ll_kno_pool"], label="LL Known Pool")

    ax.set_yscale("log")
    ax.set_xticks(x)
    ax.set_xticklabels(labels, fontsize=10)
    ax.set_ylabel("Latency (ns)  [log scale]", fontsize=10)
    ax.set_title("Percentile Comparison", fontsize=11)
    ax.legend(fontsize=9)

    # ── Right: time-series of first SERIES_LEN insertions ────────
    ax2 = axes[1]
    idx = series["Index"]
    ax2.semilogy(idx, series["Array_ns"],  color=C["array"],       lw=0.8, alpha=0.8,
                 label="Array Append")
    ax2.semilogy(idx, series["PoolLL_ns"], color=C["ll_kno_pool"], lw=0.8, alpha=0.8,
                 label="LL Known Pool")
    ax2.set_xlabel("Insertion index (first 5 000)", fontsize=10)
    ax2.set_ylabel("Latency per insertion (ns)  [log scale]", fontsize=10)
    ax2.set_title("Per-insertion Latency Time Series", fontsize=11)
    ax2.legend(fontsize=9)

    plt.tight_layout()
    plt.savefig("exp3_latency.png", dpi=150)
    plt.close()
    print("[OK] exp3_latency.png")


# ── EXP 4 — Cache Locality ───────────────────────────────────────
def plot_exp4():
    try:
        df = pd.read_csv("exp4_cache_scan.csv")
    except FileNotFoundError:
        print("[SKIP] exp4_cache_scan.csv not found")
        return

    fig, ax = plt.subplots(figsize=(10, 6))

    ax.plot(df["N"], df["Fragmented_LL_Scan"],  "o-",  color=C["frag_ll"],     lw=2,
            label="Fragmented LL Scan   (random heap layout)")
    ax.plot(df["N"], df["Continuous_LL_Scan"],  "s--", color=C["cont_ll"],     lw=2,
            label="Continuous LL Scan   (pool, sequential layout)")
    ax.plot(df["N"], df["Array_Scan"],          "D-",  color=C["array"],       lw=2,
            label="Array Scan           (spatial locality baseline)")

    ax.set_title("EXP 4 — Sequential Scan: Cache Locality Effect",
                 fontsize=13, pad=12)
    ax.set_xlabel("N (elements)", fontsize=11)
    ax.set_ylabel("Avg time per element (ns)", fontsize=11)
    ax.legend(fontsize=10)

    plt.tight_layout()
    plt.savefig("exp4_cache_scan.png", dpi=150)
    plt.close()
    print("[OK] exp4_cache_scan.png")


# ── Entry point ───────────────────────────────────────────────────
if __name__ == "__main__":
    errors = 0
    for fn in (plot_exp1, plot_exp2, plot_exp3, plot_exp4):
        try:
            fn()
        except Exception as exc:
            print(f"[ERROR] {fn.__name__}: {exc}", file=sys.stderr)
            errors += 1
    sys.exit(errors)