#include "trie.h"

void TrieInsertUnderParent(struct TrieNode *parent, struct TrieNode *node)
{
    node->next = NULL;
    node->child = NULL;
    
    if(NULL != parent)
    {
        while(NULL != parent->next)
        {
            parent = parent->next;
        }
        parent->next = node;
        node->parent = parent;
    }
}

struct TrieNode* TrieInsert(struct TrieNode *root, struct TrieNode *node, tree_key_t keys[], size_t keyCount)
{
    struct TrieNode *parent = NULL;

    node->next = NULL;
    node->child = NULL;
    node->parent = NULL;

    if(0 == keyCount)
    {
        if(NULL == root)
            return node;
        else
        {
            parent = root;
            while(NULL != parent->next)
                parent = parent->next;
            
            parent->next = node;
            node->parent = parent;
            return root;
        }
    }
    else if(NULL == root)
    {
        return NULL;
    }
    
    parent = TrieFind(root, keys, keyCount);
    if(NULL != parent)
    {
        while(NULL != parent->next)
        {
            parent = parent->next;
        }
        parent->next = node;
        node->parent = parent;
    }
    
    return root;
}

struct TrieNode *TrieFind(struct TrieNode *start, const tree_key_t *keys, size_t keyCount)
{
    size_t i = 0;

    if(0 == keyCount)
        return NULL;

    while(NULL != start)
    {       
        if(start->key == keys[i])
        {
            if(++i == keyCount)
                return start;

            start = start->child;
        }
        start = start->next;
    }

    return NULL;
}
