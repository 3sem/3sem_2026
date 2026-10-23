#ifndef PARSER_H
#define PARSER_H

#include "expression.h"

#include <stddef.h>

int parseFunction(expressionTree_t* expressionTree, const char* expression,
                  char* error, size_t errorSize);

#endif /* PARSER_H */
