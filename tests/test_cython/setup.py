from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

extensions = Extension("test", ["test.pyx"])

setup(name='Test MCA Cython App',
      ext_modules=cythonize(extensions,
                            compiler_directives={'language_level': '3'}))
