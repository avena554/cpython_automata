from distutils.core import setup, Extension

setup(name="pyta", version="1.0", ext_modules=[Extension("pyta", ["avl/avl.c", "dict.c", "automata.c", "pyta.c"])])

