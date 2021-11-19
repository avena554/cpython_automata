#ifndef AUTOMATA_H
#define AUTOMATA_H

typedef struct rule_iterator *rule_iterator;
struct rule_iterator{
  int *(*next)(rule_iterator it);
  void (*destroy)(rule_iterator it);
};


typedef struct ruleset *ruleset;
struct ruleset{
  rule_iterator (*create_iterator)(const ruleset rs);
  void (*destroy)(ruleset rs);
};


typedef struct label_to_ruleset *label_to_ruleset;
struct label_to_ruleset{
  ruleset (*query)(const label_to_ruleset map, int label);
  void (*destroy)(label_to_ruleset l_to_rs); 
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
  label_to_ruleset (*td_query)(const automaton a, int parent_state);
  label_to_ruleset (*bu_query)(const automaton a, int *children, int width);
  void (*fill_rule)(const automaton a, rule target, int rule_index);
  ruleset (*all_rules)(const automaton a);
  void (*destroy)(automaton a);
  int n_states;
  int n_rules;
  int n_symb;
};

void init_rule(int parent, int label, int *children, int width, rule target);

automaton create_explicit_automaton(int n_states, int n_symb, rule *rules, int n_rules);
void build_td_index_from_explicit(const automaton a);
void build_bu_index_from_explicit(const automaton a);

int children_cmp_fn(const void *k1, const void *k2);

#endif /*automata.h*/
