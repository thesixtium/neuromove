/**
* @file tbci_node_group.c
 *
 * @brief Generic dispatch for TBCI_NodeGroup — chains inner nodes,
 *        each operating on the same frame in-place.
 */

#include "tbci_context.h"
#include "../../include/nodes/tbci_node.h"
#include "../../include/nodes/tbci_node_group.h"

TBCI_Status group_init(TBCI_NodeGroup *group, struct TBCI_Context *ctx)
{
    if (group == NULL || ctx == NULL)
        return TBCI_ERR_INVALID_ARG;

    for (size_t i = 0; i < group->n_nodes; i++) {
        if (!group->nodes[i]->enabled)
            fprintf(stderr, "group_init: node '%s' is disabled — "
                    "did you forget to set base.enabled = true?\n",
                    group->nodes[i]->name ? group->nodes[i]->name : "(unnamed)");

        TBCI_Status s = node_init(group->nodes[i], ctx);
        if (s != TBCI_OK) return s;
    }
    return TBCI_OK;
}

TBCI_NodeResult group_process(TBCI_NodeGroup *group, void *data, struct TBCI_Context *ctx)
{
    if (group == NULL || data == NULL || ctx == NULL)
        return TBCI_NODE_ERROR;

    if (!group->base.enabled)
        return TBCI_NODE_OK; /* whole group pass-through */

    for (size_t i = 0; i < group->n_nodes; i++) {
        TBCI_NodeResult r = node_process(group->nodes[i], data, ctx);
        if (r != TBCI_NODE_OK) return r;
    }
    return TBCI_NODE_OK;
}

TBCI_Status group_reset(TBCI_NodeGroup *group)
{
    if (group == NULL)
        return TBCI_ERR_INVALID_ARG;

    for (size_t i = 0; i < group->n_nodes; i++) {
        TBCI_Status s = node_reset(group->nodes[i]);
        if (s != TBCI_OK) return s;
    }
    return TBCI_OK;
}

TBCI_Status group_add_node(TBCI_NodeGroup *group, TBCI_Node *node)
{
    if (group == NULL || node == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (group->n_nodes >= TBCI_MAX_GROUP_NODES)
        return TBCI_ERR_FULL;

    group->nodes[group->n_nodes++] = node;
    return TBCI_OK;
}

TBCI_Status group_remove_node(TBCI_NodeGroup *group, TBCI_Node *node)
{
    if (group == NULL || node == NULL)
        return TBCI_ERR_INVALID_ARG;

    for (size_t i = 0; i < group->n_nodes; i++) {
        if (group->nodes[i] == node) {
            for (size_t j = i; j + 1 < group->n_nodes; j++)
                group->nodes[j] = group->nodes[j + 1];
            group->n_nodes--;
            group->nodes[group->n_nodes] = NULL;
            return TBCI_OK;
        }
    }
    return TBCI_ERR_NOT_FOUND;
}