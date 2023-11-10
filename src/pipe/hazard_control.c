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
        // Initial assumption that no stage needs stalling or flushing.
        bool stall_fetch = false, stall_decode = false, stall_execute = false,
             stall_memory = false, stall_writeback = false;

        // Start by checking the decode stage first.
        if (D_out->status != STAT_AOK && D_out->status != STAT_BUB)
        {
            // Stall or flush pipeline stages as appropriate.
            stall_fetch = stall_decode = true;
            // Other logic for handling decode errors...
        }

        // Next, check the execute stage.
        if (X_out->status != STAT_AOK && X_out->status != STAT_BUB)
        {
            // Stall or flush pipeline stages as appropriate.
            stall_fetch = stall_decode = stall_execute = true;
            // Other logic for handling execute errors...
        }

        // Then, check the memory stage.
        if (M_out->status != STAT_AOK && M_out->status != STAT_BUB)
        {
            // Stall or flush pipeline stages as appropriate.
            stall_fetch = stall_decode = stall_execute = stall_memory = true;
            // Other logic for handling memory errors...
        }

        // Lastly, check the writeback stage.
        if (W_out->status != STAT_AOK && W_out->status != STAT_BUB)
        {
            // Stall or flush pipeline stages as appropriate.
            stall_fetch = stall_decode = stall_execute = stall_memory = stall_writeback = true;
            // Other logic for handling writeback errors...
        }

        // Apply the stalling to the stages as needed.
        pipe_control_stage(S_FETCH, false, stall_fetch);
        pipe_control_stage(S_DECODE, false, stall_decode);
        pipe_control_stage(S_EXECUTE, false, stall_execute);
        pipe_control_stage(S_MEMORY, false, stall_memory);
        pipe_control_stage(S_WBACK, false, stall_writeback);

        // Check for load-use hazard, branch misprediction, and other hazards...
        // This logic should also reflect the prioritization of status checks.
        // ...

        return;
    }
}
