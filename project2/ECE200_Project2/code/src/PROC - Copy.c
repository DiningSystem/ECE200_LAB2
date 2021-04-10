
#include <inttypes.h>
#include <stdint.h>
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
        unsigned int opcode=0; //get the opcode
        unsigned int rs=0,rt=0, funct=0;//get the rs,rt funct part
        opcode = CurrentInstruction & 0xfc000000;
        funct= CurrentInstruction & 0x0000003f;
        rs = CurrentInstruction & 0x03e00000;
        rt = CurrentInstruction & 0x001f0000;
        rs = rs >> 21;
        rt = rt >> 16;
        if (opcode==0){ //R-type
            unsigned int rd =0;
            rd = CurrentInstruction & 0x0000f800;
            rd = rd >> 11;
            if (funct==32){ //add
                RegFile[rd] = RegFile[rs] + RegFile[rt];
            }
            if (funct==33){ //addu
                RegFile[rd] = RegFile[rs] + RegFile[rt];
            }
            if (funct==34){ //sub
                RegFile[rd] = RegFile[rs] - RegFile[rt];
            }
            if (funct==35){ //subu
                RegFile[rd] = RegFile[rs] - RegFile[rt];
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
                lo = (uint32_t)a & 0x00000000ffffffff;
                hi = (uint32_t)((a>>32) & 0x00000000ffffffff);
            }
            if (funct==25){ //multu
                uint64_t a = (unsigned)RegFile[rs] * (unsigned)RegFile[rt];
                lo = (uint32_t)a & 0x00000000ffffffff;
                hi = (uint32_t)((a>>32) & 0x00000000ffffffff);
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
                unsigned int shamt = 0;
                shamt = CurrentInstruction & 0x000007c0;
                shamt = shamt >> 6;
                RegFile[rd] = RegFile[rt] << shamt;
            }
            if (funct==4){ //sllv
                int s1 = RegFile[rs];
                RegFile[rd] = RegFile[rt] << s1;
            }
            if (funct==3){ //sra
                unsigned int shamt = 0;
                shamt = CurrentInstruction & 0x000007c0;
                shamt = shamt >> 6;
                RegFile[rd] = RegFile[rt] >> shamt;
            }
            if (funct==7){ //srav
                int s1 = RegFile[rs];
                RegFile[rd] = RegFile[rt] >> s1;
            }
            if (funct==2){ //srl
                unsigned int shamt = 0;
                shamt = CurrentInstruction & 0x000007c0;
                shamt = shamt >> 6;
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
                SyscallExe(RegFile[2]);
            }
        }
        if (opcode==0x20000000){ //addi
            int32_t imme = CurrentInstruction & 0x0000ffff;
            imme <<=16;
            imme >>=16;
            RegFile[rt] = RegFile[rs] + imme;
        }
        if (opcode==0x24000000){ //addiu
            int32_t imme = CurrentInstruction & 0x0000ffff;
            imme <<=16;
            imme >>=16;
            RegFile[rt] = RegFile[rs] + imme;
        }
        if (opcode==0x30000000){ //andi
            uint32_t imme = CurrentInstruction & 0x0000ffff;
            RegFile[rt] = RegFile[rs] & imme;
        }
        if (opcode==0x38000000){ //xori
            uint32_t imme = CurrentInstruction & 0x0000ffff;
            RegFile[rt] = RegFile[rs] ^ imme;
        }
        if (opcode==0x34000000){ //ori
            uint32_t imme = CurrentInstruction & 0x0000ffff;
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
        if (opcode==0x04000000){ //REGIMM=1
                PC=PC+4;
                continue;
        }
        if (opcode==0x08000000){ //J
                PC=PC+4;
                continue;
        }
        if (opcode==0x0c000000){ //JAL
                PC=PC+4;
                continue;
        }
        if (opcode==0x3c000000){ //LUI
                RegFile[rt] = (RegFile[rt] & 0x00000000) + ((CurrentInstruction & 0x0000ffff) << 16);
        }
        if (opcode==0x80000000){ //LB
                int32_t imme = CurrentInstruction & 0x0000ffff;
                imme <<=16;
                imme >>=16;
                uint32_t addr= RegFile[rs] + imme;
                RegFile[rt] = readByte(addr, false);
        }
        if (opcode==0x90000000){ //LBU
                int32_t imme = CurrentInstruction & 0x0000ffff;
                imme <<=16;
                imme >>=16;
                uint32_t addr= RegFile[rs] + imme;
                RegFile[rt] = (unsigned)readByte(addr, false);
        }
        if (opcode==0x84000000){ //LH
                int32_t imme = CurrentInstruction & 0x0000ffff;
                imme <<=16;
                imme >>=16;
                uint32_t addr= RegFile[rs] + imme;
                RegFile[rt] = readByte(addr, false);
                int8_t nextbyte = readByte(addr+1, false);
                RegFile[rt] = RegFile[rt] + (nextbyte << 8);  
        }
        if (opcode==0x94000000){ //LHU
                int32_t imme = CurrentInstruction & 0x0000ffff;
                imme <<=16;
                imme >>=16;
                uint32_t addr= RegFile[rs] + imme;
                RegFile[rt] = (unsigned)readByte(addr, false);
                int8_t nextbyte = (unsigned)readByte(addr+1, false);
                RegFile[rt] = RegFile[rt] + (nextbyte << 8);  
        }
        if (opcode==0x8c000000){ //LW
                int32_t imme = CurrentInstruction & 0x0000ffff;
                imme <<=16;
                imme >>=16;
                uint32_t addr= RegFile[rs] + imme;
                RegFile[rt] = readWord(addr, false);
        }
        if (opcode==0xa0000000){ //SB
                int32_t imme = CurrentInstruction & 0x0000ffff;
                imme <<=16;
                imme >>=16;
                uint32_t addr= RegFile[rs] + imme;
                writeByte(addr,RegFile[rt], false);
        }
        if (opcode==0xa4000000){ //SH
                int32_t imme = CurrentInstruction & 0x0000ffff;
                imme <<=16;
                imme >>=16;
                uint32_t addr= RegFile[rs] + imme;
                int8_t firstbyte = RegFile[rt] & 0x000000ff;
                int8_t secondbyte = (RegFile[rt] & (0x0000ff00)) >> 8;
                writeByte(addr,firstbyte, false);
                writeByte(addr+1,secondbyte, false);
        }
        if (opcode==0xac000000){ //SW
                int32_t imme = CurrentInstruction & 0x0000ffff;
                imme <<=16;
                imme >>=16;
                uint32_t addr= RegFile[rs] + imme;
                writeWord(addr,RegFile[rt], false);
        }
         if (opcode==0x88000000){ //LWL

		int32_t imme = CurrentInstruction & 0x0000ffff;
		int loadAddr = RegFile[rs] + imme;
		int32_t data = 0;
		int temp = 1; //number of iterations
		int32_t byteData = 0;

		do {
			int shift = 32 - 8*temp;
			byteData = (int32_t)readByte(loadAddr,false);
			byteData = byteData << shift;
			data = data + byteData;
			loadAddr++;
			temp++;
		} while ((loadAddr % 4) != 0);

		int shift = 8*temp;
		RegFile[rt] = RegFile[rt] << shift;
		RegFile[rt] = RegFile[rt] >> shift;
		RegFile[rt] = RegFile[rt] + data;
			
        }
	if (opcode==0x98000000){ //LWR
		int32_t imme = CurrentInstruction & 0x0000ffff;
		int loadAddr = RegFile[rs] + imme;
		int32_t data = 0;
		int temp = 0; //for shift in data
		int32_t byteData = 0;

		int startAddr = loadAddr >> 2;
		startAddr = startAddr << 2;

		while(startAddr <= loadAddr){
			int shift = 8*temp;
			byteData = (int32_t)readByte(loadAddr,false);
			data = data << shift;
			data = data + byteData;
			startAddr++;
			temp++;
		}

		int shift = 8*(temp+1);
		RegFile[rt] = RegFile[rt] >> shift;
		RegFile[rt] = RegFile[rt] << shift;
		RegFile[rt] = RegFile[rt] + data;	
	}
        if (opcode==0xA8000000){ //SWL
		int32_t imme = CurrentInstruction & 0x0000ffff;
		int32_t storeAddr = RegFile[rs] + imme;
		int32_t data = RegFile[rt];
		int32_t offset = storeAddr%4;
		if(offset == 0 ){
			writeWord(storeAddr,data,false);
		}else{
			int32_t alignedAddr = storeAddr - offset;
			data = data >> offset;
			int32_t mem = readWord(alignedAddr, false);
			int memShift = 8*(4 - offset);
			mem = mem >> memShift;
			mem = mem << memShift;
			data = data + mem;
			writeWord(alignedAddr,data,false);
		}
	}
	if (opcode==0xB8000000){ //SWR
		int32_t imme = CurrentInstruction & 0x0000ffff;
		int32_t storeAddr = RegFile[rs] + imme;
		int32_t data = RegFile[rt];
		int32_t offset = storeAddr%4;
		int32_t alignedAddr = storeAddr - offset;
		if(offset == 3){
			writeWord(alignedAddr,data,false);
		}else{
			int shift = 8*(3 - offset);
			data = data << shift;
			int32_t mem = readWord(alignedAddr, false);
			int memShift = 8 * (offset+ 1);
			mem = mem << memShift;
			mem = mem >> memShift;
			data = data + mem;
			writeWord(alignedAddr,data,false);
		}
	}
                

        //here I want to do the branch delay slot
        int on_bit=1; // check for delay slot, zero means yes, one means no
        uint32_t old_addr =PC-4;
        uint32_t OldInstruction = readWord(old_addr,false);
        unsigned int opcode1=0; //get the opcode
        unsigned int rs1=0,rt1=0, funct1=0;//get the rs,rt funct part
        opcode1 = OldInstruction & 0xfc000000;
        funct1= OldInstruction & 0x0000003f;
        rs1 = OldInstruction & 0x03e00000;
        rt1 = OldInstruction & 0x001f0000;
        rs1 = rs1 >> 21;
        rt1 = rt1 >> 16;
        if (opcode1==0){ //R-type
            unsigned int rd1 = OldInstruction & 0x0000f100;
            rd1 = rd1 >> 11;
            if (funct1==9){ //jalr
                
                RegFile[rd1] = PC + 4;
                newPC = RegFile[rs1];
                on_bit=0;
            }
            if (funct1==8){ //jr
                
                newPC = RegFile[rs1];
                on_bit=0;
            }
        } 
        if (opcode1==0x10000000){ //beq
            if (RegFile[rs1]==RegFile[rt1]){
                int32_t imme1 = OldInstruction & 0x0000ffff;
                imme1 <<=16;
                imme1 >>=14;
                newPC = PC + imme1 ;
                on_bit=0;
            }
        }
        if (opcode1==0x50000000){ //beql
            int32_t imme1 = OldInstruction & 0x0000ffff;
            imme1 <<=16;
            imme1 >>=14;
            newPC = PC + imme1 ;
            on_bit=0;
        }
        if (opcode1==0x70000000){ //bgtz
            if (RegFile[rs1] > 0){
                int32_t imme1 = OldInstruction & 0x0000ffff;
                imme1 <<=16;
                imme1 >>=14;
                newPC = PC + imme1 ;
                on_bit=0;
            }
        }
        if (opcode1==0x60000000){ //blez
            if (RegFile[rs1] <= 0){
                int32_t imme1 = OldInstruction & 0x0000ffff;
                imme1 <<=16;
                imme1 >>=14;
                newPC = PC + imme1 ;
                on_bit=0;
            }
        }
        if (opcode1==0x58000000){ //blezl
            int32_t imme1 = OldInstruction & 0x0000ffff;
            imme1 <<=16;
            imme1 >>=14;
            newPC = PC + imme1 ;
            on_bit=0;
        }
        if (opcode1==0x50000000){ //bne
            if (RegFile[rs1]!=RegFile[rt1]){
                int32_t imme1 = OldInstruction & 0x0000ffff;
                imme1 <<=16;
                imme1 >>=14;
                newPC = PC + imme1 ;
                on_bit=0;
            }
        }
        if (opcode1==0x54000000){ //bnel
            int32_t imme1 = OldInstruction & 0x0000ffff;
            imme1 <<=16;
            imme1 >>=14;
            newPC = PC + imme1 ;
            on_bit=0;
        }
        if (opcode1==0x04000000){ //REGIMM=1
                if (RegFile[rt1]==1) { //bgez
                    if (RegFile[rs1]>=0){
                        int32_t imme1 = OldInstruction & 0x0000ffff;
                        imme1 <<=16;
                        imme1 >>=14;
                        newPC = PC + imme1 ;
                        on_bit=0;
                    }
                }
                if (RegFile[rt1]==17) { //bgezal
                    if (RegFile[rs1]>=0){
                        int32_t imme1 = OldInstruction & 0x0000ffff;
                        imme1 <<=16;
                        imme1 >>=14;
                        newPC = PC + imme1 ;
                        RegFile[31]=PC+4;
                        on_bit=0;
                    }
                }
                 if (RegFile[rt1]==0) { //bltz
                    if (RegFile[rs1]<0){
                        int32_t imme1 = OldInstruction & 0x0000ffff;
                        imme1 <<=16;
                        imme1 >>=14;
                        newPC = PC + imme1 ;
                        on_bit=0;
                    }
                }
                if (RegFile[rt1]==16) { //bltzal
                    if (RegFile[rs1]<0){
                        int32_t imme1 = OldInstruction & 0x0000ffff;
                        imme1 <<=16;
                        imme1 >>=14;
                        newPC = PC + imme1 ;
                        RegFile[31]=PC+4;
                        on_bit=0;
                    }
                }
        }
        if (opcode1==0x08000000){ //J
                uint32_t ins1 = OldInstruction & 0x03ffffff;
                newPC = (PC & 0xf0000000) | (ins1 << 2); 
                printf("%x",newPC);
                on_bit=0;
        }
        if (opcode1==0x0c000000){ //JAL
                uint32_t ins1 = OldInstruction & 0x03ffffff;
                newPC = (PC & 0xf0000000) | (ins1 << 2);
                printf("%x",newPC); 
                RegFile[31]=PC+4;
                on_bit=0;
        }

        //check for on bit
        if (on_bit==0){
            PC=newPC;
        }
        else if (on_bit==1){
            PC=PC+4;
        }
        
    /********************************/
                
        
    } //end fori
    
    
    //Close file pointers & free allocated Memory
    closeFDT();
    CleanUp();
    return 1;
}


