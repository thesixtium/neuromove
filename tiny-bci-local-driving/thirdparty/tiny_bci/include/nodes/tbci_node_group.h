/**
* @file tbci_node_group.h
*
* @author Michele Romani, https://github.com/BRomans
*
* @brief Generic composite node — chains inner nodes that operate on a
*        single frame in-place.
*
* TBCI_NodeGroup is the building block for PreprocessingGroup,
* PostProcessingGroup, and FeatureExtractionGroup. From the outside it
* behaves like any other TBCI_Node — the runner sees @ref TBCI_NodeGroup::base
* and calls group_process via the generic dispatch in tbci_node.c.
*
* Internally it holds an ordered array of inner nodes. Each inner node
* is unaware of the pipeline context — group_process passes the same
* float buffer through each enabled inner node in sequence, in-place.
* Inner nodes are never registered directly in the DAG.
*
* ## Enabling and disabling
*
* Set @ref TBCI_NodeGroup::base "base.enabled" to false to make the
* whole group a pass-through — group_process returns TBCI_NODE_OK
* immediately without touching @p samples. Individual inner nodes can
* also be disabled via their own @c enabled flag; group_process skips
* them while still calling the rest of the chain.
*
* ## Memory
*
* No dynamic allocation. @ref TBCI_NodeGroup::nodes is a fixed-size
* array of pointers to caller-owned TBCI_Node instances.
*/

#ifndef TBCI_NODE_GROUP_H
#define TBCI_NODE_GROUP_H

#include "tbci_node.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TBCI_Context;

/**
* @brief Generic composite node.
*
* Extends TBCI_Node via composition — @ref base must be first member
* for safe casting between TBCI_Node* and TBCI_NodeGroup*.
*/
typedef struct TBCI_NodeGroup {
    TBCI_Node  base;     /**< DAG-visible base. MUST be first member. */
    size_t     n_nodes;  /**< Number of entries in @ref nodes currently in use. */
    TBCI_Node *nodes[TBCI_MAX_GROUP_NODES]; /**< Inner nodes, invisible to the DAG. Caller-owned. */
} TBCI_NodeGroup;

/**
* @brief Initialise every inner node in the group.
*
* Calls node_init on each entry in @ref TBCI_NodeGroup::nodes in order.
* Stops and returns early on the first failure.
*
* @param[in,out] group  Pointer to an initialised group with nodes registered. Must not be NULL.
* @param[in]     ctx    Pipeline context, forwarded to each inner node's init_fn. Must not be NULL.
* @return TBCI_OK if all inner nodes initialised successfully.
* @return TBCI_ERR_INVALID_ARG if group or ctx is NULL.
* @return Any error returned by an inner node's init_fn.
*/
TBCI_API TBCI_Status     group_init(TBCI_NodeGroup *group, struct TBCI_Context *ctx);

/**
* @brief Run one tick of the group, passing samples through each inner node.
*
* If @ref TBCI_NodeGroup::base "base.enabled" is false, returns TBCI_NODE_OK
* immediately and @p samples is left unchanged (group-level pass-through).
*
* Otherwise iterates @ref TBCI_NodeGroup::nodes in order, calling node_process
* on each. Disabled inner nodes are skipped (pass-through for that node only).
* @p samples is modified in-place by each enabled inner node.
*
* Stops and returns early if any inner node returns something other than
* TBCI_NODE_OK.
*
* @param[in,out] group    Pointer to an initialised group. Must not be NULL.
* @param[in,out] data  Array of n_channels floats or TBCI_Epoch, modified in-place. Must not be NULL.
* @param[in,out] ctx      Pipeline context, forwarded to each inner node. Must not be NULL.
* @return TBCI_NODE_OK      if all enabled inner nodes processed successfully (or group disabled).
* @return TBCI_NODE_PENDING if an inner node is not yet ready.
* @return TBCI_NODE_ERROR   if group, samples, or ctx is NULL, or an inner node errored.
*/
TBCI_API TBCI_NodeResult group_process(TBCI_NodeGroup *group, void *data, struct TBCI_Context *ctx);

/**
* @brief Reset every inner node in the group to its initial state.
*
* Calls node_reset on each entry in @ref TBCI_NodeGroup::nodes in order.
* Stops and returns early on the first failure.
*
* @param[in,out] group  Pointer to an initialised group. Must not be NULL.
* @return TBCI_OK if all inner nodes reset successfully.
* @return TBCI_ERR_INVALID_ARG if group is NULL.
* @return Any error returned by an inner node's reset_fn.
*/
TBCI_API TBCI_Status     group_reset(TBCI_NodeGroup *group);


/**
* @brief Register an inner node in the group.
*
* Appends @p node to the group's internal node array. The node is not
* initialised here — call group_init afterwards (or rely on the owning
* group's _init function, which calls group_init internally).
*
* @param[in,out] group  Pointer to a group. Must not be NULL.
* @param[in]     node   Pointer to a caller-owned node. Must not be NULL.
* @return TBCI_OK on success.
* @return TBCI_ERR_INVALID_ARG if group or node is NULL.
* @return TBCI_ERR_FULL if the group already has TBCI_MAX_GROUP_NODES nodes.
*/
TBCI_API TBCI_Status group_add_node(TBCI_NodeGroup *group, TBCI_Node *node);

/**
* @brief Remove an inner node from the group by pointer identity.
*
* Shifts remaining nodes left to fill the gap. Does not call reset_fn —
* caller is responsible for any cleanup before removal.
*
* @param[in,out] group  Pointer to a group. Must not be NULL.
* @param[in]     node   Pointer to the node to remove. Must not be NULL.
* @return TBCI_OK on success.
* @return TBCI_ERR_INVALID_ARG if group or node is NULL.
* @return TBCI_ERR_NOT_FOUND if node is not in the group.
*/
TBCI_API TBCI_Status group_remove_node(TBCI_NodeGroup *group, TBCI_Node *node);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_NODE_GROUP_H */