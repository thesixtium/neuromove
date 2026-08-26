/**
 * @file tbci_sizeof.h
 *
 * @author Michele Romani, https://github.com/BRomans
 *
 * @brief Runtime memory footprint reporter for TinyBCI structs.
 *
 * Call tbci_print_sizeof() at startup to log static RAM usage.
 * Useful for embedded targets to verify memory budget before running.
 *
 * Pass all pipeline components to get an accurate total including
 * caller-provided buffers (signal storage, epoch pools, etc.).
 */

#ifndef TBCI_SIZEOF_H
#define TBCI_SIZEOF_H

#include "../pch.h"
#include "../tbci_context.h"

typedef struct {
    /* caller-provided buffer sizes in bytes — not discoverable from ctx */
    size_t sig_storage_bytes;
    size_t proc_storage_bytes;
    size_t epoch_pool_bytes;
    size_t features_pool_bytes;
    size_t output_pool_bytes;

    /* calibration buffers */
    size_t train_trials_bytes;
    size_t train_labels_bytes;
    size_t train_capacity;
    size_t input_size;
} TBCI_SizeofReport;

static inline void tbci_print_sizeof(const TBCI_Context *ctx,
                                     const TBCI_SizeofReport *r)
{
    printf("\n=== TinyBCI Runtime RAM Usage ===\n");
    printf("Build-time defines:\n");
    printf("  TBCI_MAX_NODES        = %d\n", TBCI_MAX_NODES);
    printf("  TBCI_MAX_GROUP_NODES  = %d\n", TBCI_MAX_GROUP_NODES);
    printf("  TBCI_MAX_CHANNELS     = %d\n", TBCI_MAX_CHANNELS);
    printf("\n");
    printf("%-38s %8s  %8s\n", "Component", "Bytes", "KB");
    printf("%-38s %s\n", "--------------------------------------", "-------------------");

    size_t total = 0;

#define TBCI_PRINT_ROW(label, bytes) \
    do { \
        size_t _b = (bytes); \
        printf("%-38s %8zu  (%5.1f KB)\n", label, _b, _b / 1024.0); \
        total += _b; \
    } while(0)

    /* fixed pipeline structs */
    TBCI_PRINT_ROW("TBCI_Context",            sizeof(*ctx));
    TBCI_PRINT_ROW("TBCI_SignalBuffer (raw)",  sizeof(*ctx->inputs->signal));
    TBCI_PRINT_ROW("TBCI_SignalBuffer (proc)", sizeof(TBCI_SignalBuffer));
    TBCI_PRINT_ROW("TBCI_TriggerQueue",        sizeof(*ctx->inputs->triggers));
    TBCI_PRINT_ROW("TBCI_EpochQueue (epoch)",  sizeof(TBCI_EpochQueue));
    TBCI_PRINT_ROW("TBCI_EpochQueue (feats)",  sizeof(TBCI_EpochQueue));
    TBCI_PRINT_ROW("TBCI_EpochQueue (output)", sizeof(TBCI_EpochQueue));

    /* caller-provided buffers */
    if (r->sig_storage_bytes)   TBCI_PRINT_ROW("signal storage (float[])",  r->sig_storage_bytes);
    if (r->proc_storage_bytes)  TBCI_PRINT_ROW("proc storage (float[])",    r->proc_storage_bytes);
    if (r->epoch_pool_bytes)    TBCI_PRINT_ROW("epoch pool (float[])",      r->epoch_pool_bytes);
    if (r->features_pool_bytes) TBCI_PRINT_ROW("features pool (float[])",   r->features_pool_bytes);
    if (r->output_pool_bytes)   TBCI_PRINT_ROW("output pool (float[])",     r->output_pool_bytes);

    /* walk DAG nodes */
    printf("\n  DAG nodes:\n");
    for (size_t i = 0; i < ctx->n_nodes; i++) {
        TBCI_Node *node = ctx->nodes[i];
        if (node == NULL) continue;

        size_t node_size = node->instance_size > 0 ? node->instance_size : sizeof(*node);
        TBCI_PRINT_ROW(node->name ? node->name : "(unnamed)", node_size);

        /* only walk inner nodes for group types — CoreNode is not a group */
        if (node->type == TBCI_NODE_TYPE_PREPROCESSING ||
            node->type == TBCI_NODE_TYPE_FEATURE_EXTRACTION ||
            node->type == TBCI_NODE_TYPE_DECODER) {

            TBCI_NodeGroup *group = (TBCI_NodeGroup *)node;
            for (size_t j = 0; j < group->n_nodes; j++) {
                TBCI_Node *inner = group->nodes[j];
                if (inner == NULL) continue;
                char label[48];
                snprintf(label, sizeof(label), "  └─ %s",
                         inner->name ? inner->name : "(unnamed)");
                size_t inner_size = inner->instance_size > 0
                                  ? inner->instance_size : sizeof(*inner);
                TBCI_PRINT_ROW(label, inner_size);
            }
        }
        if (node->type == TBCI_NODE_TYPE_CORE) {
            TBCI_Core *core = (TBCI_Core *)node;

            char label[48];

            snprintf(label, sizeof(label), "  └─ %s",
                     core->sync.base.name ? core->sync.base.name : "");
            TBCI_PRINT_ROW(label, core->sync.base.instance_size > 0
                                ? core->sync.base.instance_size : sizeof(core->sync));

            snprintf(label, sizeof(label), "  └─ %s",
                     core->raw_out.base.name ? core->raw_out.base.name : "");
            TBCI_PRINT_ROW(label, core->raw_out.base.instance_size > 0
                                ? core->raw_out.base.instance_size : sizeof(core->raw_out));

            snprintf(label, sizeof(label), "  └─ %s",
                     core->seg.base.name ? core->seg.base.name : "");
            TBCI_PRINT_ROW(label, core->seg.base.instance_size > 0
                                ? core->seg.base.instance_size : sizeof(core->seg));
        }
    }

    /* calibration buffers */
    if (r->train_trials_bytes) {
        printf("\n  Calibration:\n");
        TBCI_PRINT_ROW("train trials (float[])", r->train_trials_bytes);
        if (r->train_capacity && r->input_size)
            printf("    └─ %zu trials × %zu floats × 4 bytes\n",
                   r->train_capacity, r->input_size);
    }
    if (r->train_labels_bytes)
        TBCI_PRINT_ROW("train labels (uint16[])", r->train_labels_bytes);

    printf("%-38s %s\n", "--------------------------------------", "-------------------");
    printf("%-38s %8zu  (%5.1f KB)\n", "TOTAL", total, total / 1024.0);
    printf("======================================\n\n");

#undef TBCI_PRINT_ROW
}

#endif /* TBCI_SIZEOF_H */