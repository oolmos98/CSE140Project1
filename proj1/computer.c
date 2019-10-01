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
const unsigned int 	addi = 0x08,
					addiu = 0x09,
					beq = 0x04,
					blez = 0x06,
					bne = 0x05,
					bgtz = 0x07,
					lb = 0x20,
					lbu = 0x24,
					lhu = 0x25,
					lui = 0x0F,
					lw = 0x23,
					ori = 0x0D,
					sb = 0x28,
					sh = 0x29,
					slti = 0x0A,
					sltiu = 0x0B,
					sw = 0x2B;
/*  
	opcodes for J-format
*/
const unsigned int 	jal = 0x03,
					jump = 0x02;



/*

	opcodes for R-Format

*/

const unsigned int 	add = 0x20,
					addu = 0x21,
					and = 0x24,
					i_div = 0x1A,			// Another 'div' already exists oof
					divu = 0x1B,
					jr = 0x08,
					mfhi = 0x10,
					mthi = 0x11,
					mflo = 0x12,
					mtlo = 0x13,
					mult = 0x18,
					multu = 0x19,
					nor = 0x27,
					xor = 0x26,
					or = 0x25,
					slt = 0x2A,
					sltu = 0x2B,
					sll = 0x00,		// uses shamt
					srl = 0x02,		// uses shamt
					sra = 0x03,
					sub = 0x22,
					subu = 0x23;


/*

	Implementing Control

*/
enum ALUctlInputs {
	ADD = 0,
	SUB,
	OR,
	AND
};

enum RegDst {
	Rt = 0,
	Rd
};

/*
	
	Doing this to eliminate unecessary (what i mean ALOT of) if-elseif-else statements when we start to implement the ALU (execute function)..

*/

int ALUctl = -1; 	// determined by the ALUctlInputs
int RegDst = -1;
int ALUsrc = -1;	// 0 for an register input, 1 for an immediate input
int ExtOp = -1;
int MemWr = -1;
int MemToReg = -1;	//

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

	ALUctl = -1; 	// determined by the ALUctlInputs
	RegDst = -1;
	ALUsrc = -1;	// 0 for an register input, 1 for an immediate input
	ExtOp = -1;
	MemWr = -1;
	MemToReg = -1;	//


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
			d->op = 0;
			d->regs.r.rs = rVals->R_rs = (instr << 6) >> 27;
			d->regs.r.rt = rVals->R_rt = (instr << 11) >> 27;
			d->regs.r.rd = rVals->R_rd = (instr << 16) >> 27;
			d->regs.r.shamt = (instr << 21) >> 27;
			d->regs.r.funct = (instr << 26) >> 26;
			//printf("%d\n", (instr << 26) >> 26);
			//PrintInstruction(d);
			if(d->regs.r.funct == jr) 
			{
				/*
					(mips.pc + 4) << 4
					iiiiiiiiiiiiiiiiiiiiiiiiiiii0000

					((mips.pc + 4) << 4) >> 6
					000000iiiiiiiiiiiiiiiiiiiiiiiiii
				*/
				d->regs.r.rs = ((mips.pc + 4) << 4) >> 6;
				UpdatePC(d, mips.registers[31]);
			}
			break;

		case 'I':
			/*
				I-format
				opcode, rs, rt, immediate
			*/
			d->op = opcode;
			//printf("I: %d\n", opcode);
			d->regs.i.rs = (instr << 6) >> 27;
			d->regs.i.rt = (instr << 11) >> 27;
			d->regs.i.addr_or_immed = (instr << 16) >> 16;

			// ALUsrc will always be in Immediate field
			ALUsrc = 1;

			/*
				An example.
			*/
			switch(opcode)
			{
				case lw:
					ALUctl = ADD;
					MemWr = 1;
					MemToReg = 1;
					ExtOp = 1; // Signed
					RegDst = Rt;
				break;
			}

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

			d->op = opcode;
			int curr_pc = (mips.pc >> 28) << 28;

			int target_pc = ((instr << 6) >> 4) | curr_pc;


			d->regs.j.target = target_pc;

			if(opcode == jal || opcode == jump )
			{
				if(opcode == jal){
					mips.registers[31] = mips.pc+=4;
				}
				UpdatePC(d, target_pc);

			}

			
			//PrintInstruction(d);
			//UpdatePC(d, d->regs.j.target);
			break;

		default:

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
	char* instr = (char*)malloc(sizeof(char)*5);
	int supported_instr = 1;


	//printf("%d\n", d->op);
	//printf("R: %d\n", d->regs.r.funct);

	if(d->op == 0)
	{
		//printf("R: %d\n", d->regs.r.funct);
		switch(d->regs.r.funct) 
		{
			case add:
				strcpy(instr, "add");
			break;

			case addu:
				strcpy(instr, "addu");
			break;

			case and:
				strcpy(instr, "and");
			break;

			case i_div:
				strcpy(instr, "div");
			break;

			case divu:
				strcpy(instr, "divu");
			break;

			case jr:
				strcpy(instr, "jr");
			break;

			case mfhi:
				strcpy(instr, "mfhi");
			break;

			case mthi:
				strcpy(instr, "mthi");
			break;

			case mflo:
				strcpy(instr, "mflo");
			break;

			case mtlo:
				strcpy(instr, "mtlo");
			break;

			case mult:
				strcpy(instr, "mult");
			break;

			case multu:
				strcpy(instr, "multu");
			break;

			case nor:
				strcpy(instr, "nor");
			break;

			case xor:
				strcpy(instr, "xor");
			break;

			case or:
				strcpy(instr, "or");
			break;

			case slt:
				strcpy(instr, "slt");
			break;

			case sltu:
				strcpy(instr, "sltu");
			break;

			case sll:
				strcpy(instr, "sll");
			break;

			case srl:
				strcpy(instr, "srl");
			break;

			case sra:
				strcpy(instr, "sra");
			break;

			case sub:
				strcpy(instr, "sub");
			break;

			case subu:
				strcpy(instr, "subu");
			break;

			default: // gets triggered if theres an unsupported code
				supported_instr = 0;
			break;
		}
	}
	else 
	{
		switch(d->op)
		{
			//printf("%d\n", d->op);
			case addi:
				strcpy(instr, "addi");
			break;

			case addiu:
				strcpy(instr, "addiu");
			break;

			case beq:
				strcpy(instr, "beq");
			break;

			case blez:
				strcpy(instr, "blez");
			break;

			case bne:
				strcpy(instr, "bne");
			break;

			case bgtz:
				strcpy(instr, "bgtz");
			break;

			case lb:
				strcpy(instr, "lb");
			break;

			case lbu:
				strcpy(instr, "lbu");
			break;

			case lhu:
				strcpy(instr, "lhu");
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

			case sb:
				strcpy(instr, "sb");
			break;

			case sh:
				strcpy(instr, "sh");
			break;

			case slti:
				strcpy(instr, "slti");
			break;

			case sltiu:
				strcpy(instr, "sltiu");
			break;

			case sw:
				strcpy(instr, "sw");
			break;

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

	if(supported_instr == 0) {
		printf("Unsupported instruction found. Terminating program\n");
		exit(0);
	}
	

	if(d->type == R) 
	{
		if(d->regs.r.funct == sll || d->regs.r.funct == srl)
		{
			printf("%s $%d, $%d, %d\n", instr, d->regs.r.rd, d->regs.r.rs, d->regs.r.shamt);
		}
		else 
		{
			printf("%s $%d, $%d, $%d\n", instr, d->regs.r.rd, d->regs.r.rs, d->regs.r.rt);
		}
		
	}
	else if(d->type == I) 
	{

	} 
	else if(d->type == J)
	{

	}

	free(instr);
}

/* Perform computation needed to execute d, returning computed value */
int Execute(DecodedInstr *d, RegVals *rVals)
{
	/* Your code goes here */

	// if its an R-format
	if(d->op == 0)
	{
		switch(ALUctl)
		{
			case ADD:	// For ADD, branch etc.

			break;

			case SUB:



			break;

			case OR:	// For OR , NOR, and XOR instruction
				// Or: rd = rs | rt
				rVals->R_rt = d->regs.r.rt;
				rVals->R_rs = d->regs.r.rs;

				switch(d->regs.r.funct)
				{
					case or:
						rVals->R_rd = rVals->R_rs | rVals->R_rt;
					break;

					case nor:
						rVals->R_rd = ~(rVals->R_rs | rVals->R_rt);
					break;

					case xor:

					break;
				}
			break;

			case AND: // For AND instruction
				rVals->R_rt = d->regs.r.rt;
				rVals->R_rs = d->regs.r.rs;
				rVals->R_rd = rVals->R_rs & rVals->R_rt;
			break;
		}
	}
	else 
	{

		switch(ALUctl)
		{
			case ADD:	// For ADD, branch etc.

			break;

			case SUB:
				if(d->op == beq || d->op == bne) {
					rVals->R_rt = d->regs.i.rt;
					rVals->R_rs = d->regs.i.rs;

					result = rVals->R_rt - rVals->R_rs;
					if(result == 0 )
					{
						//UpdatePC();
					}
					else if(d->op == bne)
					{
						//UpdatePC();
					}
				}
				
			break;

			case OR:	// For OR , NOR, and XOR instruction
				// Or: rd = rs | rt
				rVals->R_rt = d->regs.i.rt;
				rVals->R_rs = d->regs.i.rs;
				int immediate = d->regs.i.addr_or_immed;

				switch(d->regs.r.funct)
				{
					case or:
						rVals->R_rt = rVals->R_rs | addr_or_immed;
					break;

					case nor:
						rVals->R_rt = ~(rVals->R_rs | addr_or_immed);
					break;

					case xor:

					break;
				}
			break;

			case AND: // For AND instruction
				rVals->R_rt = d->regs.r.rt;
				rVals->R_rs = d->regs.r.rs;
				rVals->R_rd = rVals->R_rs & rVals->R_rt;
			break;
		}
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

	mips.pc += 4;
	/* Your code goes here */

	if(d->op == jump || d->op == jal) 
	{
		mips.pc = d->regs.j.target;
	}
	else if (d->op == 0 && d->regs.r.funct == jr) 
	{
		mips.pc = val;
	}
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
	return 0;
}

/* 
 * Write back to register. If the instruction modified a register--
 * (including jal, which modifies $ra) --
 * put the index of the modified register in *changedReg,
 * otherwise put -1 in *changedReg.
 */
void RegWrite(DecodedInstr *d, int val, int *changedReg)
{
	/* Your code goes here */
}
