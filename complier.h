#ifndef SCRATCHCOMPILER_H
#define SCRATCHCOMPILER_H

#include <stdio.h>
#include <stdbool.h>

struct token;
struct position;
struct compile_process;
struct lex_process;
struct lex_process_functions;

int compile_file(const char* filename, const char* out_filename, int file);
struct compile_process* compile_process_create(const char* filename, const char* filename_out, int flags);
char compile_process_next_char(struct lex_process* lex_process);
char compile_process_peek_char(struct lex_process* lex_process);
void compile_process_push_char(struct lex_process* lex_process, char c);
struct lex_process* lex_process_create(struct compile_process* compiler, struct lex_process_functions* functions, void* private);
void lex_process_free(struct lex_process* process);
void* lex_process_private(struct lex_process* process);
struct vector* lex_process_tokens(struct lex_process* process);
int lex(struct lex_process* process);

enum {
    COMPILER_FILE_COMPILED_OK,
    COMPILER_FAILED_WITH_ERRORS
};

enum {
    LEXICAL_ANALYSIS_ALL_OK,
    LEXICAL_ANALYSIS_INPUT_ERROR
};

enum {
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_KEYWORD,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_SYMBOL,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_COMMENT,
    TOKEN_TYPE_NEWLINE
};

typedef char (*LEX_PROCESS_NEXT_CHAR)(struct lex_process* process);
typedef char (*LEX_PROCESS_PEEK_CHAR)(struct lex_process* process);
typedef void (*LEX_PROCESS_PUSH_CHAR)(struct lex_process* process, char c);

struct token
{
    int type;
    int flags;

    union
    {
        char cval;
        const char* sval;
        unsigned int inum;
        unsigned long lnum;
        unsigned long long llnum;
        void* any;
    };

    //True if there is awhite space between the current token and next token
    bool whitespace;
    const char* between_brackets;
    
};

struct position
{
    int line;
    int column;
    const char* filename;
};

struct compile_process
{
    //The flags to determine how this file should be compiled
    int flags;
    struct position pos;

    struct compile_process_input_file
    {
        FILE* fp;
        const char* abs_path;
    } cfile;


    FILE* ofile;
        
};

struct lex_process_functions
{
    LEX_PROCESS_NEXT_CHAR next_char;
    LEX_PROCESS_PEEK_CHAR peek_char;
    LEX_PROCESS_PUSH_CHAR push_char;
};


struct lex_process
{
    struct position pos;
    struct vector* token_vec;
    struct compile_process* compiler;

    int current_expression_count;
    struct buffer* parentheses_buffer;
    struct lex_process_functions* function;

    //This is private data that lexer doesn't understand but the person using the lexer does
    void* private;
};

#endif