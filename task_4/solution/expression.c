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

expressionRange_t estimateExpressionRange(const expressionTree_t* expressionTree,
                                          const double borders[2])
{
    const expressionRange_t invalid = {NAN, NAN};
    const unsigned int intervals = 100000;

    if (expressionTree == NULL || expressionTree->tree.root == NULL ||
        borders == NULL || !isfinite(borders[0]) || !isfinite(borders[1]) ||
        borders[0] > borders[1])
    {
        return invalid;
    }

    double first = evaluateExpression(expressionTree, borders[0]);
    if (!isfinite(first))
    {
        return invalid;
    }

    expressionRange_t range = {first, first};
    if (borders[0] == borders[1])
    {
        return range;
    }

    for (unsigned int i = 1; i <= intervals; ++i)
    {
        double t = (double)i / intervals;
        /* Weighted interpolation avoids overflow in borders[1] - borders[0]. */
        double x = i == intervals ? borders[1]
                                 : (1.0 - t) * borders[0] + t * borders[1];
        double value = evaluateExpression(expressionTree, x);
        if (!isfinite(value))
        {
            return invalid;
        }
        if (value < range.min)
        {
            range.min = value;
        }
        if (value > range.max)
        {
            range.max = value;
        }
    }
    return range;
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
