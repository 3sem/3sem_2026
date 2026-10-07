#include "expression.h"
#include "parser.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>

static expressionRange_t rangeFor(const char* formula, double a, double b)
{
    expressionTree_t tree;
    expressionTreeCtor(&tree);
    char error[128];
    assert(parseFunction(&tree, formula, error, sizeof(error)) == 0);
    const double borders[2] = {a, b};
    expressionRange_t range = estimateExpressionRange(&tree, borders);
    expressionTreeDtor(&tree);
    return range;
}

int main(void)
{
    expressionRange_t range = rangeFor("x^2", -2, 2);
    assert(range.min == 0 && range.max == 4);
    range = rangeFor("-x", -2, 3);
    assert(range.min == -3 && range.max == 2);
    range = rangeFor("-7", -2, 3);
    assert(range.min == -7 && range.max == -7);
    range = rangeFor("x^2", 3, 3);
    assert(range.min == 9 && range.max == 9);
    range = rangeFor("x", -DBL_MAX, DBL_MAX);
    assert(range.min == -DBL_MAX && range.max == DBL_MAX);
    range = rangeFor("1/x", -1, 1);
    assert(isnan(range.min) && isnan(range.max));
    range = rangeFor("x^0.5", -1, 1);
    assert(isnan(range.min) && isnan(range.max));
    range = rangeFor("x", 2, 1);
    assert(isnan(range.min) && isnan(range.max));
    range = rangeFor("x", 0, INFINITY);
    assert(isnan(range.min) && isnan(range.max));

    expressionTree_t tree;
    expressionTreeCtor(&tree);
    const double borders[2] = {0, 1};
    range = estimateExpressionRange(&tree, borders);
    assert(isnan(range.min) && isnan(range.max));
    range = estimateExpressionRange(NULL, borders);
    assert(isnan(range.min) && isnan(range.max));
    range = estimateExpressionRange(&tree, NULL);
    assert(isnan(range.min) && isnan(range.max));
    expressionTreeDtor(&tree);
    return 0;
}
