#define iMemSize 1000

// R
typedef enum {
    ADD = 0x20,
    ADDU = 0x21,
    AND = 0x24,
    JR = 0x08,
    JALR = 0x09,
    NOR = 0x27,
    OR = 0x25,
    SLT = 0x2a,
    SLTU = 0x2b,
    SLL = 0x00,
    SRL = 0x02,
    SUB = 0x22,
    SUBU = 0x23
}enum_R;

// I
typedef enum {
    ADDI = 0x08,
    ADDIU = 0x09,
    ANDI = 0x0c,
    BEQ = 0x04,
    BNE = 0x05,
    LBU = 0x24,
    LHU = 0x25,
    LL = 0x30,
    LUI = 0x0f,
    LW = 0x23,
    ORI = 0x0d,
    SLTI = 0x0a,
    SLTIU = 0x0b,
    SB = 0x28,
    SC = 0x38,
    SH = 0x29,
    SW = 0x2b
} enum_I;

// J
typedef enum {
	J = 0x02,
	JAL = 0x03
} enum_J;

typedef struct instruction {
    char format;                // R, I, O
    u_int32_t instruction;      // Inst 31-00 (출력용)
    u_int8_t opcode : 6;        // Inst 31-26
    u_int8_t rs : 5;            // Inst 25-21
    u_int8_t rt : 5;            // Inst 20-16
    u_int8_t rd : 5;            // Inst 15-11
    u_int8_t shamt : 5;         // Inst 10-06
    u_int8_t func : 6;          // Inst 05-00
    u_int32_t addr : 26;     // Inst 25-00
    u_int16_t imm : 16;   // Inst 15-00
} Instruction;

typedef struct {
    u_int8_t RegDst : 1; // regMux의 input으로, inst.rd(true) 또는 inst.rt(false)를 반환
    u_int8_t ALUSrc : 1; // ReadData2(false) 또는 Immediate(true) 둘 중 하나로 ALU에 반환
    u_int8_t MemtoReg : 1; // memMux의 input으로, readData(true) 또는 aluResult(false)로 반환
    u_int8_t RegWrite : 1; // processRegister의 input으로, 레지스터를 모드를 결정함
    u_int8_t MemRead : 1; // processData의 input으로, 메모리에서 값을 읽을 것인지(true) 안 읽을 것인지(false) 결정함
    u_int8_t MemWrite : 1; // processData의 input으로, 메모리로 값을 쓸 것인지(true) 안 쓸 것인지(false) 결정함
    u_int8_t Branch : 1; // pcAndgate의 input으로, (pc+4)+(imm<<2) 값을 pc로 할 것인지(true), (pc+4)로 할 것인지(false) 결정함
    u_int8_t Jump : 1; // processJAddress의 input으로, pc를 Jump Address 값으로 할 것인지(true), pcAndgate의 값으로 할 것인지(false) 결정함
    u_int8_t JR : 1; // processJAddress의 input으로, PC를 reg[$ra]값으로 할 것인지(true), 다른 값으로 할 것인지(false) 결정함
    u_int8_t ALUOp; // ALUOp의 input으로, opcode를 읽고 ALUControl의 값을 정함
    u_int8_t IFFlush : 1; // IF 레지스터를 0으로 초기화 하는 Flush
    u_int8_t IDFlush : 1; // ID 레지스터를 0으로 초기화 하는 Flush
    u_int8_t EXFlush : 1; // EX 레지스터를 0으로 초기화 하는 Flush
} Control;

typedef struct ifid{
	Instruction inst;
	u_int32_t PC;
}IFID;

typedef struct idex{
	Instruction inst;
    u_int32_t PC;
	u_int32_t BranchAddr;
    u_int32_t immediate;
	Control ctr;
	u_int32_t data1,data2;
	u_int32_t temp;
    u_int32_t ALUResult;
}IDEX;

typedef struct exmem{
	Instruction inst;
    u_int32_t PC;
	u_int32_t JumpAddr, BranchAddr;
    u_int32_t  immediate;
	Control ctr;
	u_int32_t data1, data2;
    u_int32_t temp;
	u_int32_t WriteReg, ALUResult, ReadData;
}EXMEM;

typedef struct memwb{
	Instruction inst;
    u_int32_t PC;
	u_int32_t JumpAddr, BranchAddr;
    u_int32_t  immediate;
	Control ctr;
	u_int32_t data1, data2;
    u_int32_t temp;
	u_int32_t WriteReg, ALUResult, ReadData;
}MEMWB;

// 레지스터 이름 열거형
typedef enum {
    $zero = 0, $at, $v0, $v1, $a0, $a1, $a2, $a3, $t0, $t1, $t2, $t3, $t4, $t5, $t6, $t7, $s0, $s1, $s2, $s3, $s4, $s5, $s6, $s7, $t8, $t9, $k0, $k1, $gp, $sp, $fp, $ra
} RegName_enum;

// 레지스터 이름 문자형
const char* RegName_str[] = {
    "$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"
};

// 데이터 메모리 배열
u_int32_t Memory[0xFFFFFFF];

// 명령어 메모리 배열
u_int32_t iMem[iMemSize];

// 레지스터 배열
u_int32_t R[32];

// PC 및 Cycle 카운트 변수
u_int32_t PC = 0;
u_int32_t cycle=0;

// 명령어 갯수, 상태 변화 카운트 변수
u_int32_t inst_count = 0;
u_int32_t exe_count = 0;
u_int32_t mem_count = 0;
u_int32_t reg_count = 0;
u_int32_t jump_count = 0;
u_int32_t R_count, I_count, J_count = 0;
u_int32_t branch_count, not_branch_count = 0;

// 파이프라인 레지스터들
IFID ifid[2];
IDEX idex[2];
EXMEM exmem[2];
MEMWB memwb[2];


// 이진 파일에서 데이터를 한 줄씩(32비트) 읽은 다음 메모리 배열에 저장하는 함수
void readMipsBinary(FILE *file);

// R, I 타입의 명령어를 분석하여 연산하는 함수
void processALU();

// BEQ나 BNE와 같은 분기 명령어를 처리하는 함수
void BranchPrediction(u_int32_t opcode, u_int32_t data1, u_int32_t data2);
void BranchTaken();

// Binary를 명령어로 분할하여 저장하는 함수
void parseInstruction();

// OPCODE에 따라 Control값을 구분하는 함수
void processControl(u_int32_t opcode);

// 16비트 immdidate를 32비트 Unsigned int로 형변환하는 함수
u_int32_t signExtend(u_int16_t immediate);

// 각 Stage별 함수
u_int8_t IF();
u_int8_t ID();
u_int8_t EX();
u_int8_t MEM();
u_int8_t WB();