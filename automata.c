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

typedef struct explicit_automaton *explicit_automaton;
struct explicit_automaton{
  struct automaton head;
  int *rules_mat;
  int max_w;
  int n_rules;
  void *td_index;
  void *bu_index;
};

typedef struct explicit_all_rules_iterator *explicit_all_rules_iterator;
struct explicit_all_rules_iterator{
  struct rule_iterator head;
  int n_rules;
  int current_rule;
};

typedef struct explicit_all_rules_ruleset *explicit_all_rules_ruleset;
struct explicit_all_rules_ruleset{
  struct ruleset head;
  int n_rules;
};

typedef struct explicit_rule_iterator *explicit_rule_iterator;
struct explicit_rule_iterator{
  struct rule_iterator head;
  dict_iterator d_it;
};

typedef struct explicit_ruleset *explicit_ruleset;
struct explicit_ruleset{
  struct ruleset head;
  void *map;
};

typedef struct explicit_label_to_ruleset *explicit_label_to_ruleset;
struct explicit_label_to_ruleset{
  struct label_to_ruleset head;
  void *map;
};

void print_int_array(int *array, int width);

int explicit_rule_storage_size(int max_width){
  return max_width + 4;
}

int max_rule_width(struct rule *rules, int n_rules){
  int max_w = 0;
  int current_w;
  for(int i = 0; i < n_rules; ++i){
    current_w = rules[i].width; 
    if(current_w >= max_w){
      max_w = current_w;
    }
  }
  return max_w;
}

void explicit_rule_iterator_destroy(rule_iterator it){
  explicit_rule_iterator e_it = (explicit_rule_iterator)it;
  if(e_it->d_it != NULL){
    e_it->d_it->destroy(e_it->d_it);
  }
  free(e_it);
}

int *explicit_rule_iterator_next(rule_iterator it){
  int *next_rule = NULL;
  dict_iterator d_it = ((explicit_rule_iterator)it)->d_it;
  if(d_it != NULL){
    dict_item next = d_it->next(d_it);    
    if (next != NULL){
      next_rule = next->key;
    }
  }
  return next_rule;
}

rule_iterator explicit_rule_iterator_create(const ruleset rs){
  dict_iterator d_it = NULL;
  explicit_ruleset e_rs = (explicit_ruleset)rs;
  explicit_rule_iterator wrapped_it = malloc(sizeof(struct explicit_rule_iterator));
  if(e_rs->map != NULL){
    d_it = dict_items(e_rs->map);
  }
  wrapped_it->d_it = d_it;
  wrapped_it->head.destroy = &explicit_rule_iterator_destroy;
  wrapped_it->head.next = &explicit_rule_iterator_next;
  return (rule_iterator)wrapped_it;
}

/* just wrap a call to get_item to allow the argument dictionary to be a NULL pointer
 * returns NULL if that's the case
 */
void *wrap_get_item(void *map, void *key){
  if(map == NULL){
    return NULL;
  }
  else{
    return get_item(map, key);
  }
}

void explicit_ruleset_destroy(ruleset wrapped_map){
  free(wrapped_map);
}

ruleset explicit_label_to_ruleset_query(const label_to_ruleset wrapped_map, int label){
  explicit_label_to_ruleset w_outer_map = (explicit_label_to_ruleset)wrapped_map;
  explicit_ruleset w_inner_map = malloc(sizeof(struct explicit_ruleset));
  void *inner_map = wrap_get_item(w_outer_map->map, &label);
  w_inner_map->map = inner_map;
  w_inner_map->head.create_iterator = &explicit_rule_iterator_create;
  w_inner_map->head.destroy = &explicit_ruleset_destroy;
  return (ruleset)w_inner_map;
}

void explicit_label_to_ruleset_destroy(label_to_ruleset wrapped_map){
  free(wrapped_map);
}

void explicit_label_to_ruleset_init(explicit_label_to_ruleset target, void *map){
  target->map = map;
  target->head.query = &explicit_label_to_ruleset_query;
  target->head.destroy = &explicit_label_to_ruleset_destroy;
}

label_to_ruleset explicit_td_query(const automaton a, int parent_state){
  explicit_automaton explicit = (explicit_automaton)a;
  explicit_label_to_ruleset wrapped_map = malloc(sizeof(struct explicit_label_to_ruleset));
  void *map = get_item(explicit->td_index, &parent_state);
  explicit_label_to_ruleset_init(wrapped_map, map);
  return (label_to_ruleset)wrapped_map;
}

label_to_ruleset explicit_bu_query(const automaton a, int *children, int width){
  explicit_automaton explicit = (explicit_automaton)a;
  explicit_label_to_ruleset wrapped_map = malloc(sizeof(struct explicit_label_to_ruleset));
  int *key = malloc((width + 1) * sizeof(int));
  key[0] = width;
  memcpy(key + 1, children, width * sizeof(int));
  void *map = get_item(explicit->bu_index, key);
  free(key);
  explicit_label_to_ruleset_init(wrapped_map, map);
  return (label_to_ruleset)wrapped_map;
}

void explicit_set_rule_from_row(int *row, struct rule *target){
  target->parent = row[1];
  target->label = row[2];
  target->width = row[3];
  target->children = row + 4;
}

void explicit_fill_rule(const automaton a, struct rule *target, int rule_index){
  explicit_automaton explicit = (explicit_automaton)a;
  int s_size = explicit_rule_storage_size(explicit->max_w);
  int *row = explicit->rules_mat + rule_index * s_size;
  explicit_set_rule_from_row(row, target);
}

void explicit_all_rules_iterator_destroy(rule_iterator it){
  free(it);
}

int *explicit_all_rules_iterator_next(rule_iterator it){
  explicit_all_rules_iterator explicit = (explicit_all_rules_iterator)it;
  if(++explicit->current_rule < explicit->n_rules){
    return &(explicit->current_rule);
  }
  else{
    return NULL;
  }
}

rule_iterator explicit_all_rules_iterator_create(ruleset rs){
  explicit_all_rules_iterator it = malloc(sizeof(struct explicit_all_rules_iterator));
  it->n_rules = ((explicit_all_rules_ruleset) rs)->n_rules;
  it->current_rule = -1;
  it->head.next=explicit_all_rules_iterator_next;
  it->head.destroy=&explicit_all_rules_iterator_destroy;
  return (rule_iterator)it;
}

void explicit_all_rules_ruleset_destroy(ruleset rs){
  free(rs);
}

ruleset explicit_all_rules(const automaton a){
  explicit_all_rules_ruleset rs = malloc(sizeof(struct explicit_all_rules_ruleset));
  rs->head.create_iterator = &explicit_all_rules_iterator_create;
  rs->head.destroy = &explicit_all_rules_ruleset_destroy;
  rs->n_rules = a->n_rules;
  return (ruleset)rs;
}

void explicit_destroy(automaton a){
  explicit_automaton explicit = (explicit_automaton)a;
  dict_destroy(explicit->td_index);
  dict_destroy(explicit->bu_index);
  free(explicit->rules_mat);
  free(explicit);
}

automaton create_explicit_automaton(int n_states, int n_symb, struct rule *rules, int n_rules){
  int max_w = max_rule_width(rules, n_rules);
  int s_size = explicit_rule_storage_size(max_w);
  int *rules_mat = malloc(n_rules * s_size * sizeof(int));
  struct rule *current_rule  = NULL;
  int *row = NULL;
  void *td_index = NULL;
  void *bu_index  = NULL;
  explicit_automaton a = malloc(sizeof(struct explicit_automaton));
  
  for(int r = 0; r < n_rules; ++r){
    current_rule = rules + r;
    row = rules_mat + r*s_size;
    // store the rule index as its name
    row[0] = r;
    row[1] = current_rule->parent;
    row[2] = current_rule->label;
    row[3] = current_rule->width;
    memcpy(row + 4, current_rule->children, current_rule->width * sizeof(int));
  }

  td_index = dict_create(&PARENT_MAP_P);
  bu_index = dict_create(&CHILDREN_MAP_P);
  
  a->rules_mat = rules_mat;
  a->max_w = max_w;
  a->n_rules = n_rules;
  a->td_index = td_index;
  a->bu_index = bu_index;

  a->head.td_query = &explicit_td_query;
  a->head.bu_query = &explicit_bu_query;
  a->head.fill_rule = &explicit_fill_rule;
  a->head.all_rules = &explicit_all_rules;
  a->head.destroy = &explicit_destroy;
  a->head.n_states = n_states;
  a->head.n_symb = n_symb;
  a->head.n_rules = n_rules;

  return (automaton)a;
}

void *get_or_create(void *d, void *key, const struct item_type_parameters *nested_dict_params){
  void *nested_dict = get_item(d, key);
  if(nested_dict == NULL){
    nested_dict = dict_create(nested_dict_params);
    set_item(d, key, nested_dict);
  }
  return nested_dict;
}

void build_td_index_from_explicit(const automaton a){
  explicit_automaton explicit = (explicit_automaton)a;
  int s_size = explicit_rule_storage_size(explicit->max_w);
  int *rules_mat = explicit->rules_mat;
  for(int i = 0; i < explicit->n_rules; ++i){
    int *rule = rules_mat + i*s_size;
    int *parent = rule + 1;
    int *label = rule + 2;
    void *l_to_rs = get_or_create(explicit->td_index, parent, &LABEL_MAP_P);
    void *rs = get_or_create(l_to_rs, label, &RULE_SET_P);
    set_item(rs, rule, NULL);
  }
}

void build_bu_index_from_explicit(const automaton a){
  explicit_automaton explicit = (explicit_automaton)a;
  int s_size = explicit_rule_storage_size(explicit->max_w);
  int *rules_mat = explicit->rules_mat;
  for(int i = 0; i < explicit->n_rules; ++i){
    int *rule = rules_mat + i*s_size;
    int *label = rule + 2;
    int *width_and_children = rule + 3;
    void *l_to_rs = get_or_create(explicit->bu_index, width_and_children, &LABEL_MAP_P);
    void *rs = get_or_create(l_to_rs, label, &RULE_SET_P);
    set_item(rs, rule, NULL);
  }
}

int children_cmp_fn(const void *k1, const void *k2){
  
  int *actual_k1 = (int *)k1;
  int *actual_k2 = (int *)k2;
 
  int cmp = 0;
  int s1 = actual_k1[0];
  int s2 = actual_k2[0];
  int size=s1;
  if(s1 > s2){
    size = s2;
  }
  
  for(int index = 1; index <= size; ++index){
    cmp = int_cmp_fn(actual_k1 + index, actual_k2 + index);
    if(cmp != 0){
      return cmp;
    }
  }
  return s1 - s2;
}

void init_rule(int parent, int label, int *children, int width, struct rule *target){
  struct rule r = {
    parent,
    label,
    children,
    width
  };
  *target = r;
}

void print_int_array(int *array, int width){
  fprintf(stderr, "[");
  for(int j = 0; j < width; ++j){
    fprintf(stderr, "%d", array[j]);
    if(j < width - 1){
      fprintf(stderr, ", ");
    }
  }
  fprintf(stderr, "]\n");
}
