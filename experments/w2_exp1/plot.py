import sys
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np

plt.rcParams.update({
    "font.family":       "sans-serif",
    "axes.spines.top":   False,
    "axes.spines.right": False,
    "axes.grid":         True,
    "grid.linestyle":    "--",
    "grid.alpha":        0.4,
})

C_MALLOC = "#e74c3c"
C_POOL   = "#27ae60"
C_NORMAL = "#2980b9"
C_SHUF   = "#e67e22"


# ── Sub-1 — Allocation Strategy ──────────────────────────────────
def plot_sub1():
    try:
        df = pd.read_csv("exp2_sub1_alloc.csv")
    except FileNotFoundError:
        print("[SKIP] exp2_sub1_alloc.csv not found")
        return

    N = df["N"].values

    fig, axes = plt.subplots(2, 1, figsize=(9, 10), sharex=True)
    fig.suptitle("EXP2-Sub1 — Allocation Strategy: per-node malloc vs Memory Pool",
                 fontsize=13, y=1.01)

    ax0 = axes[0]
    ax0.loglog(N, df["Malloc_Total_ns"] / 1e6, "o-",  color=C_MALLOC, lw=2,
               label="per-node malloc")
    ax0.loglog(N, df["Pool_Total_ns"]   / 1e6, "s--", color=C_POOL,   lw=2,
               label="Memory Pool")
    ax0.set_ylabel("Total build time (ms)  [log–log]", fontsize=11)
    ax0.set_title("Total Construction Time", fontsize=11)
    ax0.legend(fontsize=10)

    for x, ym, yp in zip(N, df["Malloc_Total_ns"] / 1e6, df["Pool_Total_ns"] / 1e6):
        ax0.annotate(f"{ym:.1f}", (x, ym), textcoords="offset points",
                     xytext=(-5, 8), fontsize=8, color=C_MALLOC)
        ax0.annotate(f"{yp:.1f}", (x, yp), textcoords="offset points",
                     xytext=(5, -12), fontsize=8, color=C_POOL)

    ax1 = axes[1]
    ax1.semilogx(N, df["Malloc_Avg_ns"], "o-",  color=C_MALLOC, lw=2,
                 label="per-node malloc")
    ax1.semilogx(N, df["Pool_Avg_ns"],   "s--", color=C_POOL,   lw=2,
                 label="Memory Pool")
    ax1.set_xlabel("N (number of nodes)  [log scale]", fontsize=11)
    ax1.set_ylabel("Avg time per node (ns)", fontsize=11)
    ax1.set_title("Per-node Average Cost", fontsize=11)
    ax1.legend(fontsize=10)

    for x, ym, yp in zip(N, df["Malloc_Avg_ns"], df["Pool_Avg_ns"]):
        ax1.annotate(f"{ym:.1f}", (x, ym), textcoords="offset points",
                     xytext=(-5, 8),  fontsize=8, color=C_MALLOC)
        ax1.annotate(f"{yp:.1f}", (x, yp), textcoords="offset points",
                     xytext=(5, -12), fontsize=8, color=C_POOL)

    ax1.set_xticks(N)
    ax1.get_xaxis().set_major_formatter(
        mticker.FuncFormatter(lambda v, _: f"{v:.0e}"))

    plt.tight_layout()
    plt.savefig("exp2_sub1_alloc.png", dpi=150, bbox_inches='tight', pad_inches=0.1)
    plt.close()
    print("[OK] exp2_sub1_alloc.png")


# ── Sub-2 — Spatial Locality ─────────────────────────────────────
def plot_sub2():
    try:
        df = pd.read_csv("exp2_sub2_locality.csv")
    except FileNotFoundError:
        print("[SKIP] exp2_sub2_locality.csv not found")
        return

    N = df["N"].values

    fig, ax = plt.subplots(figsize=(10, 6))

    ax.set_xscale("log")

    ax.semilogy(N, df["Sequential_Pool_Scan_ns"],  "o-",  color=C_NORMAL, lw=2,
                label="Sequential Pool Scan")
    ax.semilogy(N, df["Shuffled_Pool_Scan_ns"],    "s--", color=C_SHUF,   lw=2,
                label="Shuffled Pool Scan")

    for x, yn, ys in zip(N, df["Sequential_Pool_Scan_ns"], df["Shuffled_Pool_Scan_ns"]):
        ax.annotate(f"{yn:.1f}", (x, yn), textcoords="offset points",
                    xytext=(0, 8), fontsize=8, color=C_NORMAL, ha="center")
        ax.annotate(f"{ys:.1f}", (x, ys), textcoords="offset points",
                    xytext=(0, -14), fontsize=8, color=C_SHUF, ha="center")

    ratio = df["Shuffled_Pool_Scan_ns"] / df["Sequential_Pool_Scan_ns"]
    ax2 = ax.twinx()
    ax2.spines["top"].set_visible(False)
    ax2.plot(N, ratio, "D:", color="#7f8c8d", lw=1.2, markersize=5,
             label="Slowdown ratio (Shuffled / Normal)")
    ax2.set_ylabel("Slowdown ratio  (×)", fontsize=10, color="#7f8c8d")
    ax2.tick_params(axis="y", labelcolor="#7f8c8d")

    ax.set_title("EXP2-Sub2 — Spatial Locality: Cache Miss Penalty\n"
                 "Sequential vs Shuffled access within the same contiguous pool",
                 fontsize=13, pad=12)
                 
    ax.set_xlabel("N (number of nodes)  [log scale]", fontsize=11)
    ax.set_ylabel("Avg time per node (ns)  [log scale]", fontsize=11)

    ax.grid(True, which="major", linestyle="--", alpha=0.5)

    lines1, labels1 = ax.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()
    ax.legend(lines1 + lines2, labels1 + labels2, fontsize=9, loc="upper left")

    plt.tight_layout()
    plt.savefig("exp2_sub2_locality.png", dpi=150)
    plt.close()
    print("[OK] exp2_sub2_locality.png")


if __name__ == "__main__":
    errors = 0
    for fn in (plot_sub1, plot_sub2):
        try:
            fn()
        except Exception as exc:
            print(f"[ERROR] {fn.__name__}: {exc}", file=sys.stderr)
            errors += 1
    sys.exit(errors)