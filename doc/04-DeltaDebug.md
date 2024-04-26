## Pinpointing errors with Delta-Debug

[Delta-Debug (DD)](https://www.st.cs.uni-saarland.de/dd/) is a generic
bug-reduction method that allows to efficiently find a minimal set of
conditions that trigger a bug. In this case, we are going to consider the set
of floating-point instructions in the program. We are using DD to find a
minimal set of instructions responsible for the possible output instabilities
and numerical bugs.

By testing instruction sub-sets and their complement, DD is able to find
smaller failing sets step by step. DD stops when it finds a failing set where
it cannot remove any instruction. We call such a minimal set _ddmin_.  The
Delta-Debug implementation for stochastic arithmetic we use here has been
developed in the [verrou](https://github.com/edf-hpc/verrou) project.

To use delta-debug, we need to write two scripts:

   - A first script ``ddRun <output_dir>``, is responsible for running the
     program and writing its output inside the ``<output_dir>`` folder.

   - A second script ``ddCmp <reference_dir> <current_dir>``, takes as
     parameter two folders including respectively the outputs from a reference
     run and from the current run. The `ddCmp` script must return
     success when the deviation between the two runs is acceptable, and fail if
     the deviation is unacceptable.

To decide if a given set is unstable, DD will repeat the experiment by running
the program five times (the number of times can be changed by setting the
environment variable ``INTERFLOP_DD_NRUNS``).

``ddRun`` and ``ddCmp`` depend on the user's application and the error
tolerance of the application domain; therefore it is hard to provide a generic
script that fits all cases. That is why we require the user to manually write
these scripts.  Once the scripts are written, the Delta-Debug session is
launched using the following command:

```bash
$ VFC_BACKENDS="libinterflop_mca.so -p 53 -m mca" vfc_ddebug ddRun ddCmp
```

The `VFC_BACKENDS` variable selects the noise model among the available
backends. Delta-debug can be used with any backend that simulates numerical
noise (`mca`, `cancellation`, `bitmask`, ...) .  Here as an
example, we use the MCA backend with a precision of 53 in full mca mode.
`vfc_ddebug` is the delta-debug orchestration script.

`vfc_ddebug` will test instruction sub-sets. Each time an irreductible `ddmin`
set is found it is signaled to the user and asigned a number `ddmin0`,
`ddmin1`, ...., `ddminX`. The faulty instructions of `ddminX` set are stored in
the `dd.line/ddminX/dd.line.include` (these are the instructions that were
instrumented with the noise backend during the run).

The union of the _"culprit"_ instructions can also be found in
`dd.line/rddmin-cmp/dd.line.exclude`.

> [!TIP]
> A full example demonstrating delta-debug usage can be found in the
> [tutorial](https://github.com/verificarlo/verificarlo/wiki/Tutorials) and in
> the `tests/test_ddebug_archimedes`.

As an example, in the `test_ddebug_archimedes`, two ddmin sets are found:

```
$ cat dd.line/ddmin0/dd.line.include
0x000000000040136c: archimedes at archimedes.c:16
$ cat dd.line/ddmin1/dd.line.include
0x0000000000401399: archimedes at archimedes.c:17
```

indicating that the two instructions at lines `archimedes.c:16` and
`archimedes.c:17` are responsible for the numerical instability. The first
number indicates the exact assembly instruction address.

> [!TIP]
> It is possible to highlight faulty instructions inside your code editor by
> using a script such as `tests/test_ddebug_archimedes/vfc_dderrors.py`, which
> returns a [quickfix](http://vimdoc.sourceforge.net/htmldoc/quickfix.html)
> compatible output with the union of _ddmin_ instructions.



