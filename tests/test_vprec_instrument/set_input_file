#!/usr/bin/env python3
import sys
import os


def readlines(args, f):
    current_function = ""
    to_write = ""

    for line in f:

        functions_list = args['functions_list']
        precision_double = args['precision_double']
        precision_float = args['precision_float']
        range_double = args['range_double']
        range_float = args['range_float']

        fields = line.split()

        if len(fields) == 12:
            current_function = fields[0].split(
                sep="/")[1] + "/" + fields[0].split(sep="/")[2]

            # replace the fields
            if current_function in functions_list:
                fields[precision_double[1]] = str(precision_double[0])
                fields[range_double[1]] = str(range_double[0])
                fields[precision_float[1]] = str(precision_float[0])
                fields[range_float[1]] = str(range_float[0])
        elif len(fields) == 7:
            if current_function in functions_list:
                if fields[2] == '0':
                    # replace the fields
                    fields[precision_float[2]] = str(precision_float[0])
                    fields[range_float[2]] = str(range_float[0])
                elif fields[2] == '1':
                    # replace the fields
                    fields[precision_double[2]] = str(precision_double[0])
                    fields[range_double[2]] = str(range_double[0])
                else:
                    print("Unknow Argument type")
                    sys.exit(0)

        elif len(fields) != 0:
            print("Wrong input file structure")
            sys.exit(0)

        to_write += "\t".join(fields) + "\n"

    return to_write


def read(args):
    input_filename = args['input_filename']

    to_write = ''
    with open(input_filename, 'r', encoding='utf-8') as fi:
        to_write = readlines(args, fi)

        return to_write


def write(args, to_write):
    input_filename = args['input_filename']
    with open(input_filename, 'w', encoding='utf-8') as fo:
        # save modifications
        fo.write(to_write)


def parse_args():
    if len(sys.argv) < 6:
        print(("./set_input_file "
               "[input file name] [precision mantissa double] "
               "[precision exponent double] [precision mantissa float] "
               "[precision exponent float] functions ..."))
        sys.exit(0)

    # name of the input file
    input_filename = sys.argv[1]

    # precision to use for the mantissa of double
    # and corresponding field numbers
    precision_double = [int(sys.argv[2]), 5, 3]
    # precision to use for the exponent of double
    # and corresponding field numbers
    range_double = [int(sys.argv[3]), 6, 4]
    # precision to use for the mantissa of float
    # and corresponding field numbers
    precision_float = [int(sys.argv[4]), 7, 3]
    # precision to use for the exponent of float
    # and corresponding field numbers
    range_float = [int(sys.argv[5]), 8, 4]
    # list of functions to modify
    functions_list = sys.argv[6:]

    return dict(input_filename=input_filename,
                precision_double=precision_double,
                range_double=range_double,
                precision_float=precision_float,
                range_float=range_float,
                functions_list=functions_list)


def main():
    args = parse_args()

    # test if the input file exists
    if not os.path.exists(args['input_filename']):
        print("Input file not found")
        sys.exit(0)

    to_write = read(args)
    write(args, to_write)


if '__main__' == __name__:
    main()
