/**************************************************************************
 * C S 429 system emulator
 * 
 * instr_Memory.c - Memory stage of instruction processing pipeline.
 **************************************************************************/

/* Code constructed by Aaron Alvarez (aa88379) and Ryan Passaro(rjp2827)*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "err_handler.h"
#include "instr.h"
#include "instr_pipeline.h"
#include "machine.h"
#include "hw_elts.h"

extern machine_t guest;
extern mem_status_t dmem_status;

extern comb_logic_t copy_w_ctl_sigs(w_ctl_sigs_t *, w_ctl_sigs_t *);

/*
 * Memory stage logic.
 * STUDENT TO-DO:
 * Implement the memory stage.
 *
 * Use in as the input pipeline register,
 * and update the out pipeline register as output.
 *
 * You will need the following helper functions:
 * copy_w_ctl_signals and dmem.
 */

comb_logic_t memory_instr(m_instr_impl_t *in, w_instr_impl_t *out)
{
    // Checking data memory read or write
    if (in->M_sigs.dmem_read || in->M_sigs.dmem_write){
        // Initialize flag to track potential data memory errors
        bool x = 0;
        // Perfrom data memory op , storing in val_mem
        dmem(in->val_ex, in->val_b, in->M_sigs.dmem_read, in->M_sigs.dmem_write, &(out->val_mem), &(x));
        // If data memory error, set address error
        if (x){
            in->status = STAT_ADR;
        }
    }

    //copy fields from input to output instruction
    out->status = in->status;
    out->op = in->op;
    out->print_op = in->print_op;
    // copy write-back stage control signals from input to output
    copy_w_ctl_sigs(&(out->W_sigs), &(in->W_sigs));
    out->val_ex = in->val_ex;
    out->dst = in->dst;

    return;
}