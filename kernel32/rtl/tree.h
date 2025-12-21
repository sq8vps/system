#ifndef RTL_TREE_H_
#define RTL_TREE_H_

#include "defines.h"
#include <stddef.h>

EXPORT_API

typedef size_t tree_key_t;

/**
 * @brief Generic fields for any tree node
 * @param type Tree node type
 */
#define TREE_GENERAL(type) \
  union{type *left; type *child;}; \
  union{type *right; type *next;}; \
  type *parent; \
  tree_key_t key; \
  void *aux; 

END_EXPORT_API

#endif