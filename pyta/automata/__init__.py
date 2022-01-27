import pyta
from pyta.util.misc import cartesian_product
from pyta.automata.pure_python import compile_pure, intersect_pure, intersect_ac_pure
from pyta.pyta import intersect as intersect_fast, compile as compile_fast, intersect_cky as intersect_ac_fast
from pyta.util.encoders import DynamicEncoder, StaticDecoder


def core_compile(n_states, n_symb, rules, final):
    if pyta.MODE is pyta.C:
        return compile_fast(n_states, n_symb, rules, final)
    elif pyta.MODE is pyta.Python:
        return compile_pure(n_states, n_symb, rules, final)
    else:
        raise NotImplementedError


def core_intersect(a1, a2):
    if pyta.MODE is pyta.C:
        return intersect_fast(a1, a2)
    elif pyta.MODE is pyta.Python:
        return intersect_pure(a1, a2)
    else:
        raise NotImplementedError


def core_intersect_ac(a1, a2):
    if pyta.MODE is pyta.C:
        return intersect_ac_fast(a1, a2)
    elif pyta.MODE is pyta.Python:
        return intersect_ac_pure(a1, a2)
    else:
        raise NotImplementedError


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

                for i in range(-2, -len(children) - 1):
                    # for these just collect
                    agenda.append((children[i], collect_cont(cv)))

        return root_out[0]

    def decode_rules(self):
        return {self.rules_decoder.decode(r_id): (self.states_decoder.decode(rule[0]),
                                                  self.labels_decoder.decode(rule[1]),
                                                  tuple(self.states_decoder.decode(child_id)
                                                        for child_id in rule[2]))
                for r_id, rule in self.all_rules()}

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


# FIXME: this is super false and will not work for anything else that string automaton
def invhom(h, a, lhs_encoder, rhs_encoder):
    rulemap = {}

    for (l, im) in h.items():
        remapped = im.remap(rhs_encoder)
        var_list = remapped.get_vars()
        candidates = tuple([a.states] * len(var_list))
        for candidate in cartesian_product(*candidates):
            # print('candidate: ', candidate)
            reached = a.run_bu(remapped, ({state} for state in candidate))
            # print('parent: ', reached)

            rulemap.update(
                {'<%s[%s to %s]>' % (l, parent_state, str(candidate)): (parent_state, l, candidate)
                 for parent_state in reached}
            )

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
