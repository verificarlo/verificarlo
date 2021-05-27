## Postprocessing

Verificarlo includes postprocessing tools to compute floating point accuracy
information from a set of verificarlo generated outputs. 

## Find Optimal precision with vfc_precexp and vfc_report

The ``vfc_precexp`` script tries to minimize the precision for each function call 
of a code compiled with verificarlo. To use vfc_precexp, you need to compile your code
with the ``--inst-func`` and to write two scripts as for the [delta-debug](#pinpointing-errors-with-delta-debug):

   - A first script ``exrun <output_dir>``, is responsible for running the
     program and writing its output inside the ``<output_dir>`` folder. 
     During exploration the code can be broken so please think about adding 
     a timeout when you are executing your code.

   - A second script ``excmp <reference_dir> <current_dir>``, takes as
     parameter two folders including respectively the outputs from a reference
     run and from the current run. The `exmp` script must return
     success when the deviation between the two runs is acceptable, and fail if
     the deviation is unacceptable.

Once those two scripts are written you can launch the execution with:
```
./vfc_precexp exrun excmp
```
If you're looking for the optimal precision for a set of functions:
```
./vfc_precexp exrun excmp function_1 function_2 ...
```

At the end of the exploration, a ``vfc_exp_data`` directory is created and you can 
find explorations results in ``ArgumentsResults.csv `` for arguments only , 
``OperationsResults.csv`` for internal operations only, ``AllArgsResults.csv`` 
and ``AllOpsResults.csv`` for arguments and internal operations.

You can produce an html report with the ``vfc_report`` script, this will produce a 
``vfc_precexp_report.html`` in the ``vfc_exp_data`` directory.

## Unstable branch detection

It is possible to use Verificarlo to detect branches that are unstable due to
numerical errors.  To detect unstable branches we rely on
[llvm-cov](https://llvm.org/docs/CommandGuide/llvm-cov.html) coverage reports.
To activate coverage mode in verificarlo, you should use the `--coverage` flag.

This is demonstrated in
[`tests/test_unstable_branches/`](https://github.com/verificarlo/verificarlo/tree/master/tests/test_unstable_branches/);
the idea first introduced by [verrou](https://github.com/edf-hpc/verrou), is to
compare coverage reports between multiple IEEE executions and multiple MCA
executions.

Branches that are unstable only under MCA noise, are identified as numerically
unstable.

# VFC-VTK

The VTK postprocessing tool `postprocessing/vfc-vtk.py` takes multiple
VTK outputs generated with verificarlo and generates a single VTK set of files that
is enriched with accuracy information for each floating point `DataArray`.

For more information about `vfc-vtk.py`, please use the online help:

```bash
$ postprocess/vfc-vtk.py --help
```


