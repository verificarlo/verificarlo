import os
import sys

try:
    from verificarlo.optimize.piecewise import (
        NoFeasibleScheduleError,
        PiecewiseSearchOptimizer,
    )
except ImportError:
    script_dir = os.path.dirname(os.path.abspath(__file__))
    tools_dir = os.path.abspath(os.path.join(script_dir, "../../src/tools"))
    if tools_dir not in sys.path:
        sys.path.insert(0, tools_dir)
    from optimize.piecewise import (
        NoFeasibleScheduleError,
        PiecewiseSearchOptimizer,
    )


def run_search(strategy, eval_fn):
    return PiecewiseSearchOptimizer(
        size=1,
        min_prec=1,
        max_prec=4,
        strategy=strategy,
        eval_fn=eval_fn,
        output_file=None,
    ).run()


for strategy in ("piecewise", "forward", "backward"):
    # Precision 1 passes while the midpoint fails. A binary search would
    # discard the valid candidate; the exhaustive search must retain it.
    assert run_search(strategy, lambda schedule: schedule == [1]) == [1]

    try:
        run_search(strategy, lambda schedule: False)
    except NoFeasibleScheduleError:
        pass
    else:
        raise AssertionError(f"{strategy} accepted a schedule that never passed")

try:
    PiecewiseSearchOptimizer(size=0)
except ValueError:
    pass
else:
    raise AssertionError("PiecewiseSearchOptimizer accepted size <= 0")

try:
    PiecewiseSearchOptimizer(min_prec=50, max_prec=20)
except ValueError:
    pass
else:
    raise AssertionError("PiecewiseSearchOptimizer accepted min_prec > max_prec")

print("Piecewise search regression checks passed")
