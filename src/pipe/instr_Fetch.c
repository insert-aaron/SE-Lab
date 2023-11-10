/**************************************************************************
 * C S 429 system emulator
 * Ryan Passaro rjp2827 and Aaron Alvarez aa88379
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
    // Modify starting here.
    // If the current opcode is a return operation (OP_RET) and the return address
    // is the address where the main function returns (RET_FROM_MAIN_ADDR),
    // set the Program Counter (PC) to 0 to indicate an abnormal situation since PC shouldn't normally be 0.
    if (D_opcode == OP_RET && val_a == RET_FROM_MAIN_ADDR)
    {
        *current_PC = 0; // Set the Program Counter to 0
        return;          // Exit the function early
    }

    // If the memory stage opcode is a branch condition (OP_B_COND) and the condition is not met (!M_cond_val),
    // then set the PC to the sequentially succeeding address (seq_succ) as the branch is not taken.
    if (M_opcode == OP_B_COND && !M_cond_val)
    {
        *current_PC = seq_succ;
    }
    else if (D_opcode == OP_RET)
    {                        // If the current opcode is a return operation (OP_RET),
        *current_PC = val_a; // Update the PC to the return address stored in val_a.
    }
    else
    {
        // If none of the above conditions are met, update the PC to the predicted PC (pred_PC),
        // which is typically the next sequential instruction in the pipeline.
        *current_PC = pred_PC;
    }

    return; // End of the function
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
    // Modify starting here.
    // Update the sequential successor address by adding 4 to the current program counter (PC).
    // Set the sequential successor PC (Program Counter) to be the current PC plus 4 bytes.
    *seq_succ = current_PC + 4;

    // Begin a switch statement to determine the control flow based on the operation code.
    switch (op)
    {
    case OP_B:  // If the operation is an unconditional branch...
    case OP_BL: // ...or a branch with link (which saves the return address)...
        // Predict the next PC by adding the sign-extended value of a bitfield
        // from the instruction bits to the current PC. This bitfield represents
        // the branch offset. Multiply by 4 because in ARM the PC is byte addressable
        // and instructions are word-aligned (4 bytes).
        *predicted_PC = current_PC + bitfield_s64(insnbits, 0, 26) * 4;
        break;

    case OP_B_COND: // If the operation is a conditional branch...
        // Similar to the unconditional branch, but the bitfield is taken from
        // a different position in the instruction bits, reflecting the encoding
        // of the conditional branch instructions.
        *predicted_PC = current_PC + bitfield_s64(insnbits, 5, 19) * 4;
        break;

    case OP_ADRP: // If the operation is an "add relative to page" instruction...
        // The predicted PC is set to the current PC plus 4 bytes.
        *predicted_PC = current_PC + 4;
        // The sequential successor is then aligned to a 4KB page boundary
        // by clearing the lower 12 bits. This is specific to the ADRP instruction
        // which is used for page-granular addressing.
        *seq_succ = *predicted_PC & 0xFFFFF000;
        break;

    default: // For any other operation...
        // The predicted PC is set to the sequential successor which has
        // already been calculated at the beginning.
        *predicted_PC = *seq_succ;
        break;
    }

    // The function returns void, so there is an implicit return here.
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
    // Based on the current opcode, adjust the opcode to its correct form by inspecting the instruction bits.
    switch (*op)
    {
    case OP_UBFM:
        // For the UBFM instruction, decide whether it should be an LSR or LSL instruction
        // This is determined by checking if bits 10 through 15 of insnbits are all set (0x3F)
        *op = ((insnbits >> 10) & 0x3F) == 0x3F ? OP_LSR : OP_LSL;
        break;
    case OP_SUBS_RR:
        // For the SUBS instruction, check if the bottom 5 bits are all set.
        // If true, the operation is a comparison rather than a subtraction, hence set to CMP.
        *op = (insnbits & 0x1F) == 0x1F ? OP_CMP_RR : OP_SUBS_RR;
        break;
    case OP_ANDS_RR:
        // For the ANDS instruction, similar to the SUBS case,
        // check the bottom 5 bits to determine if the operation is a test (TST) instead of an AND.
        *op = (insnbits & 0x1F) == 0x1F ? OP_TST_RR : OP_ANDS_RR;
        break;
    default:
        // If none of the cases match, no change is made to the opcode.
        break;
    }
    // End of the function, return to caller.
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
    // select_PC(in->pred_PC, D_out->op, X_in->val_a, M_out->op, M_in->cond_holds, M_out->seq_succ_PC, &current_PC);
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
        // WRITE HERE!
        // get the insnbits for the current instruction
        uint32_t iword;
        imem(current_PC, &(iword), &(imem_err));

        // decode those bits to get the opcode
        // extract top 11 bits, send as input to the itable
        out->op = itable[(iword >> 21) & 0x7FF];
        fix_instr_aliases(iword, &(out->op));
        out->print_op = out->op;
        out->insnbits = iword;

        // call the various fetch helper functions as appropriate
        predict_PC(current_PC, iword, out->op, &(F_PC), &(out->seq_succ_PC));
    }

    // Handle errors and set error codes

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