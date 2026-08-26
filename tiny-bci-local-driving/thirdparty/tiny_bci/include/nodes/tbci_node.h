/**
 * @file tbci_node.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief A single processing node in the TinyBCI pipeline.
 *
 * Each node encapsulates one processing stage. The runner iterates
 * the node array in order each tick, skipping disabled nodes and
 * short-circuiting on TBCI_NODE_PENDING.
 *
 * Nodes do not share a common process() signature since each stage
 * has different typed inputs and outputs. The runner dispatches to
 * each node type explicitly via a typed call. This struct holds
 * only the metadata the runner needs to manage the node.
 *
 * ## Enabling and disabling nodes
 *
 * Set @p enabled to false to skip a node at runtime without removing
 * it from the array. This is how optional nodes (feature extraction)
 * are toggled via TBCIConfig at init time.
 */

#ifndef TBCI_NODE_H
#define TBCI_NODE_H

#include "../pch.h"
#include "../tbci_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of nodes in the TinyBCI pipeline.
 *
 * Sized to accommodate the full pipeline:
 * Preprocessing → Segmentation → Feature Extraction → Decoder.
 * Increase if additional nodes are added in future.
 */
#define TBCI_MAX_NODES 4
#define TBCI_MAX_GROUP_NODES 4

struct TBCI_Context;
struct TBCI_Node;

/**
 * @brief Identifies the concrete type of a pipeline node.
 *
 * Used by the DAG runner to dispatch to the correct typed process
 * function. Each node type has its own process signature — the runner
 * switches on this enum to call the right one.
 */
typedef enum {
    TBCI_NODE_TYPE_CORE,               /**< CoreNode  */
    TBCI_NODE_TYPE_PREPROCESSING,      /**< PreprocessingNode */
    TBCI_NODE_TYPE_DATA_OUT,           /**< OutNode  */
    TBCI_NODE_TYPE_FEATURE_EXTRACTION, /**< FeatureExtractionNode */
    TBCI_NODE_TYPE_DECODER,            /**< DecoderNode */
} TBCI_NodeType;

/**
 * @brief Top-level DAG dispatch function for a macro group.
 *
 * Called once per tick by tbci_context_tick. The node/group reads its
 * input from a ctx-owned buffer and writes its output to another
 * ctx-owned buffer — no data is passed explicitly, unlike
 * TBCI_NodeProcessFn (the inner-node, frame-shaped interface used by
 * group_process within a TBCI_NodeGroup).
 *
 * @param[in,out] self  Pointer to the node/group (cast as needed).
 * @param[in,out] ctx   Pipeline context, source and destination of all data.
 * @return TBCI_NODE_OK / TBCI_NODE_PENDING / TBCI_NODE_ERROR.
 */
typedef TBCI_NodeResult (*TBCI_TopProcessFn)(struct TBCI_Node *self, struct TBCI_Context *ctx);

typedef TBCI_Status (*TBCI_NodeInitFn)(struct TBCI_Node *self, struct TBCI_Context *ctx);

typedef TBCI_NodeResult (*TBCI_NodeProcessFn)(struct TBCI_Node *self, void *data, struct TBCI_Context *ctx);

typedef TBCI_Status (*TBCI_NodeResetFn)(struct TBCI_Node *self);

typedef struct TBCI_Node {
    void       *config;  /**< Node-specific configuration struct. Owned by the node.     */
    void       *state;   /**< Node-specific internal state. Owned by the node.           */
    const char *name;    /**< Human-readable name for debugging and logging.             */
    TBCI_NodeType type;  /**< The type of node that defines input and output types       */
    bool        enabled; /**< If false, the runner skips this node entirely this tick.   */
    TBCI_NodeInitFn    init_fn;
    TBCI_NodeProcessFn process_fn;  /**< Operates on one frame in-place. */
    TBCI_NodeResetFn   reset_fn;
    TBCI_TopProcessFn  tick_fn;
    size_t instance_size;  /**< sizeof the concrete type, set at init time. */
} TBCI_Node;


TBCI_API TBCI_Status     node_init(TBCI_Node *node, struct TBCI_Context *ctx);

TBCI_API TBCI_NodeResult node_process(TBCI_Node *node, void *data, struct TBCI_Context *ctx);

TBCI_API TBCI_Status     node_reset(TBCI_Node *node);

#ifdef __cplusplus
}
#endif

#endif /* TBCI_NODE_H */