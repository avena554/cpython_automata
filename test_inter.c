#include "automata.h"
#include <stdio.h>
#include "dict.h"

automaton make_test(){
  int c1[3] = {1, 1, 1};
  int c2[2] = {2, 2};
  int c3[] = {};
  struct rule r1 = {
    .parent = 0, .label = 0, .width = 3, .children = c1
  };
  struct rule r2 = {
    .parent = 1, .label = 1, .width = 2, .children = c2
  };
  struct rule r3 = {
    .parent =  2, .label = 0, .width = 0, .children = c3
  };
  struct rule r4 = {
    .parent =  1, .label = 0, .width = 0, .children = c3
  };

  struct rule rs[4] = {r1, r2, r3, r4};
  automaton a = create_explicit_automaton(3, 2, rs, 4, 0);
  build_td_index_from_explicit(a);
  build_bu_index_from_explicit(a);
  return a;  
}

void test_intersect(automaton a){
  struct intersection inter;
  intersect_cky(a, a, &inter);

  /*
    dict_iterator debug = dict_items(inter.state_decoder);
    for(dict_item dbg = debug->next(debug); dbg != NULL; dbg = debug->next(debug)){
    fprintf(stderr, "key: %d\n", *(int *)dbg->key);
    struct index_pair *elem = (struct index_pair *)dbg->elem;
    fprintf(stderr, "elem: (%d, %d)\n", elem->first, elem->second);
    }
    debug->destroy(debug);
  */
  
  a->destroy(a);
  dict_destroy(inter.state_decoder);
  dict_destroy(inter.rule_decoder);
  inter.a->destroy(inter.a);
}




int main(void){
  fprintf(stderr, "creating automaton...\n");
  automaton a = make_test();
  fprintf(stderr, "...done\n");
  fprintf(stderr, "intersecting a with itself...\n");
  test_intersect(a);
  fprintf(stderr, "...done\n");
}
