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

  rule rs[3] = {&r1, &r2, &r3};
  automaton a = create_explicit_automaton(3, 2, rs, 3);
  return a;
}



int main(void){
  fprintf(stderr, "creating automaton...\n");
  automaton a = make_test();
  fprintf(stderr, "...done\n");
  fprintf(stderr, "building top down index...\n");
  build_td_index_from_explicit(a);
  fprintf(stderr, "...done\n");  
  fprintf(stderr, "destroying automaton...\n");
  a->destroy(a);
  fprintf(stderr, "...done\n");
}
