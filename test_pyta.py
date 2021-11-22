import pyta

rules = [(0, 0, (1, 1, 1)), (1, 1, (1, 2)), (0, 0, (0,))]

pyta.compile(3, 2, rules)
