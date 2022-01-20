import pyta

rules = [(0, 0, (1, 1, 1)), (1, 1, (1, 2)), (0, 0, (0,))]


a = pyta.compile(3, 2, rules)

print(a.get_rule(0))
print([a.get_rule(r) for r in a.rules_from_parent(0)])
print([a.get_rule(r) for r in a.rules_from_parent(1)])
print([a.get_rule(r) for r in a.rules_from_parent(2)])

print([a.get_rule(r) for r in a.rules_from_children([1, 1, 1])])
print([a.get_rule(r) for r in a.rules_from_children((1, 2))])
print([a.get_rule(r) for r in a.rules_from_children([0,])])
print([a.get_rule(r) for r in a.rules_from_children([])])



