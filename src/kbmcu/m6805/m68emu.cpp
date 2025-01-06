#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "m68emu.h"
#include "m68_internal.h"

void m68_init(M68_CTX *ctx, const M68_CPUTYPE cpuType)
{
	switch (cpuType) {
		case M68_CPU_HC05C4:
			// 68HC05SC21 is based on the 68HC05C4 core.
			// SP is 13 bits.
			//   7 MSBs are permanently set to 0000011
			//   6 LSBs are passed through
			// Address range: 0xC0 to 0xFF
			ctx->sp_and = 0x003F;
			ctx->sp_or  = 0x00C0;

			// PC is 13 bits too
			ctx->pc_and = 0x1FFF;
			break;
		case M68_CPU_HD6805V1:
			ctx->pc_and=0x0FFF;
			// Address range: 0xC0 to 0xFF ?
			ctx->sp_and = 0x003F;
			ctx->sp_or  = 0x00C0;
			break;
		default:
			assert(0);
	}

	ctx->cpuType = cpuType;
	ctx->trace = false;
	ctx->pending_interrupts = 0;
	m68_reset(ctx);
}

void m68_reset(M68_CTX *ctx)
{
	// Read the reset vector
	ctx->reg_pc = _M68_RESET_VECTOR & ctx->pc_and;
	uint16_t rstvec = (uint16_t)ctx->read_mem(ctx, ctx->reg_pc) << 8;
	rstvec |= ctx->read_mem(ctx, ctx->reg_pc+1);

	// Set PC to the reset vector
	ctx->reg_pc = rstvec & ctx->pc_and;
	ctx->pc_next = ctx->reg_pc;

	// Reset stack pointer to 0xFF
	ctx->reg_sp = 0xFF;

	// Set the I bit in the CCR to 1 (mask off interrupts)
	ctx->reg_ccr |= M68_CCR_I;

	// Clear STOP and WAIT latches
	ctx->is_stopped = ctx->is_waiting = 0;

	// Clear external interrupt latch
	ctx->irq = 0;
}


void m68_set_interrupt_line(M68_CTX * ctx,M68_INTERRUPT i){
	if (i == M68_INT_IRQ){
		ctx->irq=true;
	}
	ctx->pending_interrupts |= (1 << i);
}

void jump_to_vector(M68_CTX *ctx,uint16_t addr)
{
	push_byte(ctx, ctx->pc_next & 0xFF);
	push_byte(ctx, ctx->pc_next >> 8);
	push_byte(ctx, ctx->reg_x);
	push_byte(ctx, ctx->reg_acc);
	push_byte(ctx, ctx->reg_ccr);

	// Mask further interrupts
	force_flags(ctx, M68_CCR_I, 1);

	// Vector fetch
	uint16_t vector;
	vector = (uint16_t)ctx->read_mem(ctx, addr & ctx->pc_and) << 8;
	vector |= ctx->read_mem(ctx, (addr+1) & ctx->pc_and);
	ctx->pc_next = vector;
}

uint64_t m68_exec_cycle(M68_CTX *ctx)
{
	uint8_t opval;
	M68_OPTABLE_ENT *opcode;

	// Save current program counter
	ctx->reg_pc = ctx->pc_next;

	if (ctx->pending_interrupts && get_flag(ctx,M68_CCR_I)==0){
		if (ctx->pending_interrupts & (1<<M68_INT_IRQ)) {
			ctx->pending_interrupts &= ~(1<<M68_INT_IRQ);
			jump_to_vector(ctx,_M68_INT_VECTOR);
		} else if (ctx->pending_interrupts& (1<<M68_INT_TIMER1)) {
			ctx->pending_interrupts &= ~(1<<M68_INT_TIMER1);
			jump_to_vector(ctx,_M68_TMR1_VECTOR);
		}
		return 11;
	}

	// Fetch and decode opcode
	opval = ctx->read_mem(ctx, ctx->pc_next++);
	if (ctx->opdecode != NULL) {
		opval = ctx->opdecode(ctx, opval);
	}
	switch (ctx->cpuType) {
		case M68_CPU_HC05C4:
		case M68_CPU_HD6805V1:
			opcode = &m68hc05_optable[opval];
			break;
		default:
			assert(0);
	}

	if (ctx->trace) {
		printf("M68 EXEC: pc %04X sp %02X opval %02X mnem '%s' amode %d cycles %d\n",
				ctx->reg_pc, ctx->reg_sp, opval, opcode->mnem, opcode->amode, opcode->cycles);
	}

	// Read the opcode parameter bytes, if any
	uint8_t opParam;		// parameter
	uint16_t dirPtr;		// direct pointer
	uint16_t opNextPC;		// next PC (if branch or jump)
	bool opResult;

	switch(opcode->amode) {
		case AMODE_DIRECT:
			// Direct addressing: parameter is an address in zero page
			dirPtr = ctx->read_mem(ctx, ctx->pc_next++);
			if (!opcode->write_only) {
				opParam = ctx->read_mem(ctx, dirPtr);
			}
			break;

		case AMODE_DIRECT_JUMP:
			// Direct addressing, jump
			opNextPC = ctx->read_mem(ctx, ctx->pc_next++);
			opParam = -1;
			break;

		case AMODE_DIRECT_REL:
			// Direct + relative addressing: parameter is an address in zero page
			//   followed by a relative jump address.
			// Direct
			dirPtr = ctx->read_mem(ctx, ctx->pc_next++);
			opParam = ctx->read_mem(ctx, dirPtr);
			// Relative
			opNextPC = ctx->pc_next + 1;
			opNextPC += (int8_t)ctx->read_mem(ctx, ctx->pc_next++);
			break;

		case AMODE_EXTENDED:
			// Extended addressing: parameter is a 16-bit address
			dirPtr = (uint16_t)ctx->read_mem(ctx, ctx->pc_next++) << 8;
			dirPtr |= ctx->read_mem(ctx, ctx->pc_next++);
			if (!opcode->write_only) {
				opParam = ctx->read_mem(ctx, dirPtr);
			}
			break;

		case AMODE_EXTENDED_JUMP:
			// Extended addressing, jump
			opNextPC = (uint16_t)ctx->read_mem(ctx, ctx->pc_next++) << 8;
			opNextPC |= ctx->read_mem(ctx, ctx->pc_next++);
			opParam = -1;
			break;

		case AMODE_IMMEDIATE:
			// Immediate addressing: parameter is an immediate value following the opcode
			opParam = ctx->read_mem(ctx, ctx->pc_next++);
			break;

		case AMODE_INDEXED0:
			// Indexed with no offset. Take the X register as an address.
			dirPtr = ctx->reg_x;
			if (!opcode->write_only) {
				opParam = ctx->read_mem(ctx, dirPtr);
			}
			break;

		case AMODE_INDEXED0_JUMP:
			// Indexed jump with no offset. Take the X register as an address.
			opNextPC = ctx->reg_x;
			opParam = -1;
			break;

		case AMODE_INDEXED1:
			// Indexed with 1-byte offset. Add X and offset.
			dirPtr = (uint16_t)ctx->read_mem(ctx, ctx->pc_next++) + ctx->reg_x;
			if (!opcode->write_only) {
				opParam = ctx->read_mem(ctx, dirPtr);
			}
			break;

		case AMODE_INDEXED1_JUMP:
			// Indexed jump with 1-byte offset. Take the X register as an address.
			opNextPC = (uint16_t)ctx->read_mem(ctx, ctx->pc_next++) + ctx->reg_x;
			opParam = -1;
			break;

		case AMODE_INDEXED2:
			// Indexed with 2-byte offset. Add X and offset.
			dirPtr = (uint16_t)ctx->read_mem(ctx, ctx->pc_next++) << 8;
			dirPtr |= ctx->read_mem(ctx, ctx->pc_next++);
			dirPtr += ctx->reg_x;
			if (!opcode->write_only) {
				opParam = ctx->read_mem(ctx, dirPtr);
			}
			break;

		case AMODE_INDEXED2_JUMP:
			// Indexed jump with 2-byte offset. Add X and offset.
			opNextPC = (uint16_t)ctx->read_mem(ctx, ctx->pc_next++) << 8;
			opNextPC |= ctx->read_mem(ctx, ctx->pc_next++);
			opNextPC += ctx->reg_x;
			opParam = -1;
			break;

		case AMODE_INHERENT:
			// Inherent addressing, affects nothing.
			opParam = -1;
			break;

		case AMODE_INHERENT_A:
			// Inherent addressing, affects Accumulator.
			opParam = ctx->reg_acc;
			break;

		case AMODE_INHERENT_X:
			// Inherent addressing, affects X register.
			opParam = ctx->reg_x;
			break;

		case AMODE_RELATIVE:
			// Relative addressing: signed relative branch or jump.
			opNextPC = ctx->pc_next + 1;
			opNextPC += (int8_t)ctx->read_mem(ctx, ctx->pc_next++);
			break;

		case AMODE_ILLEGAL:
		case AMODE_MAX:
			printf("ILLEGAL M68 EXEC: pc %04X sp %02X opval %02X mnem '%s' amode %d cycles %d\n",
					ctx->reg_pc, ctx->reg_sp, opval, opcode->mnem, opcode->amode, opcode->cycles);

			// Illegal instruction
			assert(1==2);
			break;
	}

	// Execute opcode
	opResult = opcode->opfunc(ctx, opval, &opParam);
	if (ctx->trace) {
		if (opResult) {
			printf("\t-> %3d (0x%02X)\n", opParam, opParam);
		}
	}

	// Write back result (param)
	switch(opcode->amode) {
		case AMODE_DIRECT:
		case AMODE_EXTENDED:
		case AMODE_INDEXED0:
		case AMODE_INDEXED1:
		case AMODE_INDEXED2:
			// Direct addressing: parameter is an address in zero page
			// Extended addressing: parameter is a 16-bit address
			// Indexed with no offset. Take the X register as an address.
			// Indexed with 1-byte offset. Add X and offset.
			// Indexed with 2-byte offset. Add X and offset.
			if (opResult) {
				ctx->write_mem(ctx, dirPtr, opParam);
			}
			break;

		case AMODE_DIRECT_JUMP:
		case AMODE_EXTENDED_JUMP:
		case AMODE_INDEXED0_JUMP:
		case AMODE_INDEXED1_JUMP:
		case AMODE_INDEXED2_JUMP:
		case AMODE_DIRECT_REL:
		case AMODE_RELATIVE:
			// Direct + relative addressing: parameter is an address in zero page
			//   followed by a signed relative jump address.
			// Relative addressing: signed relative branch or jump.
			//
			// If the opfunc returned true, take the jump.
			if (opResult) {
				ctx->pc_next = opNextPC & ctx->pc_and;
			}
			break;

		case AMODE_IMMEDIATE:
			// Immediate addressing: parameter is an immediate value and cannot be written back.
			break;

		case AMODE_INHERENT:
			// Inherent addressing, affects nothing.
			break;

		case AMODE_INHERENT_A:
			// Inherent addressing, affects Accumulator.
			if (opResult) {
				ctx->reg_acc = opParam;
			}
			break;

		case AMODE_INHERENT_X:
			// Inherent addressing, affects X register.
			if (opResult) {
				ctx->reg_x = opParam;
			}
			break;

		case AMODE_ILLEGAL:
		case AMODE_MAX:
			// Illegal instruction
			assert(1==2);
			break;
	}

	// Return number of cycles executed
	return opcode->cycles;
}

