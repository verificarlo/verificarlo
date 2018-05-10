from setuptools import setup

setup(name='veritracer',
      version='0.1',
      description='veritracer: context-enriched floating point tracer',
      scripts=['bin/veritracer'],
      packages=['veritracer','veritracer.veritracer_format'],
      zip_safe=False)
