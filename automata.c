#include "automata.h"
#include "dict.h"
#include <string.h>
#include <stdlib.h>
#include "list.h"
#include "utils.h"
//debug
#include "debug.h"
#include "avl/avl.h"


/***************************************************
 * Dictionaries allocation & key comparison params *
 ***************************************************/

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

const struct item_type_parameters RULESET_P = {
  &int_cmp_fn,
  NULL,
  NULL
};

/************************************************
 * Concrete extensions of abstract header types *
 ************************************************/

/*
 * data structures for explicit automata
 */

/* 
 * explicit automaton 
 */
typedef struct explicit_automaton *explicit_automaton;
struct explicit_automaton{
  struct automaton head;
  int *rules_mat;
  int max_w;
  int n_rules;
  void *td_index;
  void *bu_index;
};

/*
 * iterator over all rules
 */
typedef struct explicit_all_rules_iterator *explicit_all_rules_iterator;
struct explicit_all_rules_iterator{
  int_iterator_hd head;
  int n_rules;
  int current_rule;
};

/*
 * ruleset containing all rules
 */
typedef struct explicit_all_rules_ruleset *explicit_all_rules_ruleset;
struct explicit_all_rules_ruleset{
  intset_hd head;
  int n_rules;
};

/*
 * common base type for iterators running over a dictionnary's (integer) keys
 */
typedef struct int_keyset_iterator *int_keyset_iterator;
struct int_keyset_iterator{
  int_iterator_hd head;
  dict_iterator d_it;
};

/*
 * common base type for set of integers stored as dictionary keys
 */
typedef struct int_keyset *int_keyset;
struct int_keyset{
  intset_hd head;
  void *map;
};

/*
 * result of an intermediate td or by query (with no label provided)
 */
typedef struct explicit_label_to_ruleset *explicit_label_to_ruleset;
struct explicit_label_to_ruleset{
  struct label_to_ruleset head;
  void *map;
};

/*
 * data for an iterator traversing every rules in every of the nested rulesets of the result of a td or bu query 
 * i.e. successively queries a label_to_ruleset for every label and traverse every rule of the returned ruleset 
 */
typedef struct flat_ruleset_iterator *flat_ruleset_iterator;
struct flat_ruleset_iterator{
  int_iterator_hd head;
  label_to_ruleset lmap;
  intset outer_keyset;
  int_iterator outer_keys;
  ruleset current_ruleset;
  rule_iterator current_iterator;
};


/*************
 * Utilities *
 **************/

/*
 * debug utility
 */
void print_int_array(int *array, int width){
  debug_msg("[");
  for(int j = 0; j < width; ++j){
    debug_msg("%d", array[j]);
    if(j < width - 1){
      debug_msg(", ");
    }
  }
  debug_msg("]\n");
}

/*
 * gives the space necessary to store one rule (in sizeof(int) units)
 */
int explicit_rule_storage_size(int max_width){
  return max_width + 4;
}

/*
 * find the maximum width of the table of rules provided as argument
 */
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

/*
 * just wraps a call to get_item to allow the argument dictionary to be a NULL pointer
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

/*
 * comparison function for the bu index 
 * (compares int tuple -- children states of different rules)
 */
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

/*
 * for querying dictionaries of dictionaries
 * will create, and store a new nested dictionary under 'key'
 * if the top-level dictionary does not contains an entry for 'key'
 */
void *get_or_create(void *d, void *key, const struct item_type_parameters *nested_dict_params){
  void *nested_dict = get_item(d, key);
  if(nested_dict == NULL){
    nested_dict = dict_create(nested_dict_params);
    set_item(d, key, nested_dict);
  }
  return nested_dict;
}

/*
 * intialize a rule with relevant attributes
 */
void init_rule(int parent, int label, int *children, int width, struct rule *target){
  struct rule r = {
    .parent = parent,
    .label = label,
    .width = width,
    .children = children,
  };
  *target = r;
}


/************************************
 * Explicit automata implementation *
 *************************************/

/*
 * int_keyset_iterator implementation
 */

void int_keyset_iterator_destroy(int_iterator it){
  int_keyset_iterator c_it = (int_keyset_iterator)it;
  if(c_it->d_it != NULL){
    c_it->d_it->destroy(c_it->d_it);
  }
  free(c_it);
}

int *int_keyset_iterator_next(int_iterator it){
  int *next_value = NULL;
  dict_iterator d_it = ((int_keyset_iterator)it)->d_it;
  if(d_it != NULL){
    dict_item next = d_it->next(d_it);    
    if(next != NULL){
      next_value = next->key;
    }
  }
  return next_value;
}

int_iterator int_keyset_iterator_create(const intset s){
  dict_iterator d_it = NULL;
  int_keyset c_s = (int_keyset)s;
  int_keyset_iterator wrapped_it = malloc(sizeof(struct int_keyset_iterator));
  if(c_s->map != NULL){
    d_it = dict_items(c_s->map);
  }
  wrapped_it->d_it = d_it;
  wrapped_it->head.destroy = &int_keyset_iterator_destroy;
  wrapped_it->head.next = &int_keyset_iterator_next;
  return (int_iterator)wrapped_it;
}

/*
 * int_keyset implementation
 */

void int_keyset_destroy(intset s){
  free(s);
}

/*
 * build an int_keyset from an underlying map (with int * keys).
 */
int_keyset int_keyset_from_map(void *map){
  int_keyset w_map = malloc(sizeof(struct int_keyset));
  w_map->map = map;
  w_map->head.create_iterator = &int_keyset_iterator_create;
  w_map->head.destroy = &int_keyset_destroy;
  return w_map;
}


/*
 * implementation of a ruleset containing all rules of a td or bu query (label_to_ruleset)
 */

void flat_ruleset_iterator_destroy(int_iterator it){
  flat_ruleset_iterator c_it = (flat_ruleset_iterator)it;
  c_it->outer_keyset->destroy(c_it->outer_keyset);
  c_it->outer_keys->destroy(c_it->outer_keys);
  if(c_it->current_ruleset != NULL){
    c_it->current_iterator->destroy(c_it->current_iterator);
    c_it->current_ruleset->destroy(c_it->current_ruleset);
  }
  free(c_it);  
}

int *flat_ruleset_iterator_next(int_iterator it){
  int *next_value = NULL;
  flat_ruleset_iterator c_it = (flat_ruleset_iterator)it;
  
  if(c_it->current_ruleset != NULL){
    next_value = c_it->current_iterator->next(c_it->current_iterator);
  }

  while(next_value == NULL){
    int *next_key = c_it->outer_keys->next(c_it->outer_keys);
    if(next_key == NULL){
      return NULL;
    }
    if(c_it->current_ruleset != NULL){
      c_it->current_iterator->destroy(c_it->current_iterator);
      c_it->current_ruleset->destroy(c_it->current_ruleset);
    }
    c_it->current_ruleset = c_it->lmap->query(c_it->lmap, *next_key);
    c_it->current_iterator = c_it->current_ruleset->create_iterator(c_it->current_ruleset);
    next_value = c_it->current_iterator->next(c_it->current_iterator);
  }
  return next_value;
}

void flat_ruleset_iterator_init(label_to_ruleset lmap, intset outer_keyset, flat_ruleset_iterator it){
  it->lmap = lmap;
  it->outer_keyset = outer_keyset;
  it->outer_keys = outer_keyset->create_iterator(outer_keyset);
  it->current_ruleset = NULL;
  it->current_iterator = NULL;
  it->head.next = &flat_ruleset_iterator_next;
  it->head.destroy = &flat_ruleset_iterator_destroy;
}


/*
 * td and bu queries' result implementation (label_to_ruleset)
 */

/*
 * queries for a given label
 */
ruleset explicit_label_to_ruleset_query(const label_to_ruleset wrapped_map, int label){
  explicit_label_to_ruleset w_outer_map = (explicit_label_to_ruleset)wrapped_map;
  void *inner_map = wrap_get_item(w_outer_map->map, &label);
  return (ruleset)int_keyset_from_map(inner_map);
}

void explicit_label_to_ruleset_destroy(label_to_ruleset wrapped_map){
  free(wrapped_map);
}

intset explicit_label_to_ruleset_labels(const label_to_ruleset lmap){
  return (intset)int_keyset_from_map(((explicit_label_to_ruleset)lmap)->map);
}

rule_iterator explicit_label_to_ruleset_all_rules(const label_to_ruleset lmap){
  flat_ruleset_iterator it = malloc(sizeof(struct flat_ruleset_iterator));
  flat_ruleset_iterator_init(lmap, lmap->labels(lmap), it);
  return (rule_iterator)it;
}

void explicit_label_to_ruleset_init(explicit_label_to_ruleset target, void *map){
  target->map = map;
  target->head.query = &explicit_label_to_ruleset_query;
  target->head.labels = &explicit_label_to_ruleset_labels;
  target->head.all_rules = &explicit_label_to_ruleset_all_rules;
  target->head.destroy = &explicit_label_to_ruleset_destroy;
}

/*
 * td and bu queries
 */

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


/*
 * decode a rule from the explicit automaton's storage
 */
void explicit_set_rule_from_row(int *row, struct rule *target){
  target->parent = row[1];
  target->label = row[2];
  target->width = row[3];
  target->children = row + 4;
}

/*
 * retieve a rule with given index from an explicit automaton a
 */
void explicit_fill_rule(const automaton a, struct rule *target, int rule_index){
  explicit_automaton explicit = (explicit_automaton)a;
  int s_size = explicit_rule_storage_size(explicit->max_w);
  int *row = explicit->rules_mat + rule_index * s_size;
  explicit_set_rule_from_row(row, target);
}


/*
 * implementation of an iterator over all rules in the automaton
 */

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
  it->head.next = explicit_all_rules_iterator_next;
  it->head.destroy = &explicit_all_rules_iterator_destroy;
  return (rule_iterator)it;
}


/*
 * implementation of a ruleset containing all rules of an automaton
 */

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

/*
 * implementation of an explicit automaton
 */

/*
 * deallocate automaton
 */
void explicit_destroy(automaton a){
  explicit_automaton explicit = (explicit_automaton)a;
  dict_destroy(explicit->td_index);
  dict_destroy(explicit->bu_index);
  free(explicit->rules_mat);
  free(explicit);
}

/*
 * allocate automaton and initialize rules but do not build td and bu indexes.
 */
automaton create_explicit_automaton(int n_states, int n_symb, struct rule *rules, int n_rules, int final){
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
    /*
 * store the rule index as its name
 */
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
  a->head.final = final;

  return (automaton)a;
}


/* 
 * functions for initializing the td and bu indexes, respectively
 */

void build_td_index_from_explicit(const automaton a){
  explicit_automaton explicit = (explicit_automaton)a;
  int s_size = explicit_rule_storage_size(explicit->max_w);
  int *rules_mat = explicit->rules_mat;
  for(int i = 0; i < explicit->n_rules; ++i){
    int *rule = rules_mat + i*s_size;
    int *parent = rule + 1;
    int *label = rule + 2;
    void *l_to_rs = get_or_create(explicit->td_index, parent, &LABEL_MAP_P);
    void *rs = get_or_create(l_to_rs, label, &RULESET_P);
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
    void *rs = get_or_create(l_to_rs, label, &RULESET_P);
    set_item(rs, rule, NULL);
  }
}


/*******************************************/
/* functions using (or producing) automata */
/*******************************************/


struct agenda_item{
  int state;
  int children_expanded;
  label_to_ruleset td_resp;
};

struct pairing_entry{
  int *lhs;
  label_to_ruleset bu_resp;
};

/*
 * auxiliary needed for intersect
 */
void list_elem_destroy(void *elem){
  llist_destroy((llist)elem, NULL);
  free(elem);
}

void pe_destroy(void *elem){
  struct pairing_entry *pe = (struct pairing_entry *)elem;
  free(pe->lhs);
  pe->bu_resp->destroy(pe->bu_resp);
  free(pe);
}

void list_pe_destroy(void *elem){
  llist_destroy((llist)elem, &pe_destroy);
  free(elem);
}

void rule_destroy(void *elem){
  struct rule *rule = (struct rule *)elem;
  free(rule->children);
  free(rule);
}

int index_pair_cmp_fn(const void *k1, const void *k2){
  struct index_pair *v1 = (struct index_pair *)k1;
  struct index_pair *v2 = (struct index_pair *)k2;

  if(v1->first < v2->first){
    return -1;
  }else{
    if(v1->first > v2->first){
      return 1;
    }else{
      return int_cmp_fn(&(v1->second), &(v2->second));
    }
  }
}

const struct item_type_parameters state_pairing_params = {
  .key_cmp_fn = &int_cmp_fn,
  .key_delete_fn = NULL,
  .elem_delete_fn = &list_elem_destroy
};
const struct item_type_parameters children_pairing_params = {
  .key_cmp_fn = &children_cmp_fn,
  .key_delete_fn = &free,
    .elem_delete_fn = &list_pe_destroy
};
const struct item_type_parameters product_remap_params = {
  .key_cmp_fn = &index_pair_cmp_fn,
    .key_delete_fn = NULL,
    .elem_delete_fn = NULL
};
const struct item_type_parameters reverse_remap_params = {
  .key_cmp_fn = &int_cmp_fn,
  .key_delete_fn = &free,
  .elem_delete_fn = &free
};

/*
 * TODO: CUT THIS NONSENSE INTO PIECES !
 * intersects two explicit automata
 * the automata are assumed to share a common vocabulary
 */
void intersect_cky(const automaton a1, const automaton a2, struct intersection *target){

  struct llist agenda;
    
  void *state_pairs = dict_create(&state_pairing_params);
  int *states1 = malloc(a1->n_states * sizeof(int));
  int *states2 = malloc(a2->n_states * sizeof(int));

  // list of rules for the intersection automaton
  struct llist product_rules;
  llist_epsilon_init(&product_rules);
  
  // remapping from product states to new indices
  int n_pstates = 0;
  void *ps_remap = dict_create(&product_remap_params);
  void *reverse_ps = dict_create(&reverse_remap_params);
  // map from product rules to old rules
  int n_pr = 0;
  void *reverse_pr = dict_create(&reverse_remap_params);

  // prep states values in memory
  for(int s = 0 ; s < a1->n_states ; ++s) states1[s] = s;
  for(int s = 0 ; s < a2->n_states ; ++s) states2[s] = s;  
  
  llist_epsilon_init(&agenda);
  struct agenda_item *start = malloc(sizeof(struct agenda_item));
  start->state = a2->final;
  start->children_expanded = 0;
  start->td_resp = NULL;
  llist_insert_left(&agenda, start);

  while(!llist_is_empty(&agenda)){
    debug_msg("popping next rhs state\n");
    struct agenda_item *item = (struct agenda_item *)llist_popleft(&agenda);

    debug_msg("next rhs state is %d\n", item->state);

    // check if state has been expanded already
    if(get_item(state_pairs, &(item->state)) == NULL){
      debug_msg("state has not been expanded\n");
      if(!item->children_expanded){
	debug_msg("\tchildren have not yet been expanded\n");

	label_to_ruleset td_resp = a2->td_query(a2, item->state);
	debug_msg("\ta2 has been queried for state %d\n", item->state);
	
	// Push parent state back for pairing after children are expanded
	struct agenda_item *continuation = malloc(sizeof(struct agenda_item));
	continuation->state = item->state;
	continuation->children_expanded = 1;
	continuation->td_resp = td_resp;
	llist_insert_left(&agenda, continuation);
	debug_msg("\tstate has been pushed back for resuming it's expansion after children's expansion\n");
	
	//Push all children states for expansion	
	rule_iterator x_rules = td_resp->all_rules(td_resp);	
	for(int *rule_index = x_rules->next(x_rules); rule_index != NULL; rule_index=x_rules->next(x_rules)){
	  struct rule r;
	  a2->fill_rule(a2, &r, *rule_index);
	  debug_msg("\tfound rule with arity %d\n", r.width);
	  for(int c_index = 0; c_index < r.width; ++c_index){
	    struct agenda_item *child_item = malloc(sizeof(struct agenda_item));
	    child_item->state = r.children[c_index];
	    child_item->children_expanded = 0;
	    child_item->td_resp = 0;
	    debug_msg("\t\tpushing child %d\n", child_item->state);
	    llist_insert_left(&agenda, child_item);
	  }
	}
	x_rules->destroy(x_rules);
	debug_msg("\tchildren have been pushed for expansion\n");
		
      }else{
	debug_msg("\tchildren have already been expanded\n");
	// init the list of pair states
	llist target_pairs = malloc(sizeof(struct llist));
	int *state_key = states2 + item->state;

	//init the list of paired lhs states
	llist_epsilon_init(target_pairs);
	set_item(state_pairs, state_key, target_pairs);
	
	label_to_ruleset td_resp = item->td_resp;
	void *children_pairing = dict_create(&children_pairing_params);

	debug_msg("\tfinding all rhs children tuple\n");
	// associate each children tuple with an empty list of children-pairing entries
	rule_iterator x_rules = td_resp->all_rules(td_resp);
	for(int *rule_index = x_rules->next(x_rules); rule_index != NULL; rule_index=x_rules->next(x_rules)){
	  struct rule r;	  
	  a2->fill_rule(a2, &r, *rule_index);
	  // prep a dictionary key
	  int *tuple_key = malloc((r.width  + 1) * sizeof(int));
	  tuple_key[0] = r.width;
	  memcpy(tuple_key + 1, r.children, r.width * sizeof(int));

	  debug_msg("\t\tfound children tuple\n");
	  print_int_array(tuple_key, r.width + 1);
	  
	  if(get_item(children_pairing, tuple_key) == NULL){
	    debug_msg("\t\t\tchildren tuple is seen for first time\n");
	    llist associated_pairs = malloc(sizeof(struct llist));
	    llist_epsilon_init(associated_pairs);
	    set_item(children_pairing, tuple_key, associated_pairs);
	  }else{
	    free(tuple_key);
	  }
	}
	x_rules->destroy(x_rules);
	
	debug_msg("\tfinding all lhs children tuples\n");
	// fill the list of pairs for every children tuple
	dict_iterator lhs_tuples = dict_items(children_pairing);
	for(dict_item di = lhs_tuples->next(lhs_tuples); di != NULL; di = lhs_tuples->next(lhs_tuples)){
	  int *indices;
	  int *out;
	  int width = ((int *)di->key)[0];
	  int *rhs = ((int *)di->key) + 1;

	  debug_msg("\trhs is: \n");
	  print_int_array(rhs, width);

	  if(width > 0){
	    const int **candidates = malloc(width * sizeof(int *));
	    int *n_pairs = malloc(width  * sizeof(int));
	    // for each lhs state, find allowed pairs
	    for(int index = 0; index < width; ++index){
	      debug_msg("\t\tcandidates for child %d\n", rhs[index]);
	      llist allowed_pairs = (llist)get_item(state_pairs, rhs + index);
	      n_pairs[index] = allowed_pairs->size;
	      debug_msg("\t\tthere are %d candidates\n", n_pairs[index]);
	      // convert the list to an array
	      int *candidates_index = malloc(allowed_pairs->size * sizeof(int));
	      if(!llist_is_empty(allowed_pairs)){
		int k = 0;
		for(ll_cell c = llist_first(allowed_pairs); c != allowed_pairs->sentinelle; c = c->next){		
		  candidates_index[k] = *((int *)(c->elem)); 
		  ++k;
		}
	      }
	      candidates[index] = candidates_index;
	      debug_msg("\t\tcandidates @ %d :\n", index);
	      print_int_array((int*)candidates[index], n_pairs[index]);
	    }
	    debug_msg("\tcartesian iterator\n");
	    // use cartesian iterator to range over all rhs pair tuples
	    struct ci_state lhs_it;
	    indices = malloc(width * sizeof(int));
	    out = malloc(width * sizeof(int));
	    ci_init(candidates, n_pairs, width, indices, out, &lhs_it);
	    //fprintf(stderr, "\titerator is initialized\n");
	    
	    for(int *tuple = ci_next(&lhs_it); tuple != NULL; tuple = ci_next(&lhs_it)){
	      // allocate a new array that will survive outside current scope.
	      
	      int *lhs = malloc(width * sizeof(int));
	      // copy the rhs tuple to the new array
	      memcpy(lhs, tuple, width * sizeof(int));
	      debug_msg("\t\tlhs tuple:\n");
	      print_int_array(lhs, width);
	      // make a bu query
	      label_to_ruleset bu_resp = a1->bu_query(a1, lhs, width);
	      debug_msg("\t\tbu_query has been made:\n");
	     
	      // allocate a pairing entry that will be used as key.
	      struct pairing_entry *pe = malloc(sizeof(struct pairing_entry));
	      pe->lhs = lhs;
	      pe->bu_resp = bu_resp;

	      // store in the list
	      debug_msg("\t\tstoring in list:\n");
	      llist_insert_right((llist)(di->elem), pe);	      	      
	    }
	    // clean temp ressources allocated for cartesian iteration
	    debug_msg("\tcleaning up iterator:\n");
	    free(n_pairs);
	    free(indices);
	    free(out);
	    for(int index = 0; index < width; ++index) free((int *)candidates[index]);
	    free(candidates);
	    debug_msg("\titerator cleaned.\n");
	  }else{
	    debug_msg("terminal rule case\n");
	    label_to_ruleset bu_resp = a1->bu_query(a1, NULL, width);
	    debug_msg("\t\tbu_query has been made:\n");
	    struct pairing_entry *pe = malloc(sizeof(struct pairing_entry));
	    pe->lhs = NULL;
	    pe->bu_resp = bu_resp;

	    // store in the list
	    debug_msg("\t\tstoring in list:\n");
	    llist_insert_right((llist)(di->elem), pe);
	  }
	}
	lhs_tuples->destroy(lhs_tuples);

	debug_msg("\tbuilding product rules\n");
	x_rules = td_resp->all_rules(td_resp);
	for(int *rhs_index = x_rules->next(x_rules); rhs_index != NULL; rhs_index=x_rules->next(x_rules)){
	  struct rule rhs_rule;	  
	  a2->fill_rule(a2, &rhs_rule, *rhs_index);
	  // get the list of pairing entries for the 
	  
	  // prep a dictionary key
	  int *tuple_key = malloc((rhs_rule.width + 1) * sizeof(int));
	  tuple_key[0] = rhs_rule.width;
	  memcpy(tuple_key + 1, rhs_rule.children, rhs_rule.width * sizeof(int));

	  debug_msg("\t\trhs width and children:\n");
	  print_int_array(tuple_key, rhs_rule.width + 1);
	  
	  llist pes = get_item(children_pairing, tuple_key);
	  free(tuple_key);

	  debug_msg("\t\tentries queried\n");
	  
	  // loop over all pes
	  if(!llist_is_empty(pes)){
	    debug_msg("\t\tthere are entries:\n");
	    for(ll_cell c = llist_first(pes); c != pes->sentinelle; c = c->next){		
	      struct pairing_entry *pe = (struct pairing_entry *)(c->elem);
	      debug_msg("\t\tlhs children\n");
	      print_int_array(pe->lhs, rhs_rule.width);
		
	      debug_msg("\t\tquerying for label %d:\n", rhs_rule.label);
	      ruleset lhs_rules = pe->bu_resp->query(pe->bu_resp, rhs_rule.label);
	      debug_msg("\t\tdone, iterating over lhs rules\n");
	      rule_iterator lhs_it = lhs_rules->create_iterator(lhs_rules);
	      for(int *lhs_index = lhs_it->next(lhs_it); lhs_index != NULL; lhs_index = lhs_it->next(lhs_it)){
		struct rule lhs_rule;
		a1->fill_rule(a1, &lhs_rule, *lhs_index);
		debug_msg("\t\t\tlhs parent:  %d\n", lhs_rule.parent);

		// remap the parent product state
		debug_msg("\t\t\tremaping (%d, %d)\n", lhs_rule.parent, rhs_rule.parent);
		struct index_pair *product_parent = malloc(sizeof(struct index_pair));
		product_parent->first = lhs_rule.parent;
		product_parent->second = rhs_rule.parent;
		int *remapped_index =  get_item(ps_remap, product_parent);
		if(remapped_index == NULL){
		  debug_msg("\t\t\tneed to remap to %d\n", n_pstates);
		  remapped_index = malloc(sizeof(int));
		  *remapped_index = n_pstates;
		  set_item(ps_remap, product_parent, remapped_index);
		  set_item(reverse_ps, remapped_index, product_parent);
		  ++n_pstates;		  
		  
		  // add to allowed pairs for rhs state
		  debug_msg("\t\t\tadding to state pairs\n");
		  llist allowed_pairs = (llist)get_item(state_pairs, &(rhs_rule.parent));
		  llist_insert_right(allowed_pairs, states1 + lhs_rule.parent);
		  debug_msg("\t\t\tdone\n");
		}else{
		  free(product_parent);
		}
		debug_msg("\t\t\tremapped index is %d\n", *remapped_index);
		// make and add product rules
		struct rule *product_rule = malloc(sizeof(struct rule));
		product_rule->parent = *remapped_index;
		product_rule->label = lhs_rule.label;
		product_rule->width = lhs_rule.width;
		product_rule->children = malloc(lhs_rule.width * sizeof(int));

		// retrieve each children pair
		for(int k = 0; k < lhs_rule.width; ++k){
		  struct index_pair product_child = {
		    .first = lhs_rule.children[k],
		    .second = rhs_rule.children[k]
		  };
		  debug_msg("\t\t\tlooking for index for child (%d, %d)\n", product_child.first, product_child.second);
		  int *child_index = get_item(ps_remap, &product_child);
		  debug_msg("\t\t\tfound index %d\n", *child_index);
		  product_rule->children[k] = *child_index;
		}

		debug_msg("\t\twidth: %d\n", product_rule->width);
		debug_msg("\t\t%d->%d(", product_rule->parent, product_rule->label);
		print_int_array(product_rule->children, product_rule->width);
		debug_msg("\t\t\tinserting in res list\n");
		llist_insert_right(&product_rules, product_rule);
		debug_msg("\t\t\tdone\nnow mapping to old rules\n");
		
		// map to original rules
		struct index_pair *pr_index = malloc(sizeof(struct index_pair));
		int *rule_index = malloc(sizeof(int));

		pr_index->first = *lhs_index;
		pr_index->second = *rhs_index;
		*rule_index = n_pr;
		set_item(reverse_pr, rule_index, pr_index);
		++n_pr;
		debug_msg("\t\t\tdone, rule index is %d\n", *rule_index);
	      }
	      lhs_it->destroy(lhs_it);
	      lhs_rules->destroy(lhs_rules);
	    }
	  }
	}
	debug_msg("\tdestroying iterator\n");
	x_rules->destroy(x_rules);
	debug_msg("\tdone\n");
	//clean what needs to be cleaned
	dict_destroy(children_pairing);
      }
    }
    debug_msg("\tcleaning up agenda item\n");
    if(item->td_resp != NULL){
      item->td_resp->destroy(item->td_resp);
    }
    free(item);
    debug_msg("\tdone\n");
  }
  struct index_pair final_state = {
    .first = a1->final,
    .second = a2->final
  };
  int *fs_lookup = get_item(ps_remap, &final_state);
  int fs_code;
  if(fs_lookup == NULL){
    fs_code = -1;
  }else{
    fs_code = *fs_lookup;
  }
  
  // cleanup
  llist_destroy(&agenda, NULL);
  free(states1);
  free(states2);
  dict_destroy(state_pairs);
  dict_destroy(ps_remap);
  debug_msg("\tdone  some nice cleanup\n");
  
  
  debug_msg("\tmaking an array of product rules\n");
  struct rule *pr_array = NULL;
  if(!llist_is_empty(&product_rules)){
    pr_array = malloc(product_rules.size * sizeof(struct rule));
    struct rule *rule_copy = pr_array;
    for(ll_cell c = llist_first(&product_rules); c != product_rules.sentinelle; c = c->next){
      memcpy(rule_copy, c->elem, sizeof(struct rule));
      debug_msg("\t\twidth: %d\n", rule_copy->width);
      debug_msg("\t\t%d->%d(", rule_copy->parent, rule_copy->label);
      print_int_array(rule_copy->children, rule_copy->width);
      ++rule_copy;
    }
  }
  
  debug_msg("\tdone, creating automaton with final state %d\n", fs_code);  
  target->a = create_explicit_automaton(n_pstates, a1->n_symb, pr_array, n_pr, fs_code);
  free(pr_array);
  llist_destroy(&product_rules, &rule_destroy);
  
  debug_msg("\tsetting up target\n");
  
  target->state_decoder = reverse_ps;
  target->rule_decoder = reverse_pr;

  debug_msg("all done\n");
}

void clean_intersection(struct intersection *inter){
  clean_decoders(inter);
  inter->a->destroy(inter->a);
}

void clean_decoders(struct intersection *inter){
  dict_destroy(inter->state_decoder);
  dict_destroy(inter->rule_decoder);
}

