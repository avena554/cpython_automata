#include "utils.h"
#include <stdio.h>

int main(void){
  int e1[]  = {1, 2, 3, 4};
  int e2[] = {1, 2};
  int e3[] = {1, 2, 3, 4, 5};

  int e4[] = {};

  int sizes_1[] = {4, 2, 5};
  int sizes_2[] = {4, 0, 5};
  
  const int *data[] = {e1, e2, e3};

  int indices[4];
  int str[3];

  struct ci_state state;
  ci_init(data, sizes_1, 3, indices, str, &state);

  int i =  0;
  
  for(int *tuple = ci_next(&state); tuple != NULL; tuple = ci_next(&state)){
    printf("<%d, %d, %d>\n", tuple[0], tuple[1], tuple[2]);
    //printf("%d\n", state.v_pos);    
    ++i;
  }

  const int *empty[] = {e1, e4, e3};
  ci_init(empty, sizes_2, 3, indices, str, &state);
  for(int *tuple = ci_next(&state); tuple != NULL; tuple = ci_next(&state)){
    printf("<%d, %d, %d>\n", tuple[0], tuple[1], tuple[2]);
    //printf("%d\n", state.v_pos);    
    ++i;
  }

  printf("done\n");
  
}
