#ifndef RTL_TRIE_H_
#define RTL_TRIE_H_

#include <stddef.h>
#include "tree.h"

EXPORT_API

/**
 * @brief Generic trie node
 */
struct TrieNode
{
  TREE_GENERAL(struct TrieNode);
};

/**
 * @brief Insert a node under a parent node in the trie
 * @param parent Parent node under which to insert the new node
 * @param node Node to insert
 */
void TrieInsertUnderParent(struct TrieNode *parent, struct TrieNode *node);

/**
 * @brief Insert a node into the trie
 * @param root Root of the trie
 * @param node Node to insert
 * @param keys Array of keys representing the path to the node
 * @param keyCount Number of keys in the array
 * @return New root of the trie or NULL if insertion failed
 * @attention The path specified by keys must already exist in the trie.
 * If the path does not exist, the tree will not be modified and original root will be returned.
 */
struct TrieNode* TrieInsert(struct TrieNode *root, struct TrieNode *node, tree_key_t keys[], size_t keyCount);

/**
 * @brief Find a node in the trie by its keys
 * @param start Starting node of the search
 * @param *keys Array of keys representing the path to the node
 * @param keyCount Number of keys in the array
 * @return Node found or NULL if not found
 */
struct TrieNode *TrieFind(struct TrieNode *start, const tree_key_t *keys, size_t keyCount);

END_EXPORT_API

#endif