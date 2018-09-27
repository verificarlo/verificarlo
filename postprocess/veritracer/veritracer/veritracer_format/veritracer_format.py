#!/usr/bin/env python

from collections import namedtuple

Ko = 1024
Mo = 1024**2
Go = 1024**3
size_to_read = 10*Mo
offset = 0

ValuesLine = namedtuple('ValuesLine',['format','time','hash','address','value'])

