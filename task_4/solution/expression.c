#include "expression.h"

#include <math.h>
#include <stdlib.h>

typedef struct
{
    expressionNodeType_t type;
    double value;
} expressionNodeData_t;

static double evaluateNode(const treeNode_t* node, double x);
static void expressionNodeDataDtor(void* data);

void expressionTreeCtor(expressionTree_t* expressionTree)
{
    if (expressionTree != NULL)
    {
        treeCtor(&expressionTree->tree);
    }
}

void expressionTreeDtor(expressionTree_t* expressionTree)
{
    if (expressionTree != NULL)
    {
        treeDtor(&expressionTree->tree, expressionNodeDataDtor);
    }
}

void expressionTreeSetRoot(expressionTree_t* expressionTree, treeNode_t* root)
{
    if (expressionTree == NULL)
    {
        destroyExpressionNode(root);
        return;
    }

    expressionTreeDtor(expressionTree);
    expressionTree->tree.root = root;
}

double evaluateExpression(const expressionTree_t* expressionTree, double x)
{
    if (expressionTree == NULL || expressionTree->tree.root == NULL)
    {
        return NAN;
    }
    return evaluateNode(expressionTree->tree.root, x);
}

treeNode_t* createExpressionNode(expressionNodeType_t type, double value,
                                 treeNode_t* left, treeNode_t* right)
{
    expressionNodeData_t* data = calloc(1, sizeof(*data));
    if (data == NULL)
    {
        return NULL;
    }

    data->type = type;
    data->value = value;

    treeNode_t* node = createTreeNode(data, left, right);
    if (node == NULL)
    {
        free(data);
    }
    return node;
}

void destroyExpressionNode(treeNode_t* node)
{
    destroyTreeNode(node, expressionNodeDataDtor);
}

static double evaluateNode(const treeNode_t* node, double x)
{
    const expressionNodeData_t* data = node->data;
    switch (data->type)
    {
        case EXPR_NUMBER:   return data->value;
        case EXPR_VARIABLE: return x;
        case EXPR_ADD:      return evaluateNode(node->left, x) + evaluateNode(node->right, x);
        case EXPR_SUB:      return evaluateNode(node->left, x) - evaluateNode(node->right, x);
        case EXPR_MUL:      return evaluateNode(node->left, x) * evaluateNode(node->right, x);
        case EXPR_DIV:      return evaluateNode(node->left, x) / evaluateNode(node->right, x);
        case EXPR_POW:      return pow(evaluateNode(node->left, x), evaluateNode(node->right, x));
        case EXPR_NEG:      return -evaluateNode(node->left, x);
    }
    return NAN;
}

static void expressionNodeDataDtor(void* data)
{
    free(data);
}
