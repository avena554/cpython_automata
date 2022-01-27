from collections import defaultdict
from pyta.util.encoders import DynamicEncoder


class PurePythonCore:

    def __init__(self, n_states, n_symb, td_index, bu_index, rules, final):
        self._td_index = td_index
        self._bu_index = bu_index
        self.rules = rules
        self.final = final
        self.n_states = n_states
        self.n_symb = n_symb

    def td_query(self, q_parent, label):
        return self._td_index[q_parent][label]

    def td_all(self, q_parent):
        index_p = self._td_index[q_parent]
        for (l, r_ids) in index_p.items():
            for r in r_ids:
                yield r

    def td_labels(self, parent):
        return self._td_index[parent].keys()

    def bu_labels(self, children):
        return self._bu_index[children].keys()

    def all_bu(self, children):
        index_p = self._bu_index[children]
        for (l, r_ids) in index_p.items():
            for r in r_ids:
                yield r

    def bu_query(self, children, label):
        return self._bu_index[children][label]

    def get_rule(self, index):
        return self.rules[index]


def compile_pure(n_states, n_symb, coded_rules, final):
    td_index = defaultdict(lambda: defaultdict(set))
    bu_index = defaultdict(lambda: defaultdict(set))

    for i, rule in enumerate(coded_rules):
        td_index[rule[0]][rule[1]].add(i)
        bu_index[rule[2]][rule[1]].add(i)

    return PurePythonCore(n_states, n_symb, td_index, bu_index, coded_rules, final)


def intersect_pure(a1, a2):
    # print("intersecting with pure python")
    n_pr = 0
    ps_map = DynamicEncoder()
    pr_reverse = {}

    agenda = [(a1.final, a2.final)]
    seen = set()
    rules = []
    while agenda:
        (left, right) = agenda.pop()
        seen.add((left, right))
        for i1 in a1.td_all(left):
            r1 = a1.get_rule(i1)
            label = r1[1]
            for i2 in a2.td_query(right, label):
                r2 = a2.get_rule(i2)

                parent = ps_map.encode((r1[0], r2[0]))

                children = [-1] * len(r1[2])

                for i, child_pair in enumerate(zip(r1[2], r2[2])):
                    children[i] = ps_map.encode(child_pair)
                    if child_pair not in seen:
                        agenda.append(child_pair)

                rules.append((parent, label, tuple(children)))
                pr_reverse[n_pr] = (i1, i2)
                n_pr += 1

    final = ps_map.encode((a1.final, a2.final))
    return compile_pure(n_pr, a1.n_symb, rules, final), final, ps_map.as_maps()[1], pr_reverse
