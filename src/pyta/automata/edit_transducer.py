from pyta.automata import compile_automaton
from pyta.algebra.LeftConcAlgebra import LeftConcAlgebraTerm
from pyta.util.misc import cartesian_product
import re

INSERTION_PFX = 'Ins'
DELETION_PFX = 'Del'
SUB_PFX = 'Sub'
INIT_TR_PFX = 'Init_tr'
CLOSE_TR_PFX = 'Close_tr'
END_PFX = 'End'


def _add_rule(rulemap, name, rule):
    # Add rule
    rulemap[name] = rule


def _add_interpretation(label, h1, h2, h1_im, h2_im):
    h1[label] = h1_im
    h2[label] = h2_im


# Naming pattern for deletion and insertion rules. t
# The remaining string pattern is there for providing different names for rule with distinct parent states
# For labels it'll be replaced with the empty string
def _pattern_l(pfx, l):
    return '%s(%s)%s' % (pfx, l, '%s')


# Naming pattern for transposition and substitution rules
def _pattern_l_r(pfx, l, r):
    return '%s(%s|%s)' % (pfx, l, r)


# Naming scheme for final rules
def _pattern_final(pfx):
    return pfx


def _add_insertion_rules(pfx, rulemap, h1, h2, states, sigma, writes, skip):
    for l in sigma:
        pattern = _pattern_l(pfx, l)
        label = pattern % ''
        for q in states:
            name = pattern % ('[%s]' % q)
            _add_rule(rulemap, name, (q, label, (q,)))
        _add_interpretation(label, h1, h2, skip, writes[l])


def _add_deletion_rules(pfx, rulemap, h1, h2, states, sigma, writes, skip):
    for l in sigma:
        pattern = _pattern_l(pfx, l)
        label = pattern % ''
        for q in states:
            name = pattern % ('[%s]' % q)
            _add_rule(rulemap, name, (q, label, (q,)))
        _add_interpretation(label, h1, h2, writes[l], skip)


def _add_transposition_rules(init_pfx, close_pfx, rulemap, h1, h2, q_0, q_trs, sigma, writes):
    for l, r in cartesian_product(sigma, sigma):
        # Use the label as name in this case because there's only one rule for each label
        init_label = _pattern_l_r(init_pfx, l, r)
        close_label = _pattern_l_r(close_pfx, l, r)
        _add_rule(rulemap, init_label, (q_0, init_label, (q_trs[l][r],)))
        _add_rule(rulemap, close_label, (q_trs[l][r], close_label, (q_0,)))
        _add_interpretation(init_label, h1, h2, writes[l], writes[r])
        _add_interpretation(close_label, h1, h2, writes[r], writes[l])


def _add_substitution_rules(pfx, rulemap, h1, h2, q_0, sigma, writes):
    # Use the label as name in this case because there's only one rule for each label
    for l, r in cartesian_product(sigma, sigma):
        label = _pattern_l_r(pfx, l, r)
        _add_rule(rulemap, label, (q_0, label, (q_0,)))
        _add_interpretation(label, h1, h2, writes[l], writes[r])


def _add_final_rule(pfx, rulemap, q_0, h1, h2, epsilon):
    # Use the label as name in this case because there's only one rule for each label
    label = _pattern_final(pfx)
    _add_rule(rulemap, label, (q_0, label, ()))
    _add_interpretation(label, h1, h2, epsilon, epsilon)


def edit_transducer(sigma, sig,
                    transpositions=True, substitutions=True,
                    ins_pfx=INSERTION_PFX, del_pfx=DELETION_PFX,
                    init_tr_pfx=INIT_TR_PFX, close_tr_pfx=CLOSE_TR_PFX,
                    sub_pfx=SUB_PFX,
                    end_pfx=END_PFX):
    # Actually a string bimorphism, but coded as a monadic tree bimorphism

    # states
    q_0 = 'q_0'
    q_trs = {left: {right: 'q_tr(%s|%s)' % (left, right) for right in sigma} for left in sigma}
    states = {q_0}
    for (_, q_tr_l) in q_trs.items():
        states.update(q_tr_l.values())

    # init rulemap
    rulemap = dict()
    # Reservoir of Homomorphic images
    writes = {l: LeftConcAlgebraTerm.make_left_conc(l, LeftConcAlgebraTerm.make_var(1)) for l in sigma}
    skip = LeftConcAlgebraTerm.make_var(1)
    epsilon = LeftConcAlgebraTerm.make_epsilon()

    # init homomorphims
    h1 = dict()
    h2 = dict()

    # add (interpreted) rules
    _add_insertion_rules(ins_pfx, rulemap, h1, h2, states, sigma, writes, skip)
    _add_deletion_rules(del_pfx, rulemap, h1, h2, states, sigma, writes, skip)
    _add_final_rule(end_pfx, rulemap, q_0, h1, h2, epsilon)
    if substitutions:
        _add_substitution_rules(sub_pfx, rulemap, h1, h2, q_0, sigma, writes)
    if transpositions:
        _add_transposition_rules(init_tr_pfx, close_tr_pfx, rulemap, h1, h2, q_0, q_trs, sigma, writes)

    no_cost = re.compile('^(%s|%s)' % (close_tr_pfx, end_pfx))
    keep = re.compile('^%s\\((.)\\|(.)\\)' % sub_pfx)
    t = compile_automaton(rulemap, q_0, sig)

    # Add weights to all rules
    # Fixme: add weights when creating the rules this is ad hoc
    def compute_weights(name):
        m = keep.search(name)
        if m:
            if m.group(1) == m.group(2):
                return 0
            else:
                return -1
        elif no_cost.search(name):
            return 0
        else:
            return -1

    weights = {r_id: compute_weights(t.rules_decoder.decode(r_id)) for r_id in range(t.n_rules)}
    return t, h1, h2, weights
