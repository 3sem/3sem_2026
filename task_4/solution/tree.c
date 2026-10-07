#include "tree.h"

#include <stdlib.h>

void treeCtor(tree_t* tree)
{
    if (tree != NULL)
    {
        tree->root = NULL;
    }
}

treeNode_t* createTreeNode(void* data, treeNode_t* left, treeNode_t* right)
{
    treeNode_t* newNode = calloc(1, sizeof(*newNode));
    if (newNode == NULL)
    {
        return NULL;
    }

    newNode->data = data;
    newNode->left = left;
    newNode->right = right;
    return newNode;
}

void destroyTreeNode(treeNode_t* node, dataDtor_t dataDtor)
{
    if (node == NULL)
    {
        return;
    }

    destroyTreeNode(node->left, dataDtor);
    destroyTreeNode(node->right, dataDtor);
    if (dataDtor != NULL)
    {
        dataDtor(node->data);
    }
    free(node);
}

void treeDtor(tree_t* tree, dataDtor_t dataDtor)
{
    if (tree == NULL)
    {
        return;
    }

    destroyTreeNode(tree->root, dataDtor);
    tree->root = NULL;
}
