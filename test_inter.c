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
    .parent =  2, .label = 2, .width = 0, .children = c3
  };
  struct rule r4 = {
    .parent =  1, .label = 3, .width = 0, .children = c3
  };
  struct rule r5 = {
    .parent =  1, .label = 2, .width = 0, .children = c3
  };

  struct rule rs[5] = {r1, r2, r3, r4, r5};
  automaton a = create_explicit_automaton(3, 2, rs, 5, 0);
  build_td_index_from_explicit(a);
  build_bu_index_from_explicit(a);
  return a;  
}

void test_intersect(automaton a){
  struct intersection inter;
  fprintf(stderr, "intersecting a with itself...\n");
  intersect(a, a, &inter);
  clean_decoders(&inter);
  //intersect_cky(a, a, &inter);
  struct intersection sqinter;
  fprintf(stderr, "intersecting the intersection with a again\n");
  intersect(a, inter.a, &sqinter);
  fprintf(stderr, "...done\n");
  inter.a->destroy(inter.a);
  a->destroy(a);
  clean_intersection(&sqinter);
}




int main(void){
  fprintf(stderr, "creating automaton...\n");
  automaton a = make_test();
  fprintf(stderr, "...done\n");
  test_intersect(a);
}
