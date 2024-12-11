#include "bst.h"

 struct BstNode* BstInsert(struct BstNode *root, struct BstNode *node)
{
    node->left = NULL;
    node->right = NULL;
    
    if(NULL == root)
        return node;
    
    struct BstNode *top = root;
    
    struct BstNode *next = NULL;
    while(NULL != root)
    {
        next = root;
        if(node->key < root->key)
            root = root->left;
        else
            root = root->right;
    }
    
    if(node->key < next->key)
        next->left = node;
    else
        next->right = node;
    
    node->parent = next;
    
    return top;
}

 struct BstNode *BstFindExact(struct BstNode *root, size_t key)
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

 struct BstNode *BstFindLess(struct BstNode *root, size_t key)
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

 struct BstNode *BstFindGreaterOrEqual(struct BstNode *root, size_t key)
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

 struct BstNode *BstRemove(struct BstNode *root, struct BstNode *node)
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
    
    if((NULL == node->left) || (NULL == node->right))
    {
        //no children or one child
        struct BstNode *child = (NULL != node->left) ? node->left : node->right;
        
        if(NULL != parent)
        {
            if(node == parent->left)
                parent->left = child;
            else //if(node == parent->right)
                parent->right = child;
            
            if(NULL != child)
                child->parent = parent;
        }
        else
        {
            root = child;
            if(NULL != child)
                child->parent = NULL;
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
            if(NULL != current->right)
                current->right->parent = p;
            current->right = node->right;
            node->right->parent = current;
            current->left = node->left;
            node->left->parent = current;
        }
        else //right child is a direct successor
        {
            current->left = node->left;
            node->left->parent = current;
        }
        
        if(NULL != parent)
        {
            if(node == parent->left)
                parent->left = current;
            else
                parent->right = current;

            current->parent = parent;
        }
        else
        {
            root = current;
            current->parent = NULL;
        }
    }
    
    return root;
}