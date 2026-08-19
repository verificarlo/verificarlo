#!/usr/bin/env python3
import os
import sys
import subprocess

try:
    from verificarlo.optimize.piecewise import PiecewiseSearchOptimizer
except ImportError:
    script_dir = os.path.dirname(os.path.abspath(__file__))
    tools_dir = os.path.abspath(os.path.join(script_dir, "../../src/tools"))
    if tools_dir not in sys.path:
        sys.path.insert(0, tools_dir)
    from optimize.piecewise import PiecewiseSearchOptimizer

N_ITERS = 10

def newton_vprec_eval_fn(schedule):
    """
    Evaluates Newton-Raphson precision schedule using the VPREC backend in C.
    Writes candidate schedule to vfc_schedule.txt and runs the VPREC-instrumented C binary.
    """
    sched_file = "vfc_schedule.txt"
    with open(sched_file, "w") as f:
        for p in schedule:
            f.write(f"{p}\n")

    env = os.environ.copy()
    env["VFC_BACKENDS"] = "libinterflop_vprec.so --mode=ob"
    env["VFC_SCHEDULE_FILE"] = os.path.abspath(sched_file)

    # Execute C binary compiled with Verificarlo
    res = subprocess.run("./newton 1", shell=True, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return res.returncode == 0

if __name__ == "__main__":
    print("Running Piecewise Constant Search (with Animation Generator)...")
    opt_pw = PiecewiseSearchOptimizer(
        size=N_ITERS, min_prec=1, max_prec=53, strategy="piecewise",
        eval_fn=newton_vprec_eval_fn, output_file="vfc_schedule.txt",
        animate=True, animation_file="newton_search.gif", fps=2
    )
    p_piecewise = opt_pw.run()
    print("Piecewise schedule p_k:", p_piecewise)

    print("Running Forward Search...")
    opt_fw = PiecewiseSearchOptimizer(size=N_ITERS, min_prec=1, max_prec=53, strategy="forward", eval_fn=newton_vprec_eval_fn, output_file="vfc_schedule.txt")
    p_forward = opt_fw.run()
    print("Forward schedule p_k:  ", p_forward)

    print("Running Backward Search...")
    opt_bw = PiecewiseSearchOptimizer(size=N_ITERS, min_prec=1, max_prec=53, strategy="backward", eval_fn=newton_vprec_eval_fn, output_file="vfc_schedule.txt")
    p_backward = opt_bw.run()
    print("Backward schedule p_k: ", p_backward)
