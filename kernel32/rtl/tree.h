#ifndef RTL_TREE_H_
#define RTL_TREE_H_

#include "defines.h"
#include <stddef.h>

EXPORT_API

/**
 * @brief Generic fields for any tree node
 * @param type Tree node type
 */
#define TREE_GENERAL(type) \
  type *left, *right, *parent; \
  size_t key; \
  void *main; 

END_EXPORT_API

#endif