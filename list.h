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
  ll_cell sentinelle;
};

ll_cell first(llist); //defined only for non empty list
ll_cell last(llist);  //defined only for non empty list
void remove_first(llist); //defined only for non empty list
void remove_last(llist);  //defined only for non empty list
void *popleft(llist); //defined only for non empty list
void *popright(llist); //defined only for non empty list  
void insert_left(llist, void *); 
void insert_right(llist, void *);
int is_empty(llist);
void llist_destroy(llist, void (*dispose_of)(void *elem)); //dispose ignored if NULL


void epsilon_init(llist);

#endif
