#ifndef AUTOMATA_UTILS_H
#define AUTOMATA_UTILS_H

typedef struct ci_state *ci_state;
struct ci_state{
  int n;
  int v_pos;
  const int * const *data;
  const int *sizes;
  int *indices;
  int *tuple;
};

// args 4 and 5 must be pointers to arrays of [|arg 3 |] integers.
void ci_init(const int * const *, const int *, int, int *, int *, ci_state);
int *ci_next(ci_state);

#endif
