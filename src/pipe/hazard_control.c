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
   pipe_control_stage(S_FETCH, false, false);
    pipe_control_stage(S_DECODE, false, false);
    pipe_control_stage(S_EXECUTE, false, false);
    pipe_control_stage(S_MEMORY, false, false);
    pipe_control_stage(S_WBACK, false, false);



    if (F_out->status != STAT_AOK && F_out->status != STAT_BUB)
    {
        pipe_control_stage(S_FETCH, false, true);
    }else if (D_out->status != STAT_AOK && D_out->status != STAT_BUB){
        X_in->W_sigs.w_enable = false;
        pipe_control_stage(S_FETCH, false, true);
        pipe_control_stage(S_DECODE, false, true);
    }else if (X_out->status != STAT_AOK && X_out->status != STAT_BUB)
    { // x out: f, d, x
        pipe_control_stage(S_FETCH, false, true);
        pipe_control_stage(S_DECODE, false, true);
        pipe_control_stage(S_EXECUTE, false, true);
    }else if (M_out->status != STAT_AOK && M_out->status != STAT_BUB)
    { // m out: f, d, x, m; same as W->in
        X_in->X_sigs.set_CC = false;
        pipe_control_stage(S_FETCH, false, true);
        pipe_control_stage(S_DECODE, false, true);
        pipe_control_stage(S_EXECUTE, false, true);
        pipe_control_stage(S_MEMORY, false, true);
    }else if (W_out->status != STAT_AOK && W_out->status != STAT_BUB)
    { // w out: f, d, x, w
        pipe_control_stage(S_FETCH, false, true);
        pipe_control_stage(S_DECODE, false, true);
        pipe_control_stage(S_EXECUTE, false, true);
        pipe_control_stage(S_MEMORY, false, true);
        pipe_control_stage(S_WBACK, false, true);
    }

    if (dmem_status == IN_FLIGHT)
    {
        pipe_control_stage(S_FETCH, false, true);
        pipe_control_stage(S_DECODE, false, true);
        pipe_control_stage(S_EXECUTE, false, true);
        pipe_control_stage(S_MEMORY, false, true);
        pipe_control_stage(S_WBACK, false, false);
    }

    if (check_mispred_branch_hazard(X_opcode, X_condval) 
            && check_ret_hazard(D_opcode)){
        pipe_control_stage(S_FETCH, true, false);
        pipe_control_stage(S_DECODE, true, false);
    }else if(check_ret_hazard(D_opcode) 
        && check_load_use_hazard(D_opcode, D_src1, D_src2, X_opcode, X_dst)){
        pipe_control_stage(S_FETCH, false, true);
        pipe_control_stage(S_DECODE, true, true);
    }
    else if (check_load_use_hazard(D_opcode, D_src1, D_src2, X_opcode, X_dst))
    {
        pipe_control_stage(S_FETCH, true, true);
        pipe_control_stage(S_DECODE, true, true);
        pipe_control_stage(S_EXECUTE, true, false);
        pipe_control_stage(S_MEMORY, false, false);
        pipe_control_stage(S_WBACK, false, false);
    }
    else if (check_mispred_branch_hazard(X_opcode, X_condval))
    {
        pipe_control_stage(S_FETCH, true, false);
        pipe_control_stage(S_DECODE, true, false);
        pipe_control_stage(S_EXECUTE, true, false);
        pipe_control_stage(S_MEMORY, false, false);
        pipe_control_stage(S_WBACK, false, false);
    }
    
}
