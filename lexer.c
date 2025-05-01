#include "complier.h"
#include "string.h"
#include "helpers/vector.h"
#include "helpers/buffer.h"

#include <assert.h>
#include <ctype.h>


#define LEX_GETC_IF(buffer, c, expression)      \
    for (c = peekc(); expression; c = peekc())  \
    {                                           \
        buffer_write(buffer, c);                \
        nextc();                                \
    }

static struct lex_process* lex_process;
static struct token temp_token;

struct token* read_next_token();

static char peekc()
{
    return lex_process->function->peek_char(lex_process);
}

static void pushc(char c)
{
    lex_process->function->push_char(lex_process, c);
}

static char nextc()
{
    char c = lex_process->function->next_char(lex_process);
    lex_process->pos.column += 1;
    if (c == '\n') {
        lex_process->pos.line += 1;
        lex_process->pos.column = 1;
    }

    return c;
}

static struct position lex_file_position()
{
    return lex_process->pos;
}

struct token* token_create(struct token* _token)
{
    memcpy(&temp_token, _token, sizeof(struct token));
    temp_token.pos = lex_file_position();
    return &temp_token;
}

static struct token* lexer_last_token()
{
    return vector_back_or_null(lex_process->token_vec);
}

static void lex_finish_expression()
{
    lex_process->current_expression_count--;
    if (lex_process->current_expression_count < 0) {
        compiler_error(lex_process->compiler, "Closed an expression that was never opened");
    }
}

struct token* handle_whitespace()
{
    struct token* last_token = lexer_last_token();
    if (last_token) {
        last_token->whitespace = true;
    }

    nextc();
    return read_next_token();
}

const char* read_number_str()
{
    const char* num = NULL;
    struct buffer* buffer = buffer_create();
    char c = peekc();
    LEX_GETC_IF(buffer, c, (c >= '0' && c <= '9'));

    buffer_write(buffer, 0x00);
    return buffer_ptr(buffer);
}

unsigned long long read_number()
{
    const char* s = read_number_str();
    return atoll(s);
}

struct token* token_make_number_for_value(unsigned long number)
{
    return token_create(&(struct token){.type=TOKEN_TYPE_NUMBER,.llnum=number});
}

struct token* token_make_number()
{
    return token_make_number_for_value(read_number());
}

struct token* token_make_string(char start_delimiter, char end_delimiter)
{
    struct buffer* buffer = buffer_create();
    assert(nextc() == start_delimiter);
    char c = nextc();
    for (; c != end_delimiter && c != EOF; c = nextc()) {
        if (c == '\\') {
            // Implement code for line break
            continue;
        }

        buffer_write(buffer, c);
    }

    buffer_write(buffer, 0x00);
    return token_create(&(struct token){.type=TOKEN_TYPE_STRING,.sval=buffer_ptr(buffer)});
}

struct token* token_make_newline()
{
    nextc();
    return token_create(&(struct token){.type=TOKEN_TYPE_NEWLINE});
}

struct token* token_make_symbol()
{
    char c = nextc();
    if (c == ')') {
        lex_finish_expression();
    }
    return token_create(&(struct token){.type=TOKEN_TYPE_SYMBOL,.cval=c});
}

struct token* token_make_keyword_or_identifier()
{
    struct buffer* buffer = buffer_create();
    char c = 0;
    LEX_GETC_IF(buffer, c, (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_');
    buffer_write(buffer, 0x00);

    // Check for keyword token

    return token_create(&(struct token){.type=TOKEN_TYPE_IDENTIFIER,.sval=buffer_ptr(buffer)});
}

struct token* read_default_token()
{
    char c = peekc();
    if (isalpha(c) || c == '_') {
        return token_make_keyword_or_identifier();
    } else return NULL;
}

struct token* read_next_token()
{
    struct token* token = NULL;
    char c = peekc();
    switch (c)
    {
        NUMERIC_CASE:
            token = token_make_number();
            break;

        SYMBOL_CASE:
            token = token_make_symbol();
            break;

        case '"':
            token = token_make_string('"', '"');
            break;

        case ' ':
        case '\t':
            token = handle_whitespace();
            break;
        
        case '\n':
            token = token_make_newline();
            break;

        case EOF:
            break;
        
        default:
            token = read_default_token();
            if (token == NULL) {
                compiler_error(lex_process->compiler, "Unexpected token\n");
            } else break;
    }

    return token;
}

int lex(struct lex_process* process) 
{
    process->current_expression_count = 0;
    process->parentheses_buffer = NULL;
    process->pos.filename = process->compiler->cfile.abs_path;

    lex_process = process;
    struct token* token = read_next_token();
    while(token) {
        vector_push(process->token_vec, token);
        token = read_next_token();
    }

    return LEXICAL_ANALYSIS_ALL_OK;
}