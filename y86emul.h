#ifndef y86emul_h
#define y86emul_h

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

union getInt{
	char byte[4];
	int num;
};

typedef enum {AOK, HLT, ADR, INS} status;

void calcRegisters(int *rA, int*rB, int byte);
int readStoredInt(int location);
void storeInt(int location, int num);
int main(int argc, char ** argv);
void execute();
void setFlags(int num1, int num2, int result, int opcode);
int push(int num);
int pop();
void printProgramStatus();


#endif
