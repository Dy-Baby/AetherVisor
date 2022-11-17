#include "pch.h"
#include "forte_api.h"

/*  parameter order: rcx, rdx, r8, r9, r12, r11  */

namespace ForteVisor
{
    int RemapPageSingleNcr3(uintptr_t old_page, uintptr_t copy_page, int32_t core_id)
    {
        svm_vmmcall(VMMCALL_ID::remap_page_ncr3_specific, old_page, copy_page, core_id);

        return 0;
    }

    int SetNptHook(uintptr_t address, uint8_t* patch, size_t patch_len, int32_t noexecute_cr3_id, int32_t tag)
    {
        svm_vmmcall(VMMCALL_ID::set_npt_hook, address, patch, patch_len, noexecute_cr3_id, tag);

        return 0;
    }

    int RemoveNptHook(int32_t tag)
    {
        svm_vmmcall(VMMCALL_ID::remove_npt_hook, tag);

        return 0;
    }

    int ForEachCore(void(*callback)(void* params), void* params)
    {
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        auto core_count = sys_info.dwNumberOfProcessors;

        for (auto idx = 0; idx < core_count; ++idx)
        {
            auto affinity = pow(2, idx);

            SetThreadAffinityMask(GetCurrentThread(), affinity);

            callback(params);
        }

        return 0;
    }

    int DisableHv()
    {
        ForEachCore(
            [](void* params) -> void {
                svm_vmmcall(VMMCALL_ID::disable_hv);
            }
        );
        return 0;
    }
};