#include "aethervisor.h"

namespace AetherVisor
{
	namespace Sandbox
	{
        void DenySandboxMemAccess(void* page_addr, bool allow_reads)
        {
            svm_vmmcall(VMMCALL_ID::deny_sandbox_reads, PAGE_ALIGN(page_addr));
        }

        void DenySandboxMemAccessRange(void* base, size_t range, bool allow_reads)
        {
            auto aligned_range = (uintptr_t)PAGE_ALIGN(range + 0x1000);

            for (auto offset = (uint8_t*)base; offset < (uint8_t*)base + aligned_range; offset += PAGE_SIZE)
            {
                svm_vmmcall(VMMCALL_ID::deny_sandbox_reads, base, offset);
            }
        }


        int SandboxPage(uintptr_t address, uintptr_t tag)
        {
            Util::TriggerCOW((uint8_t*)address);

            svm_vmmcall(VMMCALL_ID::sandbox_page, address, tag);

            return 0;
        }

        void SandboxRegion(uintptr_t base, uintptr_t size)
        {
            for (auto offset = base; offset < base + size; offset += PAGE_SIZE)
            {
                AetherVisor::SandboxPage((uintptr_t)offset, NULL);
            }
        }
	}
}
