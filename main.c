#include <stdio.h>
#include <string.h>
#include "struct.h"

// 이진 파일에서 데이터를 한 줄씩(32비트) 읽은 다음 메모리 배열에 저장하는 함수
void readMipsBinary(FILE *file);

// 명령어의 타입마다 ALU를 분할해서 함수로 정의함
void ALU_R(u_int32_t func, u_int32_t data1, u_int32_t data2);
void ALU_I(u_int32_t opcode, u_int32_t data1, u_int32_t data2);
void processjAddress(u_int32_t opcode);
u_int32_t pcJump();


// BEQ나 BNE와 같은 분기 명령어를 처리하는 함수
void BranchTaken();
void BranchPrediction(u_int32_t opcode, u_int32_t data1, u_int32_t data2);
u_int32_t pcBranch();

// 각 파이프라인 레지스터의 쓰기값을 이전 읽기값으로 초기화
void Update();

// Binary를 명령어로 분할하여 저장하는 함수
void parseInstruction();

// OPCODE에 따라 Control값을 구분하는 함수
// 주요 명령어만 정의하였으나 명령어에 따라서 작동 가능
void processControl(u_int32_t opcode);

// 16비트 immdidate를 32비트 Unsigned int로 형변환하는 함수
u_int32_t signExtend(u_int16_t immediate);

// 각 Stage별 함수
u_int8_t IF();
u_int8_t ID();
u_int8_t EX();
u_int8_t MEM();
u_int8_t WB();

int main(){
    FILE *file;

    file = fopen("input.bin", "r");
    if (file == NULL){
        printf("파일을 열 수 없습니다.\n");
        return 0;
    }

	// 파일로부터 binary를 읽어서 Memory에 저장하고 파일을 닫음
	readMipsBinary(file);
	fclose(file);

    u_int32_t ret = 0;

	// 레지스터 초기화
	R[$sp] = 0x10000000;
	R[$ra] = 0xFFFFFFFF; 

	while(1){

		// 반환값이 1이 아닌 경우 즉시 종료
		ret = IF();	
		if(ret != 1) break;
		ret = ID();
		if(ret != 1) break;
		ret = EX();	
		if(ret != 1) break;	
		ret = MEM();
		if(ret != 1) break;
		ret = WB();
		if(ret != 1) break;

		// 각 레지스터의 출력값을 입력값으로 초기화 (다음 명령어 실행을 위함)
		ifid[1] = ifid[0];
		idex[1] = idex[0];
		exmem[1] = exmem[0];
		memwb[1] = memwb[0];
		
		printf("cycle: %d\n",cycle);
		cycle++;
	}

    printf("\n===========================================\n");
    printf("| Return value (%s): 0x%x(%d)\n", RegName_str[$v0], R[$v0], R[$v0]);
    printf("| Total Cycle: %u\n", cycle);
    printf("| Executed 'R' instruction: %u\n", R_count);
    printf("| Executed 'I' instruction: %u\n", I_count);
    printf("| Executed 'J' instruction: %u\n", J_count);
    printf("| Number of Branch Taken: %u\n", branch_count);
	printf("| Number of Branch Not Taken Count: %d\n",not_branch_count);
    printf("| Number of Memory Access Instructions: %u\n", mem_count);
	printf("| Jump Count: %d\n",jump_count);
    printf("===========================================\n");

	return 0;
}

void readMipsBinary(FILE *file){
    u_int32_t ret = 0;
    u_int32_t i = 0;
    u_int32_t binary;

    while(1){
        u_int32_t data = 0;
		// u_int32_t (32비트) 크기를 한줄로 하여 읽음
        ret = fread(&data, sizeof(u_int32_t), 1, file);

		// 읽은 data를 iMem 배열 한칸마다 넣음
        iMem[i++] = ntohl(data);

		// 정상적으로 32비트를 읽지 못한 경우 종료
        if(ret != 1) break;
    }
	inst_count = i;
}

void parseInstruction() {

	// opcode 먼저 계산
	u_int32_t binary = ifid[1].inst.instruction;
    idex[0].inst.opcode = (binary >> 26) & 0x3F;
    // R 명령어 분리
    if (idex[0].inst.opcode == 0) {
		R_count++;
        idex[0].inst.format = 'R';
        idex[0].inst.opcode = (binary >> 26) & 0x3F;
        idex[0].inst.rs = (binary >> 21) & 0x1F;
        idex[0].inst.rt = (binary >> 16) & 0x1F;
        idex[0].inst.rd = (binary >> 11) & 0x1F;
        idex[0].inst.shamt = (binary >> 6) & 0x1F;
        idex[0].inst.func = binary & 0x3F;

        if (binary == 0) idex[0].inst.format = 'N'; // NOP;
    } 
    // J 명령어 분리
    else if (((binary >> 26) & 0x3F) == 2 || ((binary >> 26) & 0x3F) == 3) {
        idex[0].inst.format = 'J';
        idex[0].inst.opcode = (binary >> 26) & 0x3F;
		J_count++;

        // ctr.Jump
        if (idex[0].inst.opcode == 2) {
        }
        // Juml And Link
        else if (idex[0].inst.opcode == 3) {
            idex[0].inst.rs = $ra;
            idex[0].inst.rd = $ra;
        }
        idex[0].inst.addr = binary & 0x3FFFFFF;
    } 
    // I 명령어 분리
    else {
		I_count++;
        idex[0].inst.format = 'I';
        idex[0].inst.opcode = (binary >> 26) & 0x3F;
        idex[0].inst.rs = (binary >> 21) & 0x1F;
        idex[0].inst.rt = (binary >> 16) & 0x1F;
        idex[0].inst.imm = binary & 0xFFFF;
    }

    // N이라면 출력하지 않고 넘어감
    if(idex[0].inst.format == 'N') {
        printf("\t\t(Bubble or Stalled)\n");
        idex[0].inst.instruction = 0;
    } else {
		// 출력용 값
		idex[0].inst.instruction = binary;

		printf("\t\t0x%08x || ", idex[0].inst.instruction);
		if (idex[0].inst.format == 'R') {
			printf("type : R, opcode : 0x%08x, rs : 0x%08x (R[%s]=0x%08x), rt : 0x%08x (R[%s]=0x%08x), rd : 0x%08x (R[%s]=0x%08x), shamt : 0x%08x, funct : 0x%08x\n", idex[0].inst.opcode, idex[0].inst.rs, RegName_str[idex[0].inst.rs], R[idex[0].inst.rs], idex[0].inst.rt, RegName_str[idex[0].inst.rt], R[idex[0].inst.rt], idex[0].inst.rd, RegName_str[idex[0].inst.rd], R[idex[0].inst.rd], idex[0].inst.shamt, idex[0].inst.func);
		}
		else if (idex[0].inst.format == 'I') {
			printf("type : I, opcode : 0x%08x, rs : 0x%08x (R[%s]=0x%08x), rt : 0x%08x (R[%s]=0x%08x), immediate : 0x%08x\n", idex[0].inst.opcode, idex[0].inst.rs, RegName_str[idex[0].inst.rs], R[idex[0].inst.rs], idex[0].inst.rt, RegName_str[idex[0].inst.rt], R[idex[0].inst.rt], idex[0].inst.imm);
		}
		else if (idex[0].inst.format == 'J') {
			printf("type : J, opcode : 0x%08x, rs : 0x%08x (R[%s]=0x%08x), Address : 0x%08x\n", idex[0].inst.opcode, idex[0].inst.rs, RegName_str[idex[0].inst.rs], R[idex[0].inst.rs], idex[0].inst.addr);
		}
	}
}

void processControl(u_int32_t opcode){
	u_int32_t ret = 1;

	if(opcode == 0x0){
        idex[0].ctr.RegDst = 1;
    } else {
        idex[0].ctr.RegDst = 0;
    }

    if((opcode == J) || (opcode == JAL)) {
        idex[0].ctr.Jump = 1;
    } else {
        idex[0].ctr.Jump = 0;
    }

	// Branch
    if((opcode == BEQ) || (opcode == BNE)) {
        idex[0].ctr.Branch = 1;
    } else {
        idex[0].ctr.Branch = 0;
    }

    if(opcode == LW) {
        idex[0].ctr.MemtoReg = 1;
        idex[0].ctr.MemRead = 1;
    } else {
        idex[0].ctr.MemtoReg = 0; 
        idex[0].ctr.MemRead = 0;
    }

    if(opcode == SW) {
        idex[0].ctr.MemWrite = 1;
    } else {
        idex[0].ctr.MemWrite = 0;
    }

	if((opcode != 0) && (opcode != BEQ) && (opcode != BNE)){
        idex[0].ctr.ALUSrc=1;
    } else {
        idex[0].ctr.ALUSrc=0;
    }

	if((opcode != SW) && (opcode != BEQ)&& (opcode != BNE) && (opcode != JR) && (opcode != J) && (opcode != JAL)) {
        idex[0].ctr.RegWrite = 1;
    } else {
        idex[0].ctr.RegWrite = 0;
    }	
}

u_int32_t signExtend(u_int16_t immediate) {
    // 16bit immediate값을 32bit로 signExtend
    u_int32_t extend = (u_int32_t)(int32_t)(int16_t)immediate;

    return extend;
}

u_int32_t pcJump(){
	idex[0].JumpAddr = (ifid[1].PC & 0xF0000000) | (idex[0].inst.addr <<2);
	u_int32_t j = idex[0].JumpAddr;
	return j;
}

u_int32_t pcBranch(){
	idex[1].BranchAddr = idex[1].SignExtImm << 2;
	u_int32_t b = idex[1].BranchAddr;
	return b;
}

void processjAddress(u_int32_t opcode){
    if(opcode == J){
		PC = pcJump();
		exe_count++;
		jump_count++;
	} else if (opcode == JAL){	
		R[$ra] = idex[0].PC+8;
		PC = pcJump();
		exe_count++;
		jump_count++;
	}
}

void ALU_R(u_int32_t func,u_int32_t data1, u_int32_t data2){
    switch(idex[1].inst.func){
	case ADD:
		exmem[0].ALUResult = data1 + data2;
		exe_count++;
		break;

	case ADDU:
		exmem[0].ALUResult = data1 + data2;
		exe_count++;
		break;

    case AND: 
		exmem[0].ALUResult = data1 & data2;
		exe_count++;
		break;

	case JR:
		PC=data1;
		memset(&ifid[0], 0, sizeof(IFID));
		memset(&idex[0], 0, sizeof(IDEX));
		break;

	case JALR:
		PC=data1;
		memset(&ifid[0], 0, sizeof(IFID));
		memset(&idex[0], 0, sizeof(IDEX));
		break;

    case NOR:
		exmem[0].ALUResult = !(data1|data2);
		exe_count++;
		break;

	case OR:
		exmem[0].ALUResult = (data1|data2);
		exe_count++;
		break;

	case SLT:
		exmem[0].ALUResult = ((data1<data2) ? 1:0);
		exe_count++;
		break;

	case SLTU:
		exmem[0].ALUResult = (((u_int32_t)data1<(u_int32_t)data2) ? 1:0);
		exe_count++;
		break;

	case SLL:
		exmem[0].ALUResult = data2 << idex[1].inst.shamt;	
		exe_count++;
		break;

    case SRL:
		exmem[0].ALUResult = data2 >> idex[1].inst.shamt;
		exe_count++;
		break;

	case SUB:
		exmem[0].ALUResult = data1 - data2;
		exe_count++;
		break;

    case SUBU:
		exmem[0].ALUResult = (u_int32_t)data1 - (u_int32_t)data2;
		exe_count++;
		break;

	default:
		break;
	}
}

void ALU_I(u_int32_t opcode, u_int32_t data1, u_int32_t data2) {
    u_int32_t temp;
    
    switch (opcode) {
        case ADDI:
        case ADDIU:
            exmem[0].ALUResult = data1 + idex[1].SignExtImm;
            exe_count++;
            break;

        case ANDI:
            exmem[0].ALUResult = data1 & idex[1].ZeroExtImm;
            exe_count++;
            break;

        case LBU:
        case LHU:
            exmem[0].ALUResult = (Memory[(idex[1].inst.rs + idex[1].SignExtImm) / 4]) & 0x000000FF;
            exe_count++;
            break;

        case LL:
            exmem[0].inst.rt = Memory[(idex[1].inst.rs + idex[1].SignExtImm) / 4];
            exe_count++;
            break;

        case LUI:
            exmem[0].ALUResult = idex[1].inst.imm << 16;
            exe_count++;
            break;

        case LW:
            exmem[0].temp = data1 + idex[1].SignExtImm;
            exe_count++;
            break;

        case ORI:
            exmem[0].ALUResult = data1 | idex[1].ZeroExtImm;
            exe_count++;
            break;

        case SLTI:
            exmem[0].ALUResult = (data1 < idex[1].SignExtImm) ? 1 : 0;
            exe_count++;
            break;

        case SLTIU:
            exmem[0].ALUResult = (data1 < (u_int32_t)idex[1].SignExtImm) ? 1 : 0;
            exe_count++;
            break;

        case SB:
		case SH:
            temp = Memory[(idex[1].inst.rs + idex[1].SignExtImm) / 4];
            temp = temp & 0xFFFFFF00;
            temp = (idex[1].inst.rt & 0x000000FF) | temp;
            Memory[(idex[1].inst.rs + idex[1].SignExtImm) / 4] = temp;
            break;

        case SC:
            temp = idex[1].inst.rs + idex[1].SignExtImm;
            exmem[0].ALUResult = idex[1].inst.rt;
            Memory[temp / 4] = exmem[0].ALUResult;
            exmem[0].inst.rt = 1;
            break;

        case SW:
            exmem[0].ALUResult = data2;
            exmem[0].temp = data1 + idex[1].SignExtImm;
            exe_count++;
            break;

        default:
            break;
    }
}

void BranchPrediction(u_int32_t opcode, u_int32_t data1, u_int32_t data2){
	// Branch not taken
    switch(opcode){
		case BEQ:
			if(data1 == data2){
				BranchTaken();
			} else {
				exe_count++;
				not_branch_count++;
			}
			break;
		case BNE:
			if(data1 != data2){
				BranchTaken();
			} else {
				exe_count++;
				not_branch_count++;
			}
			break;
		
    	default:
			break;
	}
}

void BranchTaken() {
	u_int32_t b = pcBranch();
	PC = idex[1].PC + b + 4;
	//flush
	memset(&ifid[0],0,sizeof(IFID));
	memset(&idex[0],0,sizeof(IDEX));		
	exe_count++;
	branch_count++;
}

u_int8_t IF(){
	// PC가 종료시점이면 0 반환
	if (PC == 0xFFFFFFFF) return 0;
	else {
		// instruction을 다루기 쉽게 Memory[PC/4]로부터 바이너리 통째로 가져옴
		ifid[0].inst.instruction = iMem[(PC)/4];
		printf("\t[IF]\n\t\t[IM] PC: 0x%0X -> 0x%08X\n", PC, ifid[0].inst.instruction);
	}

	// ifid pc는 나중에 업데이트됨 (중복 방지)
	ifid[0].PC = PC;
	// Update PC
	PC = PC + 4;

	return 1;
}

u_int8_t ID(){
	u_int32_t ret = 1;
	u_int32_t opcode;

	printf("\t[ID]\n");
	// Instruction을 유기적으로 구분하는 함수
	parseInstruction();


	// sign extend
	u_int32_t imm = ifid[1].inst.instruction;
	idex[0].SignExtImm = signExtend(imm);

	opcode = idex[0].inst.opcode;

	// Update latch
	idex[0].inst.instruction = ifid[1].inst.instruction;
	idex[0].PC = ifid[1].PC;

    processjAddress(opcode);
	processControl(opcode);

	return ret;
}

u_int8_t EX(){
	printf("\t[EX]\n");

	u_int32_t data1,data2;

	idex[0].data1 = R[idex[0].inst.rs];
    idex[0].data2 = R[idex[0].inst.rt];
    
	idex[1].ZeroExtImm = (idex[1].inst.instruction & 0x0000FFFF);

	// data forwarding/bypass
	if((idex[1].inst.rs != 0) && (idex[1].inst.rs == exmem[0].WriteReg) && (exmem[0].ctr.RegWrite)) {
		printf("\t\tFORWARDING! Rs Changed to EXMEM.ALUResult\n");
		idex[1].data1  = exmem[0].ALUResult;
	} else if((idex[1].inst.rs != 0) && (idex[1].inst.rs == memwb[0].WriteReg) && (memwb[0].ctr.RegWrite)){	
		if(memwb[0].ctr.MemtoReg==1){
			printf("\t\tFORWARDING! Rs Changed to MEMWB.ReadData\n");
			idex[1].data1 = memwb[0].ReadData;
		} else {
			printf("\t\tFORWARDING! Rs Changed to MEMWB.ALUResult\n");
			idex[1].data1 = memwb[0].ALUResult; 
		}
	}
	if((idex[1].inst.rt != 0) && (idex[1].inst.rt == exmem[0].WriteReg) && (exmem[0].ctr.RegWrite)){
		printf("\t\tFORWARDING! Rt Changed to EXMEM.ALUResult\n");
		idex[1].data2 = exmem[0].ALUResult;
	} else if((idex[1].inst.rt!=0) && (idex[1].inst.rt == memwb[0].WriteReg) && (memwb[0].ctr.RegWrite)){
		if(memwb[1].ctr.MemtoReg == 1){
			printf("\t\tFORWARDING! Rt Changed to MEMWB.ReadData\n");
			idex[1].data2 = memwb[0].ReadData;
		} else {
			printf("\t\tFORWARDING! Rt Changed to MEMWB.ALUResult\n");
			idex[1].data2 = memwb[0].ALUResult;
		}
	}

	data1 = idex[1].data1;
	data2 = idex[1].data2;

	// RegDst가 true일 경우 R 명령어, ALUSrc가 True일 경우 I 명령어, Branch가 True일 경우 BEQ 또는 BNE로 규정함
	if(idex[1].ctr.RegDst == 1){
        ALU_R(idex[1].inst.func,data1,data2);
	} else if (idex[1].ctr.ALUSrc==1){
        ALU_I(idex[1].inst.opcode,data1,data2);
	} else if (idex[1].ctr.Branch==1){
        BranchPrediction(idex[1].inst.opcode,data1,data2);
    }

	// RegDst에 따라 WriteRegister를 다르게 설정함
	if(idex[1].ctr.RegDst == 1){
		exmem[0].WriteReg = idex[1].inst.rd;
	} else {
		exmem[0].WriteReg = idex[1].inst.rt;
	}

	printf("\t\t");
	// 각 명령어에 따라 출력값을 다르게 설정
	if(idex[1].inst.format == 'R') {
		printf("Rs: 0x%x (R[%d](%s)=0x%x), Rt: 0x%x (R[%d](%s)=0x%x), Rd: 0x%x (R[%d](%s)=0x%x), Shamt: 0x%x, Func: 0x%x", idex[1].inst.rs,idex[1].inst.rs, RegName_str[idex[1].inst.rs], R[idex[1].inst.rs], idex[1].inst.rt,idex[1].inst.rt, RegName_str[idex[1].inst.rt], R[idex[1].inst.rt], idex[1].inst.rd,idex[1].inst.rd, RegName_str[idex[1].inst.rd], R[idex[1].inst.rd], idex[1].inst.shamt, idex[1].inst.func);
		printf(", ALUResult : 0x%x\n", exmem[0].ALUResult);
	} else if (idex[1].inst.format == 'I') {
		printf("rs : 0x%x (R[%d](%s)=0x%x), rt : 0x%x (R[%d](%s)=0x%x), immediate : 0x%08x\n", idex[1].inst.rs, idex[1].inst.rs, RegName_str[idex[1].inst.rs], R[idex[1].inst.rs], idex[1].inst.rt, idex[1].inst.rt, RegName_str[idex[1].inst.rt], R[idex[1].inst.rt], idex[1].inst.imm);
	} else {
		printf("(Bubble or Stalled)\n");
	}

	// IDEX 레지스터의 출력을 EXMEM 레지스터의 입력으로 초기화
	exmem[0].inst = idex[1].inst;
    exmem[0].PC = idex[1].PC;
	exmem[0].ctr = idex[1].ctr;
	exmem[0].data1 = data1;
	exmem[0].data2 = data2;

	return 1;
}

u_int8_t MEM(){
	printf("\t[MEM]\n\t\t");

	// MemRead가 true일 경우 Load Word, 반대일 경우 Store Word
    if(exmem[1].ctr.MemRead==1){ // LW
		exmem[1].ReadData=Memory[exmem[1].temp/4];
		printf("(LOAD) R[%d](%s) <- 0x%x\n", exmem[1].inst.rt, RegName_str[exmem[1].inst.rt], exmem[1].ReadData);
		mem_count++;
	} else if (exmem[1].ctr.MemWrite==1){ // SW
		Memory[exmem[1].temp/4]=exmem[1].data2;
		printf("(STORE) 0x%x -> M[0x%x]\n", exmem[1].data2, Memory[exmem[1].temp/4]);
		mem_count++;
	} else {
		printf("(Bubble or Stalled)\n");
	}
	
	// EXMEM 레지스터의 출력을 MEMWB 레지스터의 입력으로 초기화
	memwb[0].inst = exmem[1].inst;
	memwb[0].ctr = exmem[1].ctr;
	memwb[0].data1 = exmem[1].data1;
	memwb[0].data2 = exmem[1].data2;
	memwb[0].ALUResult = exmem[1].ALUResult;
	memwb[0].WriteReg = exmem[1].WriteReg;
	memwb[0].ReadData = exmem[1].ReadData;

	return 1;
}

u_int8_t WB(){
	printf("\t[WB]\n");

	if(memwb[0].ctr.RegWrite ==1 ){
		if(memwb[0].ctr.MemtoReg == 1){
			R[memwb[0].WriteReg] = memwb[0].ReadData;
			printf("\t\t(ctr.MemtoReg) R[%d](%s) <- 0x%x\n", memwb[0].WriteReg, RegName_str[memwb[0].WriteReg], R[memwb[0].WriteReg]);
		} else{
			if(memwb[0].WriteReg != 0){
				R[memwb[0].WriteReg] = memwb[0].ALUResult;
				printf("\t\t(ALUToReg) R[%d](%s) <- 0x%x\n", memwb[0].WriteReg, RegName_str[memwb[0].WriteReg], R[memwb[0].WriteReg]);
            } else {
                R[memwb[0].WriteReg] = 0;
				printf("\t\tNO REG WRITE\n");
            }
		}
		reg_count++;
	}
	return 1;
}
