#include "automata.h"
#include <stdlib.h>
#include "int_dict.h"



int max_rule_width(rule *rules, int n_rules){
  int max_w = 0;
  int current_w;
  for(int i = 0; i < n_rules; ++i){
    current_w = rules[i]->width; 
    if(current_w >= max_w){
      max_w = current_w;
    }
  }
  return max_w;
}


struct concrete_automaton_data{
  int *rules_mat;
  int n_rules;
  void *td_index;
  void *bu_index;
};


automaton create_concrete_automaton(int n_states, int n_symb, rule *rules, int n_rules){
  int max_w = max_rule_width(rules, n_rules);
  int c_size = max_w + 2;
  int *data = malloc(sizeof(int[n_rules][c_size]));
  int *sizes = malloc(sizeof(int[n_rules]));
  
  
  for(int r = 0; r < n_rules; ++r){
    current_rule = rules[r];
    row = data + r*c_size;
    row[0] = current_rule->name;
    row[1] = current_rule->parent_state;
    memcpy(row[2], current_rule->children, sizeof(int[current_rule->width]));
    sizes[r] = current_rule->width;
  }

  a = malloc(sizeof(struct automaton));
 
  
}

label_to_ruleset concrete_td_query(automaton a, int parent_state){
}




