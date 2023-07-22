# beware: this assumes no loop in the automaton, nothing is done to detect them and it will run forever if there are
def inside_weight(a, scores, semiring, as_sr_element, start, out):
    if start in out:
        return out[start]
    else:
        inside = semiring.zero
        for rule in a.td_all(start):
            for child in a.get_rule(rule)[2]:
                if child == a.get_rule(rule)[0]:
                    print("loop!")
                    print(rule, a.get_rule(rule))
                    exit(0)
            children_inside = [inside_weight(a, scores, semiring, as_sr_element, child, out)
                               for child in a.get_rule(rule)[2]]
            rule_value = as_sr_element(scores, rule)
            for ci in children_inside:
                rule_value = semiring.times(rule_value, ci)
            inside = semiring.plus(inside, rule_value)

        out[start] = inside
        return inside


class MaxPlusElement:

    def __init__(self, value=0, inf=False):
        self._inf = inf
        self.value = value

    def get_value(self):
        if self._inf:
            raise ValueError('-inf')
        else:
            return self.value

    def is_inf(self):
        return self._inf

    def __ge__(self, other):
        if other.is_inf():
            return True
        elif self.is_inf():
            return other.is_inf()
        else:
            return self.get_value() >= other.get_value()

    def __str__(self):
        if self.is_inf():
            return '-inf'
        else:
            return str(self.value)

    def __repr__(self):
        if self.is_inf():
            args = 'inf=True'
        else:
            args = 'value=%s' % repr(self.value)
        return 'MaxPlusElement(%s)' % args


class MaxPlusSemiring:

    one = (MaxPlusElement(0), None)
    zero = (MaxPlusElement(inf=True), None)

    @staticmethod
    def plus(x, y):
        (e1, bp1) = x
        (e2, bp2) = y

        if e1 >= e2:
            return x
        else:
            return y

    @staticmethod
    def times(x, y):
        if x[0].is_inf() or y[0].is_inf():
            return MaxPlusElement(inf=True), x[1]
        else:
            return MaxPlusElement(value=x[0].get_value() + y[0].get_value()), x[1]


def as_max_plus_element(scores, rule_name):
    return MaxPlusElement(value=scores[rule_name]), rule_name
