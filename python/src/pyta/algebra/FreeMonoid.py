from pyta.algebra.terms import Term
from pyta.automata import compile_automaton
import re


# Terms over a left concatenation algebra, providing homomorphic interpretations for the edit distance
class FreeMonoidTerm(Term):

    epsilon = '<epsilon>'
    conc_symb = '*'
    _const_pattern = re.compile('^<(\\w*)>$')

    @staticmethod
    def const_name(letter):
        return '<{:s}>'.format(letter)

    @staticmethod
    def extract_letter(name):
        m = FreeMonoidTerm._const_pattern.match(name)
        if m:
            return m.group(1)
        else:
            return None

    def __init__(self, label, children):
        super(FreeMonoidTerm, self).__init__(label, children)

    @staticmethod
    def make_conc(arg1, arg2):
        return FreeMonoidTerm(label=FreeMonoidTerm.conc_symb, children=[arg1, arg2])

    @staticmethod
    def make_epsilon():
        return FreeMonoidTerm(label=FreeMonoidTerm.epsilon, children=[])

    @staticmethod
    def make_letter(letter):
        return FreeMonoidTerm(label="<{:s}>".format(letter), children=[])

    def get_letter(self):
        return FreeMonoidTerm.extract_letter(self.label)

    @staticmethod
    def _repr_var_transform(_, var_id):
        return 'make_var(var_id=%d)' % var_id

    @staticmethod
    def _repr_op_transform(_, arg1, arg2):
        return 'make_conc(%s, %s)' % (repr(arg1), repr(arg2))

    @staticmethod
    def _repr_const_transform(label):
        if label == FreeMonoidTerm.epsilon:
            return 'make_epsilon()'
        else:
            return 'make_letter({:s}'.format(FreeMonoidTerm.extract_letter(label))

    def __repr__(self):
        return self.pretty_format(
           FreeMonoidTerm._repr_var_transform,
           FreeMonoidTerm._repr_op_transform,
           FreeMonoidTerm._repr_const_transform
        )


def decompose(s, labels_encoder):
    n = len(s)
    rulemap = {}
    for i in range(n):
        for j in range(i + 1, n + 1):
            for k in range(i + 1, j):
                rulemap['<{:d}, {:d}, {:d}>'.format(i, j, k)] = (
                    (i, j),
                    FreeMonoidTerm.conc_symb,
                    ((i, k), (k, j))
                )

    for i in range(n):
        rulemap['<{:d}, {:d}>'.format(i, i + 1)] = ((i, i + 1), FreeMonoidTerm.const_name(s[i]), ())

    return compile_automaton(rulemap, (0, n), labels_encoder=labels_encoder)
