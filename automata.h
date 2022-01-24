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

struct index_pair{
  int first;
  int second;
};

/*
 * Important: it is assumed that an automaton (even a lazy one)
 * is always responsible for allocating and deallocating its rules.
 * so if a lazy implementation returns a rule via td or bu query
 * he is still responsible for deallocating this rule upon its own deallocation.
 * 
 * as a corollary, the children field of the struct rule below, needs not be allocated
 * when fed to the fill_rule function.
 */
struct rule{
  int parent;
  int label;
  int width;
  int *children;
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
  int final;
};


struct intersection{
  automaton a;
  void *state_decoder; //decode state indices as state pairs (from the intersected automata)
  void *rule_decoder; // decode rule indices as rule pairs (from the intersected automata)
};



void init_rule(int parent, int label, int *children, int width, struct rule *target);


automaton create_explicit_automaton(int n_states, int n_symb, struct rule *rules, int n_rules, int final);

void build_td_index_from_explicit(const automaton a);

void build_bu_index_from_explicit(const automaton a);

void intersect_cky(const automaton a1, const automaton a2, struct intersection *target);

int children_cmp_fn(const void *k1, const void *k2);

#endif /*automata.h*/
