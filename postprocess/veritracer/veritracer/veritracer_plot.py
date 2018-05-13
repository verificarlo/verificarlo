#!/usr/bin/env python

import csv
import sys
import argparse
import numpy as np
import math
import os
import matplotlib.pyplot as plt
import matplotlib.cm as cm
from collections import deque, namedtuple
from itertools import cycle

marker_size_default = 6
marker_size = marker_size_default
alpha = 1.0

header = "hash,type,time,max,min,median,mean,std,significant_digit_number"
header_csv = header.split(',')
time_start = 0

# sdn : significant digits number
ValueTuple = namedtuple('ValueTuple',['sdn','mean','std','max','time'])

def init_module(subparsers, veritracer_plugins):
    veritracer_plugins["plot"] = run
    plot_parser = subparsers.add_parser("plot", help="Plot significant number of variables over time")
    plot_parser.add_argument('-f','--csv-filename', type=str, action='store', required=True, 
                        help="Filename of the CSV which contains variables info")
    
    plot_parser.add_argument('-v','--variables', type=int, nargs='*', action='store', metavar='',
                        help="Hash of variables to plot. Available in the locationInfo.map file")

    plot_parser.add_argument('-o','--output', action="store", type=str, metavar='',
                        help="save the figure in a PDF file")

    plot_parser.add_argument('-bt','--backtrace', action="store", type=str, metavar='',
                        help="file containing the backtrace")

    plot_parser.add_argument('--location-info-map', action='store', type=str, metavar='',
                        help="location info file")
    
    plot_parser.add_argument('--no-show', action="store_true", default=False,
                        help="not show the figure")

    plot_parser.add_argument('--mean', action="store_true", 
                        help="plot mean of values")

    plot_parser.add_argument('--std', action="store_true",
                        help="plot standard deviation of values")

    plot_parser.add_argument('--transparency', type=float, action='store', metavar='', 
                        help="No transparency for plot")
        
    plot_parser.add_argument('--invocation-mode', action='store_true',
                        help="Set time as one by one")

    plot_parser.add_argument('--normalize-time', action='store_true',
                        help="Start time at 0")

    plot_parser.add_argument('--scientific-mode', action='store_true',
                        help='Set scientific mode for x-axis')

    plot_parser.add_argument('--base', action='store', type=int, default=10, 
                        help="base for computing the number of significand digits")
        

def get_key(row):
    return row['hash']

def get_value(row):
    try:        
        value = ValueTuple(
            sdn = row['significant_digit_number'],
            mean = row['mean'],
            std = row['std'],
            max = row['max'],
            time = int(row['time'])
        )
            
    except Exception as e:
        print e
        print row
        exit(1)
    return value

def is_valid_row(variables_set, row):
    return int(row['hash']) in variables_set
    
def read_csv(args):

    variables_to_plot = args.variables
    
    if not os.path.isfile(args.csv_filename):
        print "Inexisting file: %s" % args.csv_filename
        exit(1)
        
    csv_file = open(args.csv_filename, 'rb')

    first_line = next(csv_file)
    # Must have same fields, not necessary in the same order
    header_file = map(str.strip,first_line.split(','))
    has_header = sorted(header_file) == sorted(header_csv)
    
    if not has_header:        
        dictReader = csv.DictReader(csv_file, fieldnames=header_csv)
        print "CSV header not found, default header ",dictReader.fieldnames
    else:
        csv_file.seek(0)
        dictReader = csv.DictReader(csv_file)
    
    values_dict = dict()
    get_values = values_dict.get

    if args.variables != None:
        dictReaderCleaned = filter(lambda row: is_valid_row(variables_to_plot,row), dictReader)
    else:
        dictReaderCleaned = dictReader

    if args.normalize_time:
        global time_start
        first_row = dictReaderCleaned[0]
        time_start = int(first_row['time'])

    for row in dictReaderCleaned:
        key = get_key(row)
        value = get_value(row)
        try:
            values_dict[key].append(value)
        except KeyError:
            values_dict[key] = deque([value])
            
    return values_dict
    
# Return [...,['bt_i','hash_value_i','i'],...]
def get_backtrace(args):
    if not args.backtrace:
        return 

    backtrace_filename = args.backtrace
    backtrace_file = open(backtrace_filename, 'r')

    return map(str.split, backtrace_file)

def toRGB(color):
    return map(lambda x : int(x*255), color)

# Splitting between positive and negative values
# for plotting with log scale.
def split_mean(time, mean):

    ziptm = zip(time, mean)
    pos = filter(lambda (t,m) : float(m) > 0, ziptm)
    neg = filter(lambda (t,m) : float(m) <= 0, ziptm)
    neg = map(lambda (t,m) : (t,-float(m)), neg)
    return pos,neg

def set_font(size):
    font = {'family' : 'normal',
            'weight' : 'bold',
            'size'   : size}
    matplotlib.rc('font', **font)

def get_colors_bt(args):

    if args.variables == None or len(args.variables) != 1:
        print "Error: backtrace coloration is available for one variable only"
        exit(1)
    
    backtrace = get_backtrace(args)
    if backtrace == None:
        return
        
    backtrace_filtered = filter(lambda (hval,bt,time): int(hval) == args.variables[0], backtrace)
    hash_value_list,backtrace_list,time_list = zip(*backtrace_filtered)    
    backtrace_set = set(backtrace_list)
    colors = cm.rainbow(np.linspace(0, 1, len(backtrace_set)))
    map_backtrace_to_color = dict(zip(backtrace_set, colors))
    colors_list = [map_backtrace_to_color[backtrace] for backtrace in backtrace_list]
    
    return backtrace_set, map_backtrace_to_color, colors_list

def fast_scatter(ax,x,y,alpha,legends_name,legend_name,backtrace_set,map_backtrace_to_color,colors_list):

    z = zip(x,y,colors_list)
    labels = []
    i=0
    for bt in backtrace_set:
        color = map_backtrace_to_color[bt]
        tmp = filter(lambda (x,y,c): all(c==color), z)
        xx,yy,cc = zip(*tmp)
        leg = legend_name+" bt:"+bt
        legends_name.append(leg)
        label, = ax.plot(xx,
                        yy,
                        label=leg,
                        color=color,
                        alpha=alpha,
                        marker='o',
                        markersize=marker_size_default,
                        linestyle='')
        labels.append(label)
    return labels

def get_name_dict(args):

    name_dict = {}
    if args.location_info_map:
        try:
            li_map_file = open(args.location_info_map, 'r')
            for line in li_map_file:
                hash_,li = line.split(':')
                name_dict[hash_] = li
        except ValueError as e:
            print str(e) + " for line: " + line
            exit(1)
            
    return name_dict
 
def get_name(name_dict, hash_):
    if name_dict == {}:
        return hash_
    else:
        return name_dict[hash_]

def plot_mean(ax2, time, mean, legend_name, legends_name, labels, colors):

    pos,neg = split_mean(time,mean)
    tpos,mpos,tneg,mneg = [],[],[],[]
    if pos != []:
        tpos,mpos = zip(*pos)
    if neg != []:
        tneg,mneg = zip(*neg)
            
    label, = ax2.plot(tpos, mpos,
                      label=legend_name,
                      marker='^',
                      markersize=marker_size,
                      color=colors,
                      linestyle='none',
                      alpha=alpha)
    
    if pos != []:
        labels.append(label)
        legends_name.append("$\mu^+$")
            
    label, = ax2.plot(tneg, mneg,
                      label=legend_name,
                      marker='v',
                      markersize=marker_size,
                      color=colors,
                      linestyle='none',
                      alpha=alpha)
            
    if neg != []:
        labels.append(label)
        legends_name.append("$\mu^-$")

def plot_std(ax2, time, std, legend_name, legends_name, labels, colors):
    label, = ax2.plot(time, std,
                      label=legend_name,
                      marker='x',
                      markersize=marker_size,
                      color=colors,
                      linestyle='none',
                      alpha=alpha)

    labels.append(label)
    legends_name.append("$\sigma$")

        
def plot_significant_number(values_dict, args):

    fig, ax1 = plt.subplots()

    if args.std or args.mean:
        ax2 = plt.twinx()    
        ax2.set_ylabel('Values ($\mu$,$\sigma$)')
        ax2.semilogy()

    title = ax1.set_title('Significant digits evolution', loc="center")
    ax1.set_ylabel('Significant digits (base=$%d$)' % args.base)
    
    if args.invocation_mode:
        ax1.set_xlabel('Invocation')
    else:
        ax1.set_xlabel('Time')

    ylim_base = 18
    if args.base:
        ylim_base = int(math.log(10, args.base) * 18)
        
    ax1.set_ylim(-2 , ylim_base)
    ax1.set_yticks(range(ylim_base))
        
    labels = []
    legends_name = []
    
    hmax = 0

    colors = cm.rainbow(np.linspace(0, 1, len(values_dict)))
    color_i = 0

    if args.backtrace:
        backtrace_set,map_backtrace_to_color,colors_list = get_colors_bt(args)
        
    name_dict = get_name_dict(args)
    
    # for variable, stats_values_list in d_values.iteritems():
    for variable, stats_values_list in values_dict.iteritems():
        
        name = get_name(name_dict, variable)
        legend_name =  "$s$ %s" % name
        if not args.backtrace:
            legends_name.append(legend_name)

        values_list_sorted = sorted(stats_values_list, key=lambda value: value.time)

        sdn_list = map(lambda value : value.sdn, values_list_sorted)
        time_list = map(lambda value : value.time, values_list_sorted)

        if args.base:
            snd_list = map(lambda s : float(s) * math.log(10, args.base), sdn_list)

        if args.normalize_time:
            global time_start
            shift = time_start
            time_list = map(lambda i : i-shift, time_list)
        
        if args.invocation_mode:
            time_list = xrange(len(time_list))
            max_time = max(time_list)
            ax1.set_xlim(-1, max_time + 1 )
                    
        if args.backtrace:
            labels = fast_scatter(ax1, time_list,
                                  sdn_list, alpha,legends_name,legend_name,
                                  backtrace_set,map_backtrace_to_color,colors_list)
        else:
            label, = ax1.plot(time_list,
                              sdn_list,
                              label=legend_name,
                              alpha=alpha,
                              marker='o',
                              markersize=marker_size,
                              color=colors[color_i],
                              linestyle='None')
            labels.append(label)

        if args.mean:
            mean_list = map(lambda value : value.mean, values_list_sorted)
            plot_mean(ax2, time_list, mean_list, legend_name, legends_name, labels, colors[color_i])
        if args.std:
            std_list = map(lambda value : value.std, values_list_sorted)
            plot_std(ax2, time_list, std_list, legend_name, legends_name, labels, colors[color_i])
            
        color_i += 1
    
    plt.legend(labels,
               legends_name,
               fancybox=True,
               shadow=False,
               frameon=True,
               loc='center left',
               bbox_to_anchor=(1, 0.5))
    
    plt.tight_layout()
    plt.subplots_adjust()
    
    if args.scientific_mode:
        ax1.get_xaxis().get_major_formatter().set_scientific(False)

    ax1.ticklabel_format(useOffset=False)        

    if not args.no_show:
        plt.show()
    
    if args.output:        
        plt.savefig(args.output)

def set_param(args):
    global alpha
    alpha = args.transparency if args.transparency else 1.0    
        
def run(args):
    set_param(args)
    csv_values = read_csv(args)
    plot_significant_number(csv_values, args)
    return True
