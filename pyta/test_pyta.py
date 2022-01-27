import pyta

rules = [(0, 0, (1, 1, 1)), (1, 1, (2, 2)), (2, 0, tuple()), (1, 1, tuple()), (1, 0, tuple())]

a = pyta.compile(3, 2, rules, 0)

print("td all")
print([a.get_rule(r) for r in a.td_all(0)])
print([a.get_rule(r) for r in a.td_all(1)])
print([a.get_rule(r) for r in a.td_all(2)])


print("\nbu all")
print([a.get_rule(r) for r in a.bu_all([1, 1, 1])])
print([a.get_rule(r) for r in a.bu_all((2, 2))])
print([a.get_rule(r) for r in a.bu_all([0,])])
print([a.get_rule(r) for r in a.bu_all([])])

print("\ntd labels")
print([l for l in a.td_labels(1)])
print("\nbu labels")
print([l for l in a.bu_labels(tuple())])


print("\ntd query")
print([a.get_rule(r) for r in a.td_query(1, 1)])

print("\nbu query")
print([a.get_rule(r) for r in a.bu_query(tuple(), 0)])


print("\nintersect")
s = pyta.intersect(a, a)
print(s)
