#pragma once
#include "disassembly.h"

namespace Disasm
{
	ZydisFormatter formatter;

	void MyRegContextToZydisRegContext(VcpuData* vcpu, GuestRegisters* guest_regs, ZydisRegisterContext* context)
	{
		context->values[ZYDIS_REGISTER_RAX] = vcpu->guest_vmcb.save_state_area.rax;
		context->values[ZYDIS_REGISTER_RSP] = vcpu->guest_vmcb.save_state_area.rsp;
		context->values[ZYDIS_REGISTER_RIP] = vcpu->guest_vmcb.save_state_area.rip;
		context->values[ZYDIS_REGISTER_RFLAGS] = vcpu->guest_vmcb.save_state_area.rflags.Flags;
		context->values[ZYDIS_REGISTER_RCX] = guest_regs->rcx;
		context->values[ZYDIS_REGISTER_RDX] = guest_regs->rdx;
		context->values[ZYDIS_REGISTER_RBX] = guest_regs->rbx;
		context->values[ZYDIS_REGISTER_RBP] = guest_regs->rbp;
		context->values[ZYDIS_REGISTER_RSI] = guest_regs->rsi;
		context->values[ZYDIS_REGISTER_RDI] = guest_regs->rdi;
		context->values[ZYDIS_REGISTER_R8] = guest_regs->r8;
		context->values[ZYDIS_REGISTER_R9] = guest_regs->r9;
		context->values[ZYDIS_REGISTER_R10] = guest_regs->r10;
		context->values[ZYDIS_REGISTER_R11] = guest_regs->r11;
		context->values[ZYDIS_REGISTER_R12] = guest_regs->r12;
		context->values[ZYDIS_REGISTER_R13] = guest_regs->r13;
		context->values[ZYDIS_REGISTER_R14] = guest_regs->r14;
		context->values[ZYDIS_REGISTER_R15] = guest_regs->r15;
	}

	ZydisDecodedInstruction Disassemble(uint8_t* instruction, ZydisDecodedOperand* operands)
	{
		ZydisDecodedInstruction zydis_insn;

		ZydisDecoderDecodeFull(
			&zydis_decoder,
			instruction, 16,
			&zydis_insn,
			operands,
			ZYDIS_MAX_OPERAND_COUNT_VISIBLE,
			ZYDIS_DFLAG_VISIBLE_OPERANDS_ONLY
		);

		return zydis_insn;
	}

	/*	Gets total instructions length closest to byte_length	*/
	int	LengthOfInstructions(void* address, int byte_length)
	{
		ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT_VISIBLE];

		int insns_len = 0;
		for (insns_len = 0; insns_len < byte_length;)
		{
			int cur_insn_len = Disassemble((uint8_t*)address + insns_len, operands).length;
			insns_len += cur_insn_len;
		}

		return insns_len;
	}

	void format(uintptr_t address, ZydisDecodedInstruction* instruction, char* out_buf, int size)
	{
		ZydisDecodedOperand operands[5];

		ZydisFormatterFormatInstruction(
			&formatter, instruction, operands, instruction->operand_count_visible, out_buf,
			size, address);
	}

	int Init()
	{
		ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);

		return ZydisDecoderInit(&zydis_decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
	}
};