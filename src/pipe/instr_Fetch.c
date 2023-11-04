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

    // Modify starting here.
    if (D_opcode == OP_RET && val_a == RET_FROM_MAIN_ADDR){
        *current_PC = 0; // PC can't be 0 normally.
        return;
    }
    if (M_opcode == OP_B_COND && !M_cond_val){
        *current_PC = seq_succ;
    }
    else if (D_opcode == OP_RET){
        *current_PC = val_a;
    }
    else{
        *current_PC = pred_PC;
    }
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

    // Modify starting here.

    *seq_succ = current_PC + 4;
    switch (op)
    {
    case OP_B:
    case OP_BL:
        // extract the imm26 for B1
        *predicted_PC = current_PC + bitfield_s64(insnbits, 0, 26) * 4;
        // HELPME: What to do for seq_succ here? Is it different for B and BL?
        break;
    case OP_B_COND:
        // B2 format
        *predicted_PC = current_PC + bitfield_s64(insnbits, 5, 19) * 4;
        //*predicted_PC = current_PC + ((insnbits >> 5) & 0x7FFFF) * 4;
        break;
    case OP_ADRP:
        *predicted_PC = current_PC + 4;
        *seq_succ = *predicted_PC & 0xFFFFF000;
        // *predicted_PC = current_PC + (((bitfield_u32(insnbits, 5, 19) << 2) | bitfield_u32(insnbits, 29, 2)) << 12);
        break;
    default:
        *predicted_PC = *seq_succ;
        break;
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
    switch (*op)
    {
    case OP_UBFM:
        *op = ((insnbits >> 10) & 0x3F) == 0x3F ? OP_LSR : OP_LSL;
        break;
    case OP_SUBS_RR:
        *op = (insnbits & 0x1F) == 0x1F ? OP_CMP_RR : OP_SUBS_RR;
        break;
    case OP_ANDS_RR:
        *op = (insnbits & 0x1F) == 0x1F ? OP_TST_RR : OP_ANDS_RR;
        break;
    default:
        break;
    }
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

comb_logic_t fetch_instr(f_instr_impl_t *in, d_instr_impl_t *out){
    // Dealing with errors
    bool imem_err = 0;
    uint64_t current_PC;
    //Select next program counter based on conditions and store in curPC
    select_PC(in->pred_PC, X_out->op, X_out->val_a, M_out->op, M_out->cond_holds, M_out->seq_succ_PC, &(current_PC));
    /*
     * Students: This case is for generating HLT instructions
     * to stop the pipeline. Only write your code in the **else** case.
     */
    if (!current_PC || F_in->status == STAT_HLT){
        out->insnbits = 0xD4400000U;
        out->op = OP_HLT;
        out->print_op = OP_HLT;
        imem_err = false;
    }else{
        //Var to hold fetched instruction word
        uint32_t iword;
        //Fetch instruct word from curPC
        imem(current_PC, &(iword), &(imem_err));

        //Determine opcodd from fetched instr, fixing and setting for printing
        out->op = itable[(iword >> 21) & 0x7FF];
        fix_instr_aliases(iword, &(out->op));
        out->print_op = out->op;
        out->insnbits = iword;

        // call the various fetch helper functions as appropriate
        predict_PC(current_PC, iword, out->op, &(F_PC), &(out->seq_succ_PC));
    }

    // Check for instruction memory errors or an OP_ERROR opcode in the output instruction
    if (imem_err || out->op == OP_ERROR){
        in->status = STAT_INS;
        F_in->status = in->status;
    }else if (out->op == OP_HLT){
        // set status of input & F_in to halt
        in->status = STAT_HLT;
        F_in->status = in->status;
    }else{
        //else normal operation
        in->status = STAT_AOK;
    }
    out->status = in->status;
    return;
}
