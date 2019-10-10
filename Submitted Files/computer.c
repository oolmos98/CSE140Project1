#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "computer.h"
#undef mips /* gcc already has a def for mips */

unsigned int endianSwap(unsigned int);

void PrintInfo(int changedReg, int changedMem);
unsigned int Fetch(int);
void Decode(unsigned int, DecodedInstr *, RegVals *);
int Execute(DecodedInstr *, RegVals *);
int Mem(DecodedInstr *, int, int *);
void RegWrite(DecodedInstr *, int, int *);
void UpdatePC(DecodedInstr *, int);
void PrintInstruction(DecodedInstr *);

/*Globally accessible Computer variable*/
Computer mips;
RegVals rVals;

/*  
	opcodes for I-format
*/
const unsigned int andi = 0x0C,
									 addiu = 0x09,
									 beq = 0x04,
									 bne = 0x05,
									 bgtz = 0x07,
									 lui = 0x0F,
									 lw = 0x23,
									 ori = 0x0D,
									 sw = 0x2B;
/*  
	opcodes for J-format
*/
const unsigned int jal = 0x03,
									 jump = 0x02;

/*

	function codes for R-Format

*/

const unsigned int addu = 0x21,
									 and = 0x24,
									 jr = 0x08,
									 or = 0x25,
									 slt = 0x2A,
									 sll = 0x00, // uses shamt
		srl = 0x02,								 // uses shamt
		subu = 0x23;

/*

	Implementing Control

*/
/*
 *  Return an initialized computer with the stack pointer set to the
 *  address of the end of data memory, the remaining registers initialized
 *  to zero, and the instructions read from the given file.
 *  The other arguments govern how the program interacts with the user.
 */
void InitComputer(FILE *filein, int printingRegisters, int printingMemory,
									int debugging, int interactive)
{
	int k;
	unsigned int instr;

	/* Initialize registers and memory */

	for (k = 0; k < 32; k++)
	{
		mips.registers[k] = 0;
	}

	/* stack pointer - Initialize to highest address of data segment */
	mips.registers[29] = 0x00400000 + (MAXNUMINSTRS + MAXNUMDATA) * 4;

	for (k = 0; k < MAXNUMINSTRS + MAXNUMDATA; k++)
	{
		mips.memory[k] = 0;
	}

	k = 0;
	while (fread(&instr, 4, 1, filein))
	{
		/*swap to big endian, convert to host byte order. Ignore this.*/
		mips.memory[k] = ntohl(endianSwap(instr));
		k++;
		if (k > MAXNUMINSTRS)
		{
			fprintf(stderr, "Program too big.\n");
			exit(1);
		}
	}

	mips.printingRegisters = printingRegisters;
	mips.printingMemory = printingMemory;
	mips.interactive = interactive;
	mips.debugging = debugging;
}

unsigned int endianSwap(unsigned int i)
{
	return (i >> 24) | (i >> 8 & 0x0000ff00) | (i << 8 & 0x00ff0000) | (i << 24);
}

/*
 *  Run the simulation.
 */
void Simulate()
{
	char s[40]; /* used for handling interactive input */
	unsigned int instr;
	int changedReg = -1, changedMem = -1, val;
	DecodedInstr d;

	/* Initialize the PC to the start of the code section */
	mips.pc = 0x00400000;
	while (1)
	{
		if (mips.interactive)
		{
			printf("> ");
			fgets(s, sizeof(s), stdin);
			if (s[0] == 'q')
			{
				return;
			}
		}

		/* Fetch instr at mips.pc, returning it in instr */
		instr = Fetch(mips.pc);

		printf("Executing instruction at %8.8x: %8.8x\n", mips.pc, instr);

		/* 
	 * Decode instr, putting decoded instr in d
	 * Note that we reuse the d struct for each instruction.
	 */
		Decode(instr, &d, &rVals);

		/*Print decoded instruction*/
		PrintInstruction(&d);

		/* 
	 * Perform computation needed to execute d, returning computed value 
	 * in val 
	 */
		val = Execute(&d, &rVals);

		UpdatePC(&d, val);

		/* 
	 * Perform memory load or store. Place the
	 * address of any updated memory in *changedMem, 
	 * otherwise put -1 in *changedMem. 
	 * Return any memory value that is read, otherwise return -1.
	 */
		val = Mem(&d, val, &changedMem);

		/* 
	 * Write back to register. If the instruction modified a register--
	 * (including jal, which modifies $ra) --
		 * put the index of the modified register in *changedReg,
		 * otherwise put -1 in *changedReg.
		 */
		RegWrite(&d, val, &changedReg);

		PrintInfo(changedReg, changedMem);
	}
}

/*
 *  Print relevant information about the state of the computer.
 *  changedReg is the index of the register changed by the instruction
 *  being simulated, otherwise -1.
 *  changedMem is the address of the memory location changed by the
 *  simulated instruction, otherwise -1.
 *  Previously initialized flags indicate whether to print all the
 *  registers or just the one that changed, and whether to print
 *  all the nonzero memory or just the memory location that changed.
 */
void PrintInfo(int changedReg, int changedMem)
{
	int k, addr;
	printf("New pc = %8.8x\n", mips.pc);
	if (!mips.printingRegisters && changedReg == -1)
	{
		printf("No register was updated.\n");
	}
	else if (!mips.printingRegisters)
	{
		printf("Updated r%2.2d to %8.8x\n",
					 changedReg, mips.registers[changedReg]);
	}
	else
	{
		for (k = 0; k < 32; k++)
		{
			printf("r%2.2d: %8.8x  ", k, mips.registers[k]);
			if ((k + 1) % 4 == 0)
			{
				printf("\n");
			}
		}
	}
	if (!mips.printingMemory && changedMem == -1)
	{
		printf("No memory location was updated.\n");
	}
	else if (!mips.printingMemory)
	{
		printf("Updated memory at address %8.8x to %8.8x\n",
					 changedMem, Fetch(changedMem));
	}
	else
	{
		printf("Nonzero memory\n");
		printf("ADDR	  CONTENTS\n");
		for (addr = 0x00400000 + 4 * MAXNUMINSTRS;
				 addr < 0x00400000 + 4 * (MAXNUMINSTRS + MAXNUMDATA);
				 addr = addr + 4)
		{
			if (Fetch(addr) != 0)
			{
				printf("%8.8x  %8.8x\n", addr, Fetch(addr));
			}
		}
	}
}

/*
 *  Return the contents of memory at the given address. Simulates
 *  instruction fetch. 
 */
unsigned int Fetch(int addr)
{
	return mips.memory[(addr - 0x00400000) / 4];
}

/* Decode instr, returning decoded instruction. */
void Decode(unsigned int instr, DecodedInstr *d, RegVals *rVals)
{
	//printf("Calling Decode\n");
	char format;
	/*
		
		xxxxxxyyyyyyyyyyyyyyyyyyyyyyyyyy

		instr << 26

		00000000000000000000000000xxxxxx
	
	*/
	//printf("instr: %d\n", instr);
	unsigned int opcode = instr >> 26;
	//printf("size: %d\n", opcode);

	/*
		
		Reset the control everytime a new instruction is loaded.

	*/
	rVals->R_rs = 0;
	rVals->R_rt = 0;
	rVals->R_rd = 0;

	if (opcode == 0)
		format = 'R';
	else if (opcode == jump || opcode == jal)
		format = 'J';
	else
		format = 'I';

	switch (format)
	{
	case 'R': //R Format
		/*
				R-format
				opcode, rs, rt, rd, shamt, funct
			*/
		d->type = R;
		d->op = 0;
		d->regs.r.rs = rVals->R_rs = (instr << 6) >> 27;
		d->regs.r.rt = rVals->R_rt = (instr << 11) >> 27;
		d->regs.r.rd = rVals->R_rd = (instr << 16) >> 27;
		d->regs.r.shamt = (instr << 21) >> 27;
		d->regs.r.funct = (instr << 26) >> 26;
		//printf("%d\n", (instr << 26) >> 26);
		//PrintInstruction(d);
		break;

	case 'I':
		/*
				I-format
				opcode, rs, rt, immediate
			*/
		d->type = I;
		d->op = opcode;
		//printf("I: %d\n", opcode);
		d->regs.i.rs = rVals->R_rs = (instr << 6) >> 27;
		d->regs.i.rt = rVals->R_rt = (instr << 11) >> 27;
		d->regs.i.addr_or_immed = (instr << 16) >> 16;

		/*

			if int is negative 16-bit address is:
			1xxx xxxx xxxx xxxx

			1000 0000 0000 0000 1000 0000 0000 0000
			0000 0000 0000 0000 1xxx xxxx xxxx xxxx (do XOR)

		*/

		//printf("Rs: %d\nRt: %d\n\n\n", rVals->R_rs, rVals->R_rt);

		int temp = (instr << 16) >> 16;
		if (temp >> 15)
		{
			d->regs.i.addr_or_immed = temp | 0xFFFF0000;
		}
		else
		{
			d->regs.i.addr_or_immed = temp & 0x0000FFFF;
		}

		//printf("Immediate: %d\n", d->regs.i.addr_or_immed);
		// ALUsrc will always be in Immediate field
		break;
	case 'J':
		/*
				I-format
				opcode, jump_addr

				x - opcode
				y - address

				xxxxxxyyyyyyyyyyyyyyyyyyyyyyyyyy

				instr << 8;
				yyyyyyyyyyyyyyyyyyyyyyyyyy000000

				instr >> 4;

				0000yyyyyyyyyyyyyyyyyyyyyyyyyy00


				iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii

				| (copy the first for bits of the current mips PC )

				0000yyyyyyyyyyyyyyyyyyyyyyyyyy00


			*/
		d->type = J;
		d->op = opcode;
		int curr_pc = (mips.pc >> 28) << 28;

		int target_pc = ((instr << 6) >> 4) | curr_pc;

		d->regs.j.target = target_pc;

		//PrintInstruction(d);
		//UpdatePC(d, d->regs.j.target);
		break;
	}
}

/*
 *  Print the disassembled version of the given instruction
 *  followed by a newline.
 */
void PrintInstruction(DecodedInstr *d)
{
	/* Your code goes here */
	// Check if the instruction is R-foarmat
	char *instr = (char *)malloc(sizeof(char) * 5);
	int supported_instr = 1;

	//printf("%d\n", d->op);
	//printf("R: %d\n", d->regs.r.funct);

	if (d->type == R)
	{
		//printf("R: %d\n", d->regs.r.funct);
		switch (d->regs.r.funct)
		{
		case addu:
			strcpy(instr, "addu");
			break;

		case and:
			strcpy(instr, "and");
			break;

		case jr:
			strcpy(instr, "jr");
			break;

		case or:
			strcpy(instr, "or");
			break;

		case slt:
			strcpy(instr, "slt");
			break;

		case sll:
			strcpy(instr, "sll");
			break;

		case srl:
			strcpy(instr, "srl");
			break;

		case subu:
			strcpy(instr, "subu");
			break;
		default: // gets triggered if theres an unsupported code
			supported_instr = 0;
			break;
		}
	}
	else if (d->type == I)
	{
		switch (d->op)
		{
			//printf("%d\n", d->op);

		case addiu:
			strcpy(instr, "addiu");
			break;

		case beq:
			strcpy(instr, "beq");
			break;

		case bne:
			strcpy(instr, "bne");
			break;

		case bgtz:
			strcpy(instr, "bgtz");
			break;

		case lui:
			strcpy(instr, "lui");
			break;

		case lw:
			strcpy(instr, "lw");
			break;

		case ori:
			strcpy(instr, "ori");
			break;

		case sw:
			strcpy(instr, "sw");
			break;

		case andi:
			strcpy(instr, "andi");
			break;

		default: // gets triggered if theres an unsupported code
			supported_instr = 0;
			break;
		}
	}
	else
	{
		switch (d->op)
		{
		case jal:
			strcpy(instr, "jal");
			break;

		case jump:
			strcpy(instr, "j");
			break;

		default: // gets triggered if theres an unsupported code
			supported_instr = 0;
			break;
		}
	}

	if (supported_instr == 0)
	{
		printf("Unsupported instruction found. Terminating program\n");
		exit(0);
	}

	if (d->type == R)
	{
		if (d->regs.r.funct == sll || d->regs.r.funct == srl)
		{
			printf("%s\t$%d, $%d, %d\n", instr, d->regs.r.rd, d->regs.r.rs, d->regs.r.shamt);
		}
		else if (d->regs.r.funct == jr)
		{
			printf("%s\t$%d\n", instr, 31);
		}
		else
		{
			printf("%s\t$%d, $%d, $%d\n", instr, d->regs.r.rd, d->regs.r.rs, d->regs.r.rt);
		}
	}
	else if (d->type == I)
	{
		if (d->op == bne || d->op == beq)
		{
			printf("%s\t$%d, $%d, 0x%8.8x\n", instr, d->regs.i.rs, d->regs.i.rt, mips.pc + ((4 * d->regs.i.addr_or_immed) + 4));
		}
		else if (d->op == lw || d->op == sw)
		{
			printf("%s\t$%d, %d($%d)\n", instr, d->regs.i.rt, d->regs.i.addr_or_immed, d->regs.i.rs);
		}
		else
			printf("%s\t$%d, $%d, %d\n", instr, d->regs.i.rt, d->regs.i.rs, d->regs.i.addr_or_immed);
	}
	else if (d->type == J)
	{
		printf("%s\t0x%8.8x\n", instr, d->regs.j.target);
	}

	free(instr);
}

/* Perform computation needed to execute d, returning computed value */
int Execute(DecodedInstr *d, RegVals *rVals)
{
	//printf("Calling Execute\n");
	/* Your code goes here */

	// if its an R-format
	if (d->type == R)
	{
		switch (d->regs.r.funct)
		{
		case addu:
			//mips.registers[d->regs.r.rd] = mips.registers[rVals->R_rs] + mips.registers[rVals->R_rt];
			//	printf("R_rt: %d\nR_rs: %d\n\n\n", mips.registers[rVals->R_rt], mips.registers[rVals->R_rs]);

			return mips.registers[rVals->R_rs] + mips.registers[rVals->R_rt];
			break;

		case subu:
			//	mips.registers[d->regs.r.rd] = mips.registers[rVals->R_rs] - mips.registers[rVals->R_rt];
			return mips.registers[rVals->R_rs] - mips.registers[rVals->R_rt];
			break;

		case sll:
			//mips.registers[d->regs.r.rd] = mips.registers[rVals->R_rt] << d->regs.r.shamt;
			return mips.registers[rVals->R_rt] << d->regs.r.shamt;
			break;

		case srl:
			//mips.registers[d->regs.r.rd] = mips.registers[rVals->R_rt] >> d->regs.r.shamt;
			return mips.registers[rVals->R_rt] >> d->regs.r.shamt;
			break;

		case and:
			//mips.registers[d->regs.r.rd] = mips.registers[rVals->R_rs] & mips.registers[rVals->R_rt];
			return mips.registers[rVals->R_rs] & mips.registers[rVals->R_rt];
			break;

		case or:
			//mips.registers[d->regs.r.rd] = mips.registers[rVals->R_rs] | mips.registers[rVals->R_rt];
			return mips.registers[rVals->R_rs] | mips.registers[rVals->R_rt];
			break;

		case slt:
			//mips.registers[d->regs.r.rd] = (mips.registers[rVals->R_rs] < mips.registers[rVals->R_rt]) ? 1 : 0;
			return (mips.registers[rVals->R_rs] < mips.registers[rVals->R_rt]) ? 1 : 0;
			break;

		case jr:
			return mips.registers[31];
			break;

			//return 0;
		}
	}
	if (d->type == I)
	{
		switch (d->op)
		{

		case addiu:
			//mips.registers[d->regs.i.rt] = mips.registers[d->regs.i.rs] + d->regs.i.addr_or_immed;
			//printf("R_rt: %d\nR_rs: %d\n\n\n", mips.registers[rVals->R_rt], mips.registers[rVals->R_rs]);
			return (mips.registers[d->regs.i.rs] + d->regs.i.addr_or_immed);

		case andi:
			//mips.registers[d->regs.i.rt] = mips.registers[rVals->R_rs] & d->regs.i.addr_or_immed;
			return mips.registers[rVals->R_rs] & d->regs.i.addr_or_immed;

		case ori:
			//mips.registers[d->regs.i.rt] = mips.registers[rVals->R_rs] | d->regs.i.addr_or_immed;
			return mips.registers[rVals->R_rs] | d->regs.i.addr_or_immed;
		case lui:
			return d->regs.i.addr_or_immed << 16;

		case beq:
			//printf("R_rt: %d\nR_rs: %d\n\n\n", mips.registers[d->regs.i.rt], mips.registers[d->regs.i.rs]);
			if (mips.registers[rVals->R_rt] - mips.registers[rVals->R_rs] == 0)
			{
				//printf("R_rt: %d\nR_rs: %d\n\n\n", mips.registers[rVals->R_rt], mips.registers[rVals->R_rs]);

				//printf("BEQ Output: %d\n\n\n", ((4 * d->regs.i.addr_or_immed)));
				return ((4 * d->regs.i.addr_or_immed));
			}
			break;
			//return 0;

		case bne:
			if (mips.registers[rVals->R_rt] - mips.registers[rVals->R_rs] != 0)
			{
				return d->regs.i.addr_or_immed;
			}
			break;
			//return 0;

		case lw:
			// Since our stack memory pointer is in the highest memory of our current program
			// We are subtract our stack pointer by our immediate * 4.
			return (mips.registers[d->regs.i.rs] - (d->regs.i.addr_or_immed + 4));
			break;

		case sw:
			// Since our stack memory pointer is in the highest memory of our current program
			// We are subtract our stack pointer by our immediate * 4.
			//printf("Accessing Memory: 0x%8.8x\n",mips.registers[d->regs.i.rs] - (d->regs.i.addr_or_immed ));
			return (mips.registers[d->regs.i.rs] - (d->regs.i.addr_or_immed + 4));
			break;

		default:
			return 0;
		}
	}

	if (d->op == jal || d->op == jump)
	{
		if (d->op == jal)
		{
			mips.registers[31] = mips.pc + 4;
			return d->regs.j.target;
		}
		return d->regs.j.target;
	}

	return 0;
}

/* 
 * Update the program counter based on the current instruction. For
 * instructions other than branches and jumps, for example, the PC
 * increments by 4 (which we have provided).
 */
void UpdatePC(DecodedInstr *d, int val)
{
	//printf("Calling UpdatePC\n");
	mips.pc += 4;
	if (d->type == J && (d->op == jump || d->op == jal))
	{
		mips.pc = val;
	}
	if (d->type == R && d->regs.r.funct == jr)
	{
		mips.pc = mips.registers[31];
	}
	if (d->type == I && (d->op == beq || d->op == bne))
	{
		if (val > 0)
		{
			//printf("Called\n\n");
			mips.pc += val;
		}
	}

	/* Your code goes here */
}

/*
 * Perform memory load or store. Place the address of any updated memory 
 * in *changedMem, otherwise put -1 in *changedMem. Return any memory value 
 * that is read, otherwise return -1. 
 *
 * Remember that we're mapping MIPS addresses to indices in the mips.memory 
 * array. mips.memory[0] corresponds with address 0x00400000, mips.memory[1] 
 * with address 0x00400004, and so forth.
 * 
 */
int Mem(DecodedInstr *d, int val, int *changedMem)
{
	/* Your code goes here */
	//printf("Value in Memory: %d\n", val);

	// Max size in mips.memory is 4,096
	// So we can only access mips.memory[0] up to mips.memory[4095]
	//

	*changedMem = -1;

	// Given in the project's PDF under Mem.
	int memoryLowerBound = 0x00401000,
			memoryUpperBound = 0x00403FFF;

	if (d->op == sw)
	{
		// Prevent memory access in any address accessed out of the bounds 0x00401000 and
		// 0x00403FFC

		// Check if the value is out of bounds or if the user is inputting an offset that
		// is not divisible by 4. Exit program if triggered
		if (val < memoryLowerBound || val > memoryUpperBound || val % 4 != 0)
		{
			printf("Memory Access Exception at 0x%8.8x: address 0x%8.8x\n", mips.pc, val);
			*changedMem = -1;
			exit(0);
		}

		// mips.registers[29] = 0x00400000 + (MAXNUMINSTRS + MAXNUMDATA) * 4;
		// Update Memory because you accessed and changed memory
		*changedMem = val;

		int memoryIndex = (val - 0x00400000) / 4;

		mips.memory[memoryIndex] = mips.registers[d->regs.i.rt];
	}
	if (d->op == lw)
	{
		// Prevent memory access in any address accessed out of the bounds 0x00401000 and
		// 0x00403FFC
		if (val < memoryLowerBound || val > memoryUpperBound || val % 4 != 0)
		{
			printf("Memory Access Exception at 0x%8.8x: address 0x%8.8x\n", mips.pc, val);
			*changedMem = -1;
			exit(0);
		}
		// Load word doesn't change memory, it only access it.
		*changedMem = -1;

		int memoryIndex = (val - 0x00400000) / 4;

		val = mips.memory[memoryIndex];
	}
	return val;
}

/* 
 * Write back to register. If the instruction modified a register--
 * (including jal, which modifies $ra) --
 * put the index of the modified register in *changedReg,
 * otherwise put -1 in *changedReg.
 */
void RegWrite(DecodedInstr *d, int val, int *changedReg)
{
	*changedReg = -1;
	//If Value 0, nothing should happen
	// if (val == 0)
	// {
	// 	return;
	// }
	//	printf("Value in Regw: %d\n", val);
	if (d->type == J)
	{
		if (d->op == jal)
		{

			*changedReg = 31;
			//mips.registers[31] = val;
			//return;
		}
	}
	if (d->type == R)
	{
		if (d->regs.r.funct != jr)
		{
			*changedReg = d->regs.r.rd;
			mips.registers[*changedReg] = val;
		}
		//return;
	}
	if (d->type == I)
	{
		if ((d->op == bne || d->op == beq || d->op == sw))
		{
			return;
		}
		else
		{
			//	printf("Updated RT: %d\n", val);
			*changedReg = d->regs.i.rt;
			mips.registers[*changedReg] = val;
		}
	}
	//*changedReg = -1;
}
