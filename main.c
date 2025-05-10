#include <stdio.h>
#include "complier.h"

int main()
{
    int result = compile_file("./test.c", "./test", 0);

    if (result == COMPILER_FILE_COMPILED_OK)
    {
        printf("Compilation Successful\n");
    }
    else if (result == COMPILER_FAILED_WITH_ERRORS)
    {
        printf("Compilation Failed because of Errors\n");
    }
    else
    {
        printf("Unknown Response from Compilation Unit");
    }

    return 0;
}