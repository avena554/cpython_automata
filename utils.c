#include "utils.h"
#include <stdlib.h>
#include <stdio.h>

void ci_init(const int * const *data, const int *sizes, int n, int *indices, int *tuple_str, ci_state target){
  target->n = n;
  target->data = data;
  target->sizes = sizes;
  target->indices = indices;
  target->tuple = tuple_str;
  target->v_pos = n - 1;
  
  for(int i = 0; i < n; ++i) target->indices[i] = 0;

  for(int i = 0; i < n; ++i){
    if(sizes[i] <= 0){
      target->tuple = NULL;
      return;
    }else{
      target->tuple[i] = data[i][0];
    }
  }
}

int *ci_next(ci_state state){
  if(state->tuple != NULL){
    while(state->v_pos >= 0){
      if(state->indices[state->v_pos] < state->sizes[state->v_pos]){
	state->tuple[state->v_pos] = state->data[state->v_pos][state->indices[state->v_pos]];		
	if(state->v_pos < (state->n - 1)) ++state->v_pos;
	++state->indices[state->v_pos];
	return state->tuple;
      }else{
	state->tuple[state->v_pos] = state->data[state->v_pos][0];
	state->indices[state->v_pos] = 0;
	--state->v_pos;
	if(state->v_pos >= 0){
	  ++state->indices[state->v_pos];
	}
      }
    }
    state->tuple = NULL;
  }
  return state->tuple;    
}
