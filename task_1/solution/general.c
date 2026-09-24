#include "general.h"

#include <assert.h>
#include <stdlib.h>

char** reallocArr(char** arr, size_t newSize)
{
    assert(arr);

    char** newArr = (char**) realloc( arr, newSize);
    assert(newArr);

    return newArr;
}
