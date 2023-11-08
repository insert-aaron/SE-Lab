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

extern machine_t guest;
extern mem_status_t dmem_status;

/* Use this method to actually bubble/stall a pipeline stage.
 * Call it in handle_hazards(). Do not modify this code. */
void pipe_control_stage(proc_stage_t stage, bool bubble, bool stall)
{
    pipe_reg_t *pipe;
    switch (stage)
    {
    case S_FETCH:
        pipe = F_instr;
        break;
    case S_DECODE:
        pipe = D_instr;
        break;
    case S_EXECUTE:
        pipe = X_instr;
        break;
    case S_MEMORY:
        pipe = M_instr;
        break;
    case S_WBACK:
        pipe = W_instr;
        break;
    default:
        printf("Error: incorrect stage provided to pipe control.\n");
        return;
    }
    if (bubble && stall)
    {
        printf("Error: cannot bubble and stall at the same time.\n");
        pipe->ctl = P_ERROR;
    }
    // If we were previously in an error state, stay there.
    if (pipe->ctl == P_ERROR)
        return;

    if (bubble)
    {
        pipe->ctl = P_BUBBLE;
    }
    else if (stall)
    {
        pipe->ctl = P_STALL;
    }
    else
    {
        pipe->ctl = P_LOAD;
    }
}

bool check_ret_hazard(opcode_t D_opcode)
{
    /* Students: Implement Below */
    if (D_opcode = OP_RET)
    {
        return true;
    }
    return false;
}

bool check_mispred_branch_hazard(opcode_t X_opcode, bool X_condval)
{
    /* Students: Implement Below */
    if (X_opcode == OP_B_COND && !X_condval)
    {
        return true;
    }
    return false;
}

bool check_load_use_hazard(opcode_t D_opcode, uint8_t D_src1, uint8_t D_src2,
                           opcode_t X_opcode, uint8_t X_dst)
{
    /* Students: Implement Below */
    if (X_opcode == OP_LDUR && (D_src1 == X_dst || D_src2 == X_dst))
    {
        return true;
    }
    return false;
}

comb_logic_t handle_hazards(opcode_t D_opcode, uint8_t D_src1, uint8_t D_src2,
                            opcode_t X_opcode, uint8_t X_dst, bool X_condval)
{
    /* Students: Change the below code IN WEEK TWO -- do not touch for week one */
    // Initial stall check for the Fetch stage
    bool f_stall = F_out->status == STAT_HLT || F_out->status == STAT_INS;
    bool d_stall = false;
    bool x_stall = false;
    bool m_stall = false;
    bool w_stall = false;

    // Check for a return hazard which might require stalling the fetch stage
    if (check_ret_hazard(D_opcode))
    {
        // If a return instruction is in the Decode stage, we need to stall Fetch until it's resolved.
        f_stall = true;
    }

    // Check for a mispredicted branch hazard which might require flushing the pipeline
    if (check_mispred_branch_hazard(X_opcode, X_condval))
    {
        // If a branch was mispredicted, we need to flush the pipeline from Execute backwards.
        f_stall = true;
        d_stall = true;
        x_stall = true; // Actually, this could be a flush, depending on the pipeline implementation.
        // Flushing could involve setting specific control flags or resetting the pipeline registers.
    }

    // Check for a load-use hazard which might require stalling the decode stage
    if (check_load_use_hazard(D_opcode, D_src1, D_src2, X_opcode, X_dst))
    {
        // If a load-use hazard is detected, Decode must stall until the load completes in Execute.
        d_stall = true;
    }

    // Based on the stall flags set above, control the pipeline stages accordingly
    pipe_control_stage(S_FETCH, false, f_stall);
    pipe_control_stage(S_DECODE, false, d_stall);
    pipe_control_stage(S_EXECUTE, false, x_stall);
    pipe_control_stage(S_MEMORY, false, m_stall);
    pipe_control_stage(S_WBACK, false, w_stall);

    // Construct the return value with the updated control flags.
    comb_logic_t result = {
        .f_stall = f_stall,
        .d_stall = d_stall,
        .x_stall = x_stall,
        .m_stall = m_stall,
        .w_stall = w_stall};
    return result;
}
