#include <assert.h>
#include "compiler.h"
#include "helpers/vector.h"

struct vector *node_vector = NULL;
struct vector *node_vector_root = NULL;

void node_set_vector(struct vector *vector, struct vector *vector_root)
{
    node_vector = vector;
    node_vector_root = vector_root;
}

void node_push(struct node *node)
{
    vector_push(node_vector, &node);
}

struct node *node_peek_or_null()
{
    vector_back_ptr_or_null(node_vector);
}

struct node *node_peek()
{
    return *(struct node **)(vector_back(node_vector));
}

struct node *node_pop()
{
    struct node *last_node = vector_back_ptr(node_vector);
    struct node *last_node_root = vector_empty(node_vector) ? NULL : vector_back_ptr_or_null(node_vector_root);

    vector_pop(node_vector);
    if (last_node == last_node_root)
    {
        vector_pop(node_vector_root);
    }

    return last_node;
}

struct node *node_create(struct node *_node)
{
    struct node *node = malloc(sizeof(struct node));
    memcpy(node, _node, sizeof(struct node));
#warning "Set binded owner and binded function here"
    node_push(node);
    return node;
}

bool node_is_expressionable(struct node *node)
{
    return node->type == NODE_TYPE_EXPRESSION ||
           node->type == NODE_TYPE_EXPRESSION_PARENTHESES ||
           node->type == NODE_TYPE_UNARY ||
           node->type == NODE_TYPE_IDENTIFIER ||
           node->type == NODE_TYPE_NUMBER ||
           node->type == NODE_TYPE_STRING;
}

struct node *node_peek_expressionable_or_null()
{
    struct node *node = node_peek_or_null();
    return node_is_expressionable(node) ? node : NULL;
}

void node_make_expression(struct node *left, struct node *right, const char *operator)
{
    assert(left);
    assert(right);
    node_create(&(struct node){.type = NODE_TYPE_EXPRESSION, .expression.left = left, .expression.right = right, .expression.operator = operator});
}
