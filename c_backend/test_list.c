#include "list.h"
#include <stdio.h>

int main(){
  int t[5] = {1, 2, 3, 4, 5};
  struct llist l;

  llist_epsilon_init(&l);

  fprintf(stderr, "Checking LIFO...\n");
  fprintf(stderr, "l is empty : %d\n", llist_is_empty(&l));
  for(int i = 0; i < 5; ++i){
    llist_insert_left(&l, &t[i]);
  }
  fprintf(stderr, "l is empty : %d\n", llist_is_empty(&l));
  fprintf(stderr, "first element of l : %d\n", *(int *)(llist_first(&l)->elem));
  fprintf(stderr, "last element of l : %d\n", *(int *)(llist_last(&l)->elem));
  
  fprintf(stderr, "\t[");
  for(int i = 0; i < 5; ++i){
    fprintf(stderr, " %d(%ld) ", *(int *)llist_popleft(&l), l.size);
  }
  fprintf(stderr, "]\n");
  fprintf(stderr, "l is empty : %d\n", llist_is_empty(&l));


  fprintf(stderr, "Checking FIFO...\n");
  for(int i = 0; i < 5; ++i){
    llist_insert_left(&l, &t[i]);
  }

  fprintf(stderr, "l is empty : %d\n", llist_is_empty(&l));
  fprintf(stderr, "first element of l : %d\n", *(int *)(llist_first(&l)->elem));
  fprintf(stderr, "last element of l : %d\n", *(int *)(llist_last(&l)->elem));
  
  fprintf(stderr, "\t[");
  for(int i = 0; i < 5; ++i){
    fprintf(stderr, " %d(%ld) ", *(int *)llist_popright(&l), l.size);
  }
  fprintf(stderr, "]\n");

  llist_destroy(&l, NULL);
  
  return 0;
}
