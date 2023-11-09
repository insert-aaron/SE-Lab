/**************************************************************************
 * C S 429 system emulator
 *
 * instr_Fetch.c - Fetch stage of instruction processing pipeline.
 **************************************************************************/

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

extern uint64_t F_PC;

/*
 * Select PC logic.
 * STUDENT TO-DO:
 * Write the next PC to *current_PC.
 */

static comb_logic_t
select_PC(uint64_t pred_PC,                                      // The predicted PC
          opcode_t D_opcode, uint64_t val_a,                     // Possible correction from RET
          opcode_t M_opcode, bool M_cond_val, uint64_t seq_succ, // Possible correction from B.cond
          uint64_t *current_PC)
{
    /*
     * Students: Please leave this code
     * at the top of this function.
     * You may modify below it.
     */
    if (D_opcode == OP_RET && val_a == RET_FROM_MAIN_ADDR)
    {
        *current_PC = 0; // PC can't be 0 normally.
        return;
    }

    // Checking conditional branch instruction in memory stage
    if (M_opcode == OP_B_COND && M_cond_val)
    {
        *current_PC = seq_succ; // True, set next PC to seq_succ
        return;
    }

    *current_PC = pred_PC;
    // Modify starting here.
    return;
}

/*
 * Predict PC logic. Conditional branches are predicted taken.
 * STUDENT TO-DO:
 * Write the predicted next PC to *predicted_PC
 * and the next sequential pc to *seq_succ.
 */

static comb_logic_t
predict_PC(uint64_t current_PC, uint32_t insnbits, opcode_t op,
           uint64_t *predicted_PC, uint64_t *seq_succ)
{
    /*
     * Students: Please leave this code
     * at the top of this function.
     * You may modify below it.
     */
    if (!current_PC)
    {
        return; // We use this to generate a halt instruction.
    }

    // Next sequential PC is simply current PC + 4.
    *seq_succ = current_PC + 4;

    // For conditional branch, predict it as taken.
    if (op == OP_B_COND)
    {
        // Assuming offset for branch is stored in lower bitsof insnbits.
        long offset = (insnbits & 0xFFFF) << 2;
        *predicted_PC = current_PC + offset;
    }
    else
    {
        // predicted next PC is simply the next sequential PC.
        *predicted_PC = *seq_succ;
    }
    return;
}

/*
 * Helper function to recognize the aliased instructions:
 * LSL, LSR, CMP, and TST. We do this only to simplify the
 * implementations of the shift operations (rather than having
 * to implement UBFM in full).
 */

static void fix_instr_aliases(uint32_t insnbits, opcode_t *op)
{
    return;
}

/*
 * Fetch stage logic.
 * STUDENT TO-DO:
 * Implement the fetch stage.
 *
 * Use in as the input pipeline register,
 * and update the out pipeline register as output.
 * Additionally, update F_PC for the next
 * cycle's predicted PC.
 *
 * You will also need the following helper functions:
 * select_pc, predict_pc, and imem.
 */

comb_logic_t fetch_instr(f_instr_impl_t *in, d_instr_impl_t *out)
{
    bool imem_err = 0;
    uint64_t current_PC;
    select_PC(in->pred_PC, X_out->op, X_out->val_a, M_out->op, M_out->cond_holds, M_out->seq_succ_PC, &(current_PC));

    /*
     * Students: This case is for generating HLT instructions
     * to stop the pipeline. Only write your code in the **else** case.
     */
    if (!current_PC || F_in->status == STAT_HLT)
    {
        out->insnbits = 0xD4400000U;
        out->op = OP_HLT;
        out->print_op = OP_HLT;
        imem_err = false;
    }
    else
    {
        // Fetching instructions from memory using curPC
        imem_err = imem(current_PC, &(out->insnbits));

        // Use the predict PC function to predict the next PC
        predict_PC(current_PC, out->insnbits, out->op, &predicted_PC & seq_succ);

        // Update F_PC for the next cycle's predicted PC
        F_PC = predicted_PC;

        // Decoding fetched instructions, assuming function decode to
        // to get opcode from insbits
        out->op = decode(out->insnbits);
        out->print_op = out->op;
    }

    // We do not recommend modifying the below code.
    if (imem_err || out->op == OP_ERROR)
    {
        in->status = STAT_INS;
        F_in->status = in->status;
    }
    else if (out->op == OP_HLT)
    {
        in->status = STAT_HLT;
        F_in->status = in->status;
    }
    else
    {
        in->status = STAT_AOK;
    }
    out->status = in->status;
    return;
}
