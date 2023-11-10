/**************************************************************************
 * C S 429 system emulator
 *
 * instr_Execute.c - Execute stage of instruction processing pipeline.
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "err_handler.h"    // Header for error handling related definitions
#include "instr.h"          // Header for instruction related definitions
#include "instr_pipeline.h" // Header for instruction pipeline structures
#include "machine.h"        // Header for machine state definition
#include "hw_elts.h"        // Header for hardware elements and helper functions

// Declaration of external global variables which are defined in other source files.
extern machine_t guest;          // Represents the state of the emulated machine.
extern mem_status_t dmem_status; // Represents the status of the data memory.

extern bool X_condval; // External boolean to hold the result of condition code evaluation.

// Declarations of external functions to copy control signals between pipeline stages.
extern comb_logic_t copy_m_ctl_sigs(m_ctl_sigs_t *, m_ctl_sigs_t *);
extern comb_logic_t copy_w_ctl_sigs(w_ctl_sigs_t *, w_ctl_sigs_t *);

/*
 * Execute stage logic.
 * This function is responsible for handling the execution stage of the
 * instruction pipeline, where ALU operations are performed and control
 * signals for subsequent pipeline stages are set.
 *
 * Parameters:
 *  in - Pointer to the structure holding the input pipeline register state.
 *  out - Pointer to the structure where the output pipeline register state should be stored.
 *
 * The input structure contains the instruction and associated control signals after
 * the decode stage, while the output structure represents the state going into the memory stage.
 */

comb_logic_t execute_instr(x_instr_impl_t *in, m_instr_impl_t *out)
{
    // Copying decoded instruction details to the output register
    out->status = in->status;           // Status from the decode stage to pass through.
    out->print_op = in->print_op;       // The operation to print (for debugging or logging).
    out->seq_succ_PC = in->seq_succ_PC; // The sequential successor to the program counter.

    // Copy control signals for the Memory stage from the input to output pipeline register.
    copy_m_ctl_sigs(&(out->M_sigs), &(in->M_sigs));
    // Copy control signals for the Write-Back stage from the input to output pipeline register.
    copy_w_ctl_sigs(&(out->W_sigs), &(in->W_sigs));

    out->val_b = in->val_b; // Value of operand B from the decode stage.
    out->op = in->op;       // The operation code to be executed.

    // Select the appropriate value for the second ALU operand.
    uint64_t v = in->X_sigs.valb_sel ? in->val_b : in->val_imm;

    // If the operation is a branch-and-link (BL), use the sequential PC as val_a.
    if (in->op == OP_BL)
    {
        in->val_a = in->seq_succ_PC;
    }

    // Perform the ALU operation, passing in values and control signals, and capturing the result.
    alu(in->val_a, v, in->val_hw, in->ALU_op, in->X_sigs.set_CC, in->cond, &(out->val_ex), &(X_condval));

    // Capture whether the condition for the ALU operation holds.
    out->cond_holds = X_condval; // The result of the condition code evaluation.
    out->dst = in->dst;          // The destination register for the ALU result.

    // Function ends without returning a value, despite its non-void return type.
    return;
}