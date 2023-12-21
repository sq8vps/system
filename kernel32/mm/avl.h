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
    int height; //node height
};

/**
 * @brief Insert node to AVL trees
 * @param **addressRoot Root of tree sorted by address
 * @param **sizeRoot Root of tree sorted by size
 * @param address Address for the new node
 * @param size Size for the new node
*/
struct MmAvlNode* MmAvlInsert(struct MmAvlNode **addressRoot, struct MmAvlNode **sizeRoot, uintptr_t address, uintptr_t size);

/**
 * @brief Delete node from AVL trees
 * @param **nodeRoot Root of the tree that contains the node to be deleted
 * @param **buddyRoot Root of the tree that contains the buddy of the node to be deleted
 * @param *node Node to be deleted
 * @attention The node buddy is also deleted
*/
void MmAvlDelete(struct MmAvlNode **nodeRoot, struct MmAvlNode **buddyRoot, struct MmAvlNode *node);
 
/**
 * @brief Find best-fitting free memory block in AVL tree
 * @param *root Tree root (sorted by size)
 * @param size Requested region size
 * @return Best-fitting node or NULL if unavailable
*/
struct MmAvlNode* MmAvlFindFreeMemory(struct MmAvlNode *root, uintptr_t size);

/**
 * @brief Find memory block by address (exact match)
 * @param *root Tree root (sorted by address)
 * @param address Block address to match
 * @return Block node or NULL if not found
*/
struct MmAvlNode* MmAvlFindMemoryByAddress(struct MmAvlNode *root, uintptr_t address);

/**
 * @brief Find memory block with the highest address but below the given limit
 * @param *root Tree root (sorted by address)
 * @param address Address limit, that is the returned block must start below this address
 * @return Matching block node or NULL if not found
*/
struct MmAvlNode* MmAvlFindHighestMemoryByAddressLimit(struct MmAvlNode *root, uintptr_t address);

/**
 * @}
*/

#endif