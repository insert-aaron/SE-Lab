/**************************************************************************
 * C S 429 system emulator
 *
 * forward.c - The value forwarding logic in the pipelined processor.
 **************************************************************************/

#include <stdbool.h>
#include "forward.h"
#include "machine.h"

extern machine_t client;

/* STUDENT TO-DO:
 * Implement forwarding register values from
 * execute, memory, and writeback back to decode.
 */
comb_logic_t forward_reg(uint8_t D_src1, uint8_t D_src2, uint8_t X_dst, uint8_t M_dst, uint8_t W_dst,
        uint64_t X_val_ex, uint64_t M_val_ex, uint64_t M_val_mem, uint64_t W_val_ex,
            uint64_t W_val_mem, bool M_wval_sel, bool W_wval_sel, bool X_w_enable,
                bool M_w_enable, bool W_w_enable,
                    uint64_t *val_a, uint64_t *val_b){
    if(client.proc->w_insn->in.w->W_sigs.w_enable){
        if(D_src1 == W_dst){

            if(W_wval_sel){
                if(client.proc->w_insn->in.w->W_sigs.dst_sel){
                    *val_a = client.proc->d_insn->in.d->seq_succ_PC;
                }else{
                    *val_a = W_val_ex;
                }
            }else{
                *val_a = W_val_mem;
            }
        }

        if(D_src2 == W_dst){
            if(W_wval_sel){
                *val_b = W_val_ex;
            }else{
                *val_b = W_val_mem;
            }
        }
    }

    //Checks Memory 
    if(client.proc->m_insn->in.w->W_sigs.w_enable){
        if(D_src1 == M_dst){

            if(M_wval_sel){
                *val_a = M_val_ex;
            }else{
                *val_a = M_val_mem;
            }
        }

        if(D_src2 == M_dst){
            if(M-W_wval_sel){
                *val_b = M_val_ex;
            }else{
                *val_b = M_val_mem;
            }
        }
    }

    //Checks execute
    if(client.proc->x_insn->in.w->W_sigs.w_enable 
        && !(client.proc->x_insn->in.m->M_sigs.dmem_read)){
        
        if(D_src1 == X_dst){
            *val_a = X_val_ex;
        }

        if(D_src2 == X_dst){
            *val_b = X_val_ex;
        }
    }

    return;
    
    
    
    // if (X_w_enable)
    // {
    //     if (X_dst == D_src1)
    //     {
    //         *val_a = X_val_ex;
    //     }
    //     else if (W_dst == D_src1 && W_w_enable)
    //     {
    //         *val_a = W_val_ex;
    //     }
    //     else if (M_dst == D_src1 && M_w_enable)
    //     {
    //         *val_a = M_wval_sel ? M_val_mem : M_val_ex;
    //     }

    //     if (X_dst == D_src2)
    //     {
    //         *val_b = X_val_ex;
    //     }
    //     else if (W_dst == D_src2 && W_w_enable)
    //     {
    //         *val_b = W_val_ex;
    //     }
    //     else if (M_dst == D_src2 && M_w_enable)
    //     {
    //         *val_b = M_wval_sel ? M_val_mem : M_val_ex;
    //     }
    // }
    // else
    // {
    //     if (W_w_enable)
    //     {
    //         if (W_dst == D_src1)
    //         {
    //             *val_a = W_wval_sel ? W_val_mem : W_val_ex;
    //         }
    //         if (W_dst == D_src2)
    //         {
    //             *val_b = W_wval_sel ? W_val_mem : W_val_ex;
    //         }
    //     }
    //     if (M_w_enable)
    //     {
    //         if (M_dst == D_src1)
    //         {
    //             *val_a = M_wval_sel ? M_val_mem : M_val_ex;
    //         }
    //         if (M_dst == D_src2)
    //         {
    //             *val_b = M_wval_sel ? M_val_mem : M_val_ex;
    //         }
    //     }
    // }
    // return;
}
  

