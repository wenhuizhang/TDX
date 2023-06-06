// Intel Proprietary 
// 
// Copyright 2021 Intel Corporation All Rights Reserved.
// 
// Your use of this software is governed by the TDX Source Code LIMITED USE LICENSE.
// 
// The Materials are provided “as is,” without any express or implied warranty of any kind including warranties
// of merchantability, non-infringement, title, or fitness for a particular purpose.

/**
 * @file tdh_mem_sept_rd_wr.c
 * @brief TDHMEMSEPTRD/TDHMEMSEPTWR API handler
 */
#include "tdx_vmm_api_handlers.h"
#include "tdx_basic_defs.h"
#include "auto_gen/tdx_error_codes_defs.h"
#include "x86_defs/x86_defs.h"
#include "data_structures/td_control_structures.h"
#include "memory_handlers/keyhole_manager.h"
#include "memory_handlers/pamt_manager.h"
#include "memory_handlers/sept_manager.h"
#include "helpers/helpers.h"
#include "accessors/ia32_accessors.h"
#include "accessors/data_accessors.h"


api_error_type tdh_mem_sept_rd(page_info_api_input_t gpa_page_info, uint64_t target_tdr_pa)
{
    // Local data for return values
    tdx_module_local_t  * local_data_ptr = get_local_data();
    // TDR related variables
    pa_t                  tdr_pa;                    // TDR physical address
    tdr_t               * tdr_ptr;                   // Pointer to the TDR page (linear address)
    pamt_block_t          tdr_pamt_block;            // TDR PAMT block
    pamt_entry_t        * tdr_pamt_entry_ptr;        // Pointer to the TDR PAMT entry
    bool_t                tdr_locked_flag = false;   // Indicate TDR is locked

    tdcs_t              * tdcs_ptr = NULL;           // Pointer to the TDCS structure (Multi-page)

    // GPA and SEPT related variables
    pa_t                  page_gpa = {.raw = 0};        // Target page GPA
    page_info_api_input_t gpa_mappings = gpa_page_info; // GPA and level
    ia32e_sept_t        * sept_entry_ptr = NULL;        // SEPT entry of the page
    ia32e_sept_t          sept_entry_copy;              // Cached SEPT entry of the page
    ept_level_t           sept_level_entry = gpa_mappings.level; // SEPT entry level of the page
    bool_t                sept_locked_flag = false;     // Indicate SEPT is locked

    api_error_type        return_val = UNINITIALIZE_ERROR;


    tdr_pa.raw = target_tdr_pa;

    // By default, no extended error code is returned
    local_data_ptr->vmm_regs.rcx = 0ULL;
    local_data_ptr->vmm_regs.rdx = 0ULL;

    // Check, lock and map the owner TDR page
    return_val = check_lock_and_map_explicit_tdr(tdr_pa,
                                                 OPERAND_ID_RDX,
                                                 TDX_RANGE_RO,
                                                 TDX_LOCK_SHARED,
                                                 PT_TDR,
                                                 &tdr_pamt_block,
                                                 &tdr_pamt_entry_ptr,
                                                 &tdr_locked_flag,
                                                 &tdr_ptr);
    if (return_val != TDX_SUCCESS)
    {
        TDX_ERROR("Failed to check/lock/map a TDR - error = %llx\n", return_val);
        goto EXIT;
    }

    // Check the TD state
    if ((return_val = check_td_in_correct_build_state(tdr_ptr)) != TDX_SUCCESS)
    {
        TDX_ERROR("TD is not in build state - error = %llx\n", return_val);
        goto EXIT;
    }

    // Map the TDCS structure and check the state.  No need to lock
    tdcs_ptr = map_implicit_tdcs(tdr_ptr, TDX_RANGE_RW);

    // Verify that GPA mapping input reserved fields equal zero
    if (!is_reserved_zero_in_mappings(gpa_mappings))
    {
        TDX_ERROR("Reserved fields in GPA mappings are not zero\n");
        return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RCX);
        goto EXIT;
    }
    page_gpa.page_4k_num = gpa_mappings.gpa;

    // Verify mapping level input is valid
    if (gpa_mappings.level > tdcs_ptr->executions_ctl_fields.eptp.fields.ept_pwl)
    {
        TDX_ERROR("Input GPA level (=%d) is not valid\n", gpa_mappings.level);
        return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RCX);
        goto EXIT;
    }

    // Check the page GPA is aligned to its level
    if (!is_gpa_aligned(gpa_mappings))
    {
        TDX_ERROR("Page GPA is not level (=%llx) aligned\n", gpa_mappings.level);
        return_val = api_error_with_operand_id(TDX_OPERAND_INVALID, OPERAND_ID_RCX);
        goto EXIT;
    }

    // Check GPA, lock SEPT and walk to find entry
    return_val = lock_sept_check_and_walk_private_gpa(tdcs_ptr,
                                                      OPERAND_ID_RCX,
                                                      page_gpa,
                                                      tdr_ptr->key_management_fields.hkid,
                                                      TDX_LOCK_SHARED,
                                                      &sept_entry_ptr,
                                                      &sept_level_entry,
                                                      &sept_entry_copy,
                                                      &sept_locked_flag);

    if (return_val == TDX_SUCCESS || return_val == api_error_with_operand_id(TDX_EPT_WALK_FAILED, OPERAND_ID_RCX))
    {
        set_arch_septe_details_in_vmm_regs(sept_entry_copy, sept_level_entry, local_data_ptr);
    }

    if (return_val != TDX_SUCCESS)
    {
        TDX_ERROR("Failed on GPA check, SEPT lock or walk - error = %llx\n", return_val);
        goto EXIT;
    }

EXIT:

    if (tdr_locked_flag)
    {
        pamt_unwalk(tdr_pa, tdr_pamt_block, tdr_pamt_entry_ptr, TDX_LOCK_SHARED, PT_4KB);
        free_la(tdr_ptr);
    }
    if (sept_locked_flag)
    {
        release_sharex_lock_sh(&tdcs_ptr->executions_ctl_fields.secure_ept_lock);
        if (sept_entry_ptr != NULL)
        {
            free_la(sept_entry_ptr);
        }
    }
    if (tdcs_ptr != NULL)
    {
        free_la(tdcs_ptr);
    }

    return return_val;
}
