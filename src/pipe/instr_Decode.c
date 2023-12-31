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

    if (op == OP_STUR)
    {
        D_sigs->src2_sel = 1;
    }
    else
    {
        D_sigs->src2_sel = 0;
    }
    if (op == OP_ADDS_RR || op == OP_ANDS_RR || op == OP_SUBS_RR || op == OP_CMP_RR || op == OP_TST_RR || op == OP_ORR_RR || op == OP_EOR_RR || op == OP_MVN)
    {
        X_sigs->valb_sel = 1;
    }
    else
    {
        X_sigs->valb_sel = 0;
    }
    if (op == OP_ADDS_RR || op == OP_ANDS_RR || op == OP_SUBS_RR || op == OP_CMP_RR || op == OP_TST_RR)
    {
        X_sigs->set_CC = 1;
    }
    else
    {
        X_sigs->set_CC = 0;
    }
    if (op == OP_LDUR)
    {
        M_sigs->dmem_read = 1;
    }
    else
    {
        M_sigs->dmem_read = 0;
    }
    if (op == OP_STUR)
    {
        M_sigs->dmem_write = 1;
    }
    else
    {
        M_sigs->dmem_write = 0;
    }
    if (op == OP_BL)
    {
        W_sigs->dst_sel = 1;
    }
    else
    {
        W_sigs->dst_sel = 0;
    }
    if (op == OP_LDUR)
    {
        W_sigs->wval_sel = 1;
    }
    else
    {
        W_sigs->wval_sel = 0;
    }
    if (op == OP_ERROR)
    {
        W_sigs->w_enable = 0;
    }
    if (op != OP_STUR && op != OP_B && op != OP_B_COND && op != OP_RET && op != OP_NOP && op != OP_HLT && op != OP_CMP_RR && op != OP_TST_RR)
    {
        W_sigs->w_enable = 1;
    }
    else
    {
        W_sigs->w_enable = 0;
    }
    if (op == OP_CMP_RR)
    {
        W_sigs->w_enable = 0;
    }
    return;
}

/*
 * Logic for extracting the immediate value for M-, I-, and RI-format instructions.
 * STUDENT TO-DO:
 * Extract the immediate value and write it to *imm.
 */

static comb_logic_t
extract_immval(uint32_t insnbits, opcode_t op, int64_t *imm)
{
    if (op == OP_STUR || op == OP_LDUR)
    {
        *imm = bitfield_u32(insnbits, 12, 9);
        return;
    }
    else if (op == OP_MOVK || op == OP_MOVZ)
    {
        *imm = bitfield_u32(insnbits, 5, 16);
        return;
    }
    else if (op == OP_ADRP)
    {
        *imm = ((bitfield_u32(insnbits, 5, 19) << 2) | bitfield_u32(insnbits, 29, 2)) << 12;
        return;
    }
    else if (op == OP_ADD_RI || op == OP_SUB_RI || op == OP_ASR)
    {
        *imm = bitfield_u32(insnbits, 10, 12);
        return;
    }
    else if (op == OP_LSL || op == OP_UBFM)
    {
        *imm = 64 - bitfield_u32(insnbits, 16, 6);
        return;
    }
    else if (op == OP_LSR)
    {
        *imm = bitfield_u32(insnbits, 16, 6);
        return;
    }
    else if (op == OP_B)
    {
        *imm = bitfield_s64(insnbits, 0, 26);
        return;
    }
    else
    {
        *imm = 0;
    }

    return;
}

/*
 * Logic for determining the ALU operation needed for this opcode.
 * STUDENT TO-DO:
 * Determine the ALU operation based on the given opcode
 * and write it to *ALU_op.
 */
static comb_logic_t
decide_alu_op(opcode_t op, alu_op_t *ALU_op)
{

    if (op == OP_ADD_RI || op == OP_ADDS_RR || op == OP_LDUR || op == OP_STUR || op == OP_ADRP)
    {
        *ALU_op = PLUS_OP;
    }
    else if (op == OP_SUB_RI || op == OP_SUBS_RR || op == OP_CMP_RR)
    {
        *ALU_op = MINUS_OP;
    }
    else if (op == OP_MVN)
    {
        *ALU_op = NEG_OP;
    }
    else if (op == OP_ORR_RR)
    {
        *ALU_op = OR_OP;
    }
    else if (op == OP_EOR_RR)
    {
        *ALU_op = EOR_OP;
    }
    else if (op == OP_ANDS_RR || op == OP_TST_RR)
    {
        *ALU_op = AND_OP;
    }
    else if (op == OP_MOVK || op == OP_MOVZ)
    {
        *ALU_op = MOV_OP;
    }
    else if (op == OP_LSL || op == OP_UBFM)
    {
        *ALU_op = LSL_OP;
    }
    else if (op == OP_LSR)
    {
        *ALU_op = LSR_OP;
    }
    else if (op == OP_ASR)
    {
        *ALU_op = ASR_OP;
    }
    else if (op == OP_BL || op == OP_RET || op == OP_B || op == OP_B_COND || op == OP_HLT || op == OP_NOP || op == OP_ERROR)
    {
        *ALU_op = PASS_A_OP;
    }
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

comb_logic_t
extract_regs(uint32_t insnbits, opcode_t op,
             uint8_t *src1, uint8_t *src2, uint8_t *dst)
{

    if (op == OP_STUR)
    {
        *src2 = insnbits & 0x1FU;
        *src1 = (insnbits >> 5) & 0x1FU;
        *dst = insnbits & 0x1FU;
    }
    else if (op == OP_LDUR)
    {
        *src2 = 0;
        *src1 = (insnbits >> 5) & 0x1FU;
        *dst = insnbits & 0x1FU;
    }
    else if (op == OP_MOVK)
    {
        *dst = bitfield_u32(insnbits, 0, 5);
        *src1 = bitfield_s64(insnbits, 0, 5);
        if (*dst == 31)
        {
            *dst = 32;
        }
        else
        {
            *src1 = *dst;
        }
        if (*src1 == 31)
        {
            *src1 = 32;
        }
        *src2 = 32;
    }
    else if (op == OP_MOVZ)
    {
        *dst = bitfield_u32(insnbits, 0, 5);
        if (*dst == 31)
        {
            *dst = 32;
        }
        *src1 = 32;
        *src2 = 32;
    }
    else if (op == OP_ADRP)
    {
        *dst = bitfield_u32(insnbits, 0, 5);
        *src1 = XZR_NUM;
        *src2 = XZR_NUM;
    }
    else if (op == OP_ADDS_RR)
    {
        *src1 = bitfield_u32(insnbits, 5, 5);
        *src2 = bitfield_u32(insnbits, 16, 5);
        *dst = bitfield_u32(insnbits, 0, 5);
        if (*dst == 31)
        {
            *dst = 32;
        }
        if (*src1 == 31)
        {
            *src1 = 32;
        }
        if (*src2 == 31)
        {
            *src2 = 32;
        }
    }
    else if (op == OP_SUBS_RR)
    {
        *src1 = bitfield_u32(insnbits, 5, 5);
        *src2 = bitfield_u32(insnbits, 16, 5);
        *dst = bitfield_u32(insnbits, 0, 5);
        if (*dst == 31)
        {
            *dst = 32;
        }
        if (*src1 == 31)
        {
            *src1 = 32;
        }
        if (*src2 == 31)
        {
            *src2 = 32;
        }
    }
    else if (op == OP_MVN || op == OP_ORR_RR || op == OP_EOR_RR || op == OP_ANDS_RR)
    {
        *src1 = bitfield_u32(insnbits, 5, 5);  // 5 shift
        *src2 = bitfield_u32(insnbits, 16, 5); // 16 shift
        *dst = bitfield_u32(insnbits, 0, 5);   // 32
        if (*dst == 31)
        {
            *dst = 32;
        }
        if (*src1 == 31)
        {
            *src1 = 32;
        }
        if (*src2 == 31)
        {
            *src2 = 32;
        }
    }
    else if (op == OP_CMP_RR || op == OP_TST_RR)
    {
        *src1 = bitfield_u32(insnbits, 5, 5);  // 5 shift
        *src2 = bitfield_u32(insnbits, 16, 5); // 16 shift
        *dst = XZR_NUM;                        // 32
    }
    else if (op == OP_ADD_RI)
    {
        *src1 = bitfield_u32(insnbits, 5, 5);
        *dst = bitfield_u32(insnbits, 0, 5);
        *src2 = 32;
    }
    else if (op == OP_SUB_RI)
    {
        *src1 = bitfield_u32(insnbits, 5, 5);
        *dst = bitfield_u32(insnbits, 0, 5);
        *src2 = 32;
    }
    else if (op == OP_LSL || op == OP_LSR || op == OP_UBFM || op == OP_ASR)
    {
        *src1 = bitfield_u32(insnbits, 5, 5);
        *dst = bitfield_u32(insnbits, 0, 5);
        if (*dst == 31)
        {
            *dst = 32;
        }
        if (*src1 == 31)
        {
            *src1 = 32;
        }
        *src2 = 32;
    }
    else if (op == OP_RET)
    {
        *src1 = bitfield_u32(insnbits, 5, 5);
        if (*src1 == 31)
        {
            *src1 = 32;
        }
        *dst = 32;
        *src2 = 32;
    }
    else if (op == OP_B || op == OP_B_COND || op == OP_HLT)
    {
        *dst = 32;
        *src1 = 32;
        *src2 = 32;
    }
    else if (op == OP_BL)
    {
        *dst = 30;
        *src1 = 32;
        *src2 = 32;
    }
    else if (op == OP_NOP)
    {
        *src1 = 32;
        *src2 = 32;
    }
    else
    {
        *src2 = 32;
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
    out->op = in->op;
    out->print_op = in->print_op;
    out->seq_succ_PC = in->seq_succ_PC;
    out->status = in->status;

    d_ctl_sigs_t D_sigs;
    generate_DXMW_control(in->print_op, &D_sigs, &(out->X_sigs), &(out->M_sigs), &(out->W_sigs));

    // uint8_t dst;
    uint8_t src1;
    uint8_t src2;

    extract_immval(in->insnbits, in->op, &(out->val_imm));
    decide_alu_op(in->print_op, &(out->ALU_op));
    extract_regs(in->insnbits, in->op, &src1, &src2, &(out->dst));
    regfile(src1, src2, W_out->dst, W_wval, W_out->W_sigs.w_enable, &(out->val_a), &(out->val_b));

    forward_reg(src1, src2, X_out->dst, M_out->dst, W_out->dst, M_in->val_ex, M_out->val_ex, W_in->val_mem,
                W_out->val_ex, W_out->val_mem, M_out->W_sigs.wval_sel, W_out->W_sigs.wval_sel, X_out->W_sigs.w_enable,
                M_out->W_sigs.w_enable, W_out->W_sigs.w_enable, &out->val_a, &out->val_b);

    if (in->op == OP_MVN)
    {
        out->val_a = 0;
    }
    if (in->op == OP_B_COND)
    {
        out->cond = bitfield_u32(in->insnbits, 0, 4);
    }
    if (in->op == OP_MOVK || in->op == OP_MOVZ)
    {
        out->val_hw = bitfield_u32(in->insnbits, 21, 2) * 16;
    }
    else
    {
        out->val_hw = 0;
    }
    if (in->op == OP_BL)
    {
        out->val_a = in->seq_succ_PC;
        out->val_b = 0;
    }
    if (in->op == OP_ADRP)
    {
        out->val_a = in->seq_succ_PC;
    }
    if (in->op == OP_HLT)
    {
        in->status = STAT_HLT;
        out->status = STAT_HLT;
    }

    return;
}