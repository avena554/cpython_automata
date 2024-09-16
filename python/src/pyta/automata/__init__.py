from collections import defaultdict

import pyta
from pyta.algebra.terms import Term
from pyta.util.misc import cartesian_product
from pyta.util.encoders import DynamicEncoder, StaticDecoder

import numpy as np

(core_compile, core_intersect, core_intersect_ac) = pyta.get_core()


class PyTA:

    def __init__(self, automaton, final, n_states, n_rules, n_symb, states_decoder, rules_decoder, labels_decoder):
        self.core = automaton
        self.sigma = list(range(n_symb))
        self.final = final
        self.states_decoder = states_decoder
        self.rules_decoder = rules_decoder
        self.labels_decoder = labels_decoder
        self.states = list(range(n_states))
        self.n_rules = n_rules
        self.n_symb = n_symb

    def td_query(self, parent, label):
        return self.core.td_query(parent, label)

    def td_all(self, parent):
        return self.core.td_all(parent)

    def td_labels(self, parent):
        return self.core.td_labels(parent)

    def bu_query(self, children, label):
        return self.core.bu_query(children, label)

    def bu_all(self, children):
        return self.core.bu_all(children)

    def bu_labels(self, children):
        return self.core.bu_labels(children)

    def get_rule(self, index):
        return self.core.get_rule(index)

    def all_rules(self):
        return [(r, self.get_rule(r)) for r in range(self.n_rules)]

    def decode_final(self):
        return self.states_decoder.decode(self.final)

    def run_td(self, term, parent_states):
        memory = defaultdict(lambda: defaultdict(set))
        agenda = [[term, parent_states, (), False]]

        while agenda:
            (subterm, candidates, pos, reduce) = agenda.pop()
            # print(subterm, reduce)
            if subterm.get_var_id() is not None:
                for s in candidates:
                    memory[pos][s].update([(s,)])
                continue

            if not reduce:
                agenda.append([subterm, candidates, pos, True])
                label = subterm.label
                c_candidates = [set() for _ in subterm.children]
                for parent in candidates:
                    for r in self.td_query(parent, label):
                        rule = self.get_rule(r)
                        for c_c, state in zip(c_candidates, rule[2]):
                            c_c.add(state)

                for i, (c_c, child) in enumerate(zip(c_candidates, subterm.children)):
                    agenda.append([child, c_c, pos + (i,), False])

            else:
                label = subterm.label
                for parent in candidates:
                    for r in self.td_query(parent, label):
                        children_candidates = {()}
                        rule = self.get_rule(r)
                        for i, child_state in enumerate(rule[2]):
                            next_candidates = set()
                            child_candidates = memory[pos + (i,)][child_state]
                            for prefix in children_candidates:
                                for suffix in child_candidates:
                                    next_candidates.add(prefix + suffix)

                            children_candidates = next_candidates

                        memory[pos][parent].update(children_candidates)

        return memory[()]

    def run_bu(self, term, children_states):
        root_out = []

        def reduce(l, children_values):
            # print('reducing %s with %s' % (l, str(children_values)))

            reachable_states = set()
            for children_states_here in cartesian_product(*children_values):
                # print('trying combinaison: %s' % str(children_states_here))
                rules = [self.get_rule(r_name) for r_name in self.bu_query(children_states_here, l)]
                # print('rules: %s' % str(rules))
                reachable_states.update({rule[0] for rule in rules})
                # print('reached: %s' % str(reachable_states))
            return reachable_states

            # We'll use closure here to keep track of children values via continuations
            # (on cv_cls which contains reachable states for each child node)
        def collect_cont(cv_cls):
            return lambda states: cv_cls.append(states)

        # to compose continuations (for the last sibling)
        def f_reduce_g_cont(l, f, g, cv_cls):
            def the_cont(states):
                # f intended to affect cv through side effect (for efficiency)
                # Hence not a 'classic' composition
                f(states)
                g(reduce(l, cv_cls))

            return the_cont

        agenda = [(term, collect_cont(root_out))]
        args_it = iter(children_states)

        while agenda:
            # print(agenda)
            next_item = agenda.pop()
            if next_item[0].get_var_id() is not None:
                # apply continuation
                # print('applying cont')
                next_item[1](next(args_it))
            elif next_item[0].is_const():
                next_item[1](reduce(next_item[0].label, []))
            else:
                label = next_item[0].label
                # new list of children values
                cv = []
                # get the children
                children = next_item[0].children

                # last sibling will be followed by a reduce
                # compose continuations
                agenda.append((children[-1], f_reduce_g_cont(label, collect_cont(cv), next_item[1], cv)))

                for i in range(-2, -len(children) - 1, -1):
                    # for these just collect
                    agenda.append((children[i], collect_cont(cv)))

        return root_out[0]

    def decode_rule(self, rule):
        return (self.states_decoder.decode(rule[0]),
                self.labels_decoder.decode(rule[1]),
                tuple(self.states_decoder.decode(child_id) for child_id in rule[2]))

    def decode_rules(self):
        return {self.rules_decoder.decode(r_id): self.decode_rule(rule) for r_id, rule in self.all_rules()}

    def __str__(self):
        return rulemap_as_string(self.decode_rules())


def rulemap_as_string(rulemap):
    return '\n'.join(['%s -> %s%s' % tr for tr in rulemap.values()])


def compile_automaton(rulemap, final, labels_encoder):
    states_encoder = DynamicEncoder()
    rules_encoder = DynamicEncoder()
    labels_encoder = labels_encoder
    coded_rules = [None] * len(rulemap)

    for (name, rule) in rulemap.items():
        label = labels_encoder.encode(rule[1])
        parent_id = states_encoder.encode(rule[0])
        children_ids = tuple(states_encoder.encode(child) for child in rule[2])
        rule_id = rules_encoder.encode(name)
        coded_rules[rule_id] = (parent_id, label, children_ids)

    final_code = states_encoder.encode(final)

    core = core_compile(states_encoder.next_id, labels_encoder.next_id, coded_rules, final_code)
    return PyTA(
        core, final_code,
        states_encoder.next_id, rules_encoder.next_id, labels_encoder.next_id,
        states_encoder,
        rules_encoder,
        labels_encoder
        )


# FIXME: will probably not work for anything else that string automaton
def invhom_(h, a, lhs_encoder, rhs_encoder):
    rulemap = {}
    cache = {}

    for (l, im) in h.items():
        if im not in cache:
            trs = []
            remapped = im.remap(rhs_encoder)
            var_list = remapped.get_vars()
            candidates = tuple([a.states] * len(var_list))
            for candidate in cartesian_product(*candidates):
                # print('candidate: ', candidate)
                reached = a.run_bu(remapped, ({state} for state in candidate))
                # print('parent: ', reached)
                trs.append((candidate, reached))
            cache[im] = trs

        trs = cache[im]
        for (candidate, reached) in trs:
            rulemap.update(
                {'<%s[%s to %s]>' % (l, parent_state, str(candidate)): (parent_state, l, candidate)
                 for parent_state in reached}
            )

    return compile_automaton(rulemap, a.final, lhs_encoder)


def invhom(h, a, lhs_encoder, rhs_encoder):
    rulemap = {}
    cache = {}

    for (l, im) in h.items():
        if im not in cache:
            trs = []
            remapped = im.remap(rhs_encoder)
            candidates = a.states
            inv_rules = a.run_td(remapped, candidates)
            for candidate in candidates:
                for reached in inv_rules[candidate]:
                    trs.append((candidate, reached))

            cache[im] = trs

        trs = cache[im]
        for (candidate, reached) in trs:
            rulemap['<%s[%s to %s]>' % (l, candidate, str(reached))] = (candidate, l, reached)

    return compile_automaton(rulemap, a.final, lhs_encoder)


def inter(a1, a2, labels_decoder, nsymb):
    core_inter, final, decode_s, decode_r = core_intersect(a1.core, a2.core)
    return PyTA(
        core_inter, final,
        len(decode_s), len(decode_r),
        nsymb, StaticDecoder(decode_s),
        StaticDecoder(decode_r), labels_decoder
    )


def inter_ac(a1, a2, labels_decoder, nsymb):
    core_inter, final, decode_s, decode_r = core_intersect_ac(a1.core, a2.core)
    return PyTA(
        core_inter, final,
        len(decode_s), len(decode_r),
        nsymb, StaticDecoder(decode_s),
        StaticDecoder(decode_r), labels_decoder
    )


def keep_existing(x, y):
    if x is not None:
        return x
    else:
        return y


# by default, will forward the score map that is not None to the intersection
def scores_intersection(product_a, scores_left=None, scores_right=None, op=keep_existing):
    def get_score(scores, name):
        if scores is not None:
            return scores[name]
        else:
            return None

    return {
        rule: op(
            get_score(scores_left, product_a.rules_decoder.decode(rule)[0]),
            get_score(scores_right, product_a.rules_decoder.decode(rule)[1])
        )
        for rule in range(product_a.n_rules)
    }


def _init_n_deps(rules, r_to_n, n_to_r, safe):
    for i, rule in rules:
        n = len([state for state in set(rule[2]) if state not in safe])
        r_to_n[i] = n
        n_to_r[n].add(i)


def _index_by_children(rules):
    child_index = defaultdict(set)

    for i, rule in rules:
        for state in rule[2]:
            child_index[state].add(i)

    return child_index


def prune_dead_states_pure(ta, max_arity):
    rules = ta.all_rules()
    rule_corresp = [-1 for _ in range(ta.n_rules)]
    n_pruned_rules = 0
    safe = set([ta.get_rule(r)[0] for r in ta.bu_all(())])
    r_to_n = [0 for _ in range(ta.n_rules)]
    n_to_r = [set() for _ in range(max_arity + 1)]
    _init_n_deps(rules, r_to_n, n_to_r, safe)
    child_index = _index_by_children(rules)

    pruned_rm = {}
    while n_to_r[0]:
        rule_index = n_to_r[0].pop()
        rule = ta.get_rule(rule_index)
        safe_state = rule[0]
        pruned_rm[n_pruned_rules] = rule
        rule_corresp[n_pruned_rules] = rule_index
        n_pruned_rules += 1

        if safe_state not in safe:
            safe.add(safe_state)
            for rule_to_update in child_index[safe_state]:
                old_value = r_to_n[rule_to_update]
                r_to_n[rule_to_update] = old_value - 1
                n_to_r[old_value].remove(rule_to_update)
                n_to_r[old_value - 1].add(rule_to_update)

    return pruned_rm, rule_corresp


def prune_dead_states(ta, max_arity):
    pruned_rm, corresp = prune_dead_states_pure(ta, max_arity)

    pruned_decoded_rm = {ta.rules_decoder.decode(corresp[r_id]): ta.decode_rule(rule)
                         for r_id, rule in pruned_rm.items()}
    return compile_automaton(pruned_decoded_rm, ta.decode_final(), ta.labels_decoder)


def _generate_pure(ta, weights, state, rng, use_prob=True):
    rules = ta.td_all(state)
    indices = range(len(rules))
    p = [weights[r] for r in rules]
    if not use_prob:
        p = None
    chosen_index = rng.choice(indices, p=p)
    choice = rules[chosen_index]
    logit_choice = np.log(weights[choice])
    children_states = ta.get_rule(choice)[2]

    children_and_logits = [_generate_pure(ta, weights, child_state, rng=rng, use_prob=use_prob)
                           for child_state in children_states]
    children = [c_l[0] for c_l in children_and_logits]
    all_children_logit = np.sum([c_l[1] for c_l in children_and_logits])
    logit = logit_choice + all_children_logit

    return Term(label=choice, children=children), logit


def generate_pure(ta, weights, states, rng, use_prob=True, n_samples=1):
    for _ in range(n_samples):
        yield _generate_pure(ta, weights, states, rng, use_prob)


def derive(ta, dt):
    label = ta.labels_decoder.decode(ta.get_rule(int(dt.label))[1])
    return Term(label=label, children=[derive(ta, child) for child in dt.children])


def generate(ta, weights, state, rng, use_prob=True, n_samples=1):
    for dt, logit in generate_pure(ta, weights, state, rng, use_prob, n_samples):
        derived_t = derive(ta, dt)
        yield dt, derived_t, logit
