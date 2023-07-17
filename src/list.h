#ifndef LIST_H
#define LIST_H


typedef struct ll_cell *ll_cell;
struct ll_cell{
  void *elem;
  ll_cell next;
  ll_cell prev;
};

/*
 * mostly intended to use as a simple stack or queue
 * does *not* handle deallocation of lists' element upon destruction
 * unless a disposer function to this effect is provided
 */
typedef struct llist *llist;
struct llist{
  long size;
  ll_cell sentinelle;
};

ll_cell llist_first(llist); //defined only for non empty list
ll_cell llist_last(llist);  //defined only for non empty list
void llist_remove_first(llist); //defined only for non empty list
void llist_remove_last(llist);  //defined only for non empty list
void *llist_popleft(llist); //defined only for non empty list
void *llist_popright(llist); //defined only for non empty list  
void llist_insert_left(llist, void *); 
void llist_insert_right(llist, void *);
int llist_is_empty(llist);
void llist_destroy(llist, void (*dispose_of)(void *elem)); //dispose ignored if NULL


void llist_epsilon_init(llist);

#endif
