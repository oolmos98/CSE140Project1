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
void AddInstructions(InstrSet* instr, int, char*, unsigned int);
void InitiateInstructions(InstrSet*, DecodedInstr *);
char* findInstructionName(InstrSet *, int);

/*Globally accessible Computer variable*/
Computer mips;
RegVals rVals;
InstrSet Instructions[43];

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
					div = 0x1A,
					divu = 0x1B,
					jr = 0x08,
					mfhi = 0x10,
					mthi = 0x11,
					mflo = 0x12,
					mtlo = 0x13,
					mfc0 = 0x0,
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


void AddInstructions(InstrSet* instr, int type, char* name, unsigned int code)
{
	instr->type = type;
	strcpy(instr->name, name);
	instr->funct = code;
}

void InitiateInstructions(InstrSet* instr, DecodedInstr *d)
{
	for (int i = 0; i < 43; i++)
	{
		instr[i].name = malloc(sizeof(char) * 5);
	}

	// Add the R-format Instructions
	AddInstructions(&instr[0], d->type->R, "add", 0x20);
	AddInstructions(&instr[1], d->type->R, "addu", 0x21);
	AddInstructions(&instr[2], d->type->R, "and", 0x24);
	AddInstructions(&instr[3], d->type->R, "div", 0x1A);
	AddInstructions(&instr[4], d->type->R, "divu", 0x1B);
	AddInstructions(&instr[5], d->type->R, "jr", 0x08);
	AddInstructions(&instr[6], d->type->R, "mfhi", 0x10);
	AddInstructions(&instr[7], d->type->R, "mthi", 0x11);
	AddInstructions(&instr[8], d->type->R, "mflo", 0x12);
	AddInstructions(&instr[9], d->type->R, "mtlo", 0x13);
	AddInstructions(&instr[10], d->type->R, "mfc0", 0x0);
	AddInstructions(&instr[11], d->type->R, "mult", 0x18);
	AddInstructions(&instr[12], d->type->R, "multu", 0x19);
	AddInstructions(&instr[13], d->type->R, "nor", 0x27);
	AddInstructions(&instr[14], d->type->R, "xor", 0x26);
	AddInstructions(&instr[15], d->type->R, "or", 0x25);
	AddInstructions(&instr[16], d->type->R, "slt", 0x2A);
	AddInstructions(&instr[17], d->type->R, "sltu", 0x2B);
	AddInstructions(&instr[18], d->type->R, "sll", 0x00); // uses shift amt
	AddInstructions(&instr[19], d->type->R, "srl", 0x02); // uses shift amt
	AddInstructions(&instr[20], d->type->R, "sra", 0x03);
	AddInstructions(&instr[21], d->type->R, "sub", 0x22);
	AddInstructions(&instr[22], d->type->R, "subu", 0x23);

	// Add the J-format Instructions
	AddInstructions(&instr[23], d->type->J, "j", 0x02);
	AddInstructions(&instr[24], d->type->J, "jal", 0x03);

	// Add the I-format Instructions
	AddInstructions(&instr[25], d->type->I, "addi", 0x08);
	AddInstructions(&instr[26], d->type->I, "addiu", 0x09);
	AddInstructions(&instr[27], d->type->I, "andi", 0x0C);
	AddInstructions(&instr[28], d->type->I, "beq", 0x04);
	AddInstructions(&instr[29], d->type->I, "blez", 0x06);
	AddInstructions(&instr[30], d->type->I, "bne", 0x05);
	AddInstructions(&instr[31], d->type->I, "lb", 0x20);
	AddInstructions(&instr[32], d->type->I, "lbu", 0x24);
	AddInstructions(&instr[33], d->type->I, "lhu", 0x25);
	AddInstructions(&instr[34], d->type->I, "lui", 	0x0F);
	AddInstructions(&instr[35], d->type->I, "lw", 0x23);
	AddInstructions(&instr[36], d->type->I, "ori", 0x0D);
	AddInstructions(&instr[37], d->type->I, "sb", 0x28);
	AddInstructions(&instr[38], d->type->I, "sh", 0x29);
	AddInstructions(&instr[39], d->type->I, "slti", 0x0A);
	AddInstructions(&instr[40], d->type->I, "sltiu", 0x0B);
	AddInstructions(&instr[41], d->type->I, "sw", 0x2B);
	AddInstructions(&instr[42], d->type->I, "bgtz", 0x07);
	

}

char* findInstructionName(InstrSet *instr, int code)
{
	for (int i = 0; i < 43; i++)
	{
		if(instr[i].funct == code) 
		{
			return instr[i].name;
		}
	}
	return '\0';
}

/*  
	opcodes for R-format
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


	InitiateInstructions(Instructions, d);

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
	unsigned int opcode = instr >> 26;
	if (opcode == 0)
		format = 'R';
	if (opcode == 0x02 || opcode == 0x03)
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
		PrintInstruction(d);
		break;

	case 'I':
		/*
			I-format
			opcode, rs, rt, immediate
		*/
		d->op = instr >> 26;
		d->regs->i.rs = (instr << 6) >> 27;
		d->regs->i.rt = (instr << 11) >> 27;
		d->regs->i.addr_or_immed = (instr << 16) >> 16;
		
		break;
	case 'J':
		/*
			I-format
			opcode, jump_addr

			x - opcode
			y - address

			xxxxxxyyyyyyyyyyyyyyyyyyyyyyyyyy
			
			instr << 6

			yyyyyyyyyyyyyyyyyyyyyyyyyy000000
		
			instr >> 6
			000000yyyyyyyyyyyyyyyyyyyyyyyyyy

		*/
		d->regs.j.target = (instr << 6) >> 6;
		if(opcode == jal) 
		{
			d->regs.r.rs = mips.pc;
		}
		UpdatePC(d, d->regs.j.target);
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

	char* functionName;
	int rs = 0;
	int rt = 0;
	int rd = 0;

	/* Your code goes here */
	// Check if the instruction is R-foarmat
	if(d->type == 0) 
	{
		functionName = findInstructionName(Instructions, d->regs.r.funct);
		rs = d->regs.r.rs;
		rt = d->regs.r.rt;
		rd = d->regs.r.rd;

		/*
			
			R-functions that uses all 3 registers
		
		*/
		if(d->regs.r.funct != sll || d->regs.r.funct != srl) 
		{
			printf("%s $%d, $%d, $%d", functionName, d->regs.r.rd, d->regs.r.rs, d->regs.r.rt);
		}
		else 
		{
			printf("%s $%d, $%d, %d", functionName, d->regs.r.rd, d->regs.r.rt, d->regs.r.shamt);
		}
	}
	// Check if the instruction is I-foarmat
	else if(d->type == 1) 
	{

	}
	// Check if the instruction is J-foarmat
	else if(d->type == 2) 
	{

	}
}

/* Perform computation needed to execute d, returning computed value */
int Execute(DecodedInstr *d, RegVals *rVals)
{
	/* Your code goes here */
	switch (opcode)
	{
		case addi:
			
			break;

		case addiu:
		   
			break;

		case beq:
			
			break;

		case blez:
			
			break;

		case bne:
			
			break;

		case bgtz:
			
			break;

		case lb:
			
			break;

		case lbu:
			
			break;

		case lhu:
			
			break;

		case lui:
			
			break;

		case lw:
			
			break;

		case ori:
			
			break;

		case sb:
			
			break;

		case sh:
			
			break;

		case slti:
			
			break;

		case sltiu:
			
			break;

		case sw:
			
			break;
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
	if(val>0){ 
		//mips.pc = (val+0x00400000)/4;
	}
	mips.pc += 4;
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
