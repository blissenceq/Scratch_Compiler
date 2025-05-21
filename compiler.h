#ifndef SCRATCHCOMPILER_H
#define SCRATCHCOMPILER_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

struct token;
struct position;
struct compile_process;
struct lex_process;
struct lex_process_functions;
struct node;

int compile_file(const char *filename, const char *out_filename, int file);
void compiler_error(struct compile_process *compiler, const char *message, ...);
void compiler_warning(struct compile_process *compiler, const char *message, ...);
struct compile_process *compile_process_create(const char *filename, const char *filename_out, int flags);
char compile_process_next_char(struct lex_process *lex_process);
char compile_process_peek_char(struct lex_process *lex_process);
void compile_process_push_char(struct lex_process *lex_process, char c);

struct lex_process *lex_process_create(struct compile_process *compiler, struct lex_process_functions *functions, void *private);
void lex_process_free(struct lex_process *process);
void *lex_process_private(struct lex_process *process);
struct vector *lex_process_tokens(struct lex_process *process);
int lex(struct lex_process *process);

bool is_token_keyword(struct token *token, const char *value);
bool token_is_comment_newline_or_newline_seperator(struct token *token);
bool token_is_symbol(struct token *token, char c);
static struct token *token_next();
static struct token *token_peek_next();

int parse(struct compile_process *process);
void parse_single_token_to_node();
static void parser_ignore_comment_or_newline(struct token *token);

void node_set_vector(struct vector *vector, struct vector *vector_root);
void node_push(struct node *node);
struct node *node_peek_or_null();
struct node *node_peek();
struct node *node_pop();
struct node *node_create(struct node *_node);
bool node_is_expressionable(struct node *node);
struct node *node_peek_expressionable_or_null();
void node_make_expression(struct node *left, struct node *right, const char *operator);

enum
{
    COMPILER_FILE_COMPILED_OK,
    COMPILER_FAILED_WITH_ERRORS
};

enum
{
    LEXICAL_ANALYSIS_ALL_OK,
    LEXICAL_ANALYSIS_INPUT_ERROR
};

enum
{
    PARSING_ALL_OKAY,
    PARSING_FAILED_WITH_ERROR
};

enum
{
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_KEYWORD,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_SYMBOL,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_COMMENT,
    TOKEN_TYPE_NEWLINE
};

enum
{
    NODE_TYPE_EXPRESSION,
    NODE_TYPE_EXPRESSION_PARENTHESES,
    NODE_TYPE_NUMBER,
    NODE_TYPE_IDENTIFIER,
    NODE_TYPE_STRING,
    NODE_TYPE_VARIABLE,
    NODE_TYPE_VARIABLE_LIST,
    NODE_TYPE_FUNCTION,
    NODE_TYPE_BODY,
    NODE_TYPE_STATEMENT_RETURN,
    NODE_TYPE_STATEMENT_IF,
    NODE_TYPE_STATEMENT_ELSE,
    NODE_TYPE_STATEMENT_WHILE,
    NODE_TYPE_STATEMENT_DO_WHILE,
    NODE_TYPE_STATEMENT_FOR,
    NODE_TYPE_STATEMENT_BREAK,
    NODE_TYPE_STATEMENT_CONTINUE,
    NODE_TYPE_STATEMENT_SWITCH,
    NODE_TYPE_STATEMENT_CASE,
    NODE_TYPE_STATEMENT_DEFAULT,
    NODE_TYPE_STATEMENT_GOTO,

    NODE_TYPE_UNARY,
    NODE_TYPE_TENARY,
    NODE_TYPE_LABEL,
    NODE_TYPE_STRUCT,
    NODE_TYPE_UNION,
    NODE_TYPE_BRACKET,
    NODE_TYPE_CAST,
    NODE_TYPE_BLANK
};

enum
{
    NODE_FLAG_INSIDE_EXPRESSION = 0b00000001
};

#define S_EQ(string1, string2) \
    string1 &&string2 && (strcmp(string1, string2) == 0)

#define NUMERIC_CASE \
    case '0':        \
    case '1':        \
    case '2':        \
    case '3':        \
    case '4':        \
    case '5':        \
    case '6':        \
    case '7':        \
    case '8':        \
    case '9'

#define OPERATOR_CASE_EXCLUDING_DIVISION \
    case '+':                            \
    case '-':                            \
    case '*':                            \
    case '>':                            \
    case '<':                            \
    case '^':                            \
    case '%':                            \
    case '!':                            \
    case '=':                            \
    case '~':                            \
    case '|':                            \
    case '&':                            \
    case '(':                            \
    case '[':                            \
    case ',':                            \
    case '.':                            \
    case '?'

#define SYMBOL_CASE \
    case '{':       \
    case '}':       \
    case '#':       \
    case ':':       \
    case ';':       \
    case ']':       \
    case ')':       \
    case '\\'

typedef char (*LEX_PROCESS_NEXT_CHAR)(struct lex_process *process);
typedef char (*LEX_PROCESS_PEEK_CHAR)(struct lex_process *process);
typedef void (*LEX_PROCESS_PUSH_CHAR)(struct lex_process *process, char c);

struct position
{
    int line;
    int column;
    const char *filename;
};

struct token
{
    int type;
    int flags;
    struct position pos;

    union
    {
        char cval;
        const char *sval;
        unsigned int inum;
        unsigned long lnum;
        unsigned long long llnum;
        void *any;
    };

    // True if there is a white space between the current token and next token
    bool whitespace;
    const char *between_brackets;
};

struct node
{
    int type;
    int flag;
    struct position pos;

    struct node_binded
    {
        struct node *owner;    // Pointer to the body node
        struct node *function; // Pointer to the parent function the node is part of
    } binded;

    union
    {
        char cval;
        const char *sval;
        unsigned int inum;
        unsigned long lnum;
        unsigned long long llnum;
    };

    union
    {
        struct expression
        {
            struct node *left;
            struct node *right;
            const char *operator;
        } expression;
    };
};

struct compile_process
{
    // The flags to determine how this file should be compiled
    int flags;
    struct position pos;

    struct compile_process_input_file
    {
        FILE *fp;
        const char *abs_path;
    } cfile;

    struct vector *token_vec;
    struct vector *node_vec;
    struct vector *node_tree_vec;

    FILE *ofile;
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
    struct vector *token_vec;
    struct compile_process *compiler;

    int current_expression_count;
    struct buffer *parentheses_buffer;
    struct lex_process_functions *function;

    // This is private data that lexer doesn't understand but the person using the lexer does
    void *private;
};

#endif