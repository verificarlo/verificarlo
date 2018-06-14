from setuptools import setup
import os
import sys


argv = sys.argv

if 'install' in argv:
    # export_PYTHONPATH for installing
    prefix_arg = [arg for arg in argv if arg.find('--prefix') != -1]
    prefix = prefix_arg[0].split('=')[1]
    pkg_path = "/lib/python{major}.{minor}/site-packages/".format(major=sys.version_info.major,
                                                              minor=sys.version_info.minor)
    pythonpath = prefix + os.sep + pkg_path
    os.environ["PYTHONPATH"] = pythonpath

setup(name='veritracer',
      version='0.1',
      description='veritracer: context-enriched floating point tracer',
      scripts=['bin/veritracer'],
      packages=['veritracer','veritracer.veritracer_format'],
      zip_safe=False)

    
