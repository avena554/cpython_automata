#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "automata.h"

/* 
 * TODO: Use python dedicated memory allocators instead of malloc
 * make automata.h able to receive configuration for the allocator.
 */
typedef struct{
  PyObject_HEAD
  automaton a;
} pyta_automaton;


/* C methods for the type automaton */
static PyObject *rules_from_parent(PyObject *self, PyObject *args);
static PyObject *get_rule(PyObject *self, PyObject *args);
static PyObject *rules_from_children(PyObject *self, PyObject *args);

static void pyta_automaton_dealloc(PyObject *o){
  fprintf(stderr, "Python is destroying the automaton\n");
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
    {"rules_from_parent", rules_from_parent, METH_VARARGS,
     "Return a list of indices of rules applicable from a given parent state"
    },
    {"rules_from_children", rules_from_children, METH_VARARGS,
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

static void clean_several(int n_pointers, void *const pointers[]){
  for(int i = 0; i < n_pointers; ++i){
      free(pointers[i]);
  }
}

static PyObject *PyList_from_lmap(label_to_ruleset lmap){
  // count 'em rules
  rule_iterator r_it = lmap->all_rules(lmap);
  int n_rules_out = 0;
  for(int *rule_index = r_it->next(r_it); rule_index != NULL; rule_index = r_it->next(r_it)){
      ++n_rules_out;
  }
  r_it->destroy(r_it);

  // create the returned Python list, with appropriate size
  PyObject *out_list = PyList_New(n_rules_out);
  if(out_list == NULL)
    return NULL;

  // populate the list
  r_it = lmap->all_rules(lmap);
  Py_ssize_t k = 0;
  for(int *rule_index = r_it->next(r_it); rule_index != NULL; rule_index = r_it->next(r_it)){
    PyObject *py_index = Py_BuildValue("i", *rule_index);
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
  r_it->destroy(r_it);
  return out_list;
}

static PyObject *rules_from_parent(PyObject *self, PyObject *args){
  automaton a = ((pyta_automaton *)self)->a;
  int parent;
  PyObject *out_list;

  if(!PyArg_ParseTuple(args, "i", &parent)) return NULL;
  label_to_ruleset lmap = a->td_query(a, parent);

  out_list = PyList_from_lmap(lmap);
  
  lmap->destroy(lmap);
  
  return out_list;  
}

static PyObject *rules_from_children(PyObject *self, PyObject *args){
  automaton a = ((pyta_automaton *)self)->a;
  PyObject *children;
  PyObject *out_list;
  Py_ssize_t n, i;
  int *c_children;
  
  //parse a list from the arguments
  if(!PyArg_ParseTuple(args, "O", &children) || !PySequence_Check(children)) return NULL;

  n = PySequence_Size(children);
  c_children = malloc(n*sizeof(int));

  for(i = 0; i < n; ++i){
    PyObject *child = PySequence_GetItem(children, i);    
    if(!PyLong_Check(child)){
      free(c_children);
      return NULL;
    }
    long value = PyLong_AsLong(child);
    Py_DECREF(child);
    c_children[i] = (int)value;
  }

  label_to_ruleset lmap = a->bu_query(a, c_children, (int)n);

  out_list = PyList_from_lmap(lmap);
  
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

static PyObject *compile_automaton(PyObject *self, PyObject *args){  
  PyObject *rules;
  int n_states;
  int n_symb;
  
  if(!PyArg_ParseTuple(args, "iiO", &n_states, &n_symb, &rules) || !PySequence_Check(rules)) return NULL;

  Py_ssize_t n_rules = PySequence_Size(rules);
  if(n_rules < 0) return NULL;

  // fprintf(stderr, "there are %d states, %d symbols and %d rules\n", n_states, n_symb, (int)n_rules);
  // fprintf(stderr, "allocating space for rules\n");

  struct rule *c_rules = malloc(n_rules * sizeof(struct rule));  
  int *all_rules_children = NULL;
  void *const to_clean[2] = {c_rules, all_rules_children};
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

    // fprintf(stderr, "retrieving label\n");
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

  PyObject *empty_tuple = PyTuple_New(0);
  if(empty_tuple == NULL){
    clean_several(n_to_clean, to_clean);
    return NULL;
  }
  
  pyta_automaton *pa = (pyta_automaton *)PyObject_CallObject((PyObject *)&pyta_automaton_t, empty_tuple);
  Py_DECREF(empty_tuple);
  if(pa == NULL){
    clean_several(n_to_clean, to_clean);
    return NULL;
  }
  
  pa->a = create_explicit_automaton(n_states, n_symb, c_rules, n_rules); 
  clean_several(n_to_clean, to_clean);
  build_td_index_from_explicit(pa->a);
  build_bu_index_from_explicit(pa->a);
  return (PyObject *)pa;  
}


static PyMethodDef pyta_methods[] = {
  {"compile", compile_automaton, METH_VARARGS, "Build an automaton from a list of rules."},
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
