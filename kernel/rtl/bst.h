
/**
 * @file bst.h
 * @brief Binary search tree library
 * @ingroup rtl_bst
 */
#ifndef RTL_BST_H_
#define RTL_BST_H_

#include <stddef.h>
#include "tree.h"

/**
 * @addtogroup rtl_bst General binary search tree
 * @note In order to provide tree "abstraction" locally, that is, to alias all \c Bst... symbols as \c Tree... symbols,
 * define \c BST_PROVIDE_ABSTRACTION before including this header.
 * @ingroup rtl_tree
 * @{
 */

EXPORT_API

/**
 * @brief Generic binary search tree node
 */
struct BstNode
{
  TREE_GENERAL(struct BstNode);
};

#define BSTNODE struct BstNode tree

#ifdef BST_PROVIDE_ABSTRACTION

#define TreeInsert BstInsert
#define TreeFindGreaterOrEqual BstFindGreaterOrEqual
#define TreeFindExact BstFindExact
#define TreeFindLess BstFindLess
#define TreeRemove BstRemove
#define TreeNode BstNode
#define TREENODE BSTNODE

#endif

/**
 * @brief Insert a node into the binary search tree
 * @param root Root of the tree
 * @param node Node to insert
 * @return New root of the tree
 */
struct BstNode* BstInsert(struct BstNode *root, struct BstNode *node);

/**
 * @brief Find a node with an exact key in the binary search tree
 * @param root Root of the tree
 * @param key Key to search for
 * @return Node with the exact key or NULL if not found
 */
struct BstNode *BstFindExact(struct BstNode *root, tree_key_t key);

/**
 * @brief Find the largest node with a key less than the given key
 * @param root Root of the tree
 * @param key Key to search for
 * @return Node with the largest key less than the given key or NULL if not found
 */
struct BstNode *BstFindLess(struct BstNode *root, tree_key_t key);

/**
 * @brief Find the smallest node with a key greater than or equal to the given key
 * @param root Root of the tree
 * @param key Key to search for
 * @return Node with the smallest key greater than or equal to the given key or NULL if not found
 */
struct BstNode *BstFindGreaterOrEqual(struct BstNode *root, tree_key_t key);

/**
 * @brief Remove a node from the binary search tree
 * @param root Root of the tree
 * @param node Node to remove
 * @return New root of the tree
 */
struct BstNode *BstRemove(struct BstNode *root, struct BstNode *node);

END_EXPORT_API

/**
 * @}
 */

#endif