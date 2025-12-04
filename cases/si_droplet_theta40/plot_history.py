#!/usr/bin/env python3
"""
读取 output/si_droplet_theta40/history.csv 并绘制 R/H/beta/KE/Diss/Wcap 随时间变化的子图（共用时间轴）。
"""
import csv
import sys
from pathlib import Path

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
except ImportError as exc:
    sys.stderr.write(f"matplotlib not available ({exc}); skip plotting.\n")
    sys.exit(0)


def main():
    root = Path(__file__).resolve().parent.parent.parent
    case = "si_droplet_theta40"
    hist = root / "output" / case / "history.csv"
    if not hist.exists():
        sys.stderr.write(f"history file not found: {hist}\n")
        return

    t, R, H, beta, KE, Diss, Wcap = [], [], [], [], [], [], []
    with hist.open() as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                t.append(float(row["t"]))
                R.append(float(row["R"]))
                H.append(float(row["H"]))
                beta.append(float(row["beta"]))
                KE.append(float(row["KE"]))
                Diss.append(float(row["Diss"]))
                Wcap.append(float(row["Wcap"]))
            except Exception:
                continue

    if len(t) < 2:
        sys.stderr.write("Not enough data to plot history\n")
        return

    labels = [("R (m)", R), ("H (m)", H), (r"$\beta$", beta),
              ("KE (J)", KE), ("Diss (J)", Diss), ("Wcap (J)", Wcap)]

    fig, axes = plt.subplots(nrows=len(labels), ncols=1, sharex=True, figsize=(6, 10))
    for ax, (label, series) in zip(axes, labels):
        ax.plot(t, series, lw=1.5)
        ax.set_ylabel(label)
        ax.grid(True, ls="--", alpha=0.5)
    axes[-1].set_xlabel("Time (s)")
    fig.tight_layout()

    out_png = root / "output" / case / "history_plot.png"
    fig.savefig(out_png, dpi=200)
    print(f"Saved plot to {out_png}")


if __name__ == "__main__":
    main()
