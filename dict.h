#ifndef DICT_H
#define DICT_H

/*
 * header for a generic dictionary (with elements of a fixed type)
 */

typedef struct dict_item *dict_item;
struct dict_item{
  void *key;
  void *elem;
};

struct item_type_parameters{
  int (*key_cmp_fn)(const void *k1, const void *k2);
  void (*key_delete_fn)(void *key);
  void (*elem_delete_fn)(void *elem);
};

typedef struct dict_iterator *dict_iterator;
struct dict_iterator{
  void *data;
  dict_item (*next)(dict_iterator it);
  void (*destroy_data)(dict_iterator it);
};

void *dict_create(const struct item_type_parameters *params);
void dict_destroy(void *d);

void set_item(const void *d, void *key, void *value);
void *get_item(const void *d, void *key);

int int_cmp_fn(const void *k1, const void *k2);

void init_iterator(const void *d, dict_iterator it);


#endif /*int_dict.h*/
