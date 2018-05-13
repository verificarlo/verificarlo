#!/usr/bin/env python

import argparse
import sys

import veritracer_plot
import veritracer_analyzer
import veritracer_jitter

import cProfile

veritracer_plugins = {}

parser = argparse.ArgumentParser(description="veritracer command line", prog="veritracer")
subparsers = parser.add_subparsers(help="Call veritracer modules", dest="mode")

veritracer_plot.init_module(subparsers, veritracer_plugins)
veritracer_analyzer.init_module(subparsers, veritracer_plugins)
veritracer_jitter.init_module(subparsers, veritracer_plugins)

if __name__ == "__main__":
    args = parser.parse_args()
    status = 0
    # cProfile.run('veritracer_plugins[args.mode](args)')
    if not veritracer_plugins[args.mode](args):
        status = 1
    sys.exit(status)
