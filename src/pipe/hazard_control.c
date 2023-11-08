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
    if (D_opcode == OP_RET)
    {
        return true;
    }
    return false;
}

bool check_mispred_branch_hazard(opcode_t X_opcode, bool X_condval)
{
    if (X_opcode == OP_B_COND && !X_condval)
    {
        return true;
    }
    return false;
}

bool check_load_use_hazard(opcode_t D_opcode, uint8_t D_src1, uint8_t D_src2,
                           opcode_t X_opcode, uint8_t X_dst)
{
    if (X_opcode == OP_LDUR && (D_src1 == X_dst || D_src2 == X_dst))
    {
        return true;
    }
    return false;
}

comb_logic_t handle_hazards(opcode_t D_opcode, uint8_t D_src1, uint8_t D_src2,
                            opcode_t X_opcode, uint8_t X_dst, bool X_condval)
{

    // Initialize stall and bubble flags for all stages
    bool f_stall = false, d_stall = false, d_bubble = false,
            x_bubble = false, m_bubble = false, w_bubble = false;


    f_stall = F_out->status == STAT_HLT || F_out->status == STAT_INS;

    // Detect control hazards
    // If the execute stage contains a branch instruction and the condition is validated (branch taken)
    if (is_branch(X_opcode) && X_condval)
    {
        // Bubble the execute stage and potentially the subsequent stages
        d_bubble = true;
        x_bubble = true;  // Must bubble the execute stage if branch is taken
        m_bubble = false; // Can optionally bubble the memory stage
        w_bubble = false; // Can optionally bubble the write-back stage
    }

    // Detect data hazards
    // If the execute stage's destination register is one of the decode stage's source registers
    if (X_dst != R_NONE && (X_dst == D_src1 || X_dst == D_src2))
    {
        // Stall fetch and decode stages until the data is ready (RAW hazard)
        f_stall = true;
        d_stall = true;
    }

    pipe_control_stage(S_FETCH, d_bubble, f_stall);
    pipe_control_stage(S_DECODE, x_bubble, d_stall);
    pipe_control_stage(S_EXECUTE, m_bubble, x_bubble);
    pipe_control_stage(S_MEMORY, w_bubble, m_bubble);
    pipe_control_stage(S_WBACK, false, w_bubble);

    // Create and return the comb_logic_t struct with updated control flags
    comb_logic_t control_logic = {
        .f_stall = f_stall,
        .d_stall = d_stall,
        .d_bubble = d_bubble,
        .x_bubble = x_bubble,
        .m_bubble = m_bubble,
        .w_bubble = w_bubble};

    return control_logic;

}
