/**************************************************************************
 * C S 429 architecture emulator
 *
 * instr_Writeback.c - Writeback stage of instruction processing pipeline.
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

extern int64_t W_wval;

/*
 * Write-back stage logic.
 * This function handles the write-back stage of the pipeline where the results of previous stages
 * are written back into the processor's register file or other state as necessary.
 *
 * STUDENT TO-DO:
 * The student is tasked to implement the logic for the writeback stage, ensuring that the appropriate
 * value (either from memory or the execute stage) is selected and stored in the global write-back value.
 *
 * Parameters:
 *  in - Pointer to the structure holding the state of the pipeline register after the memory stage.
 *
 * Global Variables:
 *  W_wval - The value to be written back. This should be updated based on the instruction's requirements.
 */

comb_logic_t wback_instr(w_instr_impl_t *in)
{
    // Conditionally sets the global write-back value based on the write-back control signal.
    // If the write-back value is selected to be from memory, use val_mem; otherwise, use val_ex.
    if (in->W_sigs.wval_sel)
    {
        W_wval = in->val_mem; // Selects the memory value for write-back.
    }
    else
    {
        W_wval = in->val_ex; // Selects the execution result for write-back.
    }
    // The function concludes without returning a value; should return a comb_logic_t value.
    return;
}
