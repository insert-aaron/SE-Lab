/**************************************************************************
 * C S 429 system emulator
 *
 * forward.c - The value forwarding logic in the pipelined processor.
 **************************************************************************/

#include <stdbool.h>
#include "forward.h"

/* STUDENT TO-DO:
 * Implement forwarding register values from
 * execute, memory, and writeback back to decode.
 */
comb_logic_t forward_reg(uint8_t D_src1, uint8_t D_src2, uint8_t X_dst, uint8_t M_dst, uint8_t W_dst,
                         uint64_t X_val_ex, uint64_t M_val_ex, uint64_t M_val_mem, uint64_t W_val_ex,
                         uint64_t W_val_mem, bool M_wval_sel, bool W_wval_sel, bool X_w_enable,
                         bool M_w_enable, bool W_w_enable,
                         uint64_t *val_a, uint64_t *val_b)
{

    // First, handle forwarding from the execute stage if enabled
    if (X_w_enable) {
        if (X_dst == D_src1) {
            *val_a = X_val_ex;
        }
        if (X_dst == D_src2) {
            *val_b = X_val_ex;
        }
    }

    // Then, handle forwarding from the memory stage if enabled
    if (M_w_enable) {
        if (M_dst == D_src1) {
            *val_a = M_wval_sel ? M_val_mem : M_val_ex;
        }
        if (M_dst == D_src2) {
            *val_b = M_wval_sel ? M_val_mem : M_val_ex;
        }
    }

    // Finally, handle forwarding from the writeback stage if enabled
    if (W_w_enable) {
        if (W_dst == D_src1) {
            *val_a = W_wval_sel ? W_val_mem : W_val_ex;
        }
        if (W_dst == D_src2) {
            *val_b = W_wval_sel ? W_val_mem : W_val_ex;
        }
    }
    
    // No return statement needed since we're modifying the values by pointer
}
