#include "compiler.h"
#include "string.h"
#include "helpers/vector.h"
#include "helpers/buffer.h"

#include <assert.h>
#include <ctype.h>

#define LEX_GETC_IF(buffer, c, expression)     \
    for (c = peekc(); expression; c = peekc()) \
    {                                          \
        buffer_write(buffer, c);               \
        nextc();                               \
    }

static struct lex_process *lex_process;
static struct token temp_token;

struct token *read_next_token();

bool lex_is_in_expression()
{
    return lex_process->current_expression_count > 0;
}

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
    if (lex_is_in_expression())
    {
        buffer_write(lex_process->parentheses_buffer, c);
    }

    lex_process->pos.column += 1;
    if (c == '\n')
    {
        lex_process->pos.line += 1;
        lex_process->pos.column = 1;
    }

    return c;
}

static char assert_next_character(char c)
{
    char next_c = nextc();
    assert(next_c == c);
    return nextc();
}

static struct position lex_file_position()
{
    return lex_process->pos;
}

struct token *token_create(struct token *_token)
{
    memcpy(&temp_token, _token, sizeof(struct token));
    temp_token.pos = lex_file_position();
    if (lex_is_in_expression())
    {
        temp_token.between_brackets = buffer_ptr(lex_process->parentheses_buffer);
    }
    return &temp_token;
}

static struct token *lexer_last_token()
{
    return vector_back_or_null(lex_process->token_vec);
}

static void lex_finish_expression()
{
    lex_process->current_expression_count--;
    if (lex_process->current_expression_count < 0)
    {
        compiler_error(lex_process->compiler, "Closed an expression that was never opened");
    }
}

struct token *handle_whitespace()
{
    struct token *last_token = lexer_last_token();
    if (last_token)
    {
        last_token->whitespace = true;
    }

    nextc();
    return read_next_token();
}

const char *read_number_str()
{
    const char *num = NULL;
    struct buffer *buffer = buffer_create();
    char c = peekc();
    LEX_GETC_IF(buffer, c, (c >= '0' && c <= '9'));

    buffer_write(buffer, 0x00);
    return buffer_ptr(buffer);
}

unsigned long long read_number()
{
    const char *s = read_number_str();
    return atoll(s);
}

bool iskeyword(const char *str)
{
    return S_EQ(str, "unsigned") ||
           S_EQ(str, "signed") ||
           S_EQ(str, "char") ||
           S_EQ(str, "short") ||
           S_EQ(str, "int") ||
           S_EQ(str, "long") ||
           S_EQ(str, "float") ||
           S_EQ(str, "double") ||
           S_EQ(str, "void") ||
           S_EQ(str, "struct") ||
           S_EQ(str, "union") ||
           S_EQ(str, "static") ||
           S_EQ(str, "__ignore_typecheck") ||
           S_EQ(str, "return") ||
           S_EQ(str, "include") ||
           S_EQ(str, "sizeof") ||
           S_EQ(str, "if") ||
           S_EQ(str, "else") ||
           S_EQ(str, "while") ||
           S_EQ(str, "for") ||
           S_EQ(str, "do") ||
           S_EQ(str, "break") ||
           S_EQ(str, "continue") ||
           S_EQ(str, "switch") ||
           S_EQ(str, "case") ||
           S_EQ(str, "default") ||
           S_EQ(str, "goto") ||
           S_EQ(str, "typedef") ||
           S_EQ(str, "const") ||
           S_EQ(str, "extern") ||
           S_EQ(str, "restrict");
}

struct token *token_make_number_for_value(unsigned long number)
{
    return token_create(&(struct token){.type = TOKEN_TYPE_NUMBER, .llnum = number});
}

struct token *token_make_number()
{
    return token_make_number_for_value(read_number());
}

struct token *token_make_string(char start_delimiter, char end_delimiter)
{
    struct buffer *buffer = buffer_create();
    assert(nextc() == start_delimiter);
    char c = nextc();
    for (; c != end_delimiter && c != EOF; c = nextc())
    {
        if (c == '\\')
        {
            // Implement code for line break
            continue;
        }

        buffer_write(buffer, c);
    }

    buffer_write(buffer, 0x00);
    return token_create(&(struct token){.type = TOKEN_TYPE_STRING, .sval = buffer_ptr(buffer)});
}

struct token *token_make_newline()
{
    nextc();
    return token_create(&(struct token){.type = TOKEN_TYPE_NEWLINE});
}

static bool op_treat_as_one(char c)
{
    return c == '(' || c == '[' || c == ',' || c == '*' || c == '.' || c == '?';
}

static bool is_single_operator(char op)
{
    return op == '+' ||
           op == '-' ||
           op == '/' ||
           op == '*' ||
           op == '=' ||
           op == '>' ||
           op == '<' ||
           op == '|' ||
           op == '&' ||
           op == '^' ||
           op == '%' ||
           op == '~' ||
           op == '!' ||
           op == '(' ||
           op == '[' ||
           op == ',' ||
           op == '.' ||
           op == '?';
}

static bool is_operator_valid(const char *op)
{
    return S_EQ(op, "+") ||
           S_EQ(op, "-") ||
           S_EQ(op, "*") ||
           S_EQ(op, "/") ||
           S_EQ(op, "!") ||
           S_EQ(op, "^") ||
           S_EQ(op, "+=") ||
           S_EQ(op, "-=") ||
           S_EQ(op, "*=") ||
           S_EQ(op, "/=") ||
           S_EQ(op, ">>") ||
           S_EQ(op, "<<") ||
           S_EQ(op, ">=") ||
           S_EQ(op, "<=") ||
           S_EQ(op, ">") ||
           S_EQ(op, "<") ||
           S_EQ(op, "||") ||
           S_EQ(op, "&&") ||
           S_EQ(op, "|") ||
           S_EQ(op, "&") ||
           S_EQ(op, "++") ||
           S_EQ(op, "--") ||
           S_EQ(op, "=") ||
           S_EQ(op, "!=") ||
           S_EQ(op, "==") ||
           S_EQ(op, "->") ||
           S_EQ(op, "(") ||
           S_EQ(op, "[") ||
           S_EQ(op, ",") ||
           S_EQ(op, ".") ||
           S_EQ(op, "...") ||
           S_EQ(op, "~") ||
           S_EQ(op, "?") ||
           S_EQ(op, "%");
}

void read_op_flush_back_keep_first(struct buffer *buffer)
{
    const char *data = buffer_ptr(buffer);
    int len = buffer->len;
    for (int i = len - 1; i > 0; i--)
    {
        if (data[i] == 0x00)
            continue;
        pushc(data[i]);
    }
}

const char *read_op()
{
    bool single_op = true;
    char op = nextc();
    struct buffer *buffer = buffer_create();
    buffer_write(buffer, op);

    if (!op_treat_as_one(op))
    {
        op = peekc();
        if (is_single_operator(op))
        {
            buffer_write(buffer, op);
            nextc();
            single_op = false;
        }
    }

    buffer_write(buffer, 0x00);
    char *ptr = buffer_ptr(buffer);
    if (!single_op)
    {
        if (!is_operator_valid(ptr))
        {
            read_op_flush_back_keep_first(buffer);
            ptr[1] = 0x00;
        }
    }
    else if (!is_operator_valid(ptr))
    {
        compiler_error(lex_process->compiler, "The operator %s is not valid\n", ptr);
    }
    return ptr;
}

static void lex_new_expression()
{
    lex_process->current_expression_count++;
    if (lex_process->current_expression_count == 1)
    {
        lex_process->parentheses_buffer = buffer_create();
    }
    // TODO: Take care of closing the buffer
}

struct token *token_make_operator_or_string()
{
    char op = peekc();
    if (op == '<')
    {
        struct token *last_token = lexer_last_token();
        if (is_token_keyword(last_token, "include"))
        {
            return token_make_string('<', '>');
        }
    }

    struct token *token = token_create(&(struct token){.type = TOKEN_TYPE_OPERATOR, .sval = read_op()});
    if (op == '(')
    {
        lex_new_expression();
    }
    return token;
}

struct token *token_make_symbol()
{
    char c = nextc();
    if (c == ')')
    {
        lex_finish_expression();
    }
    return token_create(&(struct token){.type = TOKEN_TYPE_SYMBOL, .cval = c});
}

struct token *token_make_keyword_or_identifier()
{
    struct buffer *buffer = buffer_create();
    char c = 0;
    LEX_GETC_IF(buffer, c, (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_');
    buffer_write(buffer, 0x00);

    if (iskeyword(buffer_ptr(buffer)))
    {
        return token_create(&(struct token){.type = TOKEN_TYPE_KEYWORD, .sval = buffer_ptr(buffer)});
    }

    return token_create(&(struct token){.type = TOKEN_TYPE_IDENTIFIER, .sval = buffer_ptr(buffer)});
}

struct token *read_default_token()
{
    char c = peekc();
    if (isalpha(c) || c == '_')
    {
        return token_make_keyword_or_identifier();
    }
    else
        return NULL;
}

struct token *token_make_single_line_comment()
{
    struct buffer *buffer = buffer_create();
    char c = 0;
    LEX_GETC_IF(buffer, c, c != EOF && c != '\n');
    buffer_write(buffer, 0x00);
    return token_create(&(struct token){.type = TOKEN_TYPE_COMMENT, .sval = buffer_ptr(buffer)});
}

struct token *token_make_multi_line_comment()
{
    struct buffer *buffer = buffer_create();
    char c = 0;

    while (true)
    {
        LEX_GETC_IF(buffer, c, c != EOF && c != '*');
        if (c == EOF)
        {
            compiler_error(lex_process->compiler, "Multiline comment not closed");
        }
        else if (c == '*')
        {
            nextc();
            if (peekc() == '/')
            {
                nextc();
                break;
            }
        }
    }

    return token_create(&(struct token){.type = TOKEN_TYPE_COMMENT, .sval = buffer_ptr(buffer)});
}

struct token *token_make_comment()
{
    char c = peekc();
    if (c == '/')
    {
        nextc();
        if (peekc() == '/')
        {
            nextc();
            return token_make_single_line_comment();
        }
        else if (peekc() == '*')
        {
            nextc();
            return token_make_multi_line_comment();
        }

        // Case: Division operator; TODO: Handle division operator
        pushc('/');
        return NULL;
    }

    return NULL;
}

char lex_get_escaped_char(char c)
{
    char output = 0;
    switch (c)
    {
    case 'n':
        output = '\n';
        break;
    case '\\':
        output = '\\';
        break;
    case 't':
        output = '\t';
        break;
    case '\'':
        output = '\'';
        break;
    }

    return output;
}

void lexer_pop_token()
{
    vector_pop(lex_process->token_vec);
}

static bool is_hex_char(char c)
{
    c = tolower(c);
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

const char *read_hex_number_string()
{
    struct buffer *buffer = buffer_create();
    char c = peekc();
    LEX_GETC_IF(buffer, c, is_hex_char(c));
    buffer_write(buffer, 0x00);
    return buffer_ptr(buffer);
}

struct token *token_make_special_number_hexadecimal()
{
    nextc();
    unsigned long number = 0;
    const char *number_str = read_hex_number_string();
    number = strtol(number_str, 0, 16);
    return token_make_number_for_value(number);
}

void lexer_validate_binary_string(const char *str)
{
    size_t len = strlen(str);
    for (int i = 0; i < len; i++)
    {
        if (str[i] != '0' && str[i] != '1')
        {
            compiler_error(lex_process->compiler, "Not a valid binary character\n");
        }
    }
}

struct token *token_make_special_number_binary()
{
    nextc();
    unsigned long number = 0;
    const char *num_str = read_number_str();
    lexer_validate_binary_string(num_str);
    number = strtol(num_str, 0, 2);
    return token_make_number_for_value(number);
}

struct token *token_make_special_number()
{
    struct token *token = NULL;
    struct token *last_token = lexer_last_token();
    if (!last_token || last_token->type != TOKEN_TYPE_NUMBER || last_token->llnum != 0)
    {
        return token_make_keyword_or_identifier();
    }

    lexer_pop_token();
    char c = peekc();
    if (c == 'x')
    {
        token = token_make_special_number_hexadecimal();
    }
    else if (c == 'b')
    {
        token = token_make_special_number_binary();
    }

    return token;
}

struct token *token_make_quote()
{
    assert_next_character('\'');
    char c = nextc();
    if (c == '\\')
    {
        c = nextc();
        c = lex_get_escaped_char(c);
    }
    if (peekc() != '\'')
    {
        compiler_error(lex_process->compiler, "Opened quote not closed");
        // TODO: check why this block gets executed for 'a' and '\\'
    }
    nextc();
    return token_create(&(struct token){.type = TOKEN_TYPE_NUMBER, .cval = c});
}

struct token *read_next_token()
{
    struct token *token = NULL;
    char c = peekc();
    token = token_make_comment();
    if (token)
    {
        return token;
    }

    switch (c)
    {
    NUMERIC_CASE:
        token = token_make_number();
        break;

    OPERATOR_CASE_EXCLUDING_DIVISION:
        token = token_make_operator_or_string();
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

    case '\'':
        token = token_make_quote();
        break;

    case 'b':
    case 'x':
        token = token_make_special_number();
        break;

    default:
        token = read_default_token();
        if (token == NULL)
        {
            compiler_error(lex_process->compiler, "Unexpected token\n");
        }
        else
            break;
    }

    return token;
}

int lex(struct lex_process *process)
{
    process->current_expression_count = 0;
    process->parentheses_buffer = NULL;
    process->pos.filename = process->compiler->cfile.abs_path;

    lex_process = process;
    struct token *token = read_next_token();
    while (token)
    {
        vector_push(process->token_vec, token);
        token = read_next_token();
    }

    return LEXICAL_ANALYSIS_ALL_OK;
}