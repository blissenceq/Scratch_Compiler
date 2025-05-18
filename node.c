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
    struct node *last_node_root = vector_empty(node_vector) ? NULL : vector_back_ptr(node_vector_root);

    vector_pop(node_vector);
    if (last_node == last_node_root)
    {
        vector_pop(node_vector_root);
    }

    return last_node;
}
