#pragma warning (disable : 4996)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "register.h"
#include "instruction.h"

#define U_SIZE sizeof(unsigned int)

// instruction
unsigned int opcode, rs, rt, rd, shamt, funct, immediate, address;
unsigned int inst; // 페치 단계에서 명령어를 읽어올 때 쓰일 변수

// control unit variables
unsigned int RegDst, RegWrite, ALUSrc, PCSrc, MemRead, MemWrite, MemtoReg, Jump, Branch;

// register & memory global variables
unsigned int Memory[0x4000000] = { 0, };
unsigned int Register[32]      = { 0, };
unsigned int PC                = 0;
int change_pc_val              = 0;
unsigned int temp1, temp2      = 0;

// instruction count variables
int cycle_count  = 0;
int R_count      = 0;
int I_count      = 0;
int J_count      = 0;
int MemAcc_count = 0;
int branch_count = 0;

// function
void init();                                         
void read_mem(FILE* file);                           
int fetch(FILE* file);                               
int decode();                                        
int Control_Signal();
int jump_Addr();
int Sign_Extend(int immediate);
int branch_Addr(int immediate);
int R_Type(int funct);
int I_Type(int opcode);
int J_Type(int opcode);
int Exe_and_WB();

int main() {
    FILE* file;
    int ret = 0;

    char* filename;
    filename = "input02.bin";

    file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: There is no file");
        return 0;
    }

    read_mem(file);
    fclose(file);

    // initiallize
    init();

    // Run program
    while (1) {
        ret = fetch(file);
        if (ret <= 0) break;

        ret = decode();
        if (ret <= 0) break;

        ret = Control_Signal();
        if (ret <= 0) break;

        ret = Exe_and_WB();
        if (ret <= 0) break;

        cycle_count++;
        printf("\n\n");
    }

    printf("================================================================================\n");
    printf("Return value (r2)                    : %d\n", Register[v0]);
    printf("Total cycle                          : %d\n", cycle_count);
    printf("Executed 'R' instruction             : %d\n", R_count);
    printf("Executed 'I' instruction             : %d\n", I_count);
    printf("Executed 'J' instruction             : %d\n", J_count);
    printf("Number of branch taken               : %d\n", branch_count);
    printf("Number of memory access instructions : %d\n", MemAcc_count);
    printf("=================================================================================\n");

    return 0;
}

// fuction
void init() {
    Register[ra] = 0xFFFFFFFF;
    Register[sp] = 0x10000000;
}

void read_mem(FILE* file) {
    unsigned int ret = 0;
    int i = 0;
    int memVal;

    while (true) {
        int data    = 0;                                    // 파일을 읽어오는 용도로 사용할 변수
        ret         = fread(&data, 1, sizeof(int), file);   // data 변수에 파일로부터 4바이트 만큼 데이터를 읽어옴. ret 변수에는 fread 함수의 읽은 바이트 수가 저장됨
        memVal      = ((data & 0xFF000000) >> 24) |
                      ((data & 0x00FF0000) >> 8)  |
                      ((data & 0x0000FF00) << 8)  |
                      ((data & 0x000000FF) << 24);
        Memory[i++] = memVal;                               // Memory에 값을 저장
  
        if (ret != 4) break;                                // 4바이트를 읽지 못했다면 루프 탈출
    }
}

int fetch(FILE* file) {
    int ret = 1;
    if (PC == 0xFFFFFFFF) return 0;                         // PC가 0xFFFFFFFF이면 종료
    inst = Memory[PC / 4];

    printf("Cycle[%03d](PC : 0x%08X)=======================================================\n\n", cycle_count + 1, PC);
    if (inst == 0) {
        PC += 4;
        cycle_count++;
        return fetch(file);  // nop 명령어일 경우 다시 fetch 호출
    }
    printf("[Fetch Inxtruction] %08X\n", inst);
    return ret;
}

int decode() {
    int ret = 1;

    opcode    = (inst & 0xFC000000);                        // 비트 연산을 이용해 명령어 추출
    opcode    = (opcode >> 26) & 0x3F;
    rs        = (inst & 0x03E00000);
    rs        = (rs >> 21) & 0x1F;
    rt        = (inst & 0x001F0000);
    rt        = (rt >> 16) & 0x1F;
    rd        = (inst & 0x0000F800);
    rd        = (rd >> 11) & 0x1F;
    shamt     = (inst & 0x000007C0);
    shamt     = shamt >> 6;
    funct     = (inst & 0x0000003F);
    immediate = (inst & 0x0000FFFF);
    address   = (inst & 0x03FFFFFF);

    switch (opcode) {                               // opcode로 명령어 유형을 판별
    case 0:                                         // R타입
        printf("[Decode Instruction] type : R\n");
        printf("\tOPCODE : 0x%X, RS : 0x%X (R[%d] = 0x%X), RT : 0x%X (R[%d] = 0x%X), RD : 0x%X (R[%d] = 0x%X), SHAMT : 0x%X, FUNCT : 0x%X\n", opcode, rs, rs, Register[rs], rt, rt, Register[rt], rd, rd, Register[rd], shamt, funct);
        R_count++;
        break;

    case J:
        printf("[Decode Instruction] type : J\n");
        printf("\tOPCODE : 0x%X, ADRESS : 0x%X\n", opcode, address);
        J_count++;
        break;

    case JAL:
        printf("[Decode Instruction] type : J\n");
        printf("\tOPCODE : 0x%X, ADRESS : 0x%X\n", opcode, address);
        J_count++;
        break;

    default:
        printf("[Decode Instruction] type : I\n");
        printf("\tOPCODE : 0x%X, RS : 0x%X (R[%d] = 0x%X), RT : 0x%X (R[%d] = 0x%X), IMM : 0x%X\n", opcode, rs, rs, Register[rs], rt, rt, Register[rt], immediate);
        I_count++;
        break;
    }

    return ret;
}

int Control_Signal() {
    int ret = 1;

    // MUX
    if (opcode == 0) {
        RegDst = 1;
    }
    else {
        RegDst = 0;
    }

    if ((opcode == J) || (opcode == JAL)) {
        Jump = 1;
    }
    else {
        Jump = 0;
    }

    if ((opcode == BEQ) || (opcode == BNE)) {
        Branch = 1;
    }
    else {
        Branch = 0;
    }

    if (opcode == LW) {
        MemtoReg = 1;
        MemRead = 1;
    }
    else {
        MemtoReg = 0;
        MemRead = 0;
    }

    if (opcode == SW) {
        MemWrite = 1;
    }
    else {
        MemWrite = 0;
    }

    if ((opcode != 0) && (opcode != BEQ) && (opcode != BNE)) {
        ALUSrc = 1;
    }
    else {
        ALUSrc = 0;
    }


    if ((opcode != SW) && (opcode != BEQ) && (opcode != BNE) && (opcode != J) && (opcode != JR)) {
        RegWrite = 1;
    }
    else {
        RegWrite = 0;
    }

    return ret;
}

int Sign_Extend(int immediate) {
    int SignExtImm;
    int SignBit = immediate >> 15;
    if (SignBit == 1) {
        SignExtImm = 0xFFFF0000 | immediate;
    }
    else {
        SignExtImm = 0x0000FFFF & immediate;
    }
    return SignExtImm;
}

int jump_Addr() {
    int j = ((PC + 4) & 0xF0000000) | (address << 2);
    return j;
}

int branch_Addr(int immediate) {
    int b = Sign_Extend(immediate) << 2;
    return b;
}

int R_Type(int funct) {
    switch (funct) {
    case ADD:
        Register[rd] = (int)((int)Register[rs] + (int)Register[rt]);
        printf("[Write Back] R[%d] <- 0x%X\n", rd, Register[rd]);
        break;

    case ADDU:
        Register[rd] = Register[rs] + Register[rt];
        printf("[Write Back] R[%d] <- 0x%X\n", rd, Register[rd]);
        break;

    case AND:
        Register[rd] = Register[rs] & Register[rt];
        printf("[Write Back] R[%d] <- 0x%X\n", rd, Register[rd]);
        break;

    case JR:
        PC = Register[rs];
        change_pc_val = 1;
        printf("[Write Back] PC <- 0x%X\n", PC);
        break;

    case JALR:
        Register[rd] = PC + 4;
        PC = Register[rs];
        change_pc_val = 1;
        printf("[Write Back] PC <- 0x%X\n", PC);
        break;

    case NOR:
        Register[rd] = ~(Register[rs] | Register[rt]);
        printf("[Write Back] R[%d] <- 0x%X\n", rd, Register[rd]);
        break;

    case OR:
        Register[rd] = Register[rs] | Register[rt];
        printf("[Write Back] R[%d] <- 0x%X\n", rd, Register[rd]);
        break;

    case SLT:
        Register[rd] = ((int)Register[rs] < (int)Register[rt]) ? 1 : 0;
        printf("[Write Back] R[%d] <- 0x%X\n", rd, Register[rd]);
        break;

    case SLTU:
        Register[rd] = (Register[rs] < Register[rt]) ? 1 : 0;
        printf("[Write Back] R[%d] <- 0x%X\n", rd, Register[rd]);
        break;

    case SLL:
        Register[rd] = Register[rt] << shamt;
        printf("[Write Back] R[%d] <- 0x%X\n", rd, Register[rd]);
        break;

    case SRL:
        Register[rd] = Register[rt] >> shamt;
        printf("[Write Back] R[%d] <- 0x%X\n", rd, Register[rd]);
        break;

    case SUB:
        Register[rd] = (int)((int)Register[rs] - (int)Register[rt]);
        printf("[Write Back] R[%d] <- 0x%X\n", rd, Register[rd]);
        break;

    case SUBU:
        Register[rd] = Register[rs] - Register[rt];
        printf("[Write Back] R[%d] <- 0x%X\n", rd, Register[rd]);
        break;

    default:
        return -1;
    }

    return change_pc_val;
}

int J_Type(int opcode) {
    unsigned int JumpAddr = jump_Addr();
    switch (opcode) {
    case J:
        PC = JumpAddr;
        change_pc_val = 1;
        printf("[Jump] PC = 0x%X", PC);
        break;

    case JAL:
        Register[ra] = (unsigned int)PC + 8;
        PC = JumpAddr;
        change_pc_val = 1;
        printf("[Jump] PC = 0x%X", PC);
        break;

    default:
        return -1;
    }

    return change_pc_val;
}

int I_Type(int opcode) {
    int ZeroExtImm = immediate;
    int SignExtImm = Sign_Extend(immediate);
    int BranchAddr = branch_Addr(immediate);
    switch (opcode) {
    case ADDI:
        Register[rt] = (int)((int)Register[rs] + SignExtImm);
        printf("[Write Back] R[%d] <- 0x%X", rt, Register[rt]);
        break;

    case ADDIU:
        Register[rt] = (unsigned int)Register[rs] + SignExtImm;
        printf("[Write Back] R[%d] <- 0x%X", rt, Register[rt]);
        break;

    case ANDI:
        Register[rt] = Register[rs] & ZeroExtImm;
        printf("[Write Back] R[%d] <- 0x%X", rt, Register[rt]);
        break;

    case BEQ:
        if (Register[rs] == Register[rt]) {
            PC = PC + 4 + BranchAddr;
            change_pc_val = 1;
            branch_count++;
        }
        
        printf("[Branch] PC : 0x%X", PC);
        break;

    case BNE:
        if (Register[rs] != Register[rt]) {
            PC = PC + 4 + BranchAddr;
            change_pc_val = 1;
            branch_count++;
        }
        
        printf("[Branch] PC : 0x%X", PC);
        break;

    case LBU:
        temp1 = Register[rs] + (unsigned int)SignExtImm & 0x000000FF;
        Register[rt] = (Memory[temp1 / U_SIZE]) & 0x000000FF;
        printf("[Load] R[%d] <- Mem[0x%X] = 0x%X", rt, temp1 / U_SIZE, Register[rt]);
        break;

    case LHU:
        temp1 = Register[rs] + (unsigned int)SignExtImm & 0x0000FFFF;
        Register[rt] = (Memory[temp1 / U_SIZE]) & 0x0000FFFF;
        printf("[Load] R[%d] <- Mem[0x%X] = 0x%X", rt, temp1 / U_SIZE, Register[rt]);
        break;

    case LL:
        Register[rt] = Memory[(Register[rs] + (unsigned int)SignExtImm) / U_SIZE];
        printf("[Load] R[%d] <- Mem[0x%X] = 0x%X", rt, (Register[rs] + (unsigned int)SignExtImm) / U_SIZE, Register[rt]);
        break;

    case LUI:
        Register[rt] = immediate << 16;
        printf("[Load] R[%d] <- 0x%X", rt, Register[rt]);
        break;

    case LW:
        Register[rt] = Memory[(Register[rs] + (unsigned int)SignExtImm) / U_SIZE];
        MemAcc_count++;
        printf("[Load] R[%d] <- Mem[0x%X] = 0x%X", rt, (Register[rs] + (unsigned int)SignExtImm) / U_SIZE, Register[rt]);
        break;

    case ORI:
        Register[rt] = Register[rs] | ZeroExtImm;
        printf("[Write Back] R[%d] <- 0x%X", rt, Register[rt]);
        break;

    case SLTI:
        Register[rt] = (Register[rs] < SignExtImm) ? 1 : 0;
        printf("[Write Back] R[%d] <- 0x%X", rt, Register[rt]);
        break;

    case SLTIU:
        Register[rt] = (Register[rs] < (unsigned int)SignExtImm) ? 1 : 0;
        printf("[Write Back] R[%d] <- 0x%X", rt, Register[rt]);
        break;

    case SB:
        temp1 = Register[rs] + (unsigned int)SignExtImm;
        temp2 = Memory[temp1 / U_SIZE];
        temp2 = temp2 & 0xFFFFFF00;
        temp2 = (Register[rt] & 0x000000FF) | temp2;
        Memory[temp1 / U_SIZE] = temp2;
        printf("[Store] Mem[0x%X] <- 0x%X", temp1 / U_SIZE, temp2);
        break;

    case SC:
        temp1 = (unsigned int)SignExtImm;
        temp2 = Register[rs] + temp1;
        temp1 = Register[rt];
        Memory[temp2 / U_SIZE] = temp1;
        Register[rt] = 1;
        printf("[Store] Mem[0x%X] <- 0x%X", temp2 / U_SIZE, temp1);
        break;

    case SH:
        temp1 = Register[rs] + (unsigned int)SignExtImm;
        temp2 = Memory[temp1 / U_SIZE];
        temp2 = temp2 & 0xFFFF0000;
        temp2 = (Register[rt] & 0x0000FFFF) | temp2;
        Memory[temp1 / U_SIZE] = temp2;
        printf("[Store] Mem[0x%X] <- 0x%X", temp1 / U_SIZE, temp2);
        break;

    case SW:
        temp1 = Register[rs] + (unsigned int)SignExtImm;
        Memory[temp1 / U_SIZE] = Register[rt];
        MemAcc_count++;
        printf("[Store] Mem[0x%X] <- R[%d] = 0x%X", temp1 / U_SIZE, rt, Register[rt]);
        break;

    default:
        return -1;
    }
    return change_pc_val;
}

int Exe_and_WB() {
    int ret = 1;
    change_pc_val = 0;

    if (opcode == 0x00) {
        change_pc_val = R_Type(funct);
    }
    else if (opcode == 0x02 || opcode == 0x03) {
        change_pc_val = J_Type(opcode);
    }
    else {
        change_pc_val = I_Type(opcode);
    }

    // PC Update
    if (change_pc_val == 0) {
        printf("[PC Update] PC <- 0x%08X = 0x%08X + 4\n", PC + 4, PC);
        PC = PC + 4;
    }
    else if (change_pc_val == -1) {
        ret = change_pc_val;
    }
    else if (change_pc_val == 1) {
        printf("[PC Update] (JUMP) PC <- 0x%08X\n", PC);
    }

    return ret;
}