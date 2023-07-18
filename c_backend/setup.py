from distutils.core import setup, Extension

setup_args = {
    'package_dir': {'': 'src'},
    'ext_modules': [
        Extension("pyta_core",
                  ["src/pyta/avl/avl.c", "src/pyta/dict.c", "src/pyta/automata.c", "src/pyta/list.c", "src/pyta/utils.c", "src/pyta/pyta_core.c"],
                  include_dirs=["src"],
                  language="c",
                  extra_compile_args = ["-DDEBUG=0"])
    ]
}
setup(**setup_args)
