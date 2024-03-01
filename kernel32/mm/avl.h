#ifndef KERNEL_AVL_H_
#define KERNEL_AVL_H_

/**
 * @file avl.h
 * @brief AVL Tree library for memory managers
 * 
 * Provides routines for AVL trees manipulation for memory management.
 * @ingroup mm
*/


#include <stdint.h>
#include <stdbool.h>

/**
 * @defgroup avl AVL Tree library
 * @ingroup mm
 * @{
*/

/**
 * @brief A generic AVL tree node structure for memory management
*/
struct MmAvlNode
{
    uintptr_t key; //node key - address or size
    
    struct MmAvlNode *left, *right; //children pointers
    struct MmAvlNode *buddy; //buddy pointer (address-size pair)
    union
    {
        uintptr_t val; //additional value
        void *ptr;
    };
    int height; //node height
    bool dynamic; //was this node dynamically allocated?
};

/**
 * @brief Insert existing node to AVL tree
 * @param *parent Parent node
 * @param *node Node to be inserted
 * @return Parent node after insertion
*/
struct MmAvlNode* MmAvlInsertExisting(struct MmAvlNode *parent, struct MmAvlNode *node);

/**
 * @brief Insert new node to AVL tree
 * @param **root Root of the tree
 * @param key Key for the new node
 * @return Created node or NULL on failure
*/
struct MmAvlNode* MmAvlInsert(struct MmAvlNode **root, uintptr_t key);

/**
 * @brief Insert pair of nodes to AVL tree
 * @param **rootA Root of the tree A
 * @param **rootB Root of the tree B
 * @param keyA Key for the new node A
 * @param keyB Key for the new node B
 * @return Pointer to node A or NULL on failure
*/
struct MmAvlNode* MmAvlInsertPair(struct MmAvlNode **rootA, struct MmAvlNode **rootB, uintptr_t keyA, uintptr_t keyB);

/**
 * @brief Delete node from AVL tree
 * @param **root Root of the tree
 * @param *node Node to be deleted
*/
void MmAvlDelete(struct MmAvlNode **root, struct MmAvlNode *node);
 
/**
 * @brief Find node with lowest possible key, but greater or equal to given key
 * @param *root Tree root
 * @param key Key
 * @return Best-fitting node or NULL if unavailable
*/
struct MmAvlNode* MmAvlFindGreaterOrEqual(struct MmAvlNode *root, uintptr_t key);

/**
 * @brief Find node
 * @param *root Tree root
 * @param key Key
 * @return Node or NULL if not found
*/
struct MmAvlNode* MmAvlFindExactMatch(struct MmAvlNode *root, uintptr_t key);

/**
 * @brief Find node with greatest possible key, but less than given key
 * @param *root Tree root
 * @param key Key
 * @return Best-fitting node or NULL if not found
*/
struct MmAvlNode* MmAvlFindLess(struct MmAvlNode *root, uintptr_t key);

/**
 * @}
*/

#endif