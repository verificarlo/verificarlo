#!/usr/bin/env python

import csv
import matplotlib.pyplot as plt
import sys
import argparse
import matplotlib.cm as cm
import numpy as np
from collections import deque
from itertools import cycle
import ast
import matplotlib
import math
import scipy.stats
import os

marker_size_default = 6
marker_size_pres = 20

header = "hash,type,time,max,min,median,mean,std,significant-digit-number"
header_csv = header.split(',')
time_start = 0

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
        sn = row['significant-digit-number']
        mean = row['mean']
        min_ = row['std']
        max_ = row['max']
        time = int(row['time'])
    except Exception as e:
        print e
        print row
        exit(1)
    return (time,sn,mean,max_,min_)

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
    
    d_values = dict()
    get_values = d_values.get

    if args.variables != None:
        dictReaderClean = filter(lambda row: is_valid_row(variables_to_plot,row), dictReader)
    else:
        dictReaderClean = dictReader

    if args.normalize_time:
        global time_start
        first_row = dictReaderClean.pop(0)
        key = get_key(first_row)
        (time,sn,mean,max_,min_) = get_value(first_row)
        time_start = time
        try:
            d_values[key].append((time,sn,mean,max_,min_))
        except KeyError:
            d_values[key] = deque([(time,sn,mean,max_,min_)])
        
    for row in dictReaderClean:
        key = get_key(row)
        value = get_value(row)
        try:
            d_values[key].append(value)
        except KeyError:
            d_values[key] = deque([value])
            
    return d_values
    
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
    
def plot_significant_number(d_values, args):

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

    marker_size = marker_size_default
    # if args.mode_pres:
    #     marker_size = marker_size_pres
        
    labels = []
    legends_name = []
    
    hmax = 0

    colors = cm.rainbow(np.linspace(0, 1, len(d_values)))
    color_i = 0

    if args.backtrace:
        backtrace_set,map_backtrace_to_color,colors_list = get_colors_bt(args)
        
    name_dict = get_name_dict(args)
    
    for k,v in d_values.iteritems():
        
        name = get_name(name_dict, k)
        legend_name =  "$s$ "+name
        if not args.backtrace:
            legends_name.append(legend_name)
        vs = sorted(v,key=lambda (t,ns,m,ma,mi):t)

        time,ns,mean,ma,std = zip(*vs)

        if args.base:
            ns = map(lambda s : float(s) * math.log(10, args.base), ns)

        if args.normalize_time:
            global time_start
            shift = min(time)
            shift = time_start
            time = map(lambda i : i-shift, time)
        
        if args.invocation_mode:
            time = [i for (i,t) in enumerate(time)]
            ax1.set_xlim(-1, max(time) + 1 )
            ax1.set_xticks(range(max(time)+1))
            
        alpha = args.transparency if args.transparency else 1.0
        
        if args.backtrace:
            labels = fast_scatter(ax1,time,ns,alpha,legends_name,legend_name,
                                  backtrace_set,map_backtrace_to_color,colors_list)
        else:
            label, = ax1.plot(time,
                              ns,
                              label=legend_name,
                              # color='b',
                              alpha=alpha,
                              marker='o',
                              markersize=marker_size,
                              color=colors[color_i],
                              linestyle='None')
            labels.append(label)
                      
        if args.mean:
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
                              color=colors[color_i],
                              # color='c',
                              linestyle='none',
                              alpha=alpha)
            labels.append(label)
            legends_name.append("$\mu^+$")
            
            label, = ax2.plot(tneg, mneg,
                              label=legend_name,
                              marker='v',
                              markersize=marker_size,
                              color=colors[color_i],
                              # color='g',
                              linestyle='none',
                              alpha=alpha)

            
            if neg != []:
                labels.append(label)
                legends_name.append("$\mu^-$")
            
        if args.std:

            label, = ax2.plot(time, std,
                              label=legend_name,
                              marker='x',
                              markersize=marker_size,
                              color=colors[color_i],
                              # color='r',
                              linestyle='none',
                              alpha=alpha)
        

            labels.append(label)
            legends_name.append("$\sigma$")
        color_i += 1
    
    if args.mode_pres:
        set_font(40)
        plt.legend(labels,
                   legends_name,
                   fancybox=True,
                   shadow=False,
                   mode='expend',
                   frameon=True,
                   prop={'size':40},
                   loc='center left')
    else:
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

def run(args):
    csv_values = read_csv(args)
    plot_significant_number(csv_values, args)
    return True
