#ifndef AUTOMATA_H
#define AUTOMATA_H

typedef struct int_iterator int_iterator_hd;
typedef int_iterator_hd *int_iterator;
typedef int_iterator rule_iterator;

struct int_iterator{
  int *(*next)(int_iterator it);
  void (*destroy)(int_iterator it);
};


typedef struct intset intset_hd;
typedef intset_hd *intset;
typedef intset ruleset;

struct intset{
  int_iterator (*create_iterator)(const intset rs);
  void (*destroy)(intset rs);
};


typedef struct label_to_ruleset *label_to_ruleset;

struct label_to_ruleset{  
  intset (*labels)(const label_to_ruleset map);
  ruleset (*query)(const label_to_ruleset map, int label);
  rule_iterator (*all_rules)(const label_to_ruleset map);
  void (*destroy)(label_to_ruleset l_to_rs); 
};


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
  void (*fill_rule)(const automaton a, struct rule *target, int rule_index);
  ruleset (*all_rules)(const automaton a);
  void (*destroy)(automaton a);
  int n_states;
  int n_rules;
  int n_symb;
};



void init_rule(int parent, int label, int *children, int width, struct rule *target);


automaton create_explicit_automaton(int n_states, int n_symb, struct rule *rules, int n_rules);

void build_td_index_from_explicit(const automaton a);

void build_bu_index_from_explicit(const automaton a);


int children_cmp_fn(const void *k1, const void *k2);

#endif /*automata.h*/
