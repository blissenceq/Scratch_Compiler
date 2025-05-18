#include "compiler.h"
#include "helpers/vector.h"

static struct compile_process *current_process;
static struct token *parser_last_token;

static void parser_ignore_comment_or_newline(struct token *token)
{
    while (token && token_is_comment_newline_or_newline_seperator(token))
    {
        vector_peek(current_process->token_vec);
        token = vector_peek_no_increment(current_process->token_vec);
    }
}

static struct token *token_next()
{
    struct token *next_token = vector_peek_no_increment(current_process->token_vec);
    parser_ignore_comment_or_new_line(next_token);
    current_process->pos = next_token->pos;
    parser_last_token = next_token;
    return vector_peek(current_process->token_vec);
}

int parse_next()
{
    return 0;
}

int parse(struct compile_process *process)
{
    current_process = process;
    struct node *node = NULL;
    vector_set_peek_pointer(process->token_vec, 0);

    while (parse_next() == 0)
    {
        node = peek_node();
        vector_push(process->node_tree_vec, &node);
    }

    return PARSING_ALL_OKAY;
}