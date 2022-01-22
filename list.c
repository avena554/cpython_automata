#include "list.h"
#include <stdlib.h>

ll_cell first(llist l){  
  return l->sentinelle->next;
}

ll_cell last(llist l){
  return l->sentinelle->prev;
}

static void destroy_cell(ll_cell c){
  c->prev = NULL;
  c->next = NULL;
  c->elem = NULL;
  free(c);
}

static void remove_cell(ll_cell c){
  c->prev->next = c->next;
  c->next->prev = c->prev;
  destroy_cell(c);
}

void remove_first(llist l){
  remove_cell(first(l));
}

void remove_last(llist l){
  remove_cell(last(l));
}

void *popleft(llist l){
  void *elem = first(l)->elem;
  remove_first(l);
  return elem;
}

void *popright(llist l){
  void *elem = last(l)->elem;
  remove_last(l);
  return elem;
}

static void insert_cell_left(ll_cell cell_after, ll_cell new_cell){
  new_cell->next = cell_after;
  new_cell->prev = cell_after->prev;
  new_cell->prev->next = new_cell;  
  cell_after->prev = new_cell;
}

static void insert_cell_right(ll_cell cell_before, ll_cell new_cell){
  new_cell->prev = cell_before;
  new_cell->next = cell_before->next;
  new_cell->next->prev = new_cell;
  cell_before->next = new_cell;
}

void insert_left(llist l, void *elem){
  ll_cell new_cell = malloc(sizeof(struct ll_cell));
  new_cell->elem = elem;
  insert_cell_right(l->sentinelle, new_cell);
}

void insert_right(llist l, void *elem){
  ll_cell new_cell = malloc(sizeof(struct ll_cell));
  new_cell->elem = elem;
  insert_cell_left(l->sentinelle, new_cell);
}

int is_empty(llist l){
  return l->sentinelle->next == l->sentinelle;
}

void llist_destroy(llist l, void (*dispose_of)(void *elem)){
  while(!is_empty(l)){
    void *elem = popleft(l);
    if(dispose_of != NULL){
      dispose_of(elem);
    }
  }
  destroy_cell(l->sentinelle);
}

void epsilon_init(llist l){
  l->sentinelle = malloc(sizeof(struct ll_cell));
  l->sentinelle->elem = NULL;
  l->sentinelle->next = l->sentinelle;
  l->sentinelle->prev = l->sentinelle;
}
