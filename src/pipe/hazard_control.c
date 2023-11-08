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

    // Handle non-nominal status by stalling appropriate pipeline stages
    if (W_out->status != STAT_AOK && W_out->status != STAT_BUB)
    {
        stall_pipeline();
    }
    if (M_out->status != STAT_AOK && M_out->status != STAT_BUB)
    {
        X_in->X_sigs.set_CC = false;
        stall_pipeline_except_wb();
    }
    if (X_out->status != STAT_AOK && X_out->status != STAT_BUB)
    {
        stall_pipeline_up_to_execute();
    }
    if (D_out->status != STAT_AOK && D_out->status != STAT_BUB)
    {
        X_in->W_sigs.w_enable = false;
        stall_pipeline_up_to_decode();
    }
    if (F_out->status != STAT_AOK && F_out->status != STAT_BUB)
    {
        pipe_control_stage(S_FETCH, false, true);
    }

    // Handle data memory operations in flight by stalling and bubbling WB
    if (dmem_status == IN_FLIGHT)
    {
        stall_pipeline();
        bubble_wb();
    }

    // Hazard checks and respective pipeline adjustments
    if (check_load_use_hazard(D_opcode, D_src1, D_src2, X_opcode, X_dst))
    {
        handle_load_use_hazard();
    }
    else if (check_mispred_branch_hazard(X_opcode, X_condval))
    {
        handle_mispred_branch_hazard();
    }
    else if (check_ret_hazard(D_opcode))
    {
        handle_ret_hazard();
    }
}

// Stall the entire pipeline
void stall_pipeline()
{
    pipe_control_stage(S_FETCH, false, true);
    pipe_control_stage(S_DECODE, false, true);
    pipe_control_stage(S_EXECUTE, false, true);
    pipe_control_stage(S_MEMORY, false, true);
    pipe_control_stage(S_WBACK, false, true);
}

// Stall the pipeline except for WB
void stall_pipeline_except_wb()
{
    pipe_control_stage(S_FETCH, false, true);
    pipe_control_stage(S_DECODE, false, true);
    pipe_control_stage(S_EXECUTE, false, true);
    pipe_control_stage(S_MEMORY, false, true);
    // Do not stall WB stage
}

// Stall the pipeline up to the execute stage
void stall_pipeline_up_to_execute()
{
    pipe_control_stage(S_FETCH, false, true);
    pipe_control_stage(S_DECODE, false, true);
    pipe_control_stage(S_EXECUTE, false, true);
    // Do not stall Memory and WB stages
}

// Stall the pipeline up to the decode stage
void stall_pipeline_up_to_decode()
{
    pipe_control_stage(S_FETCH, false, true);
    pipe_control_stage(S_DECODE, false, true);
    // Do not stall Execute, Memory, and WB stages
}

// Bubble the WB stage
void bubble_wb()
{
    pipe_control_stage(S_WBACK, true, false);
}

// Handle load-use hazard
void handle_load_use_hazard()
{
    // Implementation would go here. Typically, this would involve stalling
    // the pipeline until the data is ready to be used.
    pipe_control_stage(S_FETCH, false, true);
    pipe_control_stage(S_DECODE, false, true);
    pipe_control_stage(S_EXECUTE, true, false); // Insert bubble here to resolve the hazard
}

// Handle mispredicted branch hazard
void handle_mispred_branch_hazard()
{
    // Implementation would go here. Often involves flushing the pipeline and fetching the correct path.
    flush_pipeline();
    // Possibly set up for fetching the correct branch path
}

// Handle return instruction hazard
void handle_ret_hazard()
{
    // Similar to branch misprediction, this might involve flushing the pipeline
    flush_pipeline();
    // Setup for the correct return address fetch, if necessary
}

// Flush the entire pipeline, typically used in branch prediction hazards
void flush_pipeline()
{
    // This is a simplification, flushing should reset the pipeline but may need more logic
    pipe_control_stage(S_FETCH, true, false);
    pipe_control_stage(S_DECODE, true, false);
    pipe_control_stage(S_EXECUTE, true, false);
    pipe_control_stage(S_MEMORY, true, false);
    pipe_control_stage(S_WBACK, true, false);
}
