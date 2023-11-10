/**************************************************************************
 * C S 429 system emulator
 *
 * instr_Memory.c - Memory stage of instruction processing pipeline.
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "err_handler.h"    // Header for handling errors and exceptions
#include "instr.h"          // Header defining instruction structures and constants
#include "instr_pipeline.h" // Header defining the structures for pipeline stages
#include "machine.h"        // Header defining the simulated machine's structure
#include "hw_elts.h"        // Header defining the hardware elements and their operations

// External declarations for the guest machine state and data memory status
extern machine_t guest;          // The state of the emulated machine
extern mem_status_t dmem_status; // Status of the data memory after operations

// Declaration of external function to copy control signals for the write-back stage
extern comb_logic_t copy_w_ctl_sigs(w_ctl_sigs_t *, w_ctl_sigs_t *);

/*
 * Memory stage logic.
 * This function handles the memory stage of the pipeline where memory operations
 * specified by the instruction are carried out. Data memory reads and writes are
 * performed here based on the control signals set during the decode stage.
 *
 * Parameters:
 *  in - Pointer to the structure holding the state of the pipeline register after the execute stage.
 *  out - Pointer to the structure that will be updated with the state for the write-back stage.
 */
comb_logic_t memory_instr(m_instr_impl_t *in, w_instr_impl_t *out)
{
    // Copies the Write-Back control signals from the 'in' structure to the 'out' structure.
    copy_w_ctl_sigs(&(out->W_sigs), &(in->W_sigs));
    bool x = 0; // Temporary variable for status update, likely related to data memory (dmem) status.

    // If the instruction requires a data memory read or write, perform that operation.
    if (in->M_sigs.dmem_read || in->M_sigs.dmem_write)
    {
        // Access the data memory with the calculated address, value, and control signals.
        dmem(in->val_ex, in->val_b, in->M_sigs.dmem_read, in->M_sigs.dmem_write, &(out->val_mem), &x);
    }
    // Passing through the destination register, calculated value, and other information to the write-back stage.
    out->dst = in->dst;           // Destination register for write-back
    out->val_ex = in->val_ex;     // Result from the execute stage to pass to write-back
    out->val_b = in->val_b;       // Value of the second operand (used for write-back in some instructions)
    out->status = in->status;     // Status code passed through the pipeline stages
    out->print_op = in->print_op; // Operation to print for debugging purposes

    // If the data memory operation signaled an exceptional condition, update status.
    if (x)
    {
        out->status = STAT_ADR; // Set the status to address exception if memory access failed
    }
    return;
}