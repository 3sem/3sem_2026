#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "tree.h"

typedef enum
{
    EXPR_NUMBER,
    EXPR_VARIABLE,
    EXPR_ADD,
    EXPR_SUB,
    EXPR_MUL,
    EXPR_DIV,
    EXPR_POW,
    EXPR_NEG
} expressionNodeType_t;

typedef struct
{
    tree_t tree;
} expressionTree_t;

void expressionTreeCtor(expressionTree_t* expressionTree);
void expressionTreeDtor(expressionTree_t* expressionTree);
void expressionTreeSetRoot(expressionTree_t* expressionTree, treeNode_t* root);
double evaluateExpression(const expressionTree_t* expressionTree, double x);

typedef struct
{
    double min;
    double max;
} expressionRange_t;

expressionRange_t estimateExpressionRange(const expressionTree_t* expressionTree,
                                          const double borders[2]);

treeNode_t* createExpressionNode(expressionNodeType_t type, double value,
                                 treeNode_t* left, treeNode_t* right);
void destroyExpressionNode(treeNode_t* node);

#endif /* EXPRESSION_H */
