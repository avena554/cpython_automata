from pyta.algebra.terms import Term
from pyta.automata import compile_automaton
import re


# Terms over a left concatenation algebra, providing homomorphic interpretations for the edit distance
class LeftConcAlgebraTerm(Term):

    epsilon = '<epsilon>'
    _conc_pattern = re.compile('^<(\\w)\\.>$')

    @staticmethod
    def left_conc_name(l):
        return '<%s.>' % l

    @staticmethod
    def extract_letter(name):
        m = LeftConcAlgebraTerm._conc_pattern.match(name)
        if m:
            return m.group(1)
        else:
            return None

    def __init__(self, label, children):
        super(LeftConcAlgebraTerm, self).__init__(label, children)

    @staticmethod
    def make_left_conc(l, arg1):
        return LeftConcAlgebraTerm(label=LeftConcAlgebraTerm.left_conc_name(l), children=[arg1])

    @staticmethod
    def make_epsilon():
        return LeftConcAlgebraTerm(label=LeftConcAlgebraTerm.epsilon, children=[])

    def get_letter(self):
        return LeftConcAlgebraTerm.extract_letter(self.label)

    @staticmethod
    def _repr_var_transform(_, var_id):
        return 'make_var(var_id=%d)' % var_id

    @staticmethod
    def _repr_op_transform(label, arg):
        return 'make_left_conc(%s, %s)' % (LeftConcAlgebraTerm.extract_letter(label), repr(arg))

    @staticmethod
    def _repr_const_transform(_):
        return 'make_epsilon()'

    def __repr__(self):
        return self.pretty_format(
           LeftConcAlgebraTerm._repr_var_transform,
           LeftConcAlgebraTerm._repr_op_transform,
           LeftConcAlgebraTerm._repr_const_transform
        )


def decompose(s, labels_encoder):
    n = len(s)
    rulemap = {'%s' % i: ((i, n), LeftConcAlgebraTerm.left_conc_name(s[i]), ((i + 1, n),))
               for i in range(n)}
    rulemap['%s' % n] = ((n, n), LeftConcAlgebraTerm.epsilon, ())
    return compile_automaton(rulemap, (0, n), labels_encoder=labels_encoder)
