#include "avl.h"
#include <stdbool.h>
#include "defines.h"
#include "../cdefines.h"
#include "heap.h"
#include "assert.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
 
static int mmAvlHeight(struct MmAvlNode *node)
{
    if (NULL == node)
        return 0;

    return node->height;
}
 
static struct MmAvlNode *mmAvlRightRotate(struct MmAvlNode *root)
{
    struct MmAvlNode *newRoot = root->left;
    struct MmAvlNode *nextTree = newRoot->right;

    newRoot->right = root;
    root->left = nextTree;
 
    root->height = MAX(mmAvlHeight(root->left), mmAvlHeight(root->right)) + 1;
    newRoot->height = MAX(mmAvlHeight(newRoot->left), mmAvlHeight(newRoot->right)) + 1;

    return newRoot;
}
 

static struct MmAvlNode *mmAvlLeftRotate(struct MmAvlNode *root)
{
    struct MmAvlNode *newRoot = root->right;
    struct MmAvlNode *nextTree = newRoot->left;
 
    newRoot->left = root;
    root->right = nextTree;

    root->height = MAX(mmAvlHeight(root->left), mmAvlHeight(root->right)) + 1;
    newRoot->height = MAX(mmAvlHeight(newRoot->left), mmAvlHeight(newRoot->right)) + 1;

    return newRoot;
}
 

static int mmAvlBalanceFactor(struct MmAvlNode *node)
{
    if (NULL == node)
        return 0;
    
    return mmAvlHeight(node->left) - mmAvlHeight(node->right);
}

struct MmAvlNode* MmAvlInsertExisting(struct MmAvlNode *parent, struct MmAvlNode *node)
{
    if(NULL == parent)
        return node;
    
    if(node->key < parent->key)
    {
        ASSERT(parent != parent->left);
        parent->left = MmAvlInsertExisting(parent->left, node);
    }
    else
    {
        ASSERT(parent != parent->right);
        parent->right = MmAvlInsertExisting(parent->right, node);
    }

    parent->height = MAX(mmAvlHeight(parent->left), mmAvlHeight(parent->right)) + 1;

    int balance = mmAvlBalanceFactor(parent);

    if ((balance > 1) && (node->key < parent->left->key))
        return mmAvlRightRotate(parent);

    if ((balance < -1) && (node->key > parent->right->key))
        return mmAvlLeftRotate(parent);

    if ((balance > 1) && (node->key > parent->left->key))
    {
        parent->left = mmAvlLeftRotate(parent->left);
        return mmAvlRightRotate(parent);
    }

    if ((balance < -1) && (node->key < parent->right->key))
    {
        parent->right = mmAvlRightRotate(parent->right);
        return mmAvlLeftRotate(parent);
    }

    return parent;
}

static struct MmAvlNode* mmAvlLowestNode(struct MmAvlNode* node)
{
    while(NULL != node->left)
        node = node->left;
 
    return node;
}
 
static struct MmAvlNode* mmAvlDeleteNode(struct MmAvlNode *root, struct MmAvlNode *node)
{
    if(NULL == root)
        return root;
 
    if(node->key < root->key)
    {
        ASSERT(root != root->left);
        root->left = mmAvlDeleteNode(root->left, node);
    }
    else if(node->key > root->key)
    {
        ASSERT(root != root->right);
        root->right = mmAvlDeleteNode(root->right, node);
    }
    else if(node != root)
    {
        ASSERT(root != root->right);
        root->right = mmAvlDeleteNode(root->right, node);
    }
    else
    {
        //1 or 0 children
        if((NULL == root->left) || (NULL == root->right))
        {
            struct MmAvlNode *child = root->left ? root->left : root->right;
            
            if(child == NULL) //no children
            {
                child = root;
                root = NULL;
            }
            else //1 child
            {
                *root = *child; //copy child to root
                if(NULL != root->buddy)
                    root->buddy->buddy = root; //update buddy of the other node
            }

            if(child->dynamic)
                MmFreeKernelHeap(child);
        }
        else //2 children
        {
            //get root successor
            struct MmAvlNode* child = mmAvlLowestNode(root->right);

            root->key = child->key; //copy value
            root->val = child->val;
            root->buddy = child->buddy; //copy buddy pointer
            if(NULL != child->buddy)
                child->buddy->buddy = root; //update buddy of the other node

            root->right = mmAvlDeleteNode(root->right, child);
        }
    }

    if(NULL == root)
        return NULL;

    root->height = MAX(mmAvlHeight(root->left), mmAvlHeight(root->right)) + 1;

    int balance = mmAvlBalanceFactor(root);
 
    if ((balance > 1) && (mmAvlBalanceFactor(root->left) >= 0))
        return mmAvlRightRotate(root);
 
    if ((balance > 1) && (mmAvlBalanceFactor(root->left) < 0))
    {
        root->left =  mmAvlLeftRotate(root->left);
        return mmAvlRightRotate(root);
    }
 
    if ((balance < -1) && (mmAvlBalanceFactor(root->right) <= 0))
        return mmAvlLeftRotate(root);
 
    if ((balance < -1) && (mmAvlBalanceFactor(root->right) > 0))
    {
        root->right = mmAvlRightRotate(root->right);
        return mmAvlLeftRotate(root);
    }
 
    return root;
}



 
struct MmAvlNode* MmAvlFindGreaterOrEqual(struct MmAvlNode *root, uintptr_t key)
{
    struct MmAvlNode *best = NULL;

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

struct MmAvlNode* MmAvlFindExactMatch(struct MmAvlNode *root, uintptr_t key)
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

struct MmAvlNode* MmAvlFindLess(struct MmAvlNode *root, uintptr_t key)
{
    struct MmAvlNode *best = NULL;

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

struct MmAvlNode* MmAvlInsert(struct MmAvlNode **root, uintptr_t key)
{
    struct MmAvlNode* node = MmAllocateKernelHeap(sizeof(struct MmAvlNode));
    
    if(NULL == node)
        return NULL;
    
    node->key = key;
    node->height = 1;
    node->left = NULL;
    node->right = NULL;
    node->buddy = NULL;
    node->dynamic = true;

    *root = MmAvlInsertExisting(*root, node);

    return node;
}

struct MmAvlNode* MmAvlInsertPair(struct MmAvlNode **rootA, struct MmAvlNode **rootB, uintptr_t keyA, uintptr_t keyB)
{
    struct MmAvlNode *nA, *nB;
    nA = MmAvlInsert(rootA, keyA);
    if(NULL == nA)
        return NULL;
    nB = MmAvlInsert(rootB, keyB);
    if(NULL == nB)
    {
        MmAvlDelete(rootA, nA);
        return NULL;
    }
    nB->buddy = nA;
    nA->buddy = nB;
    return nA;
}

void MmAvlDelete(struct MmAvlNode **root, struct MmAvlNode *node)
{
    *root = mmAvlDeleteNode(*root, node);
}