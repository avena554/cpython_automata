
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "automata.h"
#include "dict.h"
#include "debug.h"

/* 
 * TODO: Use python dedicated memory allocators instead of malloc
 * make automata.h able to receive configuration for the allocator.
 */
typedef struct{
  PyObject_HEAD
  automaton a;
} pyta_automaton;


/* C methods for the type automaton */
static PyObject *td_all(PyObject *self, PyObject *args);
static PyObject *bu_all(PyObject *self, PyObject *args);
static PyObject *td_labels(PyObject *self, PyObject *args);
static PyObject *bu_labels(PyObject *self, PyObject *args);
static PyObject *td_query(PyObject *self, PyObject *args);
static PyObject *bu_query(PyObject *self, PyObject *args);
static PyObject *get_rule(PyObject *self, PyObject *args);

static void pyta_automaton_dealloc(PyObject *o){
  debug_msg("Python is destroying the automaton\n");
  pyta_automaton *pa = (pyta_automaton *)o;
  automaton a = pa->a;
  if(a != NULL){
    a->destroy(a);
  }
  Py_TYPE(pa)->tp_free((PyObject *)pa);
};

static PyObject *pyta_automaton_new(PyTypeObject *type, PyObject *arg, PyObject *kwds){
  pyta_automaton *pa = (pyta_automaton *)type->tp_alloc(type, 0);
  if(pa != NULL){
    pa -> a = NULL;
  }
  return (PyObject *)pa;
}


static PyMethodDef pyta_automaton_methods[] = {
  {"td_labels", td_labels, METH_VARARGS,
   "Return a list of label (indices) for which rules are applicable from the argument children states"
  },
  {"bu_labels", bu_labels, METH_VARARGS,
   "Return a list of label (indices) for which rules are applicable from the argument children states"
  },
  {"bu_query", bu_query, METH_VARARGS,
   "Return a list of indices of rules applicable from a given children state and label"
  },
  {"td_query", td_query, METH_VARARGS,
   "Return a list of indices of rules applicable from a given parent state and label"
  },
  {"td_all", td_all, METH_VARARGS,
   "Return a list of indices of rules applicable from a given parent state"
  },
  {"bu_all", bu_all, METH_VARARGS,
   "Return a list of indices of rules applicable for a given list of children states"
  },
  {"get_rule", get_rule, METH_VARARGS,
   "Return the rule corresponding to a given rule index"
  },
  {NULL, NULL, 0, NULL}  /* Sentinel */
};

static PyTypeObject pyta_automaton_t = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "pyta.Automaton",
  .tp_doc = "Tree automaton",
  .tp_basicsize = sizeof(pyta_automaton),
  .tp_itemsize = 0,
  .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
  .tp_new = pyta_automaton_new,
  .tp_dealloc = pyta_automaton_dealloc,
  .tp_methods = pyta_automaton_methods,
};

static void clean_several(int n_pointers, void *pointers[]){
  for(int i = 0; i < n_pointers; ++i){
    free(pointers[i]);
  }
}

static PyObject *PyList_from_iterator_factory(int_iterator (*factory)(void *), void *params){
  // count 'em all  
  int_iterator it = factory(params);
  int n = 0;
  for(int *index = it->next(it); index != NULL; index = it->next(it)){    
    ++n;
  }
  it->destroy(it);

  // create the returned Python list, with appropriate size
  PyObject *out_list = PyList_New(n);
  if(out_list == NULL)
    return NULL;
  
  // populate the list
  it = factory(params);
  Py_ssize_t k = 0;
  for(int *index = it->next(it); index != NULL; index = it->next(it)){
    PyObject *py_index = Py_BuildValue("i", *index);
    if(py_index == NULL){
      Py_DECREF(out_list);
      return NULL;
    }    
      
    if(PyList_SetItem(out_list, k, py_index) != 0){
      Py_DECREF(out_list);
      return NULL;
    }
    ++k;
  }
  it->destroy(it);
  return out_list;
}

static PyObject *td_all(PyObject *self, PyObject *args){
  automaton a = ((pyta_automaton *)self)->a;
  int parent;
  PyObject *out_list;

  if(!PyArg_ParseTuple(args, "i", &parent)) return NULL;
  label_to_ruleset lmap = a->td_query(a, parent);

  out_list = PyList_from_iterator_factory((int_iterator (*)(void *))(lmap->all_rules), lmap);
  
  lmap->destroy(lmap);
  
  return out_list;  
}

static PyObject *td_labels(PyObject *self, PyObject *args){
  automaton a = ((pyta_automaton *)self)->a;
  int parent;
  PyObject *out_list;

  if(!PyArg_ParseTuple(args, "i", &parent)) return NULL;
  label_to_ruleset lmap = a->td_query(a, parent);
  intset labels = lmap->labels(lmap);

  out_list = PyList_from_iterator_factory((int_iterator (*)(void *))(labels->create_iterator), labels);
  
  labels->destroy(labels);
  lmap->destroy(lmap);
  
  return out_list;  
}

static PyObject *td_query(PyObject *self, PyObject *args){
  automaton a = ((pyta_automaton *)self)->a;
  int parent;
  int label;
  PyObject *out_list;

  if(!PyArg_ParseTuple(args, "ii", &parent, &label)) return NULL;
  label_to_ruleset lmap = a->td_query(a, parent);
  ruleset rs = lmap->query(lmap, label);

  out_list = PyList_from_iterator_factory((int_iterator (*)(void *))(rs->create_iterator), rs);
  
  rs->destroy(rs);
  lmap->destroy(lmap);
  
  return out_list;  
}

// assume seq is already checked to be a seq type
static int retrieve_inttuple(PyObject *seq, int n, int *tuple){
  for(int i = 0; i < n; ++i){
    PyObject *item = PySequence_GetItem(seq, i);    
    if(!PyLong_Check(item)){
      return -1;
    }
    long value = PyLong_AsLong(item);
    Py_DECREF(item);
    tuple[i] = (int)value;
  }
  return 0;
}

static PyObject *bu_all(PyObject *self, PyObject *args){
  automaton a = ((pyta_automaton *)self)->a;
  PyObject *children;
  PyObject *out_list;
  Py_ssize_t n;
  int *c_children;
  
  //parse a list from the arguments
  if(!PyArg_ParseTuple(args, "O", &children) || !PySequence_Check(children)) return NULL;

  n = PySequence_Size(children);
  c_children = malloc(n*sizeof(int));

  if(retrieve_inttuple(children, n, c_children) < 0){
    free(c_children);
    return NULL;
  }

  label_to_ruleset lmap = a->bu_query(a, c_children, (int)n);
  free(c_children);

  out_list = PyList_from_iterator_factory((int_iterator (*)(void *))(lmap->all_rules), lmap);  
  lmap->destroy(lmap);

  return out_list;
}

static PyObject *bu_labels(PyObject *self, PyObject *args){
  automaton a = ((pyta_automaton *)self)->a;
  PyObject *children;
  PyObject *out_list;
  Py_ssize_t n;
  int *c_children;

  //parse a list from the arguments
  if(!PyArg_ParseTuple(args, "O", &children) || !PySequence_Check(children)) return NULL;

  n = PySequence_Size(children);
  c_children = malloc(n*sizeof(int));

  if(retrieve_inttuple(children, n, c_children) < 0){
    free(c_children);
    return NULL;
  }

  label_to_ruleset lmap = a->bu_query(a, c_children, (int)n);
  free(c_children);
  intset labels = lmap->labels(lmap);

  out_list = PyList_from_iterator_factory((int_iterator (*)(void *))(labels->create_iterator), labels);

  labels->destroy(labels);
  lmap->destroy(lmap);

  return out_list;
}

static PyObject *bu_query(PyObject *self, PyObject *args){
  automaton a = ((pyta_automaton *)self)->a;
  PyObject *children;
  PyObject *out_list;
  Py_ssize_t n;
  int *c_children;
  int label;
  
  //parse a list from the arguments
  if(!PyArg_ParseTuple(args, "Oi", &children, &label) || !PySequence_Check(children)) return NULL;

  n = PySequence_Size(children);
  c_children = malloc(n*sizeof(int));

  if(retrieve_inttuple(children, n, c_children) < 0){
    free(c_children);
    return NULL;
  }

  label_to_ruleset lmap = a->bu_query(a, c_children, (int)n);
  free(c_children);

  ruleset rs = lmap->query(lmap, label);
  out_list = PyList_from_iterator_factory((int_iterator (*)(void *))(rs->create_iterator), rs);

  rs->destroy(rs);
  lmap->destroy(lmap);  

  return out_list;
}


static PyObject *get_rule(PyObject *self, PyObject *args){
  automaton a = ((pyta_automaton *)self)->a;
  int rule_index;
  struct rule rule;

  if(!PyArg_ParseTuple(args, "i", &rule_index)) return NULL;
  a->fill_rule(a, &rule, rule_index);

  PyObject *parent = Py_BuildValue("i", rule.parent);
  if(parent == NULL){
    return NULL;
  }

  PyObject *label = Py_BuildValue("i", rule.label);
  if(parent == NULL){
    Py_DECREF(parent);
    return NULL;
  }

  PyObject *children_tuple = PyTuple_New(rule.width);
  if(children_tuple == NULL){
    Py_DECREF(parent);
    Py_DECREF(label);
    return NULL;
  }

  Py_ssize_t i;
  for(i = 0; i < rule.width; ++i){
    PyObject *child = Py_BuildValue("i", rule.children[i]);
    if(child == NULL){
      Py_DECREF(parent);
      Py_DECREF(label);
      Py_DECREF(children_tuple);
      return NULL;
    }

    if(PyTuple_SetItem(children_tuple, i, child) != 0){
      Py_DECREF(parent);
      Py_DECREF(label);      
      Py_DECREF(children_tuple);
      return NULL;
    }
  }

  PyObject *py_rule = Py_BuildValue("(OOO)", parent, label, children_tuple);
  if(py_rule == NULL){
    Py_DECREF(parent);
    Py_DECREF(label);
    Py_DECREF(children_tuple);
    return NULL;
  }
  /* we need to decref all members since they're now referenced by the py_rule
   * and we will not pass on our initial references on them.
   */
  Py_DECREF(parent);
  Py_DECREF(label);
  Py_DECREF(children_tuple);
  
  return py_rule;  
}

static PyObject *pyta_from_builtin(automaton builtin){
  PyObject *empty_tuple = PyTuple_New(0);
  if(empty_tuple == NULL){  
    return NULL;
  }
  
  pyta_automaton *pa = (pyta_automaton *)PyObject_CallObject((PyObject *)&pyta_automaton_t, empty_tuple);
  Py_DECREF(empty_tuple);
  if(pa == NULL){    
    return NULL;
  }
  
  pa->a = builtin;
  return (PyObject *)pa;
}

static PyObject *compile_automaton(PyObject *self, PyObject *args){  
  PyObject *rules;
  int n_states;
  int n_symb;
  int final;
  
  if(!PyArg_ParseTuple(args, "iiOi", &n_states, &n_symb, &rules, &final) || !PySequence_Check(rules)) return NULL;

  Py_ssize_t n_rules = PySequence_Size(rules);
  if(n_rules < 0) return NULL;

  // debug_msg("there are %d states, %d symbols and %d rules\n", n_states, n_symb, (int)n_rules);
  // debug_msg("allocating space for rules\n");

  struct rule *c_rules = malloc(n_rules * sizeof(struct rule));  
  int *all_rules_children = NULL;
  void *to_clean[2] = {c_rules, all_rules_children};
  int n_to_clean = 1;
  
  int sum_widths = 0;

  for(int i = 0; i < n_rules; ++i){
    PyObject *rule = PySequence_GetItem(rules, i);
    if(rule == NULL){
      clean_several(n_to_clean, to_clean);
      return NULL;
    }
    
    PyObject *parent = PySequence_GetItem(rule, 0);
    if(parent == NULL){
      clean_several(n_to_clean, to_clean);
      Py_DECREF(rule);
      return NULL;
    }

    int parent_index = (int)PyLong_AsLong(parent);
    if(PyErr_Occurred() != NULL){
      clean_several(n_to_clean, to_clean);
      Py_DECREF(rule);
      return NULL;
    }
    // PySequence_GetItem makes a new ref
    Py_DECREF(parent);

    // debug_msg("retrieving label\n");
    PyObject *label = PySequence_GetItem(rule, 1);
    if(label == NULL){
      clean_several(n_to_clean, to_clean);
      Py_DECREF(rule);
      return NULL;
    }
    int label_index = (int)PyLong_AsLong(label);
    if(PyErr_Occurred() != NULL){
      clean_several(n_to_clean, to_clean);
      Py_DECREF(rule);
      return NULL;
    }
    Py_DECREF(label);
    
    PyObject *children_tuple = PySequence_GetItem(rule, 2);
    if(children_tuple == NULL){
      clean_several(n_to_clean, to_clean);
      Py_DECREF(rule);
      return NULL;
    }
    
    Py_ssize_t width = PySequence_Size(children_tuple);
    if(width < 0){
      clean_several(n_to_clean, to_clean);
      Py_DECREF(rule);
      return NULL;
    }
    sum_widths += width;
    Py_DECREF(children_tuple);

    c_rules[i].parent = parent_index;
    c_rules[i].label = label_index;
    c_rules[i].width = (int)width;
    Py_DECREF(rule);
  }

  // allocating space for all rules children;
  all_rules_children = malloc(sum_widths * sizeof(int));
  to_clean[1] = all_rules_children;
  n_to_clean = 2;
  int offset = 0;
  
  for(Py_ssize_t i = 0; i < n_rules; ++i){        
    PyObject *rule = PySequence_GetItem(rules, i);
    if(rule == NULL){
      clean_several(n_to_clean, to_clean);
      return NULL;
    }
      
    PyObject *children_tuple = PySequence_GetItem(rule, (Py_ssize_t)2);
    Py_DECREF(rule);
    if(children_tuple == NULL){
      clean_several(n_to_clean, to_clean);	
      return NULL;
    }
    
    for(Py_ssize_t j = 0; j < c_rules[(int)i].width; ++j){
      PyObject *child = PySequence_GetItem(children_tuple, j);
      if(child == NULL){
	clean_several(n_to_clean, to_clean);
	Py_DECREF(children_tuple);
	return NULL;
      }
      
      long child_index = PyLong_AsLong(child);
      Py_DECREF(child);
      if(PyErr_Occurred() != NULL){
	clean_several(n_to_clean, to_clean);
	Py_DECREF(children_tuple);
	return NULL;
      }
      
      all_rules_children[offset + j] = (int)child_index;
    }
    Py_DECREF(children_tuple);
    c_rules[i].children = all_rules_children + offset;
    offset += c_rules[i].width;
  }

  
  automaton a = create_explicit_automaton(n_states, n_symb, c_rules, n_rules, final);
  clean_several(n_to_clean, to_clean);
  //free(all_rules_children);
  //free(c_rules);

  build_td_index_from_explicit(a);
  build_bu_index_from_explicit(a);

  return pyta_from_builtin(a);
}

static PyObject *intref_as_PyLong(int *ref){
  return PyLong_FromLong((long)*ref);
}

static PyObject *pair_as_PyTuple(struct index_pair *pair){
  return Py_BuildValue("(ii)", pair->first, pair->second);
}

static PyObject *dict_as_PyDict(void *d, PyObject *(*key_as_Py)(void *), PyObject *(*elem_as_Py)(void *)){
  PyObject *py_dict = PyDict_New();
  if(py_dict == NULL){
    return NULL;
  }
  
  dict_iterator d_it = dict_items(d);
  for(dict_item item = d_it->next(d_it); item != NULL; item = d_it->next(d_it)){
    PyObject *key = key_as_Py(item->key);
    if(key == NULL){
      Py_DECREF(py_dict);
      return NULL;
    }
    PyObject *elem = elem_as_Py(item->elem);
    if(elem == NULL){
      Py_DECREF(key);
      Py_DECREF(py_dict);
      return NULL;
    }
    int success = PyDict_SetItem(py_dict, key, elem);
    Py_DECREF(key);
    Py_DECREF(elem);
    if(success < 0){
      Py_DECREF(py_dict);
      return NULL;
    }
  }
  d_it->destroy(d_it);

  return py_dict;
}

static PyObject *pyta_intersect_cky(PyObject *self, PyObject *args){
  PyObject *lhs_auto;
  PyObject *rhs_auto;
  
  if(!PyArg_ParseTuple(args, "O!O!", &pyta_automaton_t, &lhs_auto, &pyta_automaton_t, &rhs_auto)) return NULL;
  automaton a1 = ((pyta_automaton *)lhs_auto)->a;
  automaton a2 = ((pyta_automaton *)rhs_auto)->a;


  if(a1 == NULL || a2 == NULL){
    PyErr_SetString(PyExc_ValueError, "One of the argument object is an unitialized builtin automaton");
    return NULL;
  }  
  
  struct intersection target;
  intersect_cky(a1, a2, &target);  
  
  PyObject *py_state_decoder = dict_as_PyDict(target.state_decoder,
					      (PyObject *(*)(void *))&intref_as_PyLong,
					      (PyObject *(*)(void *))&pair_as_PyTuple
					      );
  if(py_state_decoder == NULL){
    clean_intersection(&target);
    return NULL;
  }

  
  PyObject *py_rule_decoder = dict_as_PyDict(target.rule_decoder,
					     (PyObject *(*)(void *))&intref_as_PyLong,
					     (PyObject *(*)(void *))&pair_as_PyTuple
					     );
  if(py_rule_decoder == NULL){
    clean_intersection(&target);
    Py_DECREF(py_state_decoder); 
    return NULL;
  }
  
  // we can clean the C state and rule decoders which will be no longer of use
  // However, we obviously must keep the intersection automaton alive.
  clean_decoders(&target);
  PyObject *target_automaton = pyta_from_builtin(target.a);
  if(target_automaton == NULL){
    target.a->destroy(target.a);
    Py_DECREF(py_state_decoder);
    Py_DECREF(py_rule_decoder);
    return NULL;
  }
  
  PyObject *summary =  Py_BuildValue("(NiNN)", target_automaton, target.a->final, py_state_decoder, py_rule_decoder);
  if(summary == NULL){
    target.a->destroy(target.a);
    Py_DECREF(py_state_decoder);
    Py_DECREF(py_rule_decoder);
    Py_DECREF(target_automaton);
    return NULL;
  }

  build_td_index_from_explicit(target.a);
  build_bu_index_from_explicit(target.a);
  return summary;
}


/*
 * TODO: factorize intersection python wrappers
 */
static PyObject *pyta_intersect(PyObject *self, PyObject *args){
  PyObject *lhs_auto;
  PyObject *rhs_auto;
  
  if(!PyArg_ParseTuple(args, "O!O!", &pyta_automaton_t, &lhs_auto, &pyta_automaton_t, &rhs_auto)) return NULL;
  automaton a1 = ((pyta_automaton *)lhs_auto)->a;
  automaton a2 = ((pyta_automaton *)rhs_auto)->a;


  if(a1 == NULL || a2 == NULL){
    PyErr_SetString(PyExc_ValueError, "One of the argument object is an unitialized builtin automaton");
    return NULL;
  }  
  
  struct intersection target;
  intersect(a1, a2, &target);  
  
  PyObject *py_state_decoder = dict_as_PyDict(target.state_decoder,
					      (PyObject *(*)(void *))&intref_as_PyLong,
					      (PyObject *(*)(void *))&pair_as_PyTuple
					      );
  if(py_state_decoder == NULL){
    clean_intersection(&target);
    return NULL;
  }

  
  PyObject *py_rule_decoder = dict_as_PyDict(target.rule_decoder,
					     (PyObject *(*)(void *))&intref_as_PyLong,
					     (PyObject *(*)(void *))&pair_as_PyTuple
					     );
  if(py_rule_decoder == NULL){
    clean_intersection(&target);
    Py_DECREF(py_state_decoder); 
    return NULL;
  }
  
  // we can clean the C state and rule decoders which will be no longer of use
  // However, we obviously must keep the intersection automaton alive.
  clean_decoders(&target);
  PyObject *target_automaton = pyta_from_builtin(target.a);
  if(target_automaton == NULL){
    target.a->destroy(target.a);
    Py_DECREF(py_state_decoder);
    Py_DECREF(py_rule_decoder);
    return NULL;
  }
  
  PyObject *summary =  Py_BuildValue("(NiNN)", target_automaton, target.a->final, py_state_decoder, py_rule_decoder);
  if(summary == NULL){
    target.a->destroy(target.a);
    Py_DECREF(py_state_decoder);
    Py_DECREF(py_rule_decoder);
    Py_DECREF(target_automaton);
    return NULL;
  }

  build_td_index_from_explicit(target.a);
  build_bu_index_from_explicit(target.a);
  return summary;
}

static PyMethodDef pyta_methods[] = {
  {"compile", compile_automaton, METH_VARARGS, "Build an automaton from a list of rules."},
  {"intersect_cky", pyta_intersect_cky, METH_VARARGS, "Intersect two automata. RHS automaton must be acyclic, or else very bad things will ensue."},
  {"intersect", pyta_intersect, METH_VARARGS, "Intersect two automata."},
  {NULL, NULL, 0, NULL} /*Sentinel*/
};

static struct PyModuleDef pytamodule = {
  PyModuleDef_HEAD_INIT,
  "pyta",
  "Module providing tree automata (over integer states and symbols).",
  -1,
  pyta_methods
};


PyMODINIT_FUNC PyInit_pyta(void){
  PyObject *m;
  if(PyType_Ready(&pyta_automaton_t) < 0) return NULL;

  m = PyModule_Create(&pytamodule);
  if(m == NULL) return NULL;

  Py_INCREF(&pyta_automaton_t);
  return m;
}
