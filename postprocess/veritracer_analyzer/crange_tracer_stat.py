import ctypes 
from os import chdir
from collections import namedtuple
from subprocess import check_call

library_path = '/home/yohan/Documents/verificarlo/librangetracerstats/'
library_name = 'librangeTracerStats.so'

Functions = namedtuple('functions',
                       ['mean',
                       'std',
                       'cArrayBuild',
                       'median',
                       'min',
                        'max'])


def load_lib(N):
    check_call('make lib -C ' + library_path + ' N='+str(N), shell=True)
    return ctypes.CDLL(library_path+library_name)

def create_cArrayN_sig(N):
    return ctypes.c_double * N

def create_cdouble_sig():
    return ctypes.c_double

def create_cuint32_sig():
    return ctypes.c_uint32
        
def create_ctypes(N):
    
    c_ArrayN = create_cArrayN_sig(N)
    c_double = create_cdouble_sig()
    c_uint32 = create_cuint32_sig()
    
    return c_ArrayN,c_double,c_uint32
    
def get_array_fct(N):
    c_arrayN = create_cArrayN_sig(N)
    return c_arrayN    

def create_signature(rlib, N):
    cArrayN,c_double,c_uint32 = create_ctypes(N)

    argtypes = [cArrayN]
    argtypes_std = [c_double, cArrayN]
    restype = c_double

    rlib.mean.argtypes = argtypes
    rlib.mean.restype = restype

    rlib.std.argtypes = argtypes_std
    rlib.std.restype = restype

    rlib.median.argtypes = argtypes
    rlib.median.restype = restype

    rlib.maxd.argtypes = argtypes
    rlib.maxd.restype = restype

    rlib.mind.argtypes = argtypes
    rlib.mind.restype = restype

    arrayN = get_array_fct(N)
    
    return Functions(mean=rlib.mean,
                    std=rlib.std,
                    cArrayBuild=arrayN,
                    median=rlib.median,
                    max=rlib.maxd,
                    min=rlib.mind
    )

def load_functions(N):
    rlib = load_lib(N)
    return create_signature(rlib, N)
