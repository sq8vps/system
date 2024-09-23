#include "bst.h"
#include "mm/heap.h"

struct BstNode* BstInsertExisting(struct BstNode *parent, struct BstNode *node)
{
    if(NULL == parent)
        return node;
    
    struct BstNode *top = parent;
    
    struct BstNode *next = NULL;
    while(NULL != parent)
    {
        next = parent;
        if(node->key < parent->key)
            parent = parent->left;
        else
            parent = parent->right;
    }
    
    if(node->key < next->key)
        next->left = node;
    else
        next->right = node;
    
    return top;
}
 
static struct BstNode* BstDeleteNode(struct BstNode *root, struct BstNode *node)
{
    struct BstNode *parent = NULL;
    struct BstNode *current = root;
    while((NULL != current) && (current != node))
    {
        parent = current;
        if(node->key < current->key)
            current = current->left;
        else if(node->key > current->key)
            current = current->right;
        else //if(current != node)
            current = current->right;
    }
    
    if(NULL == current)
        return root;
    
    if(NULL != node->buddy)
        node->buddy->buddy = NULL;
    
    if((NULL == node->left) || (NULL == node->right))
    {
        //no children or one child
        struct BstNode *child = (NULL != node->left) ? node->left : node->right;
        
        if(NULL == child) //no children
        {
            //simply remove the node
            
            if(NULL != parent)
            {
                if(node == parent->left)
                    parent->left = NULL;
                else //if(node == parent->right)
                    parent->right = NULL;
            }
            else
                root = NULL;
            
            MmFreeKernelHeap(node);
        }
        else //one child
        {
            *node = *child;
            if(NULL != node->buddy)
                node->buddy->buddy = node;
            
            MmFreeKernelHeap(child);
        }
    }
    else //two children
    {
        //find successor, that is, the lowest element in the right subtree
        struct BstNode *p = node;
        current = node->right;
        while(NULL != current->left)
        {
            p = current;
            current = current->left;
        }
        
        //there is no direct successor
        if(node != p)
        {
            //update successor's parent
            p->left = current->right;
            current->right = node->right;
            current->left = node->left;
        }
        else //right child is a direct successor
        {
            current->left = node->left;
        }
        
        *node = *current;
        if(NULL != node->buddy)
            node->buddy->buddy = node;
        
        free(current);
    }
    
    return root;
}


struct BstNode* BstFindGreaterOrEqual(struct BstNode *root, uintptr_t key)
{
    struct BstNode *best = NULL;

    while(NULL != root)
    {   
        if(root->key >= key)
        {
            if((NULL == best) || 
            (best && (root->key < best->key))) //smaller that the previous best fit, but still fitting
                best = root;
        }
        
        if(root->key > key)
        {
            root = root->left;
        }
        else //if(root->key <= key)
        {
            root = root->right;
        }
    }

    if(NULL != best)
    {
        return best;
    }

    return NULL;
}

struct BstNode* BstFindExactMatch(struct BstNode *root, uintptr_t key)
{
    while(NULL != root)
    {       
        if(root->key > key)
        {
            root = root->left;
        }
        else if(root->key < key)
        {
            root = root->right;
        }
        else //if(root->key == key)
            return root;
    }

    //else no node found
    return NULL;
}

struct BstNode* BstFindLess(struct BstNode *root, uintptr_t key)
{
    struct BstNode *best = NULL;

    while(NULL != root)
    {   
        if(root->key < key)
        {
            if((NULL == best) || 
            (best && (root->key > best->key)))
                best = root; //store it as new best fit
        }
        
        if(root->key > key)
        {
            root = root->left;
        }
        else //if(root->key <= key)
        {
            root = root->right;
        }
    }

    if(NULL != best)
    {
        return best;
    }
    
    return NULL;
}

struct BstNode* BstInsert(struct BstNode **root, uintptr_t key)
{
    struct BstNode* node = MmAllocateKernelHeap(sizeof(struct BstNode));
    
    if(NULL == node)
        return NULL;
    
    node->key = key;
    node->left = NULL;
    node->right = NULL;
    node->buddy = NULL;
    node->val = 0;

    *root = BstInsertExisting(*root, node);

    return node;
}

struct BstNode* BstInsertPair(struct BstNode **rootA, struct BstNode **rootB, uintptr_t keyA, uintptr_t keyB)
{
    struct BstNode *nA, *nB;
    nA = BstInsert(rootA, keyA);
    if(NULL == nA)
        return NULL;
    nB = BstInsert(rootB, keyB);
    if(NULL == nB)
    {
        BstDelete(rootA, nA);
        return NULL;
    }
    nB->buddy = nA;
    nA->buddy = nB;
    return nA;
}

void BstDelete(struct BstNode **root, struct BstNode *node)
{
    *root = BstDeleteNode(*root, node);
}