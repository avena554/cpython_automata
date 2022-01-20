#include "automata.h"
#include <stdio.h>

automaton make_test(){
  int c1[3] = {1, 1, 1};
  int c2[2] = {1, 2};
  int c3[2] = {0};
  struct rule r1 = {
    0, 0, c1, 3
  };
  struct rule r2 = {
    1, 1, c2, 2
  };
  struct rule r3 = {
    0, 0, c3, 1
  };

  struct rule rs[3] = {r1, r2, r3};
  automaton a = create_explicit_automaton(3, 2, rs, 3);
  return a;
}

void test_query_labels(automaton a){
  fprintf(stderr, "accessing rules top down...\n");
  for(int i = 0; i < a->n_states; ++i){
    fprintf(stderr, "\tquerrying for parent %d...\n", i);    
    label_to_ruleset l_to_rs = a->td_query(a, i);
    fprintf(stderr, "\t\tquerrying all labels...\n");
    intset ls = l_to_rs->labels(l_to_rs);
    int_iterator l_it = ls->create_iterator(ls);
    for(int *label=l_it->next(l_it); label != NULL; label = l_it->next(l_it)){
      fprintf(stderr, "\t\t\tfound transition(s) with label %d...\n", *label);
    }
    l_it->destroy(l_it);
    ls->destroy(ls);
    l_to_rs->destroy(l_to_rs);
  }
  fprintf(stderr, "...done\n");

  fprintf(stderr, "accessing rules bottom-up...\n");
  int query1[3] = {1, 1, 1};
  int query2[2] = {1, 2};
  int query3[1] = {0};
  int *queries[3] = {query1, query2, query3};
  int widths[3] = {3, 2, 1};
  
  for(int i = 0; i < 3; ++i){
    fprintf(stderr, "\tquerrying for children [");
    for(int j = 0; j < widths[i]; ++j){
      fprintf(stderr, "%d", queries[i][j]);
      if(j < widths[i] - 1){
	fprintf(stderr, ", ");
      }
    }
    fprintf(stderr, "]\n");
    
    label_to_ruleset l_to_rs = a->bu_query(a, queries[i], widths[i]);
    fprintf(stderr, "\t\tquerrying all labels...\n");
    intset ls = l_to_rs->labels(l_to_rs);
    int_iterator l_it = ls->create_iterator(ls);
    for(int *label=l_it->next(l_it); label != NULL; label = l_it->next(l_it)){
      fprintf(stderr, "\t\t\tfound transition(s) with label %d...\n", *label);
    }
    l_it->destroy(l_it);
    ls->destroy(ls);
    l_to_rs->destroy(l_to_rs);
  }
  fprintf(stderr, "...done\n");
}


void test_query_all_rules_step(automaton a){
  fprintf(stderr, "testing partial enumeration...\n");
  fprintf(stderr, "\tquerrying for parent %d...\n", 0);    
  label_to_ruleset l_to_rs = a->td_query(a, 0);
  rule_iterator r_it = l_to_rs->all_rules(l_to_rs);
  
  struct rule rule_data;
  a->fill_rule(a, &rule_data, *(r_it->next(r_it)));
  r_it->destroy(r_it);
  l_to_rs->destroy(l_to_rs);
  
  fprintf(stderr, "\t\t%d->%d(", rule_data.parent, rule_data.label);
  for(int i = 0; i < rule_data.width; ++i){
    fprintf(stderr, "%d", rule_data.children[i]);
    if(i < rule_data.width - 1){
      fprintf(stderr, ", ");
    }
  }
  fprintf(stderr, ")\n");
  fprintf(stderr, "...done\n");
}

void test_query_all_rules(automaton a){
  fprintf(stderr, "accessing rules top down...\n");
  for(int i = 0; i < a->n_states; ++i){
    fprintf(stderr, "\tquerrying for parent %d...\n", i);    
    label_to_ruleset l_to_rs = a->td_query(a, i);
    fprintf(stderr, "\t\ttraversing all rules...\n");
    rule_iterator r_it = l_to_rs->all_rules(l_to_rs);
    fprintf(stderr, "\t\tbuilt iterator...\n");
   
    for(int *rule=r_it->next(r_it); rule != NULL; rule = r_it->next(r_it)){
      struct rule rule_data;
      a->fill_rule(a, &rule_data, *rule);
      fprintf(stderr, "\t\t\t%d->%d(", rule_data.parent, rule_data.label);
      for(int i = 0; i < rule_data.width; ++i){
	fprintf(stderr, "%d", rule_data.children[i]);
	if(i < rule_data.width - 1){
	  fprintf(stderr, ", ");
	}
      }
      fprintf(stderr, ")\n");
    }
    r_it->destroy(r_it);
    l_to_rs->destroy(l_to_rs);
  }
  fprintf(stderr, "...done\n");
}


int main(void){
  fprintf(stderr, "creating automaton...\n");
  automaton a = make_test();
  fprintf(stderr, "...done\n");
  fprintf(stderr, "building top-down index...\n");
  build_td_index_from_explicit(a);
  fprintf(stderr, "...done\n");
  fprintf(stderr, "building bottom-up index...\n");
  build_bu_index_from_explicit(a);
  fprintf(stderr, "...done\n");

  
  fprintf(stderr, "traversing all rules...\n");

  ruleset rs = a->all_rules(a);
  rule_iterator it = rs->create_iterator(rs);
  for(int *rule = it->next(it); rule != NULL; rule = it->next(it)){
    struct rule rule_data;
    a->fill_rule(a, &rule_data, *rule);
    fprintf(stderr, "\t%d->%d(", rule_data.parent, rule_data.label);
    for(int i = 0; i < rule_data.width; ++i){
      fprintf(stderr, "%d", rule_data.children[i]);
      if(i < rule_data.width - 1){
	fprintf(stderr, ", ");
      }
    }
    fprintf(stderr, ")\n");
  }
  it->destroy(it);
  rs->destroy(rs);
  fprintf(stderr, "...done\n");

  fprintf(stderr, "accessing rules top down...\n");
  for(int i = 0; i < a->n_states; ++i){
    fprintf(stderr, "\tquerrying for parent %d...\n", i);    
    label_to_ruleset l_to_rs = a->td_query(a, i);
    for(int label = 0; label < a->n_symb; ++label){
      fprintf(stderr, "\t\tquerrying for label %d...\n", label);
      ruleset rs = l_to_rs->query(l_to_rs, label);
      rule_iterator it = rs->create_iterator(rs);
      for(int *rule = it->next(it); rule != NULL; rule = it->next(it)){
	struct rule rule_data;
	a->fill_rule(a, &rule_data, *rule);
	fprintf(stderr, "\t\t\t%d->%d(", rule_data.parent, rule_data.label);
	for(int i = 0; i < rule_data.width; ++i){
	  fprintf(stderr, "%d", rule_data.children[i]);
	  if(i < rule_data.width - 1){
	    fprintf(stderr, ", ");
	  }
	}
	fprintf(stderr, ")\n");
      }
      it->destroy(it);
      rs->destroy(rs);
    }
    l_to_rs->destroy(l_to_rs);
  }
  fprintf(stderr, "...done\n");

  fprintf(stderr, "accessing rules bottom-up...\n");
  int query1[3] = {1, 1, 1};
  int query2[2] = {1, 2};
  int query3[1] = {0};
  int *queries[3] = {query1, query2, query3};
  int widths[3] = {3, 2, 1};
  
  for(int i = 0; i < 3; ++i){
    fprintf(stderr, "\tquerrying for children [");
    for(int j = 0; j < widths[i]; ++j){
      fprintf(stderr, "%d", queries[i][j]);
      if(j < widths[i] - 1){
	fprintf(stderr, ", ");
      }
    }
    fprintf(stderr, "]\n");
    
    label_to_ruleset l_to_rs = a->bu_query(a, queries[i], widths[i]);
    for(int label = 0; label < a->n_symb; ++label){
      fprintf(stderr, "\t\tquerrying for label %d...\n", label);
      ruleset rs = l_to_rs->query(l_to_rs, label);
      rule_iterator it = rs->create_iterator(rs);
      for(int *rule = it->next(it); rule != NULL; rule = it->next(it)){
	struct rule rule_data;
	a->fill_rule(a, &rule_data, *rule);
	fprintf(stderr, "\t\t\t%d->%d(", rule_data.parent, rule_data.label);
	for(int i = 0; i < rule_data.width; ++i){
	  fprintf(stderr, "%d", rule_data.children[i]);
	  if(i < rule_data.width - 1){
	    fprintf(stderr, ", ");
	  }
	}
	fprintf(stderr, ")\n");
      }
      it->destroy(it);
      rs->destroy(rs);
    }
    l_to_rs->destroy(l_to_rs);
  }
  fprintf(stderr, "...done\n");
  

  test_query_labels(a);
  test_query_all_rules(a);
  test_query_all_rules_step(a);
  
  
  fprintf(stderr, "destroying automaton...\n");
  a->destroy(a);
  fprintf(stderr, "...done\n");
}
