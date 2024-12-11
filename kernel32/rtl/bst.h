#ifndef RTL_BST_H_
#define RTL_BST_H_

#include <stddef.h>
#include "tree.h"

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

struct BstNode* BstInsert(struct BstNode *root, struct BstNode *node);

struct BstNode *BstFindExact(struct BstNode *root, size_t key);

struct BstNode *BstFindLess(struct BstNode *root, size_t key);

struct BstNode *BstFindGreaterOrEqual(struct BstNode *root, size_t key);

struct BstNode *BstRemove(struct BstNode *root, struct BstNode *node);

#endif