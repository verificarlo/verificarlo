#!/usr/bin/env python3
"""
Unified Plotting & Animation Generator for Newton-Raphson VPREC Experiment.
Dynamically reads experiment log files and schedule files to produce:
1. One Static Plot: plot_piecewise.pdf / plot_piecewise.png
2. Animation 2: plot_piecewise_animation.gif (Convergence process animation)
"""
import sys
import os
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.animation as animation

def read_results_file(filename):
    iters, x_vals, rel_errs, s10s, s2s = [], [], [], [], []
    if not os.path.exists(filename):
        return [], [], [], [], []
    with open(filename, 'r') as f:
        for line in f:
            if line.startswith("#") or not line.strip():
                continue
            parts = line.strip().split()
            iters.append(int(parts[0]))
            x_vals.append(float(parts[1]))
            rel_errs.append(float(parts[2]))
            s10s.append(float(parts[3]))
            s2s.append(float(parts[4]))
    return iters, x_vals, rel_errs, s10s, s2s

def read_schedule_file(filename, default_val=53, size=10):
    if not os.path.exists(filename):
        return [default_val] * size
    sched = []
    with open(filename, 'r') as f:
        for line in f:
            if line.strip():
                try:
                    val = int(line.strip().split(":")[0]) if ":" in line else int(line.strip())
                    sched.append(val)
                except ValueError:
                    pass
    if not sched:
        return [default_val] * size
    while len(sched) < size:
        sched.append(default_val)
    return sched[:size]

def plot_piecewise(ieee_file, vprec_5_file, vprec_10_file, vprec_15_file,
                   sched_5_file, sched_10_file, sched_15_file,
                   output_pdf="plot_piecewise.pdf", output_png="plot_piecewise.png"):
    k_ieee, x_ieee, err_ieee, s10_ieee, s2_ieee = read_results_file(ieee_file)
    k_5, x_5, err_5, s10_5, s2_5 = read_results_file(vprec_5_file)
    k_10, x_10, err_10, s10_10, s2_10 = read_results_file(vprec_10_file)
    k_15, x_15, err_15, s10_15, s2_15 = read_results_file(vprec_15_file)

    max_len = max(len(k_ieee), len(k_5), len(k_10), len(k_15), 10)
    p_5 = read_schedule_file(sched_5_file, size=max_len)
    p_10 = read_schedule_file(sched_10_file, size=max_len)
    p_15 = read_schedule_file(sched_15_file, size=max_len)

    # Theoretical quadratic convergence s^2
    s2_theory = [1.0 * (1.6 ** i) for i in range(max_len)]

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(9.5, 4.0), dpi=300)

    # ------------------ Left Subplot: VPREC Solution ------------------
    k_range = list(range(max_len))
    ax1.plot(k_range, p_5[:max_len],  marker='+', markersize=8, color='tab:blue',   linewidth=2.0, label='$p_k = 10^{-5}$')
    ax1.plot(k_range, p_10[:max_len], marker='+', markersize=8, color='tab:orange', linewidth=2.0, label='$p_k = 10^{-10}$')
    ax1.plot(k_range, p_15[:max_len], marker='+', markersize=8, color='tab:green',  linewidth=2.0, label='$p_k = 10^{-15}$')

    # Format threshold line at y=24
    ax1.axhline(y=24, color='black', linewidth=1.8, zorder=2)
    ax1.text(0.1, 25.5, 'binary64', fontsize=9.5, color='black')
    ax1.text(0.1, 20.5, 'binary32', fontsize=9.5, color='black')

    ax1.set_title("VPREC Solution", fontsize=12, pad=8)
    ax1.set_xlabel("iteration ($k$)", fontsize=10)
    ax1.set_ylabel("virtual precision ($p_k$)", fontsize=10)
    ax1.set_xticks(range(max_len))
    ax1.set_yticks([0, 10, 20, 24, 30, 40, 50, 53])
    ax1.set_ylim(-1, 56)
    ax1.grid(True, alpha=0.3, linestyle='-')
    ax1.legend(loc='upper left', fontsize=9, framealpha=0.9)

    # ------------------ Right Subplot: Convergence Speed ------------------
    if s2_theory:
        ax2.plot(k_range, s2_theory[:max_len], marker='x', markersize=8, color='red', linestyle='-.', linewidth=2.0, label='$s^2$')
    if s2_ieee:
        ax2.plot(k_ieee[:max_len], s2_ieee[:max_len], marker='o', markersize=5, color='tab:green', linestyle='--', linewidth=2.0, label='$s^2_k$ IEEE')
    if s2_5:
        ax2.plot(k_5[:max_len], s2_5[:max_len], marker='+', markersize=8, color='tab:blue', linestyle='-', linewidth=2.0, label='$s^2_k$ VPREC $= 10^{-5}$')
    if s2_10:
        ax2.plot(k_10[:max_len], s2_10[:max_len], marker='+', markersize=8, color='tab:orange', linestyle='-', linewidth=2.0, label='$s^2_k$ VPREC $= 10^{-10}$')
    if s2_15:
        ax2.plot(k_15[:max_len], s2_15[:max_len], marker='+', markersize=8, color='tab:green', linestyle='-', linewidth=2.0, label='$s^2_k$ VPREC $= 10^{-15}$')

    ax2.set_title("Convergence Speed", fontsize=12, pad=8)
    ax2.set_xlabel("iteration ($k$)", fontsize=10)
    ax2.set_ylabel("significant binary digits ($s^2_k$)", fontsize=10)
    ax2.set_xticks(range(max_len))
    ax2.set_yticks([0, 10, 20, 24, 30, 40, 50, 53])
    ax2.set_ylim(-1, 56)
    ax2.grid(True, alpha=0.3, linestyle='-')
    ax2.legend(loc='upper left', fontsize=8.5, framealpha=0.9)

    fig.suptitle("Piecewise", fontsize=14, y=0.98)
    plt.tight_layout()
    fig.subplots_adjust(top=0.88)

    plt.savefig(output_pdf, bbox_inches='tight')
    plt.savefig(output_png, bbox_inches='tight')
    print(f"Saved dynamic static figure: {output_pdf} and {output_png}")

def animate_plot_piecewise(ieee_file, vprec_5_file, vprec_10_file, vprec_15_file,
                            sched_5_file, sched_10_file, sched_15_file,
                            output_gif="plot_piecewise_animation.gif", fps=2):
    k_ieee, x_ieee, err_ieee, s10_ieee, s2_ieee = read_results_file(ieee_file)
    k_5, x_5, err_5, s10_5, s2_5 = read_results_file(vprec_5_file)
    k_10, x_10, err_10, s10_10, s2_10 = read_results_file(vprec_10_file)
    k_15, x_15, err_15, s10_15, s2_15 = read_results_file(vprec_15_file)

    max_len = max(len(k_ieee), len(k_5), len(k_10), len(k_15), 10)
    p_5 = read_schedule_file(sched_5_file, size=max_len)
    p_10 = read_schedule_file(sched_10_file, size=max_len)
    p_15 = read_schedule_file(sched_15_file, size=max_len)

    s2_theory = [1.0 * (1.6 ** i) for i in range(max_len)]

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(9.5, 4.0), dpi=150)

    def update(frame_idx):
        ax1.clear()
        ax2.clear()

        k_sub = list(range(frame_idx + 1))

        # Left plot: Progressive precision schedule
        ax1.set_xlim(-0.5, max_len - 0.5)
        ax1.set_ylim(-1, 56)
        ax1.set_xticks(range(max_len))
        ax1.set_yticks([0, 10, 20, 24, 30, 40, 50, 53])
        ax1.grid(True, alpha=0.3)

        ax1.axhline(y=24, color='black', linewidth=1.5)
        ax1.text(0.1, 25.5, 'binary64', fontsize=9)
        ax1.text(0.1, 20.5, 'binary32', fontsize=9)

        ax1.plot(k_sub, p_5[:frame_idx + 1], marker='+', markersize=8, color='tab:blue', linewidth=2.0, label='$p_k = 10^{-5}$')
        ax1.plot(k_sub, p_10[:frame_idx + 1], marker='+', markersize=8, color='tab:orange', linewidth=2.0, label='$p_k = 10^{-10}$')
        ax1.plot(k_sub, p_15[:frame_idx + 1], marker='+', markersize=8, color='tab:green', linewidth=2.0, label='$p_k = 10^{-15}$')

        ax1.set_title(f"VPREC Solution (k=0..{frame_idx})", fontsize=11, fontweight='bold')
        ax1.set_xlabel("iteration ($k$)", fontsize=10)
        ax1.set_ylabel("virtual precision ($p_k$)", fontsize=10)
        ax1.legend(loc='upper left', fontsize=8.5)

        # Right plot: Progressive convergence speed
        ax2.set_xlim(-0.5, max_len - 0.5)
        ax2.set_ylim(-1, 56)
        ax2.set_xticks(range(max_len))
        ax2.set_yticks([0, 10, 20, 24, 30, 40, 50, 53])
        ax2.grid(True, alpha=0.3)

        if s2_theory:
            ax2.plot(k_sub, s2_theory[:frame_idx + 1], marker='x', markersize=8, color='red', linestyle='-.', linewidth=2.0, label='$s^2$')
        if s2_ieee:
            ax2.plot(k_ieee[:frame_idx + 1], s2_ieee[:frame_idx + 1], marker='o', markersize=5, color='tab:green', linestyle='--', linewidth=2.0, label='$s^2_k$ IEEE')
        if s2_5:
            ax2.plot(k_5[:frame_idx + 1], s2_5[:frame_idx + 1], marker='+', markersize=8, color='tab:blue', linestyle='-', linewidth=2.0, label='$s^2_k$ VPREC $= 10^{-5}$')
        if s2_10:
            ax2.plot(k_10[:frame_idx + 1], s2_10[:frame_idx + 1], marker='+', markersize=8, color='tab:orange', linestyle='-', linewidth=2.0, label='$s^2_k$ VPREC $= 10^{-10}$')
        if s2_15:
            ax2.plot(k_15[:frame_idx + 1], s2_15[:frame_idx + 1], marker='+', markersize=8, color='tab:green', linestyle='-', linewidth=2.0, label='$s^2_k$ VPREC $= 10^{-15}$')

        ax2.set_title(f"Convergence Speed (k=0..{frame_idx})", fontsize=11, fontweight='bold')
        ax2.set_xlabel("iteration ($k$)", fontsize=10)
        ax2.set_ylabel("significant binary digits ($s^2_k$)", fontsize=10)
        ax2.legend(loc='upper left', fontsize=8)

        fig.suptitle("Piecewise", fontsize=13, y=0.98)
        plt.tight_layout()
        fig.subplots_adjust(top=0.88)

    anim = animation.FuncAnimation(fig, update, frames=max_len, interval=1000 // fps)
    writer = animation.PillowWriter(fps=fps)
    anim.save(output_gif, writer=writer)
    plt.close(fig)
    print(f"Saved convergence animation: {output_gif}")

if __name__ == "__main__":
    ieee_file = sys.argv[1] if len(sys.argv) > 1 else "ieee.out"
    vprec_5_file = sys.argv[2] if len(sys.argv) > 2 else "vprec_1e-5.out"
    vprec_10_file = sys.argv[3] if len(sys.argv) > 3 else "vprec_1e-10.out"
    vprec_15_file = sys.argv[4] if len(sys.argv) > 4 else "vprec_1e-15.out"

    sched_5_file = "schedule_1e-5.txt"
    sched_10_file = "schedule_1e-10.txt"
    sched_15_file = "schedule_1e-15.txt"

    plot_piecewise(ieee_file, vprec_5_file, vprec_10_file, vprec_15_file, sched_5_file, sched_10_file, sched_15_file)
    animate_plot_piecewise(ieee_file, vprec_5_file, vprec_10_file, vprec_15_file, sched_5_file, sched_10_file, sched_15_file)
