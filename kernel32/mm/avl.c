#include "avl.h"
#include <stdbool.h>
#include "defines.h"
#include "../../cdefines.h"
#include "heap.h"

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

static struct MmAvlNode* mmAvlInsertPreallocated(struct MmAvlNode *parent, struct MmAvlNode *node)
{
    if(NULL == parent)
        return node;
    
    if (node->key < parent->key)
    {
        parent->left = mmAvlInsertPreallocated(parent->left, node);
    }
    else
    {
        parent->right = mmAvlInsertPreallocated(parent->right, node);
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

struct MmAvlNode* MmAvlInsert(struct MmAvlNode **addressRoot, struct MmAvlNode **sizeRoot, uintptr_t address, uintptr_t size)
{
    if(0 == size)
        return NULL;
    
    struct MmAvlNode* addressNode = MmAllocateKernelHeap(sizeof(struct MmAvlNode));
    struct MmAvlNode* sizeNode = MmAllocateKernelHeap(sizeof(struct MmAvlNode));
    
    if((NULL == addressNode) || (NULL == sizeNode))
        return NULL;
    
    addressNode->key = address;
    addressNode->height = 1;
    addressNode->left = NULL;
    addressNode->right = NULL;
    sizeNode->key = size;
    sizeNode->height = 1;
    sizeNode->left = NULL;
    sizeNode->right = NULL;
    addressNode->buddy = sizeNode; //store buddy node pointer for both nodes
    sizeNode->buddy = addressNode;

    *addressRoot = mmAvlInsertPreallocated(*addressRoot, addressNode);
    *sizeRoot = mmAvlInsertPreallocated(*sizeRoot, sizeNode);

    return addressNode;
}

static struct MmAvlNode* mmAvlLowestNode(struct MmAvlNode* node)
{
    while (NULL != node->left)
        node = node->left;
 
    return node;
}
 
static struct MmAvlNode* mmAvlDeleteNode(struct MmAvlNode *root, uintptr_t key)
{
    if (NULL == root)
        return root;
 
    if (key < root->key)
        root->left = mmAvlDeleteNode(root->left, key);
    else if(key > root->key)
        root->right = mmAvlDeleteNode(root->right, key);
    else
    {
        //1 or 0 children
        if((NULL == root->left) || (NULL == root->right))
        {
            struct MmAvlNode *temp = root->left ? root->left : root->right;
            
            if (temp == NULL) //no children
            {
                temp = root;
                root = NULL;
            }
            else //1 child
            {
                *root = *temp; //copy child to root
                root->buddy->buddy = root; //update buddy of the other node
            }

            MmFreeKernelHeap(temp);
        }
        else //2 children
        {
            //get root successor
            struct MmAvlNode* temp = mmAvlLowestNode(root->right);

            root->key = temp->key; //copy value
            root->buddy = temp->buddy; //copy buddy pointer
            root->buddy->buddy = root; //update buddy of the other node

            root->right = mmAvlDeleteNode(root->right, temp->key);
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

void MmAvlDelete(struct MmAvlNode **nodeRoot, struct MmAvlNode **buddyRoot, struct MmAvlNode *node)
{
    *buddyRoot = mmAvlDeleteNode(*buddyRoot, node->buddy->key);
    *nodeRoot = mmAvlDeleteNode(*nodeRoot, node->key);
}
 
struct MmAvlNode* MmAvlFindFreeMemory(struct MmAvlNode *root, uintptr_t size)
{
    struct MmAvlNode *bestRegion = NULL; //best fitting region

    while(NULL != root)
    {   
        if(root->key >= size) //the region is free and big enough
        {
            if((NULL == bestRegion) || 
            (bestRegion && (root->key < bestRegion->key))) //and at the same time it is smaller that the previous best fit
                bestRegion = root; //store it as new best fit
        }
        
        if(root->key > size)
        {
            root = root->left;
        }
        else //if(root->key <= size)
        {
            root = root->right;
        }
    }

    if(NULL != bestRegion) //return best fitting region if available
    {
        return bestRegion;
    }
    //else no region found
    return NULL;
}

struct MmAvlNode* MmAvlFindMemoryByAddress(struct MmAvlNode *root, uintptr_t address)
{
    while(NULL != root)
    {       
        if(root->key > address)
        {
            root = root->left;
        }
        else if(root->key < address)
        {
            root = root->right;
        }
        else //if(root->key == address)
            return root;
    }

    //else no region found
    return NULL;
}

struct MmAvlNode* MmAvlFindHighestMemoryByAddressLimit(struct MmAvlNode *root, uintptr_t address)
{
    struct MmAvlNode *bestRegion = NULL; //best fitting region

    while(NULL != root)
    {   
        if(root->key < address)
        {
            if((NULL == bestRegion) || 
            (bestRegion && (root->key > bestRegion->key)))
                bestRegion = root; //store it as new best fit
        }
        
        if(root->key > address)
        {
            root = root->left;
        }
        else //if(root->key <= address)
        {
            root = root->right;
        }
    }

    if(NULL != bestRegion) //return best fitting region if available
    {
        return bestRegion;
    }
    //else no region found
    return NULL;
}
