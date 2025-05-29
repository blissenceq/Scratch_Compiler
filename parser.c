#include <assert.h>
#include "compiler.h"
#include "helpers/vector.h"

static struct compile_process *current_process;
static struct token *parser_last_token;
extern struct expressionable_operator_precedence_group operator_group[TOTAL_OPERATOR_GROUPS];

struct history
{
    int flags;
};

struct history *history_begin(int flags)
{
    struct history *history = calloc(1, sizeof(struct history));
    history->flags = flags;
    return history;
}

struct history *history_down(struct history *history, int flags)
{
    struct history *new_history = calloc(1, sizeof(struct history));
    memcpy(new_history, history, sizeof(struct history));
    new_history->flags = flags;
    return new_history;
}

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
    parser_ignore_comment_or_newline(next_token);
    current_process->pos = next_token->pos;
    parser_last_token = next_token;
    return vector_peek(current_process->token_vec);
}

static struct token *token_peek_next()
{
    struct token *token = vector_peek_no_increment(current_process->token_vec);
    parser_ignore_comment_or_newline(token);
    return vector_peek_no_increment(current_process->token_vec);
}

void parse_single_token_to_node()
{
    struct token *token = token_next();
    struct node *node = NULL;

    switch (token->type)
    {
    case TOKEN_TYPE_NUMBER:
        node = node_create(&(struct node){.type = NODE_TYPE_NUMBER, .llnum = token->llnum});
        break;

    case TOKEN_TYPE_IDENTIFIER:
        node = node_create(&(struct node){.type = NODE_TYPE_IDENTIFIER, .cval = token->cval});
        break;

    case TOKEN_TYPE_STRING:
        node = node_create(&(struct node){.type = NODE_TYPE_STRING, .sval = token->sval});
        break;

    default:
        compiler_error(current_process, "Not a single token that is convertable to a node");
    }
}

int parse_expressionable_single_token(struct history *history);
void parse_expressionable_token(struct history *history);

void parse_expressionable_for_operator(struct history *history, const char *operator)
{
    parse_expressionable_token(history);
}

static int parser_get_precedence_for_operator(const char *op, struct expressionable_operator_precedence_group **group_ptr)
{
    *group_ptr = NULL;
    for (int i = 0; i < TOTAL_OPERATOR_GROUPS; i++)
    {
        for (int j = 0; operator_group[i].operators[j]; j++)
        {
            const char *_op = operator_group[i].operators[j];
            if (S_EQ(op, _op))
            {
                *group_ptr = &operator_group[i];
                return i;
            }
        }
    }

    return -1;
}

static bool parser_left_operator_has_higher_priority(const char *op_left, const char *op_right)
{
    struct expressionable_operator_precedence_group *group_left = NULL;
    struct expressionable_operator_precedence_group *group_right = NULL;
    int precedence_left = parser_get_precedence_for_operator(op_left, &group_left);
    int precedence_right = parser_get_precedence_for_operator(op_right, &group_right);

    if (group_left->associtivity == ASSOCIATIVITY_RIGHT_TO_LEFT)
        return false;

    return precedence_left <= precedence_right;
}

void parser_node_shift_children_left(struct node *node)
{
    assert(node->type == NODE_TYPE_EXPRESSION);
    assert(node->expression.right == NODE_TYPE_EXPRESSION);

    struct node *new_exp_node_left = node->expression.left;
    struct node *new_exp_node_right = node->expression.right->expression.left;
    const char *new_exp_op = node->expression.operator;
    const char *node_exp_op = node->expression.right->expression.operator;
    struct node *node_exp_right = node->expression.right->expression.right;

    node_make_expression(new_exp_node_left, new_exp_node_right, new_exp_op);
    struct node *node_exp_left = node_pop();

    node->expression.left = node_exp_left;
    node->expression.right = node_exp_right;
    node->expression.operator = node_exp_op;
}

void parser_reorder_expression(struct node **node_ptr)
{
    struct node *node = *node_ptr;
    if (node->type != NODE_TYPE_EXPRESSION)
        return;
    if (node->expression.left->type != NODE_TYPE_EXPRESSION && node->expression.right->type != NODE_TYPE_EXPRESSION)
        return;

    if (node->expression.left->type != NODE_TYPE_EXPRESSION && node->expression.right && node->expression.right->type == NODE_TYPE_EXPRESSION)
    {
        const char *op_right = node->expression.right->expression.operator;
        if (parser_left_operator_has_higher_priority(node->expression.operator, op_right))
        {
            parser_node_shift_children_left(node);
            parser_reorder_expression(&node->expression.left);
            parser_reorder_expression(&node->expression.right);
        }
    }
}

void parse_expression_normal(struct history *history)
{
    struct token *token = token_peek_next();
    const char *operator = token->sval;
    struct node *node_left = node_peek_expressionable_or_null();

    if (!node_left)
    {
        return;
    }

    token_next(); // pops off the operator token
    node_pop();   // pops off the left_node
    node_left->flag |= NODE_FLAG_INSIDE_EXPRESSION;

    parse_expressionable_for_operator(history_down(history, history->flags), operator);
    struct node *node_right = node_pop();
    node_right->flag |= NODE_FLAG_INSIDE_EXPRESSION;

    node_make_expression(node_left, node_right, operator);
    struct node *node_expression = node_pop();
    parser_reorder_expression(&node_expression);
    node_push(node_expression);
}

int parse_expression(struct history *history)
{
    parse_expression_normal(history);
    return 0;
}

void parse_identifier(struct history *history)
{
    assert(token_peek_next()->type == TOKEN_TYPE_IDENTIFIER);
    parse_single_token_to_node();
}

int parse_expressionable_single_token(struct history *history)
{
    struct token *token = token_peek_next();
    if (!token)
    {
        return -1;
    }

    history->flags |= NODE_FLAG_INSIDE_EXPRESSION;
    int result = -1;

    switch (token->type)
    {
    case TOKEN_TYPE_NUMBER:
        parse_single_token_to_node();
        result = 0;
        break;

    case TOKEN_TYPE_OPERATOR:
        parse_expression(history);
        result = 0;
        break;

    case TOKEN_TYPE_IDENTIFIER:
        parse_identifier(history);
        break;
    }

    return result;
}

void parse_expressionable_token(struct history *history)
{
    while (parse_expressionable_single_token(history) == 0)
    {
    }
}

int parse_next()
{
    struct token *token = token_peek_next();
    if (!token)
        return -1;

    switch (token->type)
    {
    case TOKEN_TYPE_NUMBER:
    case TOKEN_TYPE_IDENTIFIER:
    case TOKEN_TYPE_STRING:
        parse_expressionable_token(history_begin(0));
        break;
    }

    return 0;
}

int parse(struct compile_process *process)
{
    current_process = process;
    parser_last_token = NULL;
    node_set_vector(current_process->node_vec, current_process->node_tree_vec);
    struct node *node = NULL;
    vector_set_peek_pointer(process->token_vec, 0);

    while (parse_next() == 0)
    {
        node = node_peek();
        vector_push(process->node_tree_vec, &node);
    }

    return PARSING_ALL_OKAY;
}