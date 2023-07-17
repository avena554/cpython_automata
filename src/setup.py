from distutils.core import setup, Extension

setup(name="pyta", version="1.0", packages=['pyta', 'pyta.automata', 'pyta.algebra', 'pyta.util'], ext_modules=[Extension("pyta.pyta_core", ["avl/avl.c", "dict.c", "automata.c", "list.c", "utils.c", "pyta_core.c"], extra_compile_args = ["-DDEBUG=0"])])

