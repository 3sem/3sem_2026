#ifndef TREE_H
#define TREE_H

typedef struct treeNode
{
    void* data;
    struct treeNode* left;
    struct treeNode* right;
} treeNode_t;

typedef struct
{
    treeNode_t* root;
} tree_t;

typedef void (*dataDtor_t)(void* data);

void treeCtor(tree_t* tree);
treeNode_t* createTreeNode(void* data, treeNode_t* left, treeNode_t* right);
void destroyTreeNode(treeNode_t* node, dataDtor_t dataDtor);
void treeDtor(tree_t* tree, dataDtor_t dataDtor);

#endif /* TREE_H */
