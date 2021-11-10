#ifndef AUTOMATA_H
#define AUTOMATA_H

struct int_iterator{
  void *data;
  int (*next)(int_iterator it);
  void (*destroy_data)(int_iterator it);
};
typedef struct int_iterator int_iterator;

struct ruleset{
  void *data;
  int_iterator (*create_iterator)(rule_set rs);
};
typedef struct ruleset ruleset;


struct label_to_ruleset{
  ruleset (*query)(label_to_ruleset map, int label);
};
typedef struct label_to_ruleset label_to_ruleset;

struct rule{
  int name;
  int parent_state;
  int *children;
  int width;
};
typedef struct rule *rule;

struct automaton{
  void *data;
  label_to_ruleset (*td_query)(automaton a, int parent_state);
  label_to_ruleset (*bu_query)(automaton a, int *children_states, int n_children);
  rule (*find_rule)(automaton a, int rule_name);
  ruleset (*all_rules)(automaton a);
  void (*destroy)(automaton a);
  int *n_states;
  int *n_symb;
};
typedef struct automaton *automaton;


rule create_rule(int parent, int label, int *children, int n_children);
void destroy_rule(rule r);

automaton create_concrete_automaton(int n_states, int n_symb, rule *rules, int n_rules);



#endif /*automata.h*/
