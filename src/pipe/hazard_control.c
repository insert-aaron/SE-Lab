/**************************************************************************
 * C S 429 system emulator
 *
 * Bubble and stall checking logic.
 * STUDENT TO-DO:
 * Implement logic for hazard handling.
 *
 * handle_hazards is called from proc.c with the appropriate
 * parameters already set, you must implement the logic for it.
 *
 * You may optionally use the other three helper functions to
 * make it easier to follow your logic.
 **************************************************************************/

#include "machine.h"

/* Bubble and stall checking logic.
 * STUDENT TO-DO:
 * Implement logic for hazard handling.
 *
 * handle_hazards is called from proc.c with the appropriate
 * parameters already set, you must implement the logic for it.
 *
 * You may optionally use the other three helper functions to
 * make it easier to follow your logic.
 */

extern machine_t guest;
extern mem_status_t dmem_status;

bool check_ret_hazard(opcode_t D_opcode)
{
    if(D_opcode == OP_RET)
        return true;

    else
        return false;
}

bool check_mispred_branch_hazard(opcode_t X_opcode, bool X_condval)
{
    if (X_opcode == OP_B_COND && !X_condval)
        return true;

    else
        return false;
}

bool check_load_use_hazard(opcode_t D_opcode, uint8_t D_src1, uint8_t D_src2,
                           opcode_t X_opcode, uint8_t X_dst)
{
    if(X_opcode == OP_LDUR && (X_dst == D_src1 || X_dst == D_src2))
        return true;
    
    else
        return false;
}

comb_logic_t handle_hazards(opcode_t D_opcode, uint8_t D_src1, uint8_t D_src2,
                            opcode_t X_opcode, uint8_t X_dst, bool X_condval)
{
    guest.proc->f_insn->out->bubble = false;
    guest.proc->f_insn->out->stall = false;
    guest.proc->d_insn->out->bubble = false;
    guest.proc->d_insn->out->stall = false;
    guest.proc->x_insn->out->stall = false;
    guest.proc->x_insn->out->bubble = false;
    guest.proc->m_insn->out->stall = false;
    guest.proc->m_insn->out->bubble = false;
    guest.proc->w_insn->out->stall = false;
    guest.proc->w_insn->out->stall = false;

    
    
    if(check_mispred_branch_hazard(X_opcode,X_condval) && check_ret_hazard(D_opcode))
    {
        guest.proc->f_insn->out->bubble = true;
        guest.proc->d_insn->out->bubble = true;
    }
    else if (check_ret_hazard(D_opcode) && check_load_use_hazard(D_opcode,D_src1,D_src2,X_opcode,X_dst))
    {
        guest.proc->f_insn->out->stall = true;
        guest.proc->d_insn->out->bubble = true;
    }
    else if (check_ret_hazard(D_opcode))
    {
        guest.proc->f_insn->out->bubble = true;
    }
    else if (check_load_use_hazard(D_opcode,D_src1,D_src2,X_opcode,X_dst))
    {
        guest.proc->f_insn->out->stall = true;
        guest.proc->d_insn->out->bubble = true;
    }
    else if (check_mispred_branch_hazard(X_opcode,X_condval))
    {
        guest.proc->f_insn->out->bubble = true;
        guest.proc->d_insn->out->bubble = true;
    }
    
    if (dmem_status == IN_FLIGHT)
    {
        guest.proc->f_insn->out->stall = true;
        guest.proc->d_insn->out->stall = true;
        guest.proc->x_insn->out->stall = true;
        guest.proc->m_insn->out->stall - true;
        guest.proc->f_insn->out->bubble = false;
        guest.proc->d_insn->out->bubble = false;
        guest.proc->x_insn->out->bubble = false;
        guest.proc->m_insn->out->bubble = false;
        guest.proc->w_insn->out->bubble = false;
    }
    return;
    
}
