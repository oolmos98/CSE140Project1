#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char R[32][4] =
    {
        "zero", "at", "v0", "v1",
        "a0", "a1", "a2", "a3",
        "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
        "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
        "t8", "t9",
        "k0", "k1",
        "gp", "sp", "fp", "ra"};

//typedef unsigned int uint;

struct functions
{
  char *name;
  int funct;
};

void addRfuction(struct functions *RFormat, char *name, int functioncode)
{
  //printf("Called AddR\n");
  strcpy(RFormat->name, name);
  RFormat->funct = functioncode;
}
void initialize(struct functions *RFormat)
{
  for (int i = 0; i < 23; i++)
  {
    RFormat[i].name = malloc(sizeof(char) * 5);
  }
  addRfuction(&RFormat[0], "add", 0x20);
  addRfuction(&RFormat[1], "addu", 0x21);
  addRfuction(&RFormat[2], "and", 0x24);
  addRfuction(&RFormat[3], "div", 0x1A);
  addRfuction(&RFormat[4], "divu", 0x1B);
  addRfuction(&RFormat[5], "jr", 0x08);
  addRfuction(&RFormat[6], "mfhi", 0x10);
  addRfuction(&RFormat[7], "mthi", 0x11);
  addRfuction(&RFormat[8], "mflo", 0x12);
  addRfuction(&RFormat[9], "mtlo", 0x13);
  addRfuction(&RFormat[10], "mfc0", 0x0);
  addRfuction(&RFormat[11], "mult", 0x18);
  addRfuction(&RFormat[12], "multu", 0x19);
  addRfuction(&RFormat[13], "nor", 0x27);
  addRfuction(&RFormat[14], "xor", 0x26);
  addRfuction(&RFormat[15], "or", 0x25);
  addRfuction(&RFormat[16], "slt", 0x2A);
  addRfuction(&RFormat[17], "sltu", 0x2B);
  addRfuction(&RFormat[18], "sll", 0x00);
  addRfuction(&RFormat[19], "srl", 0x02);
  addRfuction(&RFormat[20], "sra", 0x03);
  addRfuction(&RFormat[21], "sub", 0x22);
  addRfuction(&RFormat[22], "subu", 0x23);
}
char delimiter[] = ", ";

int main(int argc, char **argv)
{
  char instr[16];
  char *ins[5];
  int index = 0;
  printf("Enter an R-Format intruction: \n");
  scanf("%[^\n]", instr);
  char *temp = strtok(instr, delimiter);
  printf("WhileLoop\n");

  while (temp != NULL)
  {
    ins[index++] = temp;
    temp = strtok(NULL, delimiter);
  }

  struct functions RFunctions[23];
  initialize(RFunctions);
  //printf("printing RFunc\n");

  // printf("function: %s \nfunctioncode: %i\n", RFunctions[0].name, RFunctions[0].funct);
  // printf("function: %s \nfunctioncode: %i \n", RFunctions[1].name, RFunctions[1].funct);

  return 0;
}

/*
References:
https://stackoverflow.com/questions/26597977/split-string-with-multiple-delimiters-using-strtok-in-c
*/
