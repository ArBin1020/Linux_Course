import sys
import os
import math
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
from matplotlib.lines import Line2D

plt.rcParams.update({
    "font.family":       "sans-serif",
    "axes.spines.top":   False,
    "axes.spines.right": False,
    "axes.grid":         True,
    "grid.linestyle":    "--",
    "grid.alpha":        0.35,
})

PALETTE = {
    "per_node_malloc": "#e74c3c",
    "memory_pool":     "#27ae60",
    "sequential_scan": "#2980b9",
    "shuffled_scan":   "#e67e22",
}
MARKER = {
    "per_node_malloc": "o",
    "memory_pool":     "s",
    "sequential_scan": "o",
    "shuffled_scan":   "s",
}
LINESTYLE = {
    "per_node_malloc": "-",
    "memory_pool":     "--",
    "sequential_scan": "-",
    "shuffled_scan":   "--",
}
LABEL = {
    "per_node_malloc": "Per-node malloc  (O(1) alloc, per-syscall overhead)",
    "memory_pool":     "Memory Pool      (single malloc, sequential page touch)",
    "sequential_scan": "Sequential Scan  (0→1→2→… link order, prefetch-friendly)",
    "shuffled_scan":   "Shuffled Scan    (random link order, cache-line thrashing)",
}

# ── subplot ────────────────────────
SUBPLOTS = [
    # (column,                          ylabel,                         log_y)
    ("cycles_per_elem",                 "Cycles / element",             False),
    ("instructions_per_elem",           "Instructions / element",       False),
    ("IPC",                             "IPC (Instr. per Cycle)",       False),
    ("LLC_miss_rate_pct",               "LLC miss rate (%)",            False),
    ("cache_references_per_elem",       "LLC references / element",     True ),
    ("cache_misses_per_elem",           "LLC misses / element",         True ),
    ("L1_dcache_load_misses_per_elem",  "L1-dcache misses / element",   True ),
    ("dTLB_load_misses_per_elem",       "dTLB misses / element",        True ),
]

# ── 正規化 ────────────────────────────────────────────────────────
def normalize(df):
    factor = df["N"] * df["reps"]
    raw = ["cycles", "instructions", "cache_references", "cache_misses",
           "L1_dcache_load_misses", "dTLB_load_misses"]
           
    for c in raw:
        if c in df.columns:
            df[c + "_per_elem"] = df[c] / factor

    # LLC Miss Rate
    df["LLC_miss_rate_pct"] = np.where(
        df["cache_references"] > 0,
        df["cache_misses"] / df["cache_references"] * 100,
        0.0,
    )
    
    # 計算 IPC (Instructions Per Cycle)
    if "cycles" in df.columns and "instructions" in df.columns:
        df["IPC"] = np.where(
            df["cycles"] > 0,
            df["instructions"] / df["cycles"],
            0.0,
        )
        
    if "instructions" in df.columns:
        df["MPKI"] = np.where(
            df["instructions"] > 0,
            df["cache_misses"] / df["instructions"] * 1000,
            0.0,
        )
        
    return df


# ── 繪製實驗 ──────────────────────────────────────────────────
def plot_experiment(df_exp, title, outfile):
    labels = df_exp["mode_label"].unique()
    Ns     = sorted(df_exp["N"].unique())

    fig, axes = plt.subplots(2, 4, figsize=(20, 8))
    fig.suptitle(title, fontsize=13, y=1.01)
    
    def fmt_val(val):
        if val >= 0.001:
            return f"{val:.3g}"
        else:
            return f"{val:.6f}".rstrip('0').rstrip('.')
            
    for ax_idx, (col, ylabel, use_log) in enumerate(SUBPLOTS):
        ax = axes.flatten()[ax_idx]
        has_data = False

        for lbl in labels:
            sub = df_exp[df_exp["mode_label"] == lbl].sort_values("N")
            if sub.empty or col not in sub.columns:
                continue
            ys = sub[col].values
            if np.all(ys == 0) and col != "IPC" and col != "LLC_miss_rate_pct":
                continue
            has_data = True

            ax.plot(
                sub["N"], ys,
                marker=MARKER[lbl],
                linestyle=LINESTYLE[lbl],
                color=PALETTE[lbl],
                linewidth=2.0,
                markersize=7,
            )
            for x, y in zip(sub["N"], ys):
                if y == 0 and col != "IPC": 
                    continue
                ax.annotate(
                    fmt_val(y),
                    (x, y),
                    textcoords="offset points",
                    xytext=(0, 8),
                    ha="center",
                    fontsize=7.5,
                    color=PALETTE[lbl],
                )

        if not has_data:
            ax.text(0.5, 0.5, "<not counted>\n(hardware unsupported or missing data)",
                    transform=ax.transAxes, ha="center", va="center",
                    color="#aaa", fontsize=9)

        ax.set_xscale("log")
        ax.set_xticks(Ns)
        ax.get_xaxis().set_major_formatter(
            mticker.FuncFormatter(
                lambda v, _: f"$10^{{{int(round(math.log10(v)))}}}$"
            )
        )
        if use_log and has_data:
            ax.set_yscale("log")

        ax.set_xlabel("N (elements)", fontsize=9)
        ax.set_ylabel(ylabel, fontsize=9)
        ax.set_title(ylabel, fontsize=10)
        ax.tick_params(labelsize=8)

    handles = [
        Line2D([0], [0],
               color=PALETTE[lbl],
               marker=MARKER[lbl],
               linestyle=LINESTYLE[lbl],
               linewidth=2, markersize=7,
               label=LABEL[lbl])
        for lbl in labels if lbl in PALETTE
    ]
    fig.legend(handles, [h.get_label() for h in handles],
               loc="lower center", ncol=1, fontsize=9,
               frameon=True, bbox_to_anchor=(0.5, -0.07))

    plt.tight_layout()
    plt.savefig(outfile, dpi=150, bbox_inches="tight")
    plt.close()
    print(f"[OK] {outfile}")


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    csv_path = args[0] if args else "perf_results.csv"

    if not os.path.exists(csv_path):
        print(f"[ERROR] 找不到 {csv_path}")
        sys.exit(1)
        
    df = pd.read_csv(csv_path)
    print(f"[INFO] 讀取 {csv_path}：{len(df)} 筆")

    df = normalize(df)

    df1 = df[df["exp"] == 1].copy()
    if not df1.empty:
        plot_experiment(
            df1,
            title="Exp1 — Allocation Strategy\n"
                  "per-node malloc  vs  Memory Pool",
            outfile="perf_exp1.png",
        )

    df2 = df[df["exp"] == 2].copy()
    if not df2.empty:
        plot_experiment(
            df2,
            title="Exp2 — Spatial Locality\n"
                  "Sequential Scan  vs  Shuffled Scan",
            outfile="perf_exp2.png",
        )


if __name__ == "__main__":
    main()