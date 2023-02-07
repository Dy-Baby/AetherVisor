#include "mem_prot_key.h"
#include "logging.h"
#include "vmexit.h"
#include "memory_reader.h"

extern "C" int __rdpkru();
extern "C" int __wrpkru(int pkru);

namespace MpkHooks
{
    int hook_count;
    MpkHook first_mpk_hook;

	MpkHook* ForEachHook(bool(HookCallback)(MpkHook* hook_entry, void* data), void* callback_data)
	{
		auto hook_entry = &first_mpk_hook;

		for (int i = 0; i < hook_count; hook_entry = hook_entry->next_hook, ++i)
		{
			if (HookCallback(hook_entry, callback_data))
			{
				return hook_entry;
			}
		}
		return 0;
	}

	void Init()
    {
		CR3 cr3;
		cr3.Flags = __readcr3();

		first_mpk_hook.hookless_page = ExAllocatePool(NonPagedPool, PAGE_SIZE);
		first_mpk_hook.next_hook = (MpkHook*)ExAllocatePool(NonPagedPool, sizeof(MpkHook));

		auto hook_entry = &first_mpk_hook;

		/*	reserve memory for hooks because we shouldn't allocate memory in VM root	*/

		int max_hooks = 20;

		for (int i = 0; i < max_hooks; hook_entry = hook_entry->next_hook, ++i)
		{
			hook_entry->next_hook = (MpkHook*)ExAllocatePool(NonPagedPool, sizeof(MpkHook));
			hook_entry->next_hook->hookless_page = ExAllocatePool(NonPagedPool, PAGE_SIZE);
		}

		hook_count = 0;
    }

	MpkHook* SetMpkHook(CoreVmcbData* vcpu, void* address, uint8_t* patch, size_t patch_len)
    {        
		auto hook_entry = &first_mpk_hook;

		for (int i = 0; i < hook_count; hook_entry = hook_entry->next_hook, ++i)
		{
		}

        CR3 cr3;
        cr3.Flags = vcpu->guest_vmcb.save_state_area.Cr3;

		hook_entry->the_pte = MemoryUtils::GetPte(address, cr3.AddressOfPageDirectory << PAGE_SHIFT);
		hook_entry->the_pte->ProtectionKey = hook_count;

        hook_entry->hooked_page = PAGE_ALIGN(address);

        auto pkru = __rdpkru();

        /* set Pkru ADi bit at hook_count index, to block reads  */
        auto flag = 1 << (hook_count * 2);     
        pkru |= flag;     

        __wrpkru(pkru);

		auto irql = Utils::DisableWP();

        hook_count += 1;

        return hook_entry;
    }

    void HandlePageFaultMpk(CoreVmcbData* vcpu, GPRegs* guest_regs)
	{
		auto fault_address = (void*)vcpu->guest_vmcb.control_area.ExitInfo2;

		PageFaultErrorCode error_code;
		error_code.as_uint32 = vcpu->guest_vmcb.control_area.ExitInfo1;

		auto hook_entry = MpkHooks::ForEachHook(
			[](MpkHook* hook_entry, void* data) -> bool {

				if (PAGE_ALIGN((uintptr_t)data) == hook_entry->hooked_page)
				{
					return true;
				}

			}, (void*)fault_address
		);

		if (!hook_entry)
		{
			Logger::Log("[AMD-Hypervisor] - This is a normal page fault at %p \n", fault_address);

			InjectException(vcpu, 14, error_code.as_uint32);
			return;
		}

		Logger::Log("[AMD-Hypervisor] -This page fault is from our hooked page at %p \n", fault_address);
		
		if (error_code.fields.protection_key)
		{
			/*	set PFN to copy page	*/

			auto fault_pte = MemoryUtils::GetPte(fault_address, vcpu->guest_vmcb.save_state_area.Cr3);

			fault_pte->PageFrameNumber = MemoryUtils::GetPte(
				hook_entry->hookless_page, 
				vcpu->guest_vmcb.save_state_area.Cr3
			)->PageFrameNumber;

			fault_pte->ExecuteDisable = 1;
		}
		if (error_code.fields.execute)
		{
			/*	restore PFN to hooked page	*/

			auto fault_pte = MemoryUtils::GetPte(fault_address, vcpu->guest_vmcb.save_state_area.Cr3);

			fault_pte->PageFrameNumber = MemoryUtils::GetPte(
				hook_entry->hooked_page, 
				vcpu->guest_vmcb.save_state_area.Cr3
			)->PageFrameNumber;

			fault_pte->ExecuteDisable = 0;
		}
	}
}