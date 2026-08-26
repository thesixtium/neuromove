/**
* @file tbci_node.c
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief TinyBCI pipeline Node implementation.
 *
 * Generic dispatch helpers operating on the TBCI_Node interface.
 * Concrete nodes wire up init_fn/process_fn/reset_fn themselves;
 * these helpers provide null-checks and enabled-flag handling
 * common to every call site.
 */

#include "../../include/nodes/tbci_node.h"
#include "tbci_context.h"

TBCI_Status node_init(TBCI_Node *node, struct TBCI_Context *ctx)
{
    if (node == NULL || ctx == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (node->init_fn == NULL)
        return TBCI_OK; /* node has no init step */

    return node->init_fn(node, ctx);
}

TBCI_NodeResult node_process(TBCI_Node *node, void *data, struct TBCI_Context *ctx)
{
    if (node == NULL || data == NULL || ctx == NULL)
        return TBCI_NODE_ERROR;

    if (!node->enabled)
        return TBCI_NODE_OK; /* pass-through */

    if (node->process_fn == NULL)
        return TBCI_NODE_OK; /* no-op node */

    return node->process_fn(node, data, ctx);
}

TBCI_Status node_reset(TBCI_Node *node)
{
    if (node == NULL)
        return TBCI_ERR_INVALID_ARG;

    if (node->reset_fn == NULL)
        return TBCI_OK;

    return node->reset_fn(node);
}