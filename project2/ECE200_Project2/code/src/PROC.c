
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "RegFile.h"
#include "Syscall.h"
#include "utils/heap.h"
#include "elf_reader/elf_reader.h"

//Stats

uint32_t DynInstCount = 0;

void write_initialization_vector(uint32_t sp, uint32_t gp, uint32_t start) {
        printf("\n ----- BOOT Sequence ----- \n");
        printf("Initializing sp=0x%08x; gp=0x%08x; start=0x%08x\n", sp, gp, start);
        RegFile[28] = gp;
        RegFile[29] = sp;
        RegFile[31] = start;
        printRegFile();

    }

int main(int argc, char * argv[]) {
  
    int MaxInst = 0;
    int status = 0;
    uint32_t i; 
    uint32_t PC,newPC = 0;
    uint32_t CurrentInstruction;
    uint32_t hi = 0, lo = 0;

    if (argc < 2) {
      printf("Input argument missing \n");
      return -1;
    }
    sscanf (argv[2], "%d", &MaxInst);

    //Open file pointers & initialize Heap & Regsiters
    initHeap();
    initFDT();
    initRegFile(0);
    
    //Load required code portions into Emulator Memory
    status =  LoadOSMemory(argv[1]);
    if(status < 0) { return status; }
    
    //set Global & Stack Pointers for the Emulator
    // & provide startAddress of Program in Memory to Processor
    write_initialization_vector(exec.GSP, exec.GP, exec.GPC_START);

    printf("\n ----- Execute Program ----- \n");
    printf("Max Instruction to run = %d \n",MaxInst);
    fflush(stdout);
    PC = exec.GPC_START;
    for(i=0; i<MaxInst ; i++) {
        DynInstCount++;
        CurrentInstruction = readWord(PC,false);  
        printRegFile();
    /********************************/
    //Add your implementation here
        uint32_t OldInstruction = readWord(PC-4,false);
        int opcode1=0; //get the opcode
        int rs1=0,rt1=0, funct1=0;//get the rs,rt funct part
        opcode1 = OldInstruction & 0xfc000000;
        funct1= OldInstruction & 0x0000003f;
        rs1 = OldInstruction & 0x03e00000;
        rt1 = OldInstruction & 0x001f0000;
        rs1 = rs1 >> 21;
        rt1 = rt1 >> 16;
        if (opcode1==0){ //R-type
            int rd1 = OldInstruction & 0x0000f100;
            if (funct1==9){ //jalr
                
                RegFile[rd1] = PC + 8;
                newPC = RegFile[rs1];
            }
            if (funct1==8){ //jr
                
                newPC = RegFile[rs1];
            }
        } 
        if (opcode1==0x10000000){ //beq
            if (RegFile[rs1]==RegFile[rt1]){
                int32_t imme1 = OldInstruction & 0x0000ffff;
                newPC = PC + (imme1 << 2);
            }
        }
        if (opcode1==0x50000000){ //beql
            int32_t imme1 = OldInstruction & 0x0000ffff;
            newPC = PC + (imme1 << 2);
        }
        if (opcode1==0x70000000){ //bgtz
            if (RegFile[rs1] > 0){
                int32_t imme1 = OldInstruction & 0x0000ffff;
                newPC = PC + (imme1 << 2);
            }
        }
        if (opcode1==0x60000000){ //blez
            if (RegFile[rs1] <= 0){
                int32_t imme1 = OldInstruction & 0x0000ffff;
                newPC = PC + (imme1 << 2);
            }
        }
        if (opcode1==0x58000000){ //blezl
            int32_t imme1 = OldInstruction & 0x0000ffff;
            newPC = PC + (imme1 << 2);
        }
        if (opcode1==0x50000000){ //bne
            if (RegFile[rs1]!=RegFile[rt1]){
                int32_t imme1 = OldInstruction & 0x0000ffff;
                newPC = PC + (imme1 << 2);
            }
        }
        if (opcode1==0x54000000){ //bnel
            int32_t imme1 = OldInstruction & 0x0000ffff;
            newPC = PC + (imme1 << 2);
        }
        
        
        //here I want to do the branch delay slot

        int opcode=0; //get the opcode
        int rs=0,rt=0, funct=0;//get the rs,rt funct part
        opcode = CurrentInstruction & 0xfc000000;
        funct= CurrentInstruction & 0x0000003f;
        rs = CurrentInstruction & 0x03e00000;
        rt = CurrentInstruction & 0x001f0000;
        rs = rs >> 21;
        rt = rt >> 16;
        if (opcode==0){ //R-type
            int rd =0;
            rd = CurrentInstruction & 0x0000f100;
            rd = rd >> 11;
            if (funct==32){ //add
                RegFile[rd] = RegFile[rs] + RegFile[rt];
            }
            if (funct==33){ //addu
                RegFile[rd] = (unsigned)RegFile[rs] + (unsigned)RegFile[rt];
            }
            if (funct==34){ //sub
                RegFile[rd] = RegFile[rs] - RegFile[rt];
            }
            if (funct==35){ //subu
                RegFile[rd] = (unsigned)RegFile[rs] - (unsigned)RegFile[rt];
            }
            if (funct==26){ //div
                lo = RegFile[rs] / RegFile[rt];
                hi = RegFile[rs] % RegFile[rt];
            }
            if (funct==27){ //divu
                lo = (unsigned)RegFile[rs] / (unsigned)RegFile[rt];
                hi = (unsigned)RegFile[rs] % (unsigned)RegFile[rt];
            }
            if (funct==24){ //mult
                int64_t a = RegFile[rs] * RegFile[rt];
                lo = a & 0x00000000ffffffff;
                hi = a & 0xffffffff00000000;
            }
            if (funct==25){ //mult
                int64_t a = (unsigned)RegFile[rs] * (unsigned)RegFile[rt];
                lo = a & 0x00000000ffffffff;
                hi = a & 0xffffffff00000000;
            }
            if (funct==16){ //mfhi
                RegFile[rd]=hi;
            }
            if (funct==18){ //mflo
                RegFile[rd]=lo;
            }
            if (funct==17){ //mthi
                hi=RegFile[rs];
            }
            if (funct==19){ //mtlo
                lo=RegFile[rs];
            }
            if (funct==34){ //and
                RegFile[rd] = RegFile[rs] & RegFile[rt];
            }
            if (funct==36){ //xor
                RegFile[rd] = RegFile[rs] ^ RegFile[rt];
            }
            if (funct==37){ //nor
                RegFile[rd] = ~(RegFile[rs] | RegFile[rt]);
            }
            if (funct==35){ //or
                RegFile[rd] = RegFile[rs] | RegFile[rt];
            }
            if (funct==42){ //slt
                if( RegFile[rs] < RegFile[rt]){
                    RegFile[rd]=1;
                }
                else{
                    RegFile[rd]=0;
                }
            }
            if (funct==43){ //sltu
                if ((unsigned)RegFile[rs] < (unsigned)RegFile[rt]) {
                RegFile[rd] = 0;
                } 
                else {
                RegFile[rd] = 1;
                }
            }
            if (funct==0){ //sll
                int shamt = 0;
                shamt = CurrentInstruction & 0x000007c0;
                RegFile[rd] = RegFile[rt] << shamt;
            }
            if (funct==4){ //sllv
                int s1 = RegFile[rs];
                RegFile[rd] = RegFile[rt] << s1;
            }
            if (funct==3){ //sra
                int shamt = 0;
                shamt = CurrentInstruction & 0x000007c0;
                RegFile[rd] = RegFile[rt] >> shamt;
            }
            if (funct==7){ //srav
                int s1 = RegFile[rs];
                RegFile[rd] = RegFile[rt] >> s1;
            }
            if (funct==2){ //srl
                int shamt = 0;
                shamt = CurrentInstruction & 0x000007c0;
                RegFile[rd] = (unsigned) RegFile[rt] >> shamt;
            }
            if (funct==6){ //srlv
                int s1 = RegFile[rs];
                RegFile[rd] = (unsigned) RegFile[rt] >> s1;
            }
            if (funct==9){ //jalr
                PC=PC+4;
                continue;
            }
            if (funct==8){ //jr
                PC=PC+4;
                continue;
            }
            if (funct==12){ //syscall
                SyscallExe((uint32_t) RegFile[2]);
            }
        }
        if (opcode==0x20000000){ //addi
            int32_t imme = CurrentInstruction & 0x0000ffff;
            RegFile[rt] = RegFile[rs] + imme;
        }
        if (opcode==0x24000000){ //addiu
            uint32_t imme = CurrentInstruction & 0x0000ffff;
            RegFile[rt] = (unsigned)RegFile[rs] + (unsigned)imme;
        }
        if (opcode==0x30000000){ //andi
            int32_t imme = CurrentInstruction & 0x0000ffff;
            RegFile[rt] = RegFile[rs] & imme;
        }
        if (opcode==0x38000000){ //xori
            int32_t imme = CurrentInstruction & 0x0000ffff;
            RegFile[rt] = RegFile[rs] ^ imme;
        }
        if (opcode==0x34000000){ //ori
            int32_t imme = CurrentInstruction & 0x0000ffff;
            RegFile[rt] = RegFile[rs] | imme;
        }
        if (opcode==0x28000000){ //slti
            int32_t imme = CurrentInstruction & 0x0000ffff;
            if( RegFile[rs] < imme){
                RegFile[rt]=1;
            }
            else{
                RegFile[rt]=0;
            }
        }
        if (opcode==0x2c000000){ //sltiu
            uint32_t imme = CurrentInstruction & 0x0000ffff;
            if( (unsigned) RegFile[rs] < (unsigned) imme){
                RegFile[rt]=1;
            }
            else{
                RegFile[rt]=0;
            }
        }
        if (opcode==0x10000000){ //beq
            PC=PC+4;
            continue;
        }
        if (opcode==0x50000000){ //beql
            if (RegFile[rs]==RegFile[rt]){
                PC=PC+4;
                continue;}
        }
        if (opcode==0x70000000){ //bgtz
            PC=PC+4;
            continue;
        }
        if (opcode==0x60000000){ //blez
            PC=PC+4;
            continue;
        }
        if (opcode==0x58000000){ //blezl
            if (RegFile[rs]<=0){
                PC=PC+4;
                continue;}
        }
        if (opcode==0x50000000){ //bne
            PC=PC+4;
            continue;
        }
        if (opcode==0x54000000){ //bnel
            if (RegFile[rs]!=RegFile[rt]){
                PC=PC+4;
                continue;}
        }
        else{ //nop
            PC=PC+4;
            continue;
        }
        PC=PC+4;
    /********************************/
                
        
    } //end fori
    
    
    //Close file pointers & free allocated Memory
    closeFDT();
    CleanUp();
    return 1;
}


