#!/usr/bin/python

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

marker_size_default = 6
marker_size_pres = 20

sep_legend_name = ' | '
wildcard = '*'

header = "hash,type,time,max,min,median,mean,std,number-significant-digit"
header_csv = header.split(',')

def get_key(row):
    return row['hash']

def get_value(row):
    try:
        sn = row['number-significant-digit']
        mean = row['mean']
        min_ = row['std']
        max_ = row['max']
        time = int(row['time'])
    except:
        print row
        exit(1)
    # return (time,sn)
    return (time,sn,mean,max_,min_)

def is_valid_row(variables_set, row):
    return int(row['hash']) in variables_set
    
def read_csv(args):

    variables_to_plot = args.variables
    
    fi = open(args.csv_file, 'rb')
    dictReader = csv.DictReader(fi, fieldnames=header_csv)

    d_values = dict()
    get_values = d_values.get

    if args.variables != None:
        dictReaderClean = filter(lambda row: is_valid_row(variables_to_plot,row), dictReader)
    else:
        dictReaderClean = dictReader

    for row in dictReaderClean:
        key = get_key(row)
        value = get_value(row)
        try:
            d_values[key].append(value)
        except KeyError:
            d_values[key] = deque([value])
            
    return d_values

    
def create_legend_name(k):
    return sep_legend_name.join(k)

# Return [...,['bt_i','i'],...]
def get_backtrace(args):
    if not args.backtrace:
        return 

    file_bt = args.backtrace
    f_bt = open(file_bt, 'r')

    return map(str.split, f_bt)

def toRGB(color):
    return map(lambda x : int(x*255), color)
    
def parse_list_color(list_color):
    dict_color = dict(list_color)
    print dict_color
    fig, ax1 = plt.subplots()
    i = 1
    for hash_value, color in dict_color.iteritems():
        ax1.axvline(i, c=color, lw=10)
        i += 1
        
    xtick = range(1, len(dict_color) + 1)
    ax1.set_xticks(xtick)
    ax1.set_xticklabels(dict_color.keys(), rotation=90)
    plt.show()
    
def plot_backtrace(time, args):

    list_bt = get_backtrace(args)
    if list_bt == None:
        return
    list_bt[:] = map(lambda (h,t) : (int(h),int(t)),list_bt)
    
    hash_values = set(map(lambda x:x[0],list_bt))
    colors = cm.hsv(np.linspace(0, 1, len(hash_values)+1))
    hash_colors = dict(zip(list(hash_values),colors))

    hash_colors_str = sorted(hash_colors.iteritems())
    # parse_list_color(hash_colors_str)
        
    plt.axvline(time[0], color=hash_colors[list_bt[0][0]])
    print time[0],list_bt[0]
    
    def plt_bt((t,bt), (t_1,bt_new)):
        if bt[0] != bt_new[0]:
            print t_1,bt_new
            plt.axvline(t, color=hash_colors[bt_new[0]])
        return (t_1,bt_new)

    reduce(plt_bt, zip(time,list_bt))

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

    bt = get_backtrace(args)
    if bt == None:
        return
    

    bt_list,time_list = zip(*bt)
        
    backtrace_set = set(bt_list)
    colors = cm.hsv(np.linspace(0, 1, len(backtrace_set)))
    map_bt_color = dict(zip(backtrace_set, colors))
    
    return colors,[map_bt_color[b] for b in bt_list]


def fast_scatter(ax,x,y,map_colors,colors,alpha,legend_name):

    z = zip(x,y,colors)
    labels = []
    i=0
    for color in map_colors:
        tmp = filter(lambda (x,y,c): all(c==color), z)
        xx,yy,cc = zip(*tmp)
        label = ax.plot(xx,
                        yy,
                        label=legend_name+str(i),
                        color=color,
                        alpha=alpha,
                        marker='o',
                        markersize=marker_size_default,
                        linestyle='')
        i+=1
        labels.append(label)
    return labels

def plot_significant_number(d_values, args):

    fig, ax1 = plt.subplots()

    if args.std or args.mean:
        ax2 = plt.twinx()    
        ax2.set_ylabel('Values ($\mu$,$\sigma$)')
        ax2.semilogy()

    title = ax1.set_title('Significant digits evolution', loc="center")
    ax1.set_ylabel('Significant digits')
    
    if args.iteration_mode:
        ax1.set_xlabel('Invocation')
    else:
        ax1.set_xlabel('Time')
    
    ax1.set_ylim(-2 , 18)
    ax1.set_yticks(range(18))


    marker_size = marker_size_default
    # if args.mode_pres:
    #     marker_size = marker_size_pres
        
    labels = []
    legends_name = []
    
    hmax = 0

    colors = cm.rainbow(np.linspace(0, 1, len(d_values)))
    color_i = 0

    if args.backtrace:
        map_colors,colors = get_colors_bt(args)

    for k,v in d_values.iteritems():
        
        legend_name =  "$s$"+k
        # legend_name = k
        legends_name.append(legend_name)
        vs = sorted(v,key=lambda (t,ns,m,ma,mi):t)
        # vs = sorted(v,key=lambda (t,ns):t)
        
        # time,ns = zip(*vs)

        time,ns,mean,ma,std = zip(*vs)
        # time,ns = zip(*vs)

        if args.normalize_time:
            shift = min(time)
            time = map(lambda i : i-shift, time)
        
        if args.iteration_mode:
            time = [i for (i,t) in enumerate(time)]
           

            
        # hmax = max(hmax, len(time))

        alpha = 1.0 if args.no_transparency else 0.1

        if args.backtrace:
            fast_scatter(ax1,time,ns,map_colors,colors,alpha,legend_name)
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
                              color='c',
                              linestyle='none',
                              alpha=alpha)
            labels.append(label)
            legends_name.append("$\mu$")
            
            label, = ax2.plot(tneg, mneg,
                              label=legend_name,
                              marker='v',
                              markersize=marker_size,
                              color='g',
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
                              color='r',
                              linestyle='none',
                              alpha=alpha)
        

            labels.append(label)
            legends_name.append("$\sigma$")
        color_i += 1
    
    if args.mode_pres:
        set_font(40)
        # plt.legend(labels, legends_name,
        #            fancybox=True, shadow=False,
        #            mode='expend', frameon=True,
        #            prop={'size':40}, loc='lower right')
    else:
        plt.legend(labels, legends_name,
                   fancybox=True, shadow=False,
                   mode='expend', frameon=True)
        
    plt.tight_layout()
    plt.subplots_adjust()
    
    # ax1.xlim(xmin=-1)

    if args.scientific_mode:
        ax1.get_xaxis().get_major_formatter().set_scientific(False)

    ax1.ticklabel_format(useOffset=False)        

    if not args.no_show:
        plt.show()
    
    if args.output:
        plt.savefig(args.output+'.pdf')

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description="Plot significant number of variables over time")
    parser.add_argument('-f','--csv-file', type=str, action='store', required=True,
                        help="CSV filename which contains variables info")
    parser.add_argument('-v','--variables', type=int, nargs='*', action='store',
                        help="Hash of variables to plot")
    parser.add_argument('-bt','--backtrace', action="store", type=str,
                        help="file with backtrace for each time")
    parser.add_argument('--no-show', action="store_true", default=False,
                        help="Not show figure")
    parser.add_argument('-o','--output', action="store", default="plot.pdf",
                        help="Save figure in a PDF file")

    parser.add_argument('--mean', action="store_true",
                        help="plot mean values")
    parser.add_argument('--std', action="store_true",
                        help="plot std values")

    parser.add_argument('--no-transparency',action='store_true',
                        help="No transparency for plot")
    
    parser.add_argument('--mode-pres', action='store_true',
                        help="Large font and legends for paper")
    
    parser.add_argument('--iteration-mode', action='store_true',
                        help="Set time as one by one")

    parser.add_argument('--normalize-time', action='store_true',
                        help="Start time at 0")


    parser.add_argument('--scientific-mode', action='store_true',
                        help='Set scientific mode for x-axis')
    
    args = parser.parse_args()

    csv_values = read_csv(args)
    plot_significant_number(csv_values, args)
    
