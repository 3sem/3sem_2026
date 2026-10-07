#include "parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

static treeNode_t* parseWholeExpression(const char** curBufferPos, char* error, size_t errorSize);
static treeNode_t* parseAdditiveExpression(const char** curBufferPos, char* error,
                                           size_t errorSize);
static treeNode_t* parseMultiplicativeExpression(const char** curBufferPos, char* error,
                                                 size_t errorSize);
static treeNode_t* parseUnary(const char** curBufferPos, char* error, size_t errorSize);
static treeNode_t* parsePower(const char** curBufferPos, char* error, size_t errorSize);
static treeNode_t* parsePrimary(const char** curBufferPos, char* error, size_t errorSize);
static treeNode_t* parseNumber(const char** curBufferPos, char* error, size_t errorSize);
static void skipSpaces(const char** curBufferPos);
static void setError(char* error, size_t errorSize, const char* message);

int parseFunction(expressionTree_t* expressionTree, const char* expression,
                  char* error, size_t errorSize)
{
    if (error != NULL && errorSize > 0)
    {
        error[0] = '\0';
    }
    if (expressionTree == NULL || expression == NULL)
    {
        setError(error, errorSize, "null argument");
        return -1;
    }

    const char* curBufferPos = expression;
    treeNode_t* root = parseWholeExpression(&curBufferPos, error, errorSize);
    if (root == NULL)
    {
        return -1;
    }

    expressionTreeSetRoot(expressionTree, root);
    return 0;
}

static void skipSpaces(const char** curBufferPos)
{
    while (isspace((unsigned char)**curBufferPos))
    {
        (*curBufferPos)++;
    }
}

static void setError(char* error, size_t errorSize, const char* message)
{
    if (error != NULL && errorSize > 0 && error[0] == '\0')
    {
        snprintf(error, errorSize, "%s", message);
    }
}

static treeNode_t* parseWholeExpression(const char** curBufferPos, char* error, size_t errorSize)
{
    treeNode_t* result = parseAdditiveExpression(curBufferPos, error, errorSize);
    skipSpaces(curBufferPos);

    if (result != NULL && **curBufferPos != '\0')
    {
        destroyExpressionNode(result);
        setError(error, errorSize, "unexpected symbol after expression");
        return NULL;
    }

    return result;
}

static treeNode_t* parseAdditiveExpression(const char** curBufferPos, char* error,
                                           size_t errorSize)
{
    treeNode_t* val1 = parseMultiplicativeExpression(curBufferPos, error, errorSize);

    while (val1 != NULL)
    {
        skipSpaces(curBufferPos);
        char operation = **curBufferPos;
        if (operation != '+' && operation != '-')
        {
            break;
        }
        (*curBufferPos)++;

        treeNode_t* val2 = parseMultiplicativeExpression(curBufferPos, error, errorSize);
        if (val2 == NULL)
        {
            destroyExpressionNode(val1);
            return NULL;
        }

        treeNode_t* operationNode = createExpressionNode(operation == '+' ? EXPR_ADD : EXPR_SUB,
                                                         0, val1, val2);
        if (operationNode == NULL)
        {
            destroyExpressionNode(val1);
            destroyExpressionNode(val2);
            setError(error, errorSize, "not enough memory");
            return NULL;
        }
        val1 = operationNode;
    }

    return val1;
}

static treeNode_t* parseMultiplicativeExpression(const char** curBufferPos, char* error,
                                                 size_t errorSize)
{
    treeNode_t* val1 = parseUnary(curBufferPos, error, errorSize);

    while (val1 != NULL)
    {
        skipSpaces(curBufferPos);
        char operation = **curBufferPos;
        if (operation != '*' && operation != '/')
        {
            break;
        }
        (*curBufferPos)++;

        treeNode_t* val2 = parseUnary(curBufferPos, error, errorSize);
        if (val2 == NULL)
        {
            destroyExpressionNode(val1);
            return NULL;
        }

        treeNode_t* operationNode = createExpressionNode(operation == '*' ? EXPR_MUL : EXPR_DIV,
                                                         0, val1, val2);
        if (operationNode == NULL)
        {
            destroyExpressionNode(val1);
            destroyExpressionNode(val2);
            setError(error, errorSize, "not enough memory");
            return NULL;
        }
        val1 = operationNode;
    }

    return val1;
}

static treeNode_t* parseUnary(const char** curBufferPos, char* error, size_t errorSize)
{
    skipSpaces(curBufferPos);
    if (**curBufferPos != '+' && **curBufferPos != '-')
    {
        return parsePower(curBufferPos, error, errorSize);
    }

    char operation = **curBufferPos;
    (*curBufferPos)++;
    treeNode_t* operand = parseUnary(curBufferPos, error, errorSize);
    if (operand == NULL || operation == '+')
    {
        return operand;
    }

    treeNode_t* negateNode = createExpressionNode(EXPR_NEG, 0, operand, NULL);
    if (negateNode == NULL)
    {
        destroyExpressionNode(operand);
        setError(error, errorSize, "not enough memory");
    }
    return negateNode;
}

static treeNode_t* parsePower(const char** curBufferPos, char* error, size_t errorSize)
{
    treeNode_t* base = parsePrimary(curBufferPos, error, errorSize);
    if (base == NULL)
    {
        return NULL;
    }

    skipSpaces(curBufferPos);
    if (**curBufferPos != '^')
    {
        return base;
    }

    (*curBufferPos)++;
    treeNode_t* exponent = parseUnary(curBufferPos, error, errorSize);
    if (exponent == NULL)
    {
        destroyExpressionNode(base);
        return NULL;
    }

    treeNode_t* powerNode = createExpressionNode(EXPR_POW, 0, base, exponent);
    if (powerNode == NULL)
    {
        destroyExpressionNode(base);
        destroyExpressionNode(exponent);
        setError(error, errorSize, "not enough memory");
    }
    return powerNode;
}

static treeNode_t* parsePrimary(const char** curBufferPos, char* error, size_t errorSize)
{
    skipSpaces(curBufferPos);

    if (**curBufferPos == '(')
    {
        (*curBufferPos)++;
        treeNode_t* result = parseAdditiveExpression(curBufferPos, error, errorSize);
        skipSpaces(curBufferPos);
        if (result == NULL || **curBufferPos != ')')
        {
            destroyExpressionNode(result);
            setError(error, errorSize, "expected ')'");
            return NULL;
        }
        (*curBufferPos)++;
        return result;
    }

    if (**curBufferPos == 'x')
    {
        (*curBufferPos)++;
        treeNode_t* variable = createExpressionNode(EXPR_VARIABLE, 0, NULL, NULL);
        if (variable == NULL)
        {
            setError(error, errorSize, "not enough memory");
        }
        return variable;
    }

    return parseNumber(curBufferPos, error, errorSize);
}

static treeNode_t* parseNumber(const char** curBufferPos, char* error, size_t errorSize)
{
    char* endPos = NULL;
    double number = strtod(*curBufferPos, &endPos);

    if (endPos == *curBufferPos)
    {
        setError(error, errorSize, "expected number, x or '('");
        return NULL;
    }

    *curBufferPos = endPos;
    treeNode_t* numberNode = createExpressionNode(EXPR_NUMBER, number, NULL, NULL);
    if (numberNode == NULL)
    {
        setError(error, errorSize, "not enough memory");
    }
    return numberNode;
}

