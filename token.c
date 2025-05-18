#include "compiler.h"

bool is_token_keyword(struct token *token, const char *value)
{
    return token->type == TOKEN_TYPE_KEYWORD && S_EQ(token->sval, value);
}

bool token_is_symbol(struct token *token, char c)
{
    return token->type == TOKEN_TYPE_SYMBOL && token->cval == c;
}

bool token_is_comment_newline_or_newline_seperator(struct token *token)
{
    return token->type == TOKEN_TYPE_NEWLINE ||
           token->type == TOKEN_TYPE_COMMENT ||
           token_is_symbol(token, '\\');
}