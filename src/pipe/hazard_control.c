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
#include "forward.h"

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
                            opcode_t X_opcode, uint8_t X_dst, bool X_condval){

    uint64_t *val_a, *val_b;



    pipe_control_stage(S_FETCH,false, false);
    pipe_control_stage(S_DECODE, false, false);
    pipe_control_stage(S_EXECUTE, false, false);
    pipe_control_stage(S_MEMORY, false, false);
    pipe_control_stage(S_WBACK, false, false);

    if(check_mispred_branch_hazard(X_opcode, X_condval) && check_ret_hazard(D_opcode)){
        pipe_control_stage(S_FETCH, true, false);
        pipe_control_stage(S_DECODE,true, false);
    }else if (check_ret_hazard(D_opcode) 
        && check_load_use_hazard(D_opcode, D_src1, D_src2, X_opcode, X_dst)){
        pipe_control_stage(S_FETCH, false, true);
        pipe_control_stage(S_DECODE, true, false);
    }else if(check_load_use_hazard(D_opcode, D_src1, D_src2, X_opcode, X_dst)){
        pipe_control_stage(S_FETCH, false, true);
        pipe_control_stage(S_DECODE, true, false);
    }else if(check_mispred_branch_hazard(X_opcode, X_condval)){
        pipe_control_stage(S_FETCH, true, false);
        pipe_control_stage(S_DECODE, true, false);
    }

    if(dmem_status == IN_FLIGHT){
        pipe_control_stage(S_FETCH,false, true);
        pipe_control_stage(S_DECODE,false, true);
        pipe_control_stage(S_EXECUTE, false, true);
        pipe_control_stage(S_MEMORY, false, true);
        pipe_control_stage(S_WBACK, false, true);
    }


    return;
}
