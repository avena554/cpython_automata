from importlib.metadata import entry_points
from typing import Callable

from pyta.py_backend.core import compile_pure, intersect_pure, intersect_ac_pure

ep_compile = entry_points(group='pyta.compile')
ep_inter = entry_points(group='pyta.inter')
ep_inter_ac = entry_points(group='pyta.inter_ac')

C_NAME = 'C'
PY_NAME = 'PY'

core_fns_names = ['compile', 'inter', 'inter_ac']
py_core_fns = [compile_pure, intersect_pure, intersect_ac_pure]

bcknds = {name: {PY_NAME: f} for (name, f) in zip(core_fns_names, py_core_fns)}
c_available = {name: True for name in core_fns_names}

for name in core_fns_names:
    ep = entry_points(group=('pyta.' + name))
    try:
        bcknd = ep[C_NAME].load()
        bcknds[name][C_NAME] = bcknd
    except KeyError:
        bcknds[name][C_NAME] = bcknds[name][PY_NAME]
        c_available[name] = False

MODE = {fn: C_NAME for fn in core_fns_names}
for name in core_fns_names:
    if not c_available[name]:
        print('C backend not available for {:s}. Falling back to python implementation'.format(name))
        MODE[name] = PY_NAME


class ModeSwitch(Callable):
    def __init__(self, name):
        self.name = name

    def __call__(self, *args, **kwargs):
        return bcknds[self.name][MODE[self.name]](*args, **kwargs)


core = [ModeSwitch(name) for name in core_fns_names]


def get_core():
    return core


def c_mode():
    for name in core_fns_names:
        MODE[name] = C_NAME


def py_mode():
    for name in core_fns_names:
        MODE[name] = PY_NAME
