/**************************************************************************
 * C S 429 system emulator
 *
 * instr_Decode.c - Decode stage of instruction processing pipeline.
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
#include "forward.h"
#include "machine.h"
#include "hw_elts.h"

#define SP_NUM 31
#define XZR_NUM 32

extern machine_t guest;
extern mem_status_t dmem_status;

extern int64_t W_wval;

/*
 * Control signals for D, X, M, and W stages.
 * Generated by D stage logic.
 * D control signals are consumed locally.
 * Others must be buffered in pipeline registers.
 * STUDENT TO-DO:
 * Generate the correct control signals for this instruction's
 * future stages and write them to the corresponding struct.
 */

static comb_logic_t
generate_DXMW_control(opcode_t op, d_ctl_sigs_t *D_sigs, x_ctl_sigs_t *X_sigs, m_ctl_sigs_t *M_sigs, w_ctl_sigs_t *W_sigs)
{

    D_sigs->src2_sel = op == OP_STUR;
    X_sigs->valb_sel = (op >= OP_ADDS_RR && op <= OP_TST_RR && op != OP_SUB_RI && op != OP_MVN);
    X_sigs->set_CC = X_sigs->valb_sel && op != OP_ORR_RR && op != OP_EOR_RR;
    M_sigs->dmem_read = op == OP_LDUR;
    M_sigs->dmem_write = op == OP_STUR;
    W_sigs->dst_sel = op == OP_BL;
    W_sigs->wval_sel = op == OP_LDUR;
    W_sigs->w_enable = op == OP_LDUR || 
        (op > OP_STUR && op < OP_CMP_RR) || (op > OP_CMP_RR && op < OP_TST_RR) || (op > OP_TST_RR && op < OP_B) || op == OP_BL;
    return;
}

/*
 * Logic for extracting the immediate value for M-, I-, and RI-format instructions.
 * STUDENT TO-DO:
 * Extract the immediate value and write it to *imm.
 */

static comb_logic_t
extract_immval(uint32_t insnbits, opcode_t op, int64_t *imm){
    if (op == OP_LDUR || op == OP_STUR){
        *imm = bitfield_s64(insnbits, 12, 9);
    }
    else if (op == OP_MOVK || op == OP_MOVZ){
        *imm = bitfield_u32(insnbits, 5, 16);
    }
    else if (op == OP_ADRP){
        *imm = (bitfield_s64(insnbits, 5, 19) << 14) | (bitfield_s64(insnbits, 29, 2) << 12);
    }
    else if (op == OP_ADD_RI 
        || op == OP_SUB_RI || op == OP_UBFM || op == OP_ASR){
        *imm = bitfield_u32(insnbits, 10, 12);
    }
    else if (op == OP_LSL){
        *imm = bitfield_u32(insnbits, 10, 6) % 32 + 1;
    }
    else if (op == OP_LSR){
        *imm = bitfield_u32(insnbits, 10, 6) - bitfield_u32(insnbits, 16, 6) + 1;
    }
    else if (op == OP_B || op == OP_BL){
        *imm = bitfield_s64(insnbits, 0, 26);
    }
    else{
        *imm = 0;
    }

}
/*
 * Logic for determining the ALU operation needed for this opcode.
 * STUDENT TO-DO:
 * Determine the ALU operation based on the given opcode
 * and write it to *ALU_op.
 */
static comb_logic_t
decide_alu_op(opcode_t op, alu_op_t *ALU_op){
    // Initialize the ALU_op to a default value (in case no matching opcode is found)
    *ALU_op = PASS_A_OP;

    // Check the value of op and set ALU_op accordingly
    if (op == OP_ADD_RI || op == OP_ADDS_RR || op == OP_LDUR || op == OP_STUR || op == OP_ADRP){
        *ALU_op = PLUS_OP;
    }
    else if (op == OP_SUB_RI || op == OP_SUBS_RR || op == OP_CMP_RR){
        *ALU_op = MINUS_OP;
    }
    else if (op == OP_MVN){
        *ALU_op = NEG_OP;
    }
    else if (op == OP_ORR_RR){
        *ALU_op = OR_OP;
    }
    else if (op == OP_EOR_RR){
        *ALU_op = EOR_OP;
    }
    else if (op == OP_ANDS_RR || op == OP_TST_RR){
        *ALU_op = AND_OP;
    }
    else if (op == OP_MOVK || op == OP_MOVZ){
        *ALU_op = MOV_OP;
    }
    else if (op == OP_LSL){
        *ALU_op = LSL_OP;
    }
    else if (op == OP_LSR){
        *ALU_op = LSR_OP;
    }
    else if (op == OP_ASR){
        *ALU_op = ASR_OP;
    }

    // No need for a "default" case since we've already initialized ALU_op to a default value

    return;
}


/*
 * Utility functions for copying over control signals across a stage.
 * STUDENT TO-DO:
 * Copy the input signals from the input side of the pipeline
 * register to the output side of the register.
 */

comb_logic_t
copy_m_ctl_sigs(m_ctl_sigs_t *dest, m_ctl_sigs_t *src)
{
    dest->dmem_read = src->dmem_read;
    dest->dmem_write = src->dmem_write;
    return;
}

comb_logic_t
copy_w_ctl_sigs(w_ctl_sigs_t *dest, w_ctl_sigs_t *src)
{
    dest->dst_sel = src->dst_sel;
    dest->w_enable = src->w_enable;
    dest->wval_sel = src->wval_sel;
    return;
}

// uint8_t remap_register(uint8_t register){
//     return (register == 31) ? 32 : register;
// }

comb_logic_t
extract_regs(uint32_t insnbits, opcode_t op,
             uint8_t *src1, uint8_t *src2, uint8_t *dst)
{
    if ((op <= OP_ASR && (op < OP_MOVK || op > OP_ADRP)) || op == OP_RET)
    {
        *src1 = (insnbits >> 5) & 0x1F;
    }
    else if (op == OP_MOVZ || op == OP_B_COND)
    {
        *src1 = XZR_NUM;
    }
    else if (op == OP_MOVK)
    {
        *src1 = insnbits & 0x1F;
    }

    if (op == OP_MVN)
    {
        *src1 = XZR_NUM;
    }

    if ((op >= OP_SUBS_RR && op <= OP_TST_RR) || op == OP_ADDS_RR)
    {
        *src2 = (insnbits >> 16) & 0x1F;
    }
    else if (op == OP_MOVZ || op == OP_MOVK || op == OP_B_COND || (op >= OP_LSL && op <= OP_UBFM))
    {
        *src2 = XZR_NUM;
    }
    else if (op == OP_STUR)
    {
        *src2 = insnbits & 0x1F;
    }

    if (op >= OP_LDUR && op <= OP_ASR)
    {
        *dst = insnbits & 0x1F;
    }

    return;
}


/*
 * Decode stage logic.
 * STUDENT TO-DO:
 * Implement the decode stage.
 *
 * Use `in` as the input pipeline register,
 * and update the `out` pipeline register as output.
 * Additionally, make sure the register file is updated
 * with W_out's output when you call it in this stage.
 *
 * You will also need the following helper functions:
 * generate_DXMW_control, regfile, extract_immval,
 * and decide_alu_op.
 */

comb_logic_t decode_instr(d_instr_impl_t *in, x_instr_impl_t *out)
{
    // Initialize a structure for control signals with default values (all set to false)
    d_ctl_sigs_t dsigs = {false};
    // Generate control signals for the DXMW stages based on the input opcode and store them in dsigs
    generate_DXMW_control(in->op, &(dsigs), &(out->X_sigs), &(out->M_sigs), &(out->W_sigs));

    // Initialize source register identifiers
    uint8_t src1 = 0;
    uint8_t src2 = 0;
    // Extract source register identifiers and destination register identifier from the instruction bits
    extract_regs(in->insnbits, in->op, &(src1), &(src2), &out->dst);
    // Access the register file to get values from source registers, and store them in val_a and val_b
    regfile(src1, src2, W_out->dst, W_wval, W_out->W_sigs.w_enable, &(out->val_a), &(out->val_b));
    // Extract immediate values based on the opcode and store them in val_imm
    extract_immval(in->insnbits, in->op, &(out->val_imm));
    // Determine the ALU operation based on the opcode and store it in ALU_op
    decide_alu_op(in->op, &out->ALU_op);

    // If the opcode is MOVK or MOVZ, compute val_hw based on instruction bits, else set it to 0

    if (in->op == OP_MOVK || in->op == OP_MOVZ){
        out->val_hw = ((in->insnbits >> 21) & 0x3) * 16;
    }
    else{
        out->val_hw = 0;
    }

    out->op = in->op;
    out->print_op = in->print_op;
    out->status = in->status;
    out->seq_succ_PC = in->seq_succ_PC;

    // If the opcode is OP_B_COND, extract and store the condition code

    if (in->op == OP_B_COND){
        out->cond = bitfield_u32(in->insnbits, 0, 4);
    }

    // If the opcode is ADRP, set val_a to the sequential successor program counter

    if (in->op == OP_ADRP)
    {
        out->val_a = in->seq_succ_PC;
    }

    return;
}