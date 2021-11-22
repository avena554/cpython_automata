import pyta

import tracemalloc

tracemalloc.start()

rules = [(0, 0, (1, 1, 1)), (1, 1, (1, 2)), (0, 0, (0,))]

snap1 = tracemalloc.take_snapshot()

a = pyta.compile(3, 2, rules)

snap2 = tracemalloc.take_snapshot()

stats = snap2.compare_to(snap1, "lineno")

for stat in stats:
    print(stat)

del a
del rules

snap3 = tracemalloc.take_snapshot()

stats = snap3.compare_to(snap2, "lineno")

for stat in stats:
    print(stat)
