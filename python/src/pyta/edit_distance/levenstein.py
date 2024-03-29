import pyta
from pyta.edit_distance.edit_transducer import make_transducer
from pyta.automata import inter, inter_ac, scores_intersection, invhom, DynamicEncoder
from pyta.automata.semirings import inside_weight, MaxPlusSemiring, as_max_plus_element
from pyta.algebra.LeftConcAlgebra import decompose
import argparse
from pyta.edit_distance.decoding import decode, read_rewrite_seq


def inform(message, verbose=True):
    if verbose: 
        print(message)


def levenstein_distance(s1, s2, transpositions=True, substitutions=True, verbose=True):
    # compute the transducer
    inform('computing transducer...', verbose)
    inform('\ts1: %d characters, s2: %d characters' % (len(s1), len(s2)), verbose)
    voc = set(s1).union(set(s2))
    inform('\tvocabulary of %d symbols' % len(voc), verbose)
    t_sig = DynamicEncoder()
    (t, h1, h2, scores) = make_transducer(voc, t_sig, transpositions=transpositions, substitutions=substitutions)

    inform('\t...done: %d rules, %d states, %d labels' % (t.n_rules, len(t.states), len(t.sigma)), verbose)
    inform('decomposing entries in a left-concatenation algebra', verbose)
    inform('decomposing first string...', verbose)
    d_sig = DynamicEncoder()
    d1 = decompose(s1, labels_encoder=d_sig)
    # print('d1', d1)
    inform('...done', verbose)
    inform('decomposing second string...', verbose)
    d2 = decompose(s2, labels_encoder=d_sig)
    # print('d2', d2)
    inform('...done', verbose)

    inform('parsing...', verbose)

    inform('\tinverting first homomorphism...', verbose)
    inv1 = invhom(h1, d1, t_sig, d_sig)
    inform('\t...done: %d rules, %d states, %d labels' % (inv1.n_rules, len(inv1.states), len(inv1.sigma)), verbose)
    inform('\tinverting second homomorphism...', verbose)
    inv2 = invhom(h2, d2, t_sig, d_sig)
    inform('\t...done: %d rules, %d states, %d labels' % (inv2.n_rules, len(inv2.states), len(inv2.sigma)), verbose)

    # print('inv1', inv1)
    # print('inv2', inv2)


    inform('\tcombining entries...', verbose)
    partial_chart = inter(inv1, inv2, t_sig, t.n_symb)

    inform('\t...done: %d rules, %d states, %d labels'
           % (partial_chart.n_rules, len(partial_chart.states), len(partial_chart.sigma)), verbose)

    inform('\tparsing...', verbose)
    chart = inter_ac(t, partial_chart, t_sig, t.n_symb)
    weights = scores_intersection(chart, scores_left=scores)
    inform('\t...done: %d rules, %d states, %d labels' % (chart.n_rules, len(chart.states), len(chart.sigma)), verbose)
    inform('...done parsing', verbose)

    inform('Computing the edit distance with Viterbi...', verbose)
    inside_map = {}
    inside_weight(chart, weights, MaxPlusSemiring, as_max_plus_element, chart.final, inside_map)
    inform('...done', verbose)

    d = -inside_map[chart.final][0].get_value()

    inform('finding an optimal sequence...', verbose)
    seq = []
    decode(chart, inside_map, chart.final, seq)
    inform('...done', verbose)


    # print(seq)
    rw_seq, rw_ops = read_rewrite_seq(seq)
    print('optimal rewrite sequence: ', ' -> '.join(rw_seq))
    print('rewrite operations: ', ' ; '.join(rw_ops))
    return d


def main():

    parser = argparse.ArgumentParser(description='compute the edit distance between two strings')
    parser.add_argument('first', metavar='s1', type=str, help='the first string')
    parser.add_argument('second', metavar='s2', type=str, help='the second string')
    parser.add_argument('--disable-tr', dest='d_tr', action='store_true', help='disable adjacent transpositions if present')
    parser.add_argument('--disable-sub', dest='d_sub', action='store_true', help='disable substitutions if present')
    parser.add_argument('--verbose', dest='verbose', action='store_true', help='makes the program talk a lot')
    parser.add_argument('--pure', dest='pure', action='store_true', help='use a pure python implementation of automata')
    args = parser.parse_args()

    s1 = args.first
    s2 = args.second

    if args.pure:
        pyta.py_mode()
        inform("Using pure Python", args.verbose)
    else:
        pyta.c_mode()
        inform("Using the C extension", args.verbose)

        d = levenstein_distance(s1, s2, transpositions=not args.d_tr, substitutions=not args.d_sub, verbose=args.verbose)
        print('edit distance: %d' % d)


if __name__ == "__main__":
    main()
