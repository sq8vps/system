#ifndef BST_H_
#define BST_H_

#include <stdint.h>

struct BstNode
{
  struct BstNode *left, *right;
  struct BstNode *buddy;
  uintptr_t key;
  uintptr_t val;
};

struct BstNode* BstInsertExisting(struct BstNode *parent, struct BstNode *node);

struct BstNode* BstFindExactMatch(struct BstNode *root, uintptr_t key);

struct BstNode* BstInsert(struct BstNode **root, uintptr_t key);

struct BstNode* BstInsertPair(struct BstNode **rootA, struct BstNode **rootB, uintptr_t keyA, uintptr_t keyB);

void BstDelete(struct BstNode **root, struct BstNode *node);

struct BstNode* BstFindLess(struct BstNode *root, uintptr_t key);

struct BstNode* BstFindGreaterOrEqual(struct BstNode *root, uintptr_t key);

#endif