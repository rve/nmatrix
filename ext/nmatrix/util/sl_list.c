//
// SciRuby is Copyright (c) 2010 - 2012, Ruby Science Foundation
// NMatrix is Copyright (c) 2012, Ruby Science Foundation
//
// Please see LICENSE.txt for additional copyright notices.
//
// == Contributing
//
// By contributing source code to SciRuby, you agree to be bound by
// our Contributor Agreement:
//
// * https://github.com/SciRuby/sciruby/wiki/Contributor-Agreement
//
// == sl_list.c
//
// Singly-linked list implementation

/*
 * Standard Includes
 */

/*
 * Project Includes
 */

#include "sl_list.h"

/*
 * Macros
 */

/*
 * Global Variables
 */

/*
 * Forward Declarations
 */

/*
 * Functions
 */

////////////////
// Lifecycle //
///////////////

/*
 * Creates an empty linked list.
 */
LIST* list_create(void) {
  LIST* list;
  
  //if (!(list = malloc(sizeof(LIST)))) return NULL;
  list = ALLOC( LIST );

  //fprintf(stderr, "    create_list LIST: %p\n", list);

  list->first = NULL;
  return list;
}

/*
 * Deletes the linked list and all of its contents. If you want to delete a
 * list inside of a list, set recursions to 1. For lists inside of lists inside
 *  of the list, set it to 2; and so on. Setting it to 0 is for no recursions.
 */
void list_delete(LIST* list, size_t recursions) {
  NODE* next;
  NODE* curr = list->first;

  while (curr != NULL) {
    next = curr->next;

    if (recursions == 0) {
      //fprintf(stderr, "    free_val: %p\n", curr->val);
      free(curr->val);
      
    } else {
      //fprintf(stderr, "    free_list: %p\n", list);
      delete_list(curr->val, recursions - 1);
    }

    free(curr);
    curr = next;
  }
  //fprintf(stderr, "    free_list: %p\n", list);
  free(list);
}

/*
 * Documentation goes here.
 */
void list_mark(LIST* list, size_t recursions) {
  NODE* next;
  NODE* curr = list->first;

  while (curr != NULL) {
    next = curr->next;
    
    if (recursions == 0) {
    	rb_gc_mark(*((VALUE*)(curr->val)));
    	
    } else {
    	mark_list(curr->val, recursions - 1);
    }
    
    curr = next;
  }
}

///////////////
// Accessors //
///////////////

/* 
 * Given a list and a key/value-ptr pair, create a node (and return that node).
 * If NULL is returned, it means insertion failed.
 * If the key already exists in the list, replace tells it to delete the old
 * value and put in your new one. !replace means delete the new value.
 */
NODE* list_insert(LIST* list, bool replace, size_t key, void* val) {
  NODE *ins;

  if (list->first == NULL) {
  	// List is empty
  	
    //if (!(ins = malloc(sizeof(NODE)))) return NULL;
    ins = ALLOC(NODE);
    ins->next             = NULL;
    ins->val              = val;
    ins->key              = key;
    list->first           = ins;
    
    return ins;

  } else if (key < list->first->key) {
  	// Goes at the beginning of the list
  	
    //if (!(ins = malloc(sizeof(NODE)))) return NULL;
    ins = ALLOC(NODE);
    ins->next             = list->first;
    ins->val              = val;
    ins->key              = key;
    list->first           = ins;
    
    return ins;
  }

  // Goes somewhere else in the list.
  ins = list_find_nearest_from(list->first, key);

  if (ins->key == key) {
    // key already exists
    if (replace) {
      free(ins->val);
      ins->val = val;
      
    } else {
    	free(val);
    }
    
    return ins;

  } else {
  	return list_insert_after(ins, key, val);
  }
}

/*
 * Documentation goes here.
 */
NODE* list_insert_after(NODE* node, size_t key, void* val) {
  NODE* ins;

  //if (!(ins = malloc(sizeof(NODE)))) return NULL;
  ins = ALLOC(NODE);

  // insert 'ins' between 'node' and 'node->next'
  ins->next  = node->next;
  node->next = ins;

  // initialize our new node
  ins->key   = key;
  ins->val   = val;

  return ins;
}

/*
 * Returns the value pointer (not the node) for some key. Note that it doesn't
 * free the memory for the value stored in the node -- that pointer gets
 * returned! Only the node is destroyed.
 */
void* list_remove(LIST* list, size_t key) {
  NODE *f, *rm;
  void* val;

  if (!list->first || list->first->key > key) {
  	// empty list or def. not present
  	return NULL;
  }

  if (list->first->key == key) {
    val = list->first->val;
    rm  = list->first;
    
    list->first = rm->next;
    free(rm);
    
    return val;
  }

  f = list_find_preceding_from(list->first, key);
  if (!f || !f->next) {
  	// not found, end of list
  	return NULL;
  }

  if (f->next->key == key) {
    // remove the node
    rm      = f->next;
    f->next = rm->next;

    // get the value and free the memory for the node
    val = rm->val;
    free(rm);
    return val;
  }

  return NULL; // not found, middle of list
}

///////////
// Tests //
///////////

/*
 * Are all values in the two lists equal? If one is missing a value, but the
 * other isn't, does the value in the list match the default value?
 *
 * FIXME: Add templating.
 */
bool list_eqeq_list(const LIST* left, const LIST* right, const void* left_val, const void* right_val, int8_t dtype, size_t recursions, size_t* checked) {
  NODE *lnext, *lcurr = left->first, *rnext, *rcurr = right->first;
	
	// Select the appropriate equality tester.
  (*eqeq)(const void*, const void*, const int, const int) = ElemEqEq[dtype][0];
	
  //fprintf(stderr, "list_eqeq_list: recursions=%d\n", recursions);

  if (lcurr) lnext = lcurr->next;
  if (rcurr) rnext = rcurr->next;

  while (lcurr && rcurr) {

    if (lcurr->key == rcurr->key) {
    	// MATCHING KEYS
    	
      if (recursions == 0) {
        ++(*checked);
        
        if (!eqeq(lcurr->val, rcurr->val, 1, nm_sizeof[dtype])) {
        	return false;
        }
        
      } else if (!list_eqeq_list(lcurr->val, rcurr->val, left_val, right_val, dtype, recursions - 1, checked)) {
        return false;
      }

      // increment both iterators
      rcurr = rnext;
      if (rcurr) rnext = rcurr->next;
      lcurr = lnext;
      if (lcurr) lnext = lcurr->next;

    } else if (lcurr->key < rcurr->key) {
    	// NON-MATCHING KEYS

      if (recursions == 0) {
        // compare left entry to right default value
        ++(*checked);
        
        if (!eqeq(lcurr->val, right_val, 1, nm_sizeof[dtype])) {
        	return false;
        }
        
      } else if (!list_eqeq_value(lcurr->val, right_val, dtype, recursions - 1, checked)) {
        return false;
      }

      // increment left iterator
      lcurr = lnext;
      if (lcurr) lnext = lcurr->next;

    } else {
			// if (rcurr->key < lcurr->key)
      
      if (recursions == 0) {
        // compare right entry to left default value
        ++(*checked);
        if (!eqeq(rcurr->val, left_val, 1, nm_sizeof[dtype])) {
        	return false;
        }
        
      } else if (!list_eqeq_value(rcurr->val, left_val, dtype, recursions - 1, checked)) {
        return false;
      }

      // increment right iterator
      rcurr = rnext;
      if (rcurr) rnext = rcurr->next;
    }

  }

  /*
   * One final check, in case we get to the end of one list but not the other
   * one.
   */
  if (lcurr) {
  	// nothing left in right-hand list
    if (!eqeq(lcurr->val, right_val, 1, nm_sizeof[dtype])) {
    	return false;
    }
    
  } else if (rcurr) {
  	// nothing left in left-hand list
    if (!eqeq(rcurr->val, left_val, 1, nm_sizeof[dtype])) {
    	return false;
    }
  }

  /*
   * Nothing different between the two lists -- but make sure after this return
   * that you compare the default values themselves, if we haven't visited
   * every value in the two matrices.
   */
  return true;
}

/*
 * Do all values in a list == some value?
 *
 * FIXME: Add templating.
 */
bool list_eqeq_value(const LIST* l, const void* v, int8_t dtype, size_t recursions, size_t* checked) {
  NODE *next, *curr = l->first;
  
  // Select the appropriate equality tester.
  (*eqeq)(const void*, const void*, const int, const int) = ElemEqEq[dtype][0];

  while (curr) {
    next = curr->next;

    if (recursions == 0) {
      ++(*checked);
      if (!eqeq(curr->val, v, 1, nm_sizeof[dtype])) {
      	return false;
      }
      
    } else if (!list_eqeq_value(curr->val, v, dtype, recursions - 1, checked)) {
      return false;
    }

    curr = next;
  }
  
  return true;
}

/////////////
// Utility //
/////////////

/*
 * Find some element in the list and return the node ptr for that key.
 */
NODE* list_find(LIST* list, size_t key) {
  NODE* f;
  if (!list->first) {
  	// empty list -- does not exist
  	return NULL;
  }

  // see if we can find it.
  f = list_find_nearest_from(list->first, key);
  
  if (!f || f->key == key) {
  	return f;
  }
  
  return NULL;
}

/*
 * Finds the node that should go before whatever key we request, whether or not
 * that key is present.
 */
NODE* list_find_preceding_from(NODE* prev, size_t key) {
  NODE* curr = prev->next;

  if (!curr || key <= curr->key) {
  	return prev;
  	
  } else {
  	return list_find_preceding_from(curr, key);
  }
}

/*
 * Finds the node or, if not present, the node that it should follow. NULL
 * indicates no preceding node.
 */
NODE* list_find_nearest(LIST* list, size_t key) {
  return list_find_nearest_from(list->first, key);
}

/*
 * Finds a node or the one immediately preceding it if it doesn't exist.
 */
NODE* list_find_nearest_from(NODE* prev, size_t key) {
  NODE* f;

  if (prev && prev->key == key) {
  	return prev;
  }

  f = list_find_preceding_from(prev, key);

  if (!f->next) {
  	return f;
  	
  } else if (key == f->next->key) {
  	return f->next;
  	
  } else {
  	return prev;
  }
}

/////////////////////////
// Copying and Casting //
/////////////////////////

/*
 * Documentation goes here.
 *
 * FIXME: Add templating.
 */
void list_cast_copy_contents(LIST* lhs, LIST* rhs, int8_t lhs_dtype, int8_t rhs_dtype, size_t recursions) {
  NODE *lcurr, *rcurr;

  if (rhs->first) {
    // copy head node
    rcurr = rhs->first;
    lcurr = lhs->first = ALLOC( NODE );

    while (rcurr) {
      lcurr->key = rcurr->key;

      if (recursions == 0) {
      	// contents is some kind of value
      	
        lcurr->val = ALLOC_N(char, nm_sizeof[lhs_dtype]);
        //fprintf(stderr, "    create_val: %p\n", lcurr->val);

        if (lhs_dtype == rhs_dtype) {
        	memcpy(lcurr->val, rcurr->val, nm_sizeof[lhs_dtype]);
        	
        } else {
        	SetFuncs[lhs_dtype][rhs_dtype](1, lcurr->val, 0, rcurr->val, 0);
        }

      } else {
      	// contents is a list
      	
        lcurr->val = ALLOC( LIST );
        //fprintf(stderr, "    create_list: %p\n", lcurr->val);

        cast_copy_list_contents(lcurr->val, rcurr->val, lhs_dtype, rhs_dtype, recursions-1);
      }
      if (rcurr->next) {
      	lcurr->next = ALLOC( NODE );
      	
      } else {
      	lcurr->next = NULL;
      }

      lcurr = lcurr->next;
      rcurr = rcurr->next;
    }
    
  } else {
    lhs->first = NULL;
  }
}
