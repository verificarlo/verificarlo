#!/usr/bin/env python3
"""
vfc_piecewise: Generic Variable Precision Search Tool
Implements top-down piecewise constant search, forward search, and backward search strategies.
Quantization and floating-point operations are performed natively by the VPREC backend in C.
Includes animation generator to visualize step-by-step search progression.
"""
import argparse
import sys
import subprocess
import os
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.animation as animation

class NoFeasibleScheduleError(RuntimeError):
    """Raised when no precision in the requested range passes evaluation."""


class PiecewiseSearchOptimizer:
    def __init__(self, size=10, min_prec=1, max_prec=53, strategy="piecewise",
                 eval_fn=None, output_file="vfc_schedule.txt",
                 animate=False, animation_file="piecewise_search.gif", fps=2):
        self.size = size
        self.min_prec = min_prec
        self.max_prec = max_prec
        self.strategy = strategy
        self.eval_fn = eval_fn
        self.output_file = output_file
        self.animate = animate
        self.animation_file = animation_file
        self.fps = fps
        self.trace = []

    def evaluate(self, schedule, start, end, candidate_p):
        if self.eval_fn is not None:
            passed = self.eval_fn(schedule)
            if self.animate:
                self.trace.append({
                    "step": len(self.trace) + 1,
                    "schedule": list(schedule),
                    "active_domain": (start, end),
                    "candidate_p": candidate_p,
                    "passed": bool(passed)
                })
            return passed
        return False

    def search_domain(self, sched, start, end, p_min=None, p_max=None):
        if p_min is None:
            p_min = self.min_prec
        if p_max is None:
            p_max = self.max_prec

        # A program-level correctness predicate is not necessarily monotone in
        # floating-point precision. Search the bounded range exhaustively so a
        # passing lower precision is not discarded after a higher one fails.
        for candidate_p in range(p_min, p_max + 1):
            test_sched = list(sched)
            for i in range(start, end):
                test_sched[i] = candidate_p
            if self.evaluate(test_sched, start, end, candidate_p):
                return candidate_p

        raise NoFeasibleScheduleError(
            f"No passing precision in [{p_min}, {p_max}] "
            f"for domain [{start}, {end})"
        )

    def search_piecewise(self):
        schedule = [self.max_prec] * self.size

        # Step 0: Find minimal uniform precision across all domains
        p_init = self.search_domain(schedule, 0, self.size, self.min_prec, self.max_prec)
        for i in range(self.size):
            schedule[i] = p_init

        # Queue of subdomains to split recursively: (start, end)
        queue = [(0, self.size)]

        while queue:
            level_size = len(queue)
            for _ in range(level_size):
                start, end = queue.pop(0)
                if end - start <= 1:
                    p_opt = self.search_domain(schedule, start, end, self.min_prec, schedule[start])
                    schedule[start] = p_opt
                    continue

                mid = (start + end) // 2
                p_left = self.search_domain(schedule, start, mid, self.min_prec, schedule[start])
                for i in range(start, mid):
                    schedule[i] = p_left

                p_right = self.search_domain(schedule, mid, end, self.min_prec, schedule[mid])
                for i in range(mid, end):
                    schedule[i] = p_right

                if mid - start > 0:
                    queue.append((start, mid))
                if end - mid > 0:
                    queue.append((mid, end))

        return schedule

    def search_forward(self):
        schedule = [self.max_prec] * self.size
        for k in range(self.size):
            schedule[k] = self.search_domain(
                schedule, k, k + 1, self.min_prec, self.max_prec
            )
        return schedule

    def search_backward(self):
        schedule = [self.max_prec] * self.size
        for k in reversed(range(self.size)):
            schedule[k] = self.search_domain(
                schedule, k, k + 1, self.min_prec, self.max_prec
            )
        return schedule

    def save(self, schedule):
        if not self.output_file:
            return
        with open(self.output_file, "w") as f:
            for p in schedule:
                f.write(f"{p}\n")

    def generate_animation(self):
        if not self.trace:
            print("No trace available for animation.")
            return

        fig, ax = plt.subplots(figsize=(7, 4.5))

        def update(frame_idx):
            ax.clear()
            entry = self.trace[frame_idx]
            sched = entry["schedule"]
            start, end = entry["active_domain"]
            passed = entry["passed"]
            step_num = entry["step"]
            cand_p = entry["candidate_p"]

            x = list(range(len(sched)))

            # Strictly fix X and Y bounds on every frame to prevent scale shifts
            ax.set_xlim(-0.5, self.size - 0.5)
            ax.set_ylim(0, 58)
            ax.set_xticks(range(self.size))

            # Sleek indigo domain highlight with subtle boundary lines
            ax.axvspan(start - 0.5, end - 0.5, color='#6366f1', alpha=0.15, zorder=1, label="Active domain")
            ax.axvline(start - 0.5, color='#4f46e5', linestyle='--', alpha=0.4, linewidth=1, zorder=2)
            ax.axvline(end - 0.5, color='#4f46e5', linestyle='--', alpha=0.4, linewidth=1, zorder=2)

            # Muted reference precision lines
            ax.axhline(y=53, color='#94a3b8', linestyle=':', alpha=0.6)
            ax.axhline(y=24, color='#94a3b8', linestyle='--', alpha=0.6)
            ax.axhline(y=11, color='#94a3b8', linestyle=':', alpha=0.6)
            ax.text(0.1, 54, "binary64 (53)", fontsize=8, color='#64748b')
            ax.text(0.1, 25, "binary32 (24)", fontsize=8, color='#64748b')
            ax.text(0.1, 12, "binary16 (11)", fontsize=8, color='#64748b')

            # Normal line plot linking points directly
            ax.plot(x, sched, marker='o', markersize=6, color='#2563eb', linewidth=2.5, label="$p_k$ schedule", zorder=3)

            # Modern status badge
            status_str = "✓ PASS" if passed else "✗ FAIL"
            status_bg = "#16a34a" if passed else "#dc2626"

            ax.set_title(f"Step {step_num}/{len(self.trace)}: [{self.strategy.upper()}] Domain [{start}, {end}) p={cand_p}", fontsize=11, fontweight='bold', pad=10)
            ax.set_xlabel("Iteration ($k$)", fontsize=10)
            ax.set_ylabel("Virtual precision ($p_k$)", fontsize=10)
            ax.grid(True, alpha=0.2, linestyle='--')

            ax.text(0.95, 0.92, status_str, transform=ax.transAxes,
                    fontsize=11, fontweight='bold', color='white',
                    bbox=dict(boxstyle="round,pad=0.4,rounding_size=0.3", fc=status_bg, ec="none"),
                    ha="right", va="top", zorder=5)

            ax.legend(loc="lower right", fontsize=8, framealpha=0.9)

        anim = animation.FuncAnimation(fig, update, frames=len(self.trace), interval=1000 // self.fps)

        if self.animation_file.endswith(".html"):
            writer = animation.HTMLWriter(fps=self.fps)
            anim.save(self.animation_file, writer=writer)
        else:
            writer = animation.PillowWriter(fps=self.fps)
            anim.save(self.animation_file, writer=writer)

        plt.close(fig)
        print(f"Saved search process animation ({len(self.trace)} steps) to: {self.animation_file}")

    def run(self):
        self.trace = []
        if self.strategy == "piecewise":
            res = self.search_piecewise()
        elif self.strategy == "forward":
            res = self.search_forward()
        elif self.strategy == "backward":
            res = self.search_backward()
        else:
            raise ValueError(f"Unknown strategy: {self.strategy}")

        self.save(res)
        if self.animate:
            self.generate_animation()
        return res

def main():
    parser = argparse.ArgumentParser(
        description="vfc_piecewise: Variable Precision Search Tool with Animation Generator"
    )
    parser.add_argument("-n", "--size", type=int, default=10, help="Number of domain intervals/iterations (default: 10)")
    parser.add_argument("-s", "--strategy", choices=["piecewise", "forward", "backward"], default="piecewise", help="Search algorithm strategy")
    parser.add_argument("--min-precision", type=int, default=1, help="Minimum significand precision (default: 1)")
    parser.add_argument("--max-precision", type=int, default=53, help="Maximum significand precision (default: 53)")
    parser.add_argument("-o", "--output-file", type=str, default="vfc_schedule.txt", help="Output file path for saving configuration")
    parser.add_argument("-r", "--run-cmd", type=str, required=True, help="Shell command to run target program under VPREC backend")
    parser.add_argument("-c", "--cmp-cmd", type=str, help="Shell command to validate output (exit 0 = success)")
    parser.add_argument("--animate", action="store_true", help="Generate animation of search precision process")
    parser.add_argument("--animation-file", type=str, default="piecewise_search.gif", help="Output animation file (.gif or .html)")
    parser.add_argument("--fps", type=int, default=2, help="Animation frames per second (default: 2)")

    args = parser.parse_args()

    def command_eval_fn(schedule):
        with open(args.output_file, "w") as f:
            for p in schedule:
                f.write(f"{p}\n")

        if not args.run_cmd:
            return False

        env = os.environ.copy()
        env["VFC_SCHEDULE_FILE"] = os.path.abspath(args.output_file)

        res = subprocess.run(args.run_cmd, shell=True, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if res.returncode != 0:
            return False

        if args.cmp_cmd:
            cmp_res = subprocess.run(args.cmp_cmd, shell=True, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            return cmp_res.returncode == 0
        return True

    opt = PiecewiseSearchOptimizer(
        size=args.size,
        min_prec=args.min_precision,
        max_prec=args.max_precision,
        strategy=args.strategy,
        eval_fn=command_eval_fn,
        output_file=args.output_file,
        animate=args.animate,
        animation_file=args.animation_file,
        fps=args.fps
    )

    try:
        result_schedule = opt.run()
    except NoFeasibleScheduleError as exc:
        parser.exit(1, f"vfc_piecewise: error: {exc}\n")
    print(f"Optimal precision schedule ({args.strategy}):", result_schedule)

if __name__ == "__main__":
    main()
