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

## VFC-VTK

The VTK postprocessing tool `postprocessing/vfc-vtk.py` takes multiple
VTK outputs generated with verificarlo and generates a single VTK set of files that
is enriched with accuracy information for each floating point `DataArray`.

For more information about `vfc-vtk.py`, please use the online help:

```bash
$ postprocess/vfc-vtk.py --help
```

## Verificarlo CI

Verificarlo CI is a tool that allows you to define, run and automatize custom
Verificarlo tests. These tests can then be visualized by serving an HTML test
report. It is designed to integrate with your Git repository (hosted on either
GitHub or GitLab) and produce new test results everytime you push modifications.
However, it can also be used manually and independently of Git. Verificarlo CI
is used through a single command line interface, `vfc_ci`, that provides
several subcommands. To get more information about `vfc_ci` :

```
vfc_ci --help
```

If you are interested in a particular subcommand :

```
vfc_ci SUBCOMMAND --help
```


### Instrument your code with ```vfc_probes```

In order to extract data from your tests, Verificarlo CI uses the `vfc_probes.h`
header, which allows you to place  "probes" on your variables so they can be
read from outside your program. Each probe stores the value of one variable,
and is identified by a combination of test and variable names. It's completely
up to you to choose these names, and the variable name of your probe can be
completely different from your actual variable name. However, when picking these
names, you should keep in mind that :

- A test/variable names combination must be unique. Registering the same probe
twice will result in an error.
- The names you choose will be used directly in the report. More specifically,
a test will be shown as a hierarchical level that can include multiple variables.

To use `vfc_probes` in your tests, simply include its header file :

```
#include <vfc_probes.h>
```

... and build your code with the the `-lvfc_probes` flag to link the library.

**Note** : If you were to compile some `vfc_probes` tests with another
compiler than Verificarlo, you would get errors because of undefined references
to some functions used by `vfc_probes`. If you ever need to write tests that
could be compiled with or without Verificarlo, you should probably wrap calls
to `vfc_probes` functions inside preprocessor conditionals.

All your probes will be stored in the `vfc_probes` structure. Here is how it
should be initialized :

```
vfc_probes probes = vfc_init_probes();
```

Below is a list of the different functions you may use to manipulate the
`vfc_probes` structure :

```
// Add a new probe. If an issue with the key is detected (forbidden characters
// or a duplicate key), an error will be thrown. (no check)
int vfc_probe(vfc_probes *probes, char *testName, char *varName, double val);

// Similar to vfc_probe, but with an optional accuracy threshold (absolute
// check).
int vfc_probe_check(vfc_probes *probes, char *testName, char *varName,
                     double val, double accuracyThreshold);

// Similar to vfc_probe, but with an optional accuracy threshold (relative
// check).
int vfc_probe_check_relative(vfc_probes *probes, char *testName, char *varName,
                              double val, double accuracyThreshold);
// Free all probes
void vfc_free_probes(vfc_probes *probes);

// Return the number of probes stored in the hashmap
unsigned int vfc_num_probes(vfc_probes *probes);

// Dump your probes for export and free them
int vfc_dump_probes(vfc_probes *probes);
```

To export your variables to a file, `vfc_probes` relies on the
`VFC_PROBES_OUTPUT`environment variable. It is automatically set when executing
your code through a Verificarlo CI test run, so you usually won't have to care
about it,  but if you were to manually execute a program that calls the
`vfc_dump_probes` function without this variable, you would be notified by a
runtime warning explaining that your probes cannot be exported.

Finally, probes can be used with an optional "check". Checks are accuracy
targets that we want to reach on test variables. If a probe is created with a
check, the tool will estimate its error and compare it to the specified
accuracy target. However, the definition of this error can vary depending on the
backend and the check types. Backends are separated in two categories, non-
deterministic (such as the MCA backend) and deterministic (such as the VPREC backend),
and checks precision can be absolute (by default) or relative. This results in four
different conditions to validate the probe, which are summed up in the following
table :

| |Absolute|Relative|
--- | --- | ---
|Non-deterministic|Standard deviation < Target | Standard deviation / \|Empirical average\| < Target
|Deterministic|\|IEEE value -  Backend value\| < Target|\|IEEE value -  Backend value\| / \|IEEE value\| < Target

By default, the standard deviation is used to estimate the error, and must be
inferior to the accuracy threshold for the probe to pass (or inferior to the
std. dev. / avg. quotient in the case of a relative check).
But in the case of a deterministic backend, the standard deviation is undefined.
Instead, the probe is computed in IEEE standard arithmetic, and this value is
used as a reference to compute the error introduced by the deterministic backend.
The status of a probe (passing/failing) can then be consulted in the report (see
the ["Visualize your test results"](#visualize-your-test-results) part).

**Fortran specific** : `vfc_probes` also comes with a Fortran interface. In
order to use it, import the `vfc_probes_f` module, as well as
[ISO_C_BINDING](https://gcc.gnu.org/onlinedocs/gfortran/ISO_005fC_005fBINDING.html).
The functions are exactly the same, except that values stored inside probes should be
of type `REAL(kind=C_DOUBLE)`. Moreover, since Fortran passes all functions arguments
by address, you don't have to worry about pointers and can pass the arguments directly.
To build the code, you should link both `libvfc_probes` (as before) and
`libvfc_probes_f`.


### Configure and run your tests

Once you've written your tests, you need to tell Verificarlo CI how they should
be executed. More precisely, this means specifying which executables to run,
with which backends, and how many times. This is done through the
`vfc_tests_config.json` file, which you should place at the root of your
project. Here is an example showcasing the expected structure of this file :

```
{
    "make_command": "make tests",
    "executables": [
        {
            "executable": "bin/test",
            "parameters" : "foo",
            "vfc_backends": [
                {
                    "name": "libinterflop_mca.so --mode=rr",
                    "repetitions": 50
                }
            ]
        }
    ]
}
```

The `parameters` field is optional and will default to an empty string when not
specified. Each executable can be run with different backends, which have
themselves different numbers of repetitions.

Moreover, the `repetitions` field can also be omitted. However, the backend will
then be considered as deterministic, which will result in a different data
processing pipeline being used. For this reason, you should only omit this
parameter when using backends that are actually deterministic (such as VPREC),
and specify it in any other case.

Note that :

- Specifying a high enough number of repetitions is important to obtain reliable
 metrics in the report (with non-deterministic backends).
- The path to the executable should be relative to the root of the project.

Once this step is complete, you are ready to execute your first test run. This
can be done by issuing the following command (once again from the root of your
project) :

```
vfc_ci test
```

This will create a `****.vfcrun.h5` file which contains statistics about the
run. Note that the raw test results are not exported by default. For more
information about what the `run` subcommand can do :  `vfc_ci test --help`.

By comparing the data contained in different run files, you will be able to
follow the evolution of the numerical accuracy of your code over the different
changes made to it. The following part explains how this process can be
automatized if your project is hosted on GitHub or GitLab.


### Generate a CI workflow

Verificarlo CI takes advantage of
[GitHub Actions](https://github.com/features/actions) and
[GitLab CI/CD](https://docs.gitlab.com/ee/ci/) to lauch test runs and save
their results everytime you push to your repository. The run files are stored
on a dedicated orphan branch named with the `vfc_ci_` prefix : if you were to
integrate Verificarlo CI into a `dev` branch, you could access your run files
on the `vfc_ci_dev` branch.

Setting this up is quite straightforward : first, checkout to the branch you
want to install Verificarlo CI on, and ensure you have a clean work tree
without any unstaged commits. Then, from the root of your repository, enter :

```
vfc_ci setup [github|gitlab]
```

... depending on where your repository is hosted. This will create (and commit)
either a `.github/workflows/vfc_ci_workflow.yml`  or a `.gitlab-ci.yml` file
on the development branch, and initialize the CI branch.

 > :warning: **GitLab specific :** First of all, if you already have a
 `.gitlab-ci.yml` on your branch, it will be overwritten by `vfc_ci setup`.
 Moreover, GitLab doesn't provide a bot account to commit on repositories as
 GitHub does.  For this reason, you will be prompted for a user whose account
 will be used to commit in the CI pipeline. Then, you  will need to create a
 [personal access token](https://docs.gitlab.com/ee/user/profile/personal_access_tokens.html)
 with `write_repository` permissions for this user. Finally, add this token as
 a `CI_PUSH_TOKEN` [CI/CD variable](https://docs.gitlab.com/ee/ci/variables/)
 on your repository.

The result of `vfc_ci setup` will be a minimal workflow running on the
[Verificarlo Docker image](https://hub.docker.com/r/verificarlo/verificarlo/)
that you can modify depending on your needs. For instance, if your code relies
on other dependencies, you can edit the workflow's configuration file to install
them or pull your own Docker image.

### Visualize your test results

Generating and accessing the HTML report is done through the `vfc_ci serve`
subcommand, which should be run from the directory containing your data files.
By default, this will start a server on port 8080 with a report containing
results from the last 100 run files. Moreover, the run files can come from
different repositories and contain completely different test variables. This
subcommand has many options to specify on which port to run the server, add a
custom logo to the report, etc... For more details :

```
vfc_ci serve --help
```

The report is split into a few main views :

- **Compare runs :** lets you select a test/variable/backend combination, and
compare the evolution of the corresponding variable over the different runs
(significant digits, distribution, average, standard deviation). Moreover, if
some probes are associated to a check, fails will appear in red on the
plots. This view is itself separated into two parts, one for the non-deterministic
backends, and one for the deterministic backends (which only has a plot to show the
probe's values).
- **Inspect runs :** lets you select a specific run. From there you can select
one factor to group by and one factor to filter by your data (between test,
variable and backend). For each group, this will create new distributions for
significants digits, standard deviation, and compute aggregated averages. This
view only works for non-deterministic backends.
- **checks table :** lets you see which probes are associated to a check, and
which ones are failing. This view is itself separated into two parts, one for the
non-deterministic backends, and one for the deterministic backends. Depending
on the backend type, data used for the probe validation will also be shown in
the table.

> [!WARNING]
> In the "Inspect runs" mode, you have 6 different selection
> possibilities. Depending on your test setup, all of these combinations might
> not make sense (combining results from different backends, especially, might
> not yield easily interpretable results). This is another factor that you should
> take into account when designing your tests.
