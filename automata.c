#include "automata.h"
#include <stdlib.h>
#include "int_dict.h"

const struct item_type_parameters PARENT_MAP_P = {
  &int_cmp_fn,
  NULL,
  &dict_destroy
};

const struct item_type_parameters CHILDREN_MAP_P = {
  &children_cmp_fn,
  NULL,
  &dict_destroy
};

struct children{
  int *data;
  int *size;  
};

struct concrete_automaton_data{
  int *rules_mat;
  int *sizes;
  int n_rules;
  void *td_index;
  void *bu_index;
};

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

automaton create_concrete_automaton(int n_states, int n_symb, rule *rules, int n_rules){
  int max_w = max_rule_width(rules, n_rules);
  int c_size = max_w + 2;
  int *rules_mat = malloc(sizeof(int[n_rules][c_size]));
  int *sizes = malloc(sizeof(int[n_rules]));
  
  
  for(int r = 0; r < n_rules; ++r){
    current_rule = rules[r];
    row = data + r*c_size;
    row[0] = current_rule->name;
    row[1] = current_rule->parent_state;
    memcpy(row[2], current_rule->children, sizeof(int[current_rule->width]));
    sizes[r] = current_rule->width;
  }

  struct concrete_automaton_data *data = malloc(sizeof(struct automaton));
  td_index = dict_create(PARENT_MAP_P);
  bu_index = dict_create(CHILDREN_MAP_P);
  
  data->rules_mat = rules_mat;
  data->sizes = sizes;
  data->n_rules = n_rules;
  data->td_index = td_index;
  data->bu_index = bu_index;

  automaton a = malloc(sizeof(struct automaton));
  a->data = data;
  a->td_query = &explicit_td_query;
  a->bu_query = &explicit_bu_query;
  a->find_rule = &explicit_find_rule;
  a->all_rules = &explicit_all_rules;
  a->n_states = n_states;
  a->n_symb = n_symb;

  return a;
}


void build_td_index_from_explicit(automaton a){

}

void build_bu_index_from_explicit(automaton a){
  
}


label_to_ruleset concrete_td_query(automaton a, int parent_state){
}

int children_cmp_fn(const void *k1, const void *k2){
  struct children *actual_k1 = (struct children *)k1;
  struct children *actual_k2 = (struct children *)k2;

  int cmp = 0;
  int index = 0;
  int s1 = k1->size;
  int s2 = k2->size;
  int size=s1;
  if(s2 > s1){
    size = s2;
  }
  
  while(index <= size && cmp=0){
    cmp = int_cmp_fn(k1->data[index], k2->data[index]);
  }
  return cmp;
}




