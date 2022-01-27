from distutils.core import setup, Extension

setup(name="pyta", version="1.0", ext_modules=[Extension("pyta", ["avl/avl.c", "dict.c", "automata.c", "list.c", "utils.c", "pyta.c"], extra_compile_args = ["-DDEBUG=0"])])

