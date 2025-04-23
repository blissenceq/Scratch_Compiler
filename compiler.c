#include <stdarg.h>
#include <stdlib.h>
#include "complier.h"

struct lex_process_functions compiler_lex_functions = {
    .next_char = compile_process_next_char,
    .peek_char = compile_process_peek_char,
    .push_char = compile_process_push_char
};


int compile_file(const char* filename, const char* out_filename, int flags) {

    struct compile_process* compile_process = compile_process_create(filename, out_filename, flags);
    if (!compile_process) 
        return COMPILER_FAILED_WITH_ERRORS;

    // perform lexical analysis
    struct lex_process* lex_process = lex_process_create(compile_process, &compiler_lex_functions, NULL);
    if (!lex_process) 
        return COMPILER_FAILED_WITH_ERRORS;

    if (lex(lex_process) != LEXICAL_ANALYSIS_ALL_OK) 
        return COMPILER_FAILED_WITH_ERRORS;

    // perform parsing

    // perform code generation

    return COMPILER_FILE_COMPILED_OK;
}

void compiler_error(struct compile_process* compiler, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
    fprintf(stderr, " on line %i, col %i in file %s\n", compiler->pos.line, compiler->pos.column, compiler->pos.filename);
    exit(-1);
}

void compiler_warning(struct compile_process* compiler, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
    fprintf(stderr, " on line %i, col %i in file %s\n", compiler->pos.line, compiler->pos.column, compiler->pos.filename);
}