import sys


range = int(sys.argv[1])
precision = int(sys.argv[2])

emax = 2 ** (range - 1) - 1
emin = 1 - emax

smallest_positive_subnormal_number = 2 ** (emin - precision)
largest_subnormal_number = 2**emin * (1 - 2**-precision)
smallest_positive_normal_number = 2**emin
largest_normal_number = 2**emax * (2 - 2**-precision)
largest_number_less_than_one = 1 - 2 ** (-precision - 1)
one = 1.0
smallest_number_larger_than_one = 1 + 2 ** (-precision)

print("=" * 10)

# print("range", range)
# print("precision", precision)
# print("emax", emax)
# print("emin", emin)
# print("-" * 10)


def print_float(name, x):
    print(f"{name:<35}:", f"{float(x):.17e}", float(x).hex())


print_float("smallest_positive_subnormal_number", smallest_positive_subnormal_number)
print_float("largest_subnormal_number", largest_subnormal_number)
print_float("smallest_positive_normal_number", smallest_positive_normal_number)
print_float("largest_normal_number", largest_normal_number)
print_float("largest_number_less_than_one", largest_number_less_than_one)
print_float("one", one)
print_float("smallest_number_larger_than_one", smallest_number_larger_than_one)

print("=" * 10)
