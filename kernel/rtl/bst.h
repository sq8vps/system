
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

DRIVER_API

/**
 * @brief Generic binary search tree node
 */
struct BstNode
{
  TREE_GENERAL(struct BstNode);
};

#define BSTNODE struct BstNode tree

#define BST_KEY(node) (((struct BstNode*)(node))->key)

#ifdef BST_PROVIDE_ABSTRACTION

#define TreeInsert BstInsert
#define TreeInsertEx BstInsertEx
#define TreeFindGreater BstFindGreater
#define TreeFindGreaterOrEqual BstFindGreaterOrEqual
#define TreeFindExact BstFindExact
#define TreeFindLess BstFindLess
#define TreeFindLessOrEqual BstFindLessOrEqual
#define TreeRemove BstRemove
#define TreeRemoveEx BstRemoveEx
#define TreeNode BstNode
#define TREENODE BSTNODE
#define TREE_KEY(node) BST_KEY(node)

#endif

/**
 * @brief External comparison function prototype
 * @param *A Node A
 * @param *B Node B
 * @return -1 if node A key < node B key, 0 if node A key = node B key, 1 if node A key < node B key
 */
typedef int (*BstCompareFunction)(struct BstNode *A, struct BstNode *B);

/**
 * @brief Insert a node into the binary search tree
 * @param root Root of the tree
 * @param node Node to insert
 * @return New root of the tree
 */
struct BstNode* BstInsert(struct BstNode *root, struct BstNode *node);

/**
 * @brief Insert a node into the binary search tree using the provided comparison function
 * @param **root (New) Root of the tree
 * @param *node Node to insert
 * @param cmp Comparison function
 * @return -1 when \a node is the new leftmost element
 * @return 1 when \a node is the new rightmost element
 * @return 0 when \a node is not the leftmost nor the rightmost element
 * @note By convention, when tree is empty, -1 is returned
 */
int BstInsertEx(struct BstNode **root, struct BstNode *node, BstCompareFunction cmp);

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
 * @brief Find the largest node with a key less than or equal to the given key
 * @param root Root of the tree
 * @param key Key to search for
 * @return Node with the largest key less than or equal to the given key or NULL if not found
 */
struct BstNode *BstFindLessOrEqual(struct BstNode *root, tree_key_t key);


/**
 * @brief Find the smallest node with a key greater than to the given key
 * @param root Root of the tree
 * @param key Key to search for
 * @return Node with the smallest key greater than the given key or NULL if not found
 */
struct BstNode *BstFindGreater(struct BstNode *root, tree_key_t key);

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

/**
 * @brief Remove a node from the binary search tree using the provided comparison function
 * @param root Root of the tree
 * @param node Node to remove
 * @param cmp Comparison function
 * @return New root of the tree
 */
struct BstNode *BstRemoveEx(struct BstNode *root, struct BstNode *node, BstCompareFunction cmp);

END_DRIVER_API

/**
 * @}
 */

#endif