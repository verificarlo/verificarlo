#!/usr/bin/env python3
import sys

try:
    from verificarlo.optimize.piecewise import (
        NoFeasibleScheduleError,
        PiecewiseSearchOptimizer,
    )
except ImportError:
    sys.path.insert(0, "../../src/tools")
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

print("Piecewise search regression checks passed")
