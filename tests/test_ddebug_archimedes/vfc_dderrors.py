#!/usr/bin/env python3
# 
# vfc_dderrors.py <binary> <dd_output_file>
# returns a quickfix compatible output, for pinpointing delta-debug errors.

import sys
import re
import subprocess

if len(sys.argv) != 3:
    print('usage {} <binary> <dd_output_file>'.format(sys.argv[0]),
            file=sys.stderr)
    sys.exit(1)

binary_path = sys.argv[1]
exclude_path = sys.argv[2]

output = []

with open(exclude_path, 'r') as exclude_file:
    for n, inputline in enumerate(exclude_file):
        m = re.match(r'([0-9a-fx]+):(.*?) at (.*?):([0-9]+)', inputline)
        if not m:
            print('syntax error at {}:{}'.format(exclude_path, n+1), file=sys.stderr)
            sys.exit(1)
        else:
            address = int(m.group(1), 16)
            function = m.group(2)
            filename = m.group(3)
            line = int(m.group(4))
            command = ['objdump', '-dC', '--start-address=%s' % address,
                    '--stop-address=%s' % (address+5), binary_path]
            objdump = subprocess.check_output(command).decode()
            objdump = objdump.split('\n')[-2]
            im = re.match(r'.*? <_(.*?)>', objdump)
            if not im:
                print('error parsing assembly for {}:{} in {}'.format(
                    exclude_path, n+1, binary_path), file=sys.stderr)
                sys.exit(1)
            else:
                instruction = im.group(1)

            output.append((filename, line,
                           '{}:{}: error: {} belongs to ddebug set'.format(
                               filename, line, instruction)))

    output.sort()
    print("\n".join([m for _, _, m in output]))
