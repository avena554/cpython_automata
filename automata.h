#ifndef AUTOMATA_H
#define AUTOMATA_H

typedef struct int_iterator *int_iterator;
struct int_iterator{
  void *data;
  int *(*next)(int_iterator it);
  void (*destroy_data)(int_iterator it);
};


typedef struct ruleset *ruleset;
struct ruleset{
  void *data;
  void (*init_iterator)(const ruleset rs, int_iterator it);
};


typedef struct label_to_ruleset *label_to_ruleset;
struct label_to_ruleset{
  void *data;
  void (*query)(const label_to_ruleset map, int label, ruleset target);
};

typedef struct rule *rule;
struct rule{
  int parent;
  int label;
  int *children;
  int width;
};

typedef struct automaton *automaton;
struct automaton{
  void *data;
  void (*td_query)(const automaton a, int parent_state, label_to_ruleset l_to_rs);
  void (*bu_query)(const automaton a, int *children_states, int width, label_to_ruleset l_to_rs);
  void (*set_rule)(const automaton a, int rule_index, rule target);
  void (*all_rules)(const automaton a, ruleset target);
  void (*destroy)(automaton a);
  int n_states;
  int n_rules;
  int n_symb;
};

void init_rule(int parent, int label, int *children, int width, rule target);

automaton create_explicit_automaton(int n_states, int n_symb, rule *rules, int n_rules);
void build_td_index_from_explicit(const automaton a);

int children_cmp_fn(const void *k1, const void *k2);

#endif /*automata.h*/
