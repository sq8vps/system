/**
 * @file tree.h
 * @brief General tree definition
 * @ingroup rtl_tree
 */

#ifndef RTL_TREE_H_
#define RTL_TREE_H_

#include "defines.h"
#include <stddef.h>

/**
 * @addtogroup rtl_tree Tree implementations
 * @ingroup rtl
 * @{
 */

DRIVER_API

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
  union {int8_t i8; uint8_t u8; int16_t i16; uint16_t u16; int32_t i32; uint32_t u32; int64_t i64; uint64_t u64; size_t sz; void *v} aux; 

END_DRIVER_API

/**
 * @}
 */

#endif