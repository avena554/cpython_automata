from distutils.core import setup, Extension

setup_args = {
    'ext_modules': [
        Extension("pyta_c_backend",
                  ["avl/avl.c", "dict.c", "automata.c", "list.c", "utils.c", "pyta_c_backend.c"],
                  language="c",
                  extra_compile_args = ["-DDEBUG=0"])
    ]
}
setup(**setup_args)
