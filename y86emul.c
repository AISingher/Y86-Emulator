#include "y86emul.h"

int reg[8];//the 8 registers
int flag[3];// the 3 flags
status programStatus;// the 4 statuses
union getInt u; //4 bytes from memory to form a 32-bit int
unsigned char *memory; //memory that'll hold addresses, machine instructions, data
unsigned int instructionStart,memorySize,programCounter;// start of instructions, size of memory allocated, address of current instruction


void calcRegisters(int *rA, int*rB, int byte){ //rA(source) and rB(destination) from given byte

*rB=byte%16;
byte=byte/16;
*rA=byte%16;
}

int readStoredInt(int location){//returns 4 bytes or 32-bit int stored in memory

u.byte[0]= memory[location]; location++;
u.byte[1]= memory[location]; location++;
u.byte[2]= memory[location]; location++;
u.byte[3]= memory[location];

return u.num;
}

void storeInt(int location, int num){ //stores 4 bytes or 32-bit int in memory

u.num=num;

memory[location]= u.byte[0]; location++;
memory[location]= u.byte[1]; location++;
memory[location]= u.byte[2]; location++;
memory[location]= u.byte[3];
}

int main(int argc, char ** argv){

FILE *fp;//file pointer to .y86 file
unsigned int address,data;//address and data in memory
char *directive;//directives to get data and machine instructions
//read characters from the file one at a time
int readChar; 
char temp[3];


	if(argc!=2){
		fprintf(stderr,"Incorrect number of args\n.");
		return 0;
	}
	if(strcmp(argv[1],"-h")==0){
		printf("Format: ./y86emul <y86 input file>\n");
		return 0;	
	}	

	if((fp = fopen(argv[1],"r"))==NULL){
        fprintf(stderr,"No file exists\n.");
		return 0;
	}
	programStatus=AOK;
    
    //memory given by the .size directive assuming .size is at the top of the file 
	directive=(char *)malloc(sizeof(char)*8);
	fscanf(fp,"%s\t%x",directive,&memorySize); 
	memory=(unsigned char *)malloc(sizeof(unsigned char)*memorySize);

	while(fscanf(fp, "%s", directive)!=EOF){ //time to read the file/directives and populate the memory
        
		if(strcmp(directive, ".text")==0){
			fscanf(fp, "\t %x \t", &instructionStart);
			programCounter =instructionStart;
			while(((readChar=fgetc(fp))!='\n')){
				temp[0]=readChar;
				readChar=fgetc(fp);
				temp[1]=readChar;
				temp[2]='\0';
				sscanf(temp, "%x", &data); 
				memory[instructionStart]=data;
				instructionStart++;
			}
		}
		else if(strcmp(directive,".byte")==0){
			fscanf(fp, "\t %x \t %x",&address, &data);
			memory[address]=data;
		}
		else if(strcmp(directive, ".string")==0){
			fscanf(fp, "\t %x \t", &address);
			readChar=fgetc(fp);
			while((readChar=getc(fp))!='\"'){
				memory[address]=readChar;
				address++;
			}
		}
		else if(strcmp(directive, ".long")==0){
			fscanf(fp, "\t %x \t %x", &address, &data);
			storeInt(address,data);
		}
	}

	execute();
	printProgramStatus();
	fclose(fp);
	free(memory);
	free(directive);
	return 0;
}

void execute(){//instructions executed

int opcode1,opcode2,rA,rB,byte;
	
    if(programCounter+1<0||programCounter+1>memorySize){//if next instruction address is negative or above memory capacity 
                programStatus=ADR;
            }
    
	while(1){//continuous 
		
		byte=memory[programCounter];
		opcode2=byte%16;
		byte=byte/16;
		opcode1=byte%16;

		if(opcode1==0){//nop
			programCounter++;
		}
		else if(opcode1==1){//halt
			programStatus=HLT;
			break;
		}
		else if(opcode1==2){//rrmovl
			calcRegisters(&rA, &rB, memory[programCounter+1]);
			reg[rB]=reg[rA];
			programCounter+=2;
		}
		else if(opcode1==3){//irmovl
			calcRegisters(&rA, &rB, memory[programCounter+1]);
			reg[rB]= readStoredInt(programCounter+2);
			programCounter+=6;
		}
		else if(opcode1==4){//rrmovl
			calcRegisters(&rA, &rB, memory[programCounter+1]);
			storeInt(readStoredInt(programCounter+2)+reg[rB],reg[rA]);
			programCounter+=6;
		}
		else if(opcode1==5){//mrmovl
			calcRegisters(&rA, &rB, memory[programCounter+1]);
			reg[rA]=readStoredInt(readStoredInt(programCounter+2)+reg[rB]);
			programCounter+=6;
		}
		else if(opcode1==6){//op1 instructions 			     			     			     			     			     
			int result;
			calcRegisters(&rA, &rB, memory[programCounter+1]);

			if(opcode2==0){//addl
				result=reg[rB]+reg[rA];
				setFlags(reg[rB], reg[rA], result, opcode2);
				reg[rB]=result;
			}
			else if (opcode2==1){//subl
				result=reg[rB]-reg[rA];
				setFlags(reg[rA], reg[rB], result, opcode2);
				reg[rB]=result;
			}
			else if(opcode2==2){//andl
				result=reg[rB]&reg[rA];
				setFlags(reg[rB], reg[rA], result, opcode2);
				reg[rB]=result;
			}
			else if(opcode2==3){//xorl
				result=reg[rB]^reg[rA];
				setFlags(reg[rB], reg[rA], result, opcode2);
				reg[rB]=result;
			}
			else if(opcode2==4){//mull
				result=reg[rB]*reg[rA];
				setFlags(reg[rB], reg[rA], result, opcode2);
				if(flag[1]==1){
					printf("ERROR: Overflow\n");
				/*	programStatus=HLT;
					break;*/
				}
				reg[rB]=result;
			}
		/*	cmpl
       			else if(opcode2 ==5){
				result= reg[rB]-reg[rA];
				setFlags(reg[rA],reg[rB],result,opcode2);
			}*/

			programCounter+=2;
		}
                           
		else if(opcode1==7){//jump instructions jXX
			int result;
			result=readStoredInt(programCounter+1);
            
			if(opcode2==0){//jmp
				programCounter=result;
				continue;
			}
			else if(opcode2==1){//jle
				if((flag[2]^flag[1]) | flag[0]){
					programCounter=result;
					continue;
				}
			}
			else if(opcode2==2){//jl
				if((flag[2]^flag[1])){
					programCounter=result;
					continue;
				}
			}
			else if(opcode2==3){//je
				if((flag[0])){
					programCounter=result;
					continue;
				}
			}
			else if(opcode2==4){//jne
				if(!(flag[0])){
					programCounter=result;
					continue;
				}
			}
			else if(opcode2==5){//jge
				if(!(flag[2]^flag[1])){
					programCounter=result;
					continue;
				}
			}
			else if(opcode2==6){//jg
				if(!(flag[2]^flag[1]) & (!flag[0])){
					programCounter=result;
					continue;
				}
			}
			programCounter+=5;
		}
		else if(opcode1==8){//CALL

			if(push(programCounter+5)){//overflow
				printf("ERROR: overflow\n");
				break;
			}
			programCounter=readStoredInt(programCounter+1);
			continue;
		}
		else if(opcode1==9){//ret
			programCounter=pop();
			
		}
		else if(opcode1==10){//pushl
			calcRegisters(&rA, &rB, memory[programCounter+1]);

			if(push(reg[rA])){//overflow
				printf("ERROR: overflow\n");
				break;
			}
			programCounter+=2;

		}
		else if(opcode1==11){//popl
			calcRegisters(&rA, &rB, memory[programCounter+1]);
			reg[rA]=pop();
			programCounter+=2;
		}
		else if(opcode1==12){//readx
			int displacement;	
			displacement=readStoredInt(programCounter+2);
			calcRegisters(&rA, &rB, memory[programCounter+1]);
			setFlags(0,0,0,12);
            
			if(opcode2==0){//readb
				int descriptor;
				char readByte;
				flag[0]=0;
				descriptor=scanf("%c", &readByte);
				if(descriptor== EOF){
					flag[0]=1;
				}
				else{
					memory[reg[rA]+displacement]=readByte;
				}
			}
			else if (opcode2==1){//readl
				int descriptor;
				flag[0]=0;
				descriptor=scanf("%d", &byte);
				if(descriptor==EOF){
					flag[0]=1;
				}
				else{
					storeInt(reg[rA]+displacement, byte);
				}	
			}
			programCounter+=6;
		}
		else if(opcode1==13){//writex
			int displacement;
			displacement=readStoredInt(programCounter+2);
			calcRegisters(&rA, &rB, memory[programCounter+1]);
            
			if(opcode2==0){//writeb
				printf("%c", memory[reg[rA]+displacement]);
			}
			else if(opcode2==1){//writel
				printf("%d", readStoredInt(reg[rA]+displacement));
			}
			programCounter+=6;
		}
		else if(opcode1==14){//movsbl
			int displacement;
			long result;
			char tempByte;

			displacement=readStoredInt(programCounter+2);
			calcRegisters(&rA,&rB,memory[programCounter+1]);
			tempByte=memory[reg[rB]+displacement];
			result=(long)(tempByte-'0');
			storeInt(reg[rA],result);
			
			setFlags(0,0,tempByte,14);
			programCounter+=6;
		}
		else{
			programStatus = INS;
			break;
		}
	}
	return;
}

void setFlags(int num1, int num2, int result, int opcode){//set flags for conditions in previous opcodes 
    
flag[0]=0;//ZF
flag[1]=0;//OF
flag[2]=0;//SF
    
	if(result==0){//0
		flag[0]=1;
		flag[1]=0;
		flag[2]=0;
	}
	else if(result<0){//negative number
		flag[0]=0;
		flag[1]=0;
		flag[2]=1;
	}
	else{//overflow
		if(opcode==0){
			if(((num1<0)==(num2<0) ) && ( (result<0)!=(num1<0) ) ){
				flag[0]=0;
				flag[1]=1;
				flag[2]=0;
			}		
		}
		else if(opcode==1){
			if((num1<0 && num2>0 && result <0) || (num1>0 && num2<0 && result >0) ){
				flag[0]=0;
				flag[1]=1;
				flag[2]=0;
			}
		}
		else if(opcode==4){
			if((result/num2)!=num1 ){
				flag[0]=0;
				flag[1]=1;
				flag[2]=0;
			}
		}
	/*	else if(opcode==5){
			if((result/num2)!=vauel ){
				flag[0]=0;
				flag[1]=1;
				flag[2]=0;
			}
		}*/
	}

	if(opcode==2||opcode==3){	
		flag[1]=0;
	}
	if(opcode==12){
		flag[1]=0;
		flag[2]=0;
	}
}

int push(int num){
	reg[4]= reg[4]-4;
	storeInt(reg[4], num);
	return 0;
}

int pop(){
	int num=readStoredInt(reg[4]);
	reg[4]= reg[4]+4;
	return num;
}

void printProgramStatus(){
	if(programStatus==AOK){
		printf("Everything is fine.\n");
	}
	else if(programStatus==HLT){
		printf("Halt instruction.\n");
	}
	else if(programStatus==ADR){
		printf("Invalid address.\n");
	}
	else if(programStatus==INS){
		printf("Invalid instruction.\n");
	}

}


