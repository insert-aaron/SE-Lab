/**************************************************************************
 * C S 429 system emulator
 *
 * instr_Execute.c - Execute stage of instruction processing pipeline.
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

extern bool X_condval;

extern comb_logic_t copy_m_ctl_sigs(m_ctl_sigs_t *, m_ctl_sigs_t *);
extern comb_logic_t copy_w_ctl_sigs(w_ctl_sigs_t *, w_ctl_sigs_t *);

/*
 * Execute stage logic.
 * STUDENT TO-DO:
 * Implement the execute stage.
 *
 * Use in as the input pipeline register,
 * and update the out pipeline register as output.
 *
 * You will need the following helper functions:
 * copy_m_ctl_signals, copy_w_ctl_signals, and alu.
 */

comb_logic_t execute_instr(x_instr_impl_t *in, m_instr_impl_t *out)
{
    // Copy the memory stage control signals from the input instruction (in) to the output instruction (out)
    copy_m_ctl_sigs(&(out->M_sigs), &(in->M_sigs));
    // Copy the write-back stage control signals from the input instruction (in) to the output instruction (out)
    copy_w_ctl_sigs(&(out->W_sigs), &(in->W_sigs));
    // Initialize a variable val_B to hold the value for the second operand of the ALU operation
    uint64_t val_B;
    // Check if valb_sel is set in control signals or if the opcode is OP_MVN
    if (in->X_sigs.valb_sel || in->op == OP_MVN)
    {
        // If true, use the value from register val_b as the second operand
        val_B = in->val_b;
    }
    else
    {
        // Otherwise, use the immediate value (val_imm) as the second operand
        val_B = in->val_imm;
    }
    // Store the result in val_ex and the condition flag in X_condval
    alu(in->val_a, val_B, in->val_hw, in->ALU_op, in->X_sigs.set_CC, in->cond, &(out->val_ex), &(X_condval));
    // Copy some fields from the input instruction (in) to the output instruction (out)
    out->print_op = in->print_op;
    out->op = in->op;
    out->val_b = in->val_b;
    out->seq_succ_PC = in->seq_succ_PC;
    out->dst = in->dst;
    out->status = in->status;
    out->dst = in->dst;
    out->W_sigs.w_enable = in->W_sigs.w_enable;
    out->cond_holds = X_condval;
    return;
}