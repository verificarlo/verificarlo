# VPREC Function Instrumentation

## Function instrumentation

Verificarlo can instrument the functions of a code by using the flag `--inst-func`; this flag is required by some backends which can operate at the level of function calls. For example, VPREC takes advantage this feature to explore custom precision at function granularity. This is presented in the next section.

```bash
   $ verificarlo-c main.c -o main --inst-func
```

Verificarlo will instrument every call-site, inputs and outputs can be modified by the backends. Each call-site is represented by an ID composed of his file, the name of the called function and the line of the call. This feature is complementary to the standard instrumentation of arithmetic operations inside the functions made by verificarlo and can be used together to study the floating point precision of a code more precisely.


## VPREC custom precision

With the function instrumentation, VPREC backend allows you to customize the precision at the function granularity and the precision of every arguments of a called function. First, the code should be compiled with the function instrumentation flag.

```bash
   $ verificarlo-c main.c -o main --inst-func
```

Then you can execute your code with the VPREC backend and set a precision profiling output file with the `--prec-output-file` parameter. 

```bash
   $ export VFC_BACKENDS="libinterflop_vprec.so --prec-output-file=output.txt" ./main
```

In this file you can see information on the functions and on floating point arguments with the following structure: 

```
file/parent/name/line/id  isInt isLib useFloat  useDouble precision_binary64 range_binary64  precision_binary32  range_binary32  nb_inputs nb_outputs  nb_calls
input:  arg_name  type  precision   range   smallest_value  biggest_value
...
input:  arg_name  type  precision   range   smallest_value  biggest_value
output: arg_name  type  precision   range   smallest_value  biggest_value
```

Where:
  - `file` is the file which contains the call of the function
  - `parent` is the function name where the call is executed
  - `name` is the name of the called function
  - `line` is the line where the function is called in the source code
  - `id` is a unique integer used for identification
  - `isInt` is a boolean which indicates if the function comes from an intrinsic
  - `isLib` is a boolean which indicates if the function comes from a library
  - `useFloat` is a boolean which indicates if the function uses 32 bit float
  - `useDouble` is a boolean which indicates if the function uses 64 bit float
  - `precision_binary64` controls the length of the mantissa for a 64 bit float for operations inside the function
  - `range_binary64` controls the length of the exponent for a 64 bit float for operations inside the function
  - `precision_binary32` control the length of the mantissa for a 32 bit float for operations inside the function
  - `range_binary32` control the length of the exponent for a 64 bit float for operations inside the function
  - `nb_inputs` is the number of floating point inputs intercepted
  - `nb_outputs` is the number of floating point outputs intercepted
  - `nb_calls` is the number of calls to the function from this call-site
  - `type` is the type of the argument (0 = float and 1 = double)
  - `arg_name` is the name of the argument or his number
  - `precision` is the length of the mantissa for this argument
  - `range` is the length of the exponent for this argument
  - `smallest_value` is the smallest value of this argument during execution
  - `biggest_value` is the biggest value of this argument during execution

Only floating point arguments are managed by vprec, so for example this code:

```c
double print(int n, double a, double b) {
  for (int i = 0; i < n; i++)
    printf("%lf %lf\n", a, b);
  return a + b;
}

...

double res = print(1, 2.0, 3.5);
```
will produce this profile file:

```
main.c/main/print/11/11 0 0 0 1 52  11  23  8 2 1 1
# a
input:  a 1 52  11  2 2
# b
input:  b 1 52  11  3 4
# res
output: return_value  1 52  11  5 6
```

You are now able to customize the length of the mantissa/exponent for each argument but also to set the internal precision for floating point operations in a function compiled with verificarlo.
To do that you can change the desired field in the profile file and use it as an input with the following command:

```bash
   $ export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=output.txt --instrument=all" ./main
```

The `--instrument` parameters set the behavior of the backend:
  - `arguments` apply given precisions to arguments only
  - `operations` apply given precisions to arithmetic operations inside the function only
  - `all` apply given given precisions to arithmetic operations inside the function and to arguments
  - `none` (default) does not apply any custom precision

The program is now executed with the given configuration.

You can produce a log file to summarize the vprec backend activity during the execution by giving the name of the file with the `--prec-log-file` parameter. The produced file will have the following structure:

```
  enter in file/parent/name/line/id  precision_binary64 range_binary64  precision_binary32  range_binary32
   - file/parent/name/line/id  input type  arg_name value_before_rounding  ->    value_after_rounding  (precision, range)
   - file/parent/name/line/id  input type  arg_name value_before_rounding  ->    value_after_rounding  (precision, range)

   ...

  exit of file/parent/name/line/id precision_binary64 range_binary64  precision_binary32  range_binary32
   - file/parent/name/line/id  output  type  return_value  value_before_rounding  ->    value_after_rounding  (precision, range)
```


