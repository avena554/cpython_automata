#include "dict.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv){

  fprintf(stderr, "setting up keys and values\n");
  char *e1 = malloc(4*sizeof(int));
  char *e2 = malloc(4*sizeof(int));
  char *e3 = malloc(4*sizeof(int));
  memcpy(e1, &"aaa", 4);
  memcpy(e2, &"bbb", 4);
  memcpy(e3, &"ccc", 4);
  int k1 = 1;
  int k2 = 2;
  int k3 = 42;

  fprintf(stderr, "\te1:%s\n", e1);
  fprintf(stderr, "\te2:%s\n", e2);
  fprintf(stderr, "\te3:%s\n", e3);
  fprintf(stderr, "...done\n");
  
  fprintf(stderr, "creating dict...\n");
  struct item_type_parameters params = {
    &int_cmp_fn,
    NULL,
    (void (*)(const void *))(&free)
  };
  void *d = dict_create(&params);
  fprintf(stderr, "...done\n");
  fprintf(stderr, "storing elements\n");
  set_item(d, &k1, e1);
  set_item(d, &k2, e2);
  set_item(d, &k3, e3);
  fprintf(stderr, "...done\n");
  fprintf(stderr, "retrieving elements...\n");
  int vals[3] = {1, 2, 42};
  char *retrieved;
  for(int i = 0; i < 3; ++i){
    fprintf(stderr, "\tretrieving %d\n", vals[i]);
    retrieved = ((char *)get_item(d, vals + i));
    fprintf(stderr, "\tretrieved: %s\n", retrieved);
  }
  fprintf(stderr, "...done\n");
  fprintf(stderr, "traversing dict\n");
  struct dict_iterator it;
  init_iterator(d, &it);
  for(dict_item current = (*(it.next))(&it); current != NULL; current = (*(it.next))(&it)){
    fprintf(stderr, "\tretrieved: %s\n", (char *)current->elem);
  }
  (*(it.destroy))(&it);
  fprintf(stderr, "...done\n");
  fprintf(stderr, "destroying dict\n");
  dict_destroy(d);
  fprintf(stderr, "...done\n");
}
