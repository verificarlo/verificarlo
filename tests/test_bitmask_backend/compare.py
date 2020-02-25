#!/usr/bin/env python3

import argparse

if "__main__" == __name__:

    parser = argparse.ArgumentParser(description="Compare libinterflop_mca.so and libinterlop_bitmask.so backends outputs")
    parser.add_argument('--s-bitmask', required=True, type=float, help='Number of significant digits for the bitmask backend')
    parser.add_argument('--s-mca', required=True, type=float, help='Number of significant digits for the mca backend')
    
    args, other = parser.parse_known_args()

    if (abs(args.s_mca - args.s_bitmask) > 2):
        print("Error threshold: s_mca-s_bitmask={s}".format(s=abs(args.s_mca - args.s_bitmask)))
        exit(1)        
