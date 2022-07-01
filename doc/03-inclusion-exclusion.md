## Verificarlo inclusion / exclusion options

If you only wish to instrument a specific function in your program, use the
`--function` option:

```bash
   $ verificarlo-c *.c -o ./program --function=specificfunction
```

For more complex scenarios, a included-list / excluded-list mechanism is also
available through the options `--include-file INCLUSION-FILE` and
`--exclude-file EXCLUSION-FILE`.

`INCLUSION-FILE` and `EXCLUSION-FILE` are files specifying which modules and
functions should be included or excluded from Verificarlo instrumentation.
Each line has a module name followed by a function name. Both the module or
function name can be replaced by the wildcard `*`. You can also use 
the wildcard like a regex expression.
Empty lines or lines starting with `#` are ignored.

```
# include.txt
# this inclusion file will instrument f1 in main.c and util.c, and instrument
# f2 in util.c everything else will be excluded.
main f1
util f1
util f2

# exclude.txt
# this exclusion file will exclude f3 from all modules and all functions in
# module3.c
* f3
module3 *

# include.txt
# this inclusion file will instrument any function starting by g in main
# and all functions f in any module of the directory dir
main g*
dir/* *
```

Inclusion and exclusion files can be used together, in that case inclusion
takes precedence over exclusion.


