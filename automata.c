#include "automata.h"
#include "dict.h"
#include <string.h>
#include <stdlib.h>
//debug
#include <stdio.h>
#include "avl/avl.h"

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

const struct item_type_parameters LABEL_MAP_P = {
  &int_cmp_fn,
  NULL,
  &dict_destroy
};

const struct item_type_parameters RULE_SET_P = {
  &int_cmp_fn,
  NULL,
  NULL
};

struct children{
  int *data;
  int size;  
};

struct explicit_automaton_data{
  int *rules_mat;
  int *widths;
  int max_w;
  int n_rules;
  void *td_index;
  void *bu_index;
};

struct all_rules_iterator_data{
  int n_rules;
  int current_rule;
};

int explicit_rule_storage_size(int max_width){
  return max_width + 3;
}

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

void explicit_rule_iterator_destroy_data(int_iterator it){
  dict_iterator actual_iterator = (dict_iterator)it->data;
  (*(actual_iterator->destroy_data))(actual_iterator);
  free(actual_iterator);
}

int *explicit_rule_iterator_next(int_iterator it){
  dict_iterator actual_iterator = (dict_iterator)it->data;
  dict_item next = (*(actual_iterator->next))(actual_iterator);
  int *next_rule = NULL;
  if (next != NULL){
    next_rule = next->key;
  }
  return next_rule;
}

void explicit_rule_iterator_init(const ruleset rs, int_iterator it){
  // dynamic allocation here because we want to hide the dict-based implementation
  dict_iterator decorated = malloc(sizeof(struct dict_iterator));
  init_iterator(rs->data, decorated);
  it->data = decorated;
  it->destroy_data = &explicit_rule_iterator_destroy_data;
  it->next = &explicit_rule_iterator_next;
}

void explicit_ruleset_init(const label_to_ruleset map, int label, ruleset target){
  void *data = get_item(map, &label);
  target->data = data;
  target->init_iterator = &explicit_rule_iterator_init;
}

void explicit_td_query(const automaton a, int parent_state, label_to_ruleset l_to_rs){
  struct explicit_automaton_data *explicit = (struct explicit_automaton_data *)a->data;
  void *data = get_item(explicit->td_index, &parent_state);
  l_to_rs->data = data;
  l_to_rs->query = &explicit_ruleset_init;
}

void explicit_bu_query(const automaton a, int *children_states, int width, label_to_ruleset l_to_rs){
}

void explicit_set_rule_from_row(int *row, rule target, int width){
  target->parent = row[1];
  target->label = row[2];
  target->children = row + 3;
  target->width = width;
}

void explicit_set_rule(const automaton a, int rule_index, rule target){
  struct explicit_automaton_data *explicit = (struct explicit_automaton_data  *)a->data;
  int s_size = explicit_rule_storage_size(explicit->max_w);
  int *row = explicit->rules_mat + rule_index*s_size;
  explicit_set_rule_from_row(row, target, explicit->widths[rule_index]);
}

void explicit_all_rules_destroy_data(int_iterator it){
  free(it->data);
}

int *explicit_all_rules_next(int_iterator it){
  struct all_rules_iterator_data *data = (struct all_rules_iterator_data *)it->data;
  if(data->current_rule < data->n_rules){
    return &(data->current_rule);
  }
  else{
    return NULL;
  }
}

void explicit_all_rules_init_iterator(const ruleset rs, int_iterator it){
  struct all_rules_iterator_data *data = malloc(sizeof(struct all_rules_iterator_data));
  data->n_rules = ((automaton)rs->data)->n_rules;
  data->current_rule = 0;
  it->data=data;
  it->next=&explicit_all_rules_next;
  it->destroy_data=&explicit_all_rules_destroy_data;
}

void explicit_all_rules(const automaton a, ruleset target){
  target->data = a;
  target->init_iterator = &explicit_all_rules_init_iterator;
}

void explicit_destroy(automaton a){
  struct explicit_automaton_data *explicit = (struct explicit_automaton_data *)a->data;
  dict_destroy(explicit->td_index);
  dict_destroy(explicit->bu_index);
  free(explicit->widths);
  free(explicit->rules_mat);
  free(explicit);
  free(a);
}

automaton create_explicit_automaton(int n_states, int n_symb, rule *rules, int n_rules){
  int max_w = max_rule_width(rules, n_rules);
  int s_size = explicit_rule_storage_size(max_w);
  int *rules_mat = malloc(sizeof(int[n_rules][s_size]));
  int *widths = malloc(sizeof(int[n_rules]));
  rule current_rule  = NULL;
  int *row = NULL;
  void *td_index = NULL;
  void *bu_index  = NULL;
  
  for(int r = 0; r < n_rules; ++r){
    current_rule = rules[r];
    row = rules_mat + r*s_size;
    // store the rule index as its name
    row[0] = r;
    row[1] = current_rule->parent;
    row[2] = current_rule->label;
    memcpy(row + 3, current_rule->children, sizeof(int[current_rule->width]));
    widths[r] = current_rule->width;
  }

  struct explicit_automaton_data *data = malloc(sizeof(struct explicit_automaton_data));
  td_index = dict_create(&PARENT_MAP_P);
  bu_index = dict_create(&CHILDREN_MAP_P);
  
  data->rules_mat = rules_mat;
  data->widths = widths;
  data->max_w = max_w;
  data->n_rules = n_rules;
  data->td_index = td_index;
  data->bu_index = bu_index;

  automaton a = malloc(sizeof(struct automaton));
  a->data = data;
  a->td_query = &explicit_td_query;
  a->bu_query = &explicit_bu_query;
  a->set_rule = &explicit_set_rule;
  a->all_rules = &explicit_all_rules;
  a->destroy = &explicit_destroy;
  a->n_states = n_states;
  a->n_symb = n_symb;
  a->n_rules = n_rules;

  return a;
}

void *get_or_create(void *d, void *key, const struct item_type_parameters *nested_dict_params){
  fprintf(stderr, "inside call\n");
  fprintf(stderr, "%p\n", d);
  fprintf(stderr, "key: %d\n", *((int*)key));  
  void *nested_dict = get_item(d, key);
  fprintf(stderr, "after get_item\n");
  if(nested_dict == NULL){
    fprintf(stderr, "found NULL\n");
    nested_dict = dict_create(nested_dict_params);
    fprintf(stderr, "created\n");
    set_item(d, key, nested_dict);
    fprintf(stderr, "added\n");
  }
  return nested_dict;
}

void build_td_index_from_explicit(const automaton a){
  struct explicit_automaton_data *data = (struct explicit_automaton_data *)a->data;
  int s_size = explicit_rule_storage_size(data->max_w);
  int *rules_mat = data->rules_mat;
  fprintf(stderr, "got data\n");
  for(int i = 0; i < data->n_rules; ++i){
    int *rule = rules_mat + i*s_size;
    int *parent = rule + 1;
    int *label = rule + 2;
    fprintf(stderr, "%d: retrieving parent\n", i);
    void *l_to_rs = get_or_create(data->td_index, parent, &LABEL_MAP_P);
    fprintf(stderr, "%d: retrieving label\n", i);
    void *rs = get_or_create(l_to_rs, label, &RULE_SET_P);
    fprintf(stderr, "%d: adding rule\n", i);
    set_item(rs, rule, NULL);
  }
}

void build_bu_index_from_explicit(automaton a){
  
}

int children_cmp_fn(const void *k1, const void *k2){
  struct children *actual_k1 = (struct children *)k1;
  struct children *actual_k2 = (struct children *)k2;

  int cmp = 0;
  int index = 0;
  int s1 = actual_k1->size;
  int s2 = actual_k2->size;
  int size=s1;
  if(s2 > s1){
    size = s2;
  }
  
  while(index <= size && cmp == 0){
    cmp = int_cmp_fn(actual_k1->data + index, actual_k2->data + index);
  }
  return cmp;
}

void init_rule(int parent, int label, int *children, int width, rule target){
  struct rule r = {
    parent,
    label,
    children,
    width
  };
  *target = r;
}
