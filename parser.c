#include "compiler.h"
#include "helpers/vector.h"

struct compile_process *current_process;

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
        // node = peek_node();
        vector_push(process->node_tree_vec, &node);
    }

    return PARSING_ALL_OKAY;
}