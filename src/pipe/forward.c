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
                 uint64_t *val_a, uint64_t *val_b) {
    /* Students: Implement Below */
    if (W_w_enable)
    {
        if (W_dst == D_src1)
        {
            *val_a = W_val_ex;
        }
        if (W_dst == D_src2)
        {
            *val_b = W_val_ex;
        }
    }
    if (M_w_enable)
    {
        if (M_dst == D_src1)
        {
            *val_a = M_val_ex;
        }
        if (M_dst == D_src2)
        {
            *val_b = M_val_ex;
        }
    }
    if (X_w_enable)
    {
        if (X_dst == D_src1)
        {                      // x dest = dx
            *val_a = X_val_ex; // val_A = M_in->val_a && vx = X_val_ex
        }                      // don't need an else cuz regval_a = val_a
        if (X_dst == D_src2)
        {
            *val_b = X_val_ex;
        } // don't need an else cuz regval_b = val_b
    }

    // load-use hazards
    if (W_wval_sel)
    {
        if (W_dst == D_src1)
        {
            *val_a = W_val_mem;
        }
        if (W_dst == D_src2)
        {
            *val_b = W_val_mem;
        }
    }
    if (M_wval_sel)
    {
        if (M_dst == D_src1)
        {
            *val_a = M_val_mem;
        }
        if (M_dst == D_src2)
        {
            *val_b = M_val_mem;
        }
    }

    return;
}
