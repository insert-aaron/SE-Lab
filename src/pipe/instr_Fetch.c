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
    if (M_opcode == OP_B_COND && !M_cond_val)
    {
        *current_PC = seq_succ;
    }
    else if (D_opcode == OP_RET)
    {
        *current_PC = val_a;
    }
    else
    {
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
    if (op == OP_B || op == OP_BL)
    { // unconditional branch
        uint64_t boffset = bitfield_s64(insnbits, 0, 26) * 4;
        if (boffset < 0)
        {
            *predicted_PC = current_PC - (-boffset);
        }
        else
        {
            *predicted_PC = current_PC + boffset;
        }
    }
    else if (op == OP_B_COND)
    {
        uint64_t boffset = bitfield_s64(insnbits, 5, 19) * 4;
        if (boffset < 0)
        {
            *predicted_PC = current_PC - (-boffset);
        }
        else
        {
            *predicted_PC = current_PC + boffset;
        }
    }
    else
    {
        *predicted_PC = current_PC + 4;
    }

    if (op == OP_ADRP)
    {
        *seq_succ = (current_PC >> 12) << 12;
    }
    else
    {
        *seq_succ = current_PC + 4;
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
    if (*op == OP_UBFM)
    {
        // check of lsl or lsr
        uint32_t immshift = bitfield_u32(insnbits, 10, 6); // instruction value
        if (immshift == 63)
        { // 0x3f
            // all bits = 1
            *op = OP_LSR;
        }
        else
        {
            *op = OP_LSL;
        }
    }
    else if (*op == OP_SUBS_RR)
    {
        uint32_t immdest = bitfield_u32(insnbits, 0, 5); // instruction value
        if (immdest == 31)
        { // 0x1f
            *op = OP_CMP_RR;
        }
    }
    else if (*op == OP_ANDS_RR)
    {
        uint32_t immdest = bitfield_u32(insnbits, 0, 5);
        if (immdest == 31)
        {
            *op = OP_TST_RR;
        }
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

comb_logic_t fetch_instr(f_instr_impl_t *in, d_instr_impl_t *out)
{
    bool imem_err = 0;
    uint64_t current_PC;
    uint64_t pred_PC = in->pred_PC;         // Assuming F_PC is the predicted PC from the previous cycle.
    opcode_t D_opcode = X_out->op;          // Opcode from the Decode pipeline register.
    uint64_t val_a = X_out->val_a;          // A value used for RET correction from the Decode pipeline register.
    opcode_t M_opcode = M_out->op;          // Opcode from the Memory pipeline register.
    bool M_cond_val = M_out->cond_holds;    // The condition flag from the Memory pipeline register.
    uint64_t seq_succ = M_out->seq_succ_PC; // Assuming this is passed down from Decode stage.
    select_PC(pred_PC, D_opcode, val_a, M_opcode, M_cond_val, seq_succ, &current_PC);
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
        // write instruction bits
        imem(current_PC, &(out->insnbits), &imem_err);
        if (imem_err)
        { // if out of range or not a multiple of 4
            in->status = STAT_INS;
            out->status = STAT_INS;
            out->op = OP_NOP;
            out->print_op = OP_NOP;
        }
        else
        {
            uint32_t opcodeaddr = bitfield_u32(out->insnbits, 21, 11); // extract ins bits
            out->op = itable[opcodeaddr];                              // get opcode from table
            // error checks
            if (out->op == OP_ERROR)
            {
                in->status = STAT_INS;
                out->status = STAT_INS;
            }

            // call predict PC
            predict_PC(current_PC, out->insnbits, out->op, &(F_PC), &(out->seq_succ_PC));
            out->print_op = out->op;                          // set print op
            fix_instr_aliases(out->insnbits, &out->op);       // fix aliases
            fix_instr_aliases(out->insnbits, &out->print_op); // fix aliases
        }
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