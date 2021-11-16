#include "dict.h"
#include "avl/avl.h"
#include <stdlib.h>
#include <stdio.h>

int int_cmp_fn(const void *k1, const void *k2){
  int x = *((int *)k1);
  int y = *((int *)k2);
  if(x < y){
    return -1;
  }else if(x > y){
    return 1;
  }else{
    return 0;
  }
}

int cmp_item(const void *item1, const void *item2, void *params){
  struct item_type_parameters *actual_params = (struct item_type_parameters *) params; 
  return (*(actual_params->key_cmp_fn))(((dict_item)item1)->key, ((dict_item)item2)->key);
}

void destroy_item(void *item, void *params){
  struct item_type_parameters *actual_params = (struct item_type_parameters *) params;
  dict_item actual_item = (dict_item) item;
  if(actual_params->elem_delete_fn != NULL){
    (*(actual_params->elem_delete_fn))(actual_item->elem);
  }
  if(actual_params->key_delete_fn != NULL){
    (*(actual_params->key_delete_fn))(actual_item->key);
  }
  free(item);
}

void *dict_create(const struct item_type_parameters *params){
  void *avl_params = (void *)params;
  return avl_create(&cmp_item, avl_params, NULL);
}

void dict_destroy(void *d){
  avl_destroy((struct avl_table *)d, &destroy_item);
}

void set_item(const void *d, void *key, void *elem){
  dict_item item = malloc(sizeof(struct dict_item));
  item->key = key;
  item->elem = elem;
  avl_insert((struct avl_table *)d, item);    
}

void *get_item(const void *d, void *key){
  struct dict_item item = {
    key,
    NULL
  };
  void *retrieved = avl_find((struct avl_table *)d, &item);
  if(retrieved != NULL){
    return ((dict_item)retrieved)->elem;
  }
  else{
    return NULL;
  }
}

dict_item next(dict_iterator it){
  return (dict_item)(avl_t_next(((struct avl_traverser*)(it->data))));
}

void destroy_data(dict_iterator it){
  free(it->data);
}

void init_iterator(const void *d, dict_iterator it){
  it->data = malloc(sizeof(struct avl_traverser));
  avl_t_init(it->data, (struct avl_table *)d);
  it->next = &next;
  it->destroy_data = &destroy_data;
}




