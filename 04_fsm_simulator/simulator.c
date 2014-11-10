/* FSM for LC */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
 
#define NUMMEMORY 65536 /* maximum number of words in memory */
#define NUMREGS 8 /* number of machine registers */
#define MAXLINELENGTH 1000
 
typedef struct stateStruct {
    int pc;
    int mem[NUMMEMORY];
    int reg[NUMREGS];
    int memoryAddress;
    int memoryData;
    int instrReg;
    int aluOperand;
    int aluResult;
    int numMemory;
} stateType;
 
void printState(stateType *, char *);
void run(stateType);
int memoryAccess(stateType *, int);
int convertNum(int);
 
int main(int argc, char *argv[])
{
    int i;
    char line[MAXLINELENGTH];
    stateType state;
    FILE *filePtr;
 
    if (argc != 2) {
        printf("error: usage: %s <machine-code file>n", argv[0]);
        exit(1);
    }
 
    /* initialize memories and registers */
    for (i=0; i<NUMMEMORY; i++) {
        state.mem[i] = 0;
    }
    for (i=0; i<NUMREGS; i++) {
        state.reg[i] = 0;
    }
 
    /* read machine-code file into instruction/data memory (starting at
        address 0) */
 
    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s\n", argv[1]);
        perror("fopen");
        exit(1);
    }
 
    for (state.numMemory=0; fgets(line, MAXLINELENGTH, filePtr) != NULL;
        state.numMemory++) {
        if (sscanf(line, "%d", state.mem+state.numMemory) != 1) {
            printf("error in reading address %d\n", state.numMemory);
            exit(1);
        }
        printf("memory[%d]=%d\n", state.numMemory, state.mem[state.numMemory]);
    }
 
    printf("\n");
 
    /* run never returns */
    state.pc=0;
    run(state);
 
    return(0);
}
 
void printState(stateType *statePtr, char *stateName)
{
    int i;
    static int cycle = 0;
    printf("\n@@@\nstate %s (cycle %d)\n", stateName, cycle++);
    printf("\tpc %d\n", statePtr->pc);
    printf("\tmemory:\n");
        for (i=0; i<statePtr->numMemory; i++) {
            printf("\t\tmem[ %d ] %d\n", i, statePtr->mem[i]);
        }
    printf("\tregisters:\n");
        for (i=0; i<NUMREGS; i++) {
            printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
        }
    printf("\tinternal registers:\n");
    printf("\t\tmemoryAddress %d\n", statePtr->memoryAddress);
    printf("\t\tmemoryData %d\n", statePtr->memoryData);
    printf("\t\tinstrReg %d\n", statePtr->instrReg);
    printf("\t\taluOperand %d\n", statePtr->aluOperand);
    printf("\t\taluResult %d\n", statePtr->aluResult);
}
 
/*
 * Access memory:
 *     readFlag=1 ==> read from memory
 *     readFlag=0 ==> write to memory
 * Return 1 if the memory operation was successful, otherwise return 0
 */
int memoryAccess(stateType *statePtr, int readFlag)
{
    static int lastAddress = -1;
    static int lastReadFlag = 0;
    static int lastData = 0;
    static int delay = 0;
 
    if (statePtr->memoryAddress < 0 || statePtr->memoryAddress >= NUMMEMORY) {
        printf("memory address out of range\n");
        exit(1);
    }
 
    /*
     * If this is a new access, reset the delay clock.
     */
    if ( (statePtr->memoryAddress != lastAddress) ||
             (readFlag != lastReadFlag) ||
             (readFlag == 0 && lastData != statePtr->memoryData) ) {
        delay = statePtr->memoryAddress % 3;
        lastAddress = statePtr->memoryAddress;
        lastReadFlag = readFlag;
        lastData = statePtr->memoryData;
    }
 
    if (delay == 0) {
        /* memory is ready */
        if (readFlag) {
            statePtr->memoryData = statePtr->mem[statePtr->memoryAddress];
        } else {
            statePtr->mem[statePtr->memoryAddress] = statePtr->memoryData;
        }
        return(1);
    } else {
        /* memory is not ready */
        delay--;
        return(0);
    }
}
 
int convertNum(int num)
{
    /* convert a 16-bit number into a 32-bit integer */
    if (num & (1 << 15) ) {
        num -= (1 << 16);
    }
    return(num);
}
 
// void printState(stateType *statePtr, char *stateName)
// int memoryAccess(stateType *statePtr, int readFlag)
//    readFlag = {1:read, 0:write}
//    return = {1:success, 0:failure} 
// int convertNum(int num)
//    convert a 16-bit number into a 32-bit integer

//  state
//  int pc;
//  int mem[NUMMEMORY];
//  int reg[NUMREGS];
//  int memoryAddress;
//  int memoryData;
//  int instrReg;
//  int aluOperand;
//  int aluResult;
//  int numMemory;


#define __STATE__(type) type: printState(&state, #type);
#define _READMEM() memoryAccess(&state, 1)
#define _WRITEMEM() memoryAccess(&state, 0)
#define _DISPATCH(opcode, label) if (((state.instrReg >> 22) & 0x7) == opcode) goto label
#define _REG_A state.reg[(state.instrReg >> 19) & 0x7]
#define _REG_B state.reg[(state.instrReg >> 16) & 0x7]
#define _REG_DEST state.reg[state.instrReg & 0x7]
#define _OFFSET convertNum(state.instrReg & 0x0000ffff)

void run(stateType state)
{
    int bus;

    __STATE__(fetch)
        bus = state.pc++;
        state.memoryAddress = bus;
        _READMEM(); 

    __STATE__(fetch_delay)
        if (_READMEM())
        {
            bus = state.memoryData;
            state.instrReg = state.memoryData;
            goto branch;
        }
        goto fetch_delay;

    __STATE__(branch)
        _DISPATCH(0x0, add);
        _DISPATCH(0x1, nand);
        _DISPATCH(0x2, lw);
        _DISPATCH(0x3, sw);
        _DISPATCH(0x4, beq);
        _DISPATCH(0x5, jalr);
        _DISPATCH(0x6, halt);
        _DISPATCH(0x7, noop);

    // add RegDest = RegA + RegB
    __STATE__(add)
        bus = _REG_A;
        state.aluOperand = bus;
        goto add_calc;

    __STATE__(add_calc)
        bus = _REG_B;
        state.aluResult = bus + state.aluOperand;
        goto add_done;

    __STATE__(add_done)
        bus = state.aluResult;
        _REG_DEST = bus;
        goto fetch;

    // nand RegDest = ~(RegA & RegB)
    __STATE__(nand)
        bus = _REG_A;
        state.aluOperand = bus;
        goto nand_calc;

    __STATE__(nand_calc)
        bus = _REG_B;
        state.aluResult = ~(bus & state.aluOperand);
        goto nand_done;

    __STATE__(nand_done)
        bus = state.aluResult;
        _REG_DEST = bus;
        goto fetch;

    // lw RegB = M[RegA + offset]
    __STATE__(lw)
        bus = _REG_A;
        state.aluOperand = bus;
        goto lw_addr;

    __STATE__(lw_addr)
        bus = convertNum(_OFFSET);
        state.aluResult = state.aluOperand + bus;
        goto lw_read;

    __STATE__(lw_read)
        bus = state.aluResult;
        state.memoryAddress = bus;
        _READMEM();
        goto lw_read_delay;

    __STATE__(lw_read_delay)
        if (_READMEM())
        {
            bus = state.memoryData;
            _REG_B = bus;
            goto fetch;
        }
        else
            goto lw_read_delay;

    // sw M[RegA + offset] = RegB
    __STATE__(sw)
        bus = _REG_A;
        state.aluOperand = bus;
        goto sw_addr;

    __STATE__(sw_addr)
        bus = convertNum(_OFFSET);
        state.aluResult = state.aluOperand + bus;
        goto sw_allocate;

    __STATE__(sw_allocate)
        bus = state.aluResult;
        state.memoryAddress = bus;
        goto sw_write;

    __STATE__(sw_write)
        bus = _REG_B;
        state.memoryData = bus;
        _WRITEMEM();
        goto sw_write_delay;

    __STATE__(sw_write_delay)
        if (_WRITEMEM())
            goto fetch;
        else
            goto sw_write_delay;

    // beq if RegA == RegB: PC = PC+1+offset 
    __STATE__(beq)
        bus = _REG_A;
        state.aluOperand = bus;
        goto beq_calc;

    __STATE__(beq_calc);
        bus = _REG_B;
        state.aluResult = state.aluOperand - bus;
        goto beq_judge;

    __STATE__(beq_judge)
        if (state.aluResult)
            goto fetch;
        else
        {
            // signed convert
            // cannot use converNum
            bus = _OFFSET;
            state.aluOperand = bus;
            goto beq_addr;
        }

    __STATE__(beq_addr)
        bus = state.pc;
        state.aluResult = state.aluOperand + bus;
        goto beq_pc;

    __STATE__(beq_pc)
        bus = state.aluResult;
        state.pc = bus;
        goto fetch;
    
    // jalr regb = pc+1; pc = rega
    __STATE__(jalr)
        bus = state.pc;
        _REG_B = bus;
        goto jalr_a;

    __STATE__(jalr_a)
        bus = _REG_A;
        state.pc = bus;
        goto fetch;
        
    // halt
    __STATE__(halt)
        return;

    // noop
    __STATE__(noop)
        goto fetch;
}
