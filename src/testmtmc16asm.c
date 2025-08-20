#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

#define PAIV_MTMC_IMPLEMENTATION
#include "paiv_mtmc16.h"
#define PAIV_MTMCASM_IMPLEMENTATION
#include "paiv_mtmc16asm.h"
#include "platform.c"


int MtmcExecutableLoad(FILE* file, struct MtmcExecutable* exe) {
    FatalError();
    return 1;
}


int MtmcAssemblerLinkExecutable(struct MtmcExeObject* exe, FILE* output) {
    FatalError();
    return 1;
}


static struct Platform _platform = {};


static void _TestLoadProgramAt(struct MtmcEmu* emu, const char* program, i16 offset) {
    int bufsize = strlen(program);
    char buf[bufsize+1];
    strncpy(buf, program, bufsize + 1);

    struct MtmcExeObject obj = {0};
    FILE* source = fmemopen(buf, bufsize, "rb");
    assert(source != NULL);
    int res = MtmcAssemblerCompileSource(source, &obj, NULL);
    fclose(source);
    assert(res == 0);

    struct MtmcExecutable exe = (struct MtmcExecutable) {
        .format = obj.format,
        .codesize = obj.codesize,
        .datasize = obj.datasize,
    };
    memcpy(exe.code, obj.code, obj.codesize);
    memcpy(exe.data, obj.data, obj.datasize);

    PlatformInit(&_platform);
    emu->platform = &_platform;
    // emu->trace_level = 1;
    MtmcLoad(emu, &exe);
    if (offset > 0) {
        memmove(&emu->memory[offset], &emu->memory[0],
            exe.codesize + exe.datasize);
    }
}

static void _TestLoadProgram(struct MtmcEmu* emu, const char* program) {
    _TestLoadProgramAt(emu, program, 0);
}

static void _TestStep(struct MtmcEmu* emu) {
    emu->status = MtmcEmuStatus_EXECUTING;
    MtmcPulse(emu, 1);
}

static void testSysCall(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sys exit");
    assert(MtmcGetStatus(&emu) == MtmcEmuStatus_READY);
    MtmcRun(&emu);
    assert(MtmcGetStatus(&emu) == MtmcEmuStatus_FINISHED);
}

static void testMov(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "mov t1 t0");
    MtmcSetRegisterValue(&emu, T0, 10);
    MtmcSetRegisterValue(&emu, T1, 0);
    MtmcRun(&emu);
    assert(10 == MtmcGetRegisterValue(&emu, T0));
    assert(10 == MtmcGetRegisterValue(&emu, T1));
}

static void testInc(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "inc t0");
    MtmcSetRegisterValue(&emu, T0, 0);
    MtmcRun(&emu);
    assert(1 == MtmcGetRegisterValue(&emu, T0));
}

static void testInc3(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "inc t0 3");
    MtmcSetRegisterValue(&emu, T0, 0);
    MtmcRun(&emu);
    assert(3 == MtmcGetRegisterValue(&emu, T0));
}

static void testDec(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "dec t0");
    MtmcSetRegisterValue(&emu, T0, 0);
    MtmcRun(&emu);
    assert(-1 == MtmcGetRegisterValue(&emu, T0));
}

static void testDec3(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "dec t0 3");
    MtmcSetRegisterValue(&emu, T0, 0);
    MtmcRun(&emu);
    assert(-3 == MtmcGetRegisterValue(&emu, T0));
}

static void testSeti(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "seti t0 3");
    MtmcSetRegisterValue(&emu, T0, 0);
    MtmcRun(&emu);
    assert(3 == MtmcGetRegisterValue(&emu, T0));
}

/*
static void testNoOp(void) {
    struct MtmcEmu emu = {0};
    _MtmcTestInit(&emu);

    short[] originalRegisters = Arrays.copyOf(MtmcRegisterFile, MtmcRegisterFile.length);
    byte[] originalMemory = Arrays.copyOf(MtmcMemory, MtmcMemory.length);

    short noop = 0b0000111111111111; // no-op
    MtmcExecInstruction(&emu, noop);

    boolean registersEqual = Arrays.equals(originalRegisters, MtmcRegisterFile);
    assert(registersEqual != 0);
    boolean memoryEqual = Arrays.equals(originalMemory, MtmcMemory);
    assert(memoryEqual != 0);
}
*/

static void testAdd(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "add t0 t1");
    MtmcSetRegisterValue(&emu, T0, 5);
    MtmcSetRegisterValue(&emu, T1, 10);
    MtmcRun(&emu);
    assert(15 == MtmcGetRegisterValue(&emu, T0));
    assert(10 == MtmcGetRegisterValue(&emu, T1));
}

static void testImmediateAdd(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "addi t0 10");
    MtmcSetRegisterValue(&emu, T0, 5);
    MtmcRun(&emu);
    assert(15 == MtmcGetRegisterValue(&emu, T0));
}

static void testSub(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sub t0 t1");
    MtmcSetRegisterValue(&emu, T0, 10);
    MtmcSetRegisterValue(&emu, T1, 3);
    MtmcRun(&emu);
    assert(7 == MtmcGetRegisterValue(&emu, T0));
    assert(3 == MtmcGetRegisterValue(&emu, T1));
}

static void testImmediateSub(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "subi t0 3");
    MtmcSetRegisterValue(&emu, T0, 10);
    MtmcRun(&emu);
    assert(7 == MtmcGetRegisterValue(&emu, T0));
}

static void testMul(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "mul t0 t1");
    MtmcSetRegisterValue(&emu, T0, 10);
    MtmcSetRegisterValue(&emu, T1, 5);
    MtmcRun(&emu);
    assert(50 == MtmcGetRegisterValue(&emu, T0));
    assert(5 == MtmcGetRegisterValue(&emu, T1));
}

static void testImmediateMul(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "muli t0 5");
    MtmcSetRegisterValue(&emu, T0, 10);
    MtmcRun(&emu);
    assert(50 == MtmcGetRegisterValue(&emu, T0));
}

static void testDiv(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "div t0 t1");
    MtmcSetRegisterValue(&emu, T0, 10);
    MtmcSetRegisterValue(&emu, T1, 5);
    MtmcRun(&emu);
    assert(2 == MtmcGetRegisterValue(&emu, T0));
    assert(5 == MtmcGetRegisterValue(&emu, T1));
}

static void testImmediateDiv(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "divi t0 5");
    MtmcSetRegisterValue(&emu, T0, 10);
    MtmcRun(&emu);
    assert(2 == MtmcGetRegisterValue(&emu, T0));
}

static void testMod(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "mod t0 t1");
    MtmcSetRegisterValue(&emu, T0, 10);
    MtmcSetRegisterValue(&emu, T1, 5);
    MtmcRun(&emu);
    assert(0 == MtmcGetRegisterValue(&emu, T0));
    assert(5 == MtmcGetRegisterValue(&emu, T1));
}

static void testImmediateMod(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "modi t0 7");
    MtmcSetRegisterValue(&emu, T0, 10);
    MtmcRun(&emu);
    assert(3 == MtmcGetRegisterValue(&emu, T0));
}

static void testAnd(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "and t0 t1");
    MtmcSetRegisterValue(&emu, T0, 0b110);
    MtmcSetRegisterValue(&emu, T1, 0b011);
    MtmcRun(&emu);
    assert(0b010 == MtmcGetRegisterValue(&emu, T0));
    assert(0b011 == MtmcGetRegisterValue(&emu, T1));
}

static void testImmediateAnd(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "andi t0 0b011");
    MtmcSetRegisterValue(&emu, T0, 0b110);
    MtmcRun(&emu);
    assert(0b010 == MtmcGetRegisterValue(&emu, T0));
}

static void testOr(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "or t0 t1");
    MtmcSetRegisterValue(&emu, T0, 0b100);
    MtmcSetRegisterValue(&emu, T1, 0b001);
    MtmcRun(&emu);
    assert(0b101 == MtmcGetRegisterValue(&emu, T0));
    assert(0b001 == MtmcGetRegisterValue(&emu, T1));
}

static void testImmediateOr(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "ori t0 0b001");
    MtmcSetRegisterValue(&emu, T0, 0b100);
    MtmcRun(&emu);
    assert(0b101 == MtmcGetRegisterValue(&emu, T0));
}

static void testXor(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "xor t0 t1");
    MtmcSetRegisterValue(&emu, T0, 0b101);
    MtmcSetRegisterValue(&emu, T1, 0b011);
    MtmcRun(&emu);
    assert(0b110 == MtmcGetRegisterValue(&emu, T0));
    assert(0b011 == MtmcGetRegisterValue(&emu, T1));
}

static void testImmediateXor(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "xori t0 0b011");
    MtmcSetRegisterValue(&emu, T0, 0b101);
    MtmcRun(&emu);
    assert(0b110 == MtmcGetRegisterValue(&emu, T0));
}

static void testShiftLeft(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "shl t0 t1");
    MtmcSetRegisterValue(&emu, T0, 0b0101);
    MtmcSetRegisterValue(&emu, T1, 1);
    MtmcRun(&emu);
    assert(0b01010 == MtmcGetRegisterValue(&emu, T0));
    assert(1 == MtmcGetRegisterValue(&emu, T1));
}

static void testImmediateShiftLeft(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "shli t0 1");
    MtmcSetRegisterValue(&emu, T0, 0b0101);
    MtmcRun(&emu);
    assert(0b01010 == MtmcGetRegisterValue(&emu, T0));
}

static void testShiftRight(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "shr t0 t1");
    MtmcSetRegisterValue(&emu, T0, 0b0101);
    MtmcSetRegisterValue(&emu, T1, 1);
    MtmcRun(&emu);
    assert(0b0010 == MtmcGetRegisterValue(&emu, T0));
    assert(1 == MtmcGetRegisterValue(&emu, T1));
}

static void testImmediateShiftRight(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "shri t0 1");
    MtmcSetRegisterValue(&emu, T0, 0b0101);
    MtmcRun(&emu);
    assert(0b0010 == MtmcGetRegisterValue(&emu, T0));
}

static void testMin(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "min t0 t1");
    MtmcSetRegisterValue(&emu, T0, 20);
    MtmcSetRegisterValue(&emu, T1, 10);
    MtmcRun(&emu);
    assert(10 == MtmcGetRegisterValue(&emu, T0));
    assert(10 == MtmcGetRegisterValue(&emu, T1));
}

static void testMin2(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "min t0 t1");
    MtmcSetRegisterValue(&emu, T0, 10);
    MtmcSetRegisterValue(&emu, T1, 20);
    MtmcRun(&emu);
    assert(10 == MtmcGetRegisterValue(&emu, T0));
    assert(20 == MtmcGetRegisterValue(&emu, T1));
}

static void testImmediateMin(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "mini t0 10");
    MtmcSetRegisterValue(&emu, T0, 20);
    MtmcRun(&emu);
    assert(10 == MtmcGetRegisterValue(&emu, T0));
}

static void testMax(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "max t0 t1");
    MtmcSetRegisterValue(&emu, T0, 20);
    MtmcSetRegisterValue(&emu, T1, 10);
    MtmcRun(&emu);
    assert(20 == MtmcGetRegisterValue(&emu, T0));
    assert(10 == MtmcGetRegisterValue(&emu, T1));
}

static void testMax2(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "max t0 t1");
    MtmcSetRegisterValue(&emu, T0, 10);
    MtmcSetRegisterValue(&emu, T1, 20);
    MtmcRun(&emu);
    assert(20 == MtmcGetRegisterValue(&emu, T0));
    assert(20 == MtmcGetRegisterValue(&emu, T1));
}

static void testImmediateMax(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "maxi t0 20");
    MtmcSetRegisterValue(&emu, T0, 10);
    MtmcRun(&emu);
    assert(20 == MtmcGetRegisterValue(&emu, T0));
}

static void testBitwiseNot(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "not t0");
    MtmcSetRegisterValue(&emu, T0, 0b010101);
    MtmcRun(&emu);
    assert(~0b010101 == MtmcGetRegisterValue(&emu, T0));
}

static void testNotWithZero(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "lnot t0");
    MtmcSetRegisterValue(&emu, T0, 0);
    MtmcRun(&emu);
    assert(1 == MtmcGetRegisterValue(&emu, T0));
}

static void testNotWithNonZeros1(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "lnot t0");
    MtmcSetRegisterValue(&emu, T0, 1);
    MtmcRun(&emu);
    assert(0 == MtmcGetRegisterValue(&emu, T0));
}

static void testNotWithNonZeros10(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "lnot t0");
    MtmcSetRegisterValue(&emu, T0, 10);
    MtmcRun(&emu);
    assert(0 == MtmcGetRegisterValue(&emu, T0));
}

static void testNegate(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "neg t0");
    MtmcSetRegisterValue(&emu, T0, 10);
    MtmcRun(&emu);
    assert(-10 == MtmcGetRegisterValue(&emu, T0));
}

static void testImmediateInstruction(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "imm add t0 10");
    MtmcSetRegisterValue(&emu, T0, 7);
    MtmcRun(&emu);
    assert(17 == MtmcGetRegisterValue(&emu, T0));
}

static void testPushSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "push t0 sp");
    MtmcSetRegisterValue(&emu, T0, 5);
    MtmcRun(&emu);
    int newStackAddress = Mtmc_MEMORY_SIZE - 2;
    assert(newStackAddress == MtmcGetRegisterValue(&emu, SP));
    assert(5 == MtmcFetchWordFromMemory(&emu, newStackAddress));
}

static void testPush(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "push t0");
    MtmcSetRegisterValue(&emu, T0, 5);
    MtmcRun(&emu);
    int newStackAddress = Mtmc_MEMORY_SIZE - 2;
    assert(newStackAddress == MtmcGetRegisterValue(&emu, SP));
    assert(5 == MtmcFetchWordFromMemory(&emu, newStackAddress));
}

static void testPopSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "pop t0 sp");
    MtmcSetRegisterValue(&emu, T0, 0);
    MtmcSetRegisterValue(&emu, SP, 100);
    MtmcWriteWordToMemory(&emu, 100, (short) 10);
    MtmcRun(&emu);
    assert(10 == MtmcGetRegisterValue(&emu, T0));
    assert(102 == MtmcGetRegisterValue(&emu, SP));
}

static void testPop(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "pop t0");
    MtmcSetRegisterValue(&emu, T0, 0);
    MtmcSetRegisterValue(&emu, SP, 100);
    MtmcWriteWordToMemory(&emu, 100, (short) 10);
    MtmcRun(&emu);
    assert(10 == MtmcGetRegisterValue(&emu, T0));
    assert(102 == MtmcGetRegisterValue(&emu, SP));
}

static void testDuplicateSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "dup sp");
    MtmcSetRegisterValue(&emu, SP, 100);
    MtmcWriteWordToMemory(&emu, 100, (short) 10);
    MtmcRun(&emu);
    assert(98 == MtmcGetRegisterValue(&emu, SP));
    assert(10 == MtmcFetchWordFromMemory(&emu, 98));
}

static void testDuplicate(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "dup");
    MtmcSetRegisterValue(&emu, SP, 100);
    MtmcWriteWordToMemory(&emu, 100, (short) 10);
    MtmcRun(&emu);
    assert(98 == MtmcGetRegisterValue(&emu, SP));
    assert(10 == MtmcFetchWordFromMemory(&emu, 98));
}

static void testSwapSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "swap sp");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 10);
    MtmcWriteWordToMemory(&emu, 98, (short) 20);
    MtmcRun(&emu);
    assert(98 == MtmcGetRegisterValue(&emu, SP));
    assert(10 == MtmcFetchWordFromMemory(&emu, 98));
    assert(20 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSwap(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "swap");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 10);
    MtmcWriteWordToMemory(&emu, 98, (short) 20);
    MtmcRun(&emu);
    assert(98 == MtmcGetRegisterValue(&emu, SP));
    assert(10 == MtmcFetchWordFromMemory(&emu, 98));
    assert(20 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testDropSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "drop sp");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 10);
    MtmcWriteWordToMemory(&emu, 98, (short) 20);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
}

static void testDrop(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "drop");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 10);
    MtmcWriteWordToMemory(&emu, 98, (short) 20);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
}

static void testOverSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "over sp");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 10);
    MtmcWriteWordToMemory(&emu, 98, (short) 20);
    MtmcRun(&emu);
    assert(96 == MtmcGetRegisterValue(&emu, SP));
    assert(10 == MtmcFetchWordFromMemory(&emu, 100));
    assert(20 == MtmcFetchWordFromMemory(&emu, 98));
    assert(10 == MtmcFetchWordFromMemory(&emu, 96));
}

static void testOver(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "over");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 10);
    MtmcWriteWordToMemory(&emu, 98, (short) 20);
    MtmcRun(&emu);
    assert(96 == MtmcGetRegisterValue(&emu, SP));
    assert(10 == MtmcFetchWordFromMemory(&emu, 100));
    assert(20 == MtmcFetchWordFromMemory(&emu, 98));
    assert(10 == MtmcFetchWordFromMemory(&emu, 96));
}

static void testRotSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "rot sp");
    MtmcSetRegisterValue(&emu, SP, 96);
    MtmcWriteWordToMemory(&emu, 100, (short) 10);
    MtmcWriteWordToMemory(&emu, 98, (short) 20);
    MtmcWriteWordToMemory(&emu, 96, (short) 30);
    MtmcRun(&emu);
    assert(96 == MtmcGetRegisterValue(&emu, SP));
    assert(20 == MtmcFetchWordFromMemory(&emu, 100));
    assert(30 == MtmcFetchWordFromMemory(&emu, 98));
    assert(10 == MtmcFetchWordFromMemory(&emu, 96));
}

static void testRot(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "rot");
    MtmcSetRegisterValue(&emu, SP, 96);
    MtmcWriteWordToMemory(&emu, 100, (short) 10);
    MtmcWriteWordToMemory(&emu, 98, (short) 20);
    MtmcWriteWordToMemory(&emu, 96, (short) 30);
    MtmcRun(&emu);
    assert(96 == MtmcGetRegisterValue(&emu, SP));
    assert(20 == MtmcFetchWordFromMemory(&emu, 100));
    assert(30 == MtmcFetchWordFromMemory(&emu, 98));
    assert(10 == MtmcFetchWordFromMemory(&emu, 96));
}

static void testSopAddSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop add sp");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 10);
    MtmcWriteWordToMemory(&emu, 98, (short) 20);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(30 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSopAdd(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop add");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 10);
    MtmcWriteWordToMemory(&emu, 98, (short) 20);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(30 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSopSubSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop sub sp");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 20);
    MtmcWriteWordToMemory(&emu, 98, (short) 10);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(10 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSopSub(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop sub");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 20);
    MtmcWriteWordToMemory(&emu, 98, (short) 10);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(10 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSopMulSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop mul sp");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 20);
    MtmcWriteWordToMemory(&emu, 98, (short) 10);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(200 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSopMul(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop mul");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 20);
    MtmcWriteWordToMemory(&emu, 98, (short) 10);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(200 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSopDivSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop div sp");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 20);
    MtmcWriteWordToMemory(&emu, 98, (short) 10);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(2 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSopDiv(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop div");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 20);
    MtmcWriteWordToMemory(&emu, 98, (short) 10);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(2 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSopModSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop mod sp");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 20);
    MtmcWriteWordToMemory(&emu, 98, (short) 10);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(0 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSopMod(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop mod");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 20);
    MtmcWriteWordToMemory(&emu, 98, (short) 10);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(0 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSopAndSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop and sp");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 0b110);
    MtmcWriteWordToMemory(&emu, 98, (short) 0b011);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(0b010 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSopAnd(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop and");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 0b110);
    MtmcWriteWordToMemory(&emu, 98, (short) 0b011);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(0b010 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSopOrSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop or sp");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 0b100);
    MtmcWriteWordToMemory(&emu, 98, (short) 0b001);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(0b101 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSopOr(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop or");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 0b100);
    MtmcWriteWordToMemory(&emu, 98, (short) 0b001);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(0b101 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSopXorSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop xor sp");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 0b110);
    MtmcWriteWordToMemory(&emu, 98, (short) 0b011);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(0b101 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSopXor(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop xor");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 0b110);
    MtmcWriteWordToMemory(&emu, 98, (short) 0b011);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(0b101 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSopShlSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop shl sp");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 0b110);
    MtmcWriteWordToMemory(&emu, 98, (short) 1);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(0b1100 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSopShl(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop shl");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 0b110);
    MtmcWriteWordToMemory(&emu, 98, (short) 1);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(0b1100 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSopShrSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop shr sp");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 0b110);
    MtmcWriteWordToMemory(&emu, 98, (short) 1);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(0b011 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSopShr(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop shr");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 0b110);
    MtmcWriteWordToMemory(&emu, 98, (short) 1);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(0b011 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSopMinSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop min sp");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 10);
    MtmcWriteWordToMemory(&emu, 98, (short) 20);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(10 == MtmcFetchWordFromMemory(&emu, MtmcGetRegisterValue(&emu, SP)));
}

static void testSopMin(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop min");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 10);
    MtmcWriteWordToMemory(&emu, 98, (short) 20);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(10 == MtmcFetchWordFromMemory(&emu, MtmcGetRegisterValue(&emu, SP)));
}

static void testSopMin2SP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop min sp");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 98, (short) 20);
    MtmcWriteWordToMemory(&emu, 100, (short) 10);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(10 == MtmcFetchWordFromMemory(&emu, MtmcGetRegisterValue(&emu, SP)));
}

static void testSopMin2(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop min");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 98, (short) 20);
    MtmcWriteWordToMemory(&emu, 100, (short) 10);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(10 == MtmcFetchWordFromMemory(&emu, MtmcGetRegisterValue(&emu, SP)));
}

static void testSopMaxSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop max sp");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 20);
    MtmcWriteWordToMemory(&emu, 98, (short) 10);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(20 == MtmcFetchWordFromMemory(&emu, MtmcGetRegisterValue(&emu, SP)));
}

static void testSopMax(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop max");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 100, (short) 20);
    MtmcWriteWordToMemory(&emu, 98, (short) 10);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(20 == MtmcFetchWordFromMemory(&emu, MtmcGetRegisterValue(&emu, SP)));
}

static void testSopMax2SP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop max sp");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 98, (short) 10);
    MtmcWriteWordToMemory(&emu, 100, (short) 20);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(20 == MtmcFetchWordFromMemory(&emu, MtmcGetRegisterValue(&emu, SP)));
}

static void testSopMax2(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop max");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 98, (short) 10);
    MtmcWriteWordToMemory(&emu, 100, (short) 20);
    MtmcRun(&emu);
    assert(100 == MtmcGetRegisterValue(&emu, SP));
    assert(20 == MtmcFetchWordFromMemory(&emu, MtmcGetRegisterValue(&emu, SP)));
}

static void testSopNotSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop not sp");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 98, (short) 0b01010101);
    MtmcRun(&emu);
    assert(98 == MtmcGetRegisterValue(&emu, SP));
    assert(~0b01010101 == MtmcFetchWordFromMemory(&emu, 98));
}

static void testSopNot(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop not");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 98, (short) 0b01010101);
    MtmcRun(&emu);
    assert(98 == MtmcGetRegisterValue(&emu, SP));
    assert(~0b01010101 == MtmcFetchWordFromMemory(&emu, 98));
}

static void testSopLnotWhenNonZeroSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop lnot sp");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 98, (short) 1);
    MtmcRun(&emu);
    assert(98 == MtmcGetRegisterValue(&emu, SP));
    assert(0 == MtmcFetchWordFromMemory(&emu, 98));
}

static void testSopLnotWhenNonZero(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop lnot");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 98, (short) 1);
    MtmcRun(&emu);
    assert(98 == MtmcGetRegisterValue(&emu, SP));
    assert(0 == MtmcFetchWordFromMemory(&emu, 98));
}

static void testSopLnotWhenZeroSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop lnot sp");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 98, (short) 0);
    MtmcRun(&emu);
    assert(98 == MtmcGetRegisterValue(&emu, SP));
    assert(1 == MtmcFetchWordFromMemory(&emu, 98));
}

static void testSopLnotWhenZero(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop lnot");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 98, (short) 0);
    MtmcRun(&emu);
    assert(98 == MtmcGetRegisterValue(&emu, SP));
    assert(1 == MtmcFetchWordFromMemory(&emu, 98));
}

static void testSopNegSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop neg sp");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 98, (short) 10);
    MtmcRun(&emu);
    assert(98 == MtmcGetRegisterValue(&emu, SP));
    assert(-10 == MtmcFetchWordFromMemory(&emu, 98));
}

static void testSopNeg(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sop neg");
    MtmcSetRegisterValue(&emu, SP, 98);
    MtmcWriteWordToMemory(&emu, 98, (short) 10);
    MtmcRun(&emu);
    assert(98 == MtmcGetRegisterValue(&emu, SP));
    assert(-10 == MtmcFetchWordFromMemory(&emu, 98));
}

static void testPushImmediateSP(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "pushi 1024 sp");
    MtmcRun(&emu);
    assert(1024 == MtmcFetchWordFromMemory(&emu, MtmcGetRegisterValue(&emu, SP)));
}

static void testPushImmediate(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "pushi 1024");
    MtmcRun(&emu);
    assert(1024 == MtmcFetchWordFromMemory(&emu, MtmcGetRegisterValue(&emu, SP)));
}

static void testEqWhenEq(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "eq t0 t1");
    MtmcSetRegisterValue(&emu, T0, 1024);
    MtmcSetRegisterValue(&emu, T1, 1024);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) != 0);
}

static void testEqWhenNotEq(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "eq t0 t1");
    MtmcSetRegisterValue(&emu, T0, 1024);
    MtmcSetRegisterValue(&emu, T1, 2);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
}

static void testNeqWhenNeq(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "neq t0 t1");
    MtmcSetRegisterValue(&emu, T0, 1024);
    MtmcSetRegisterValue(&emu, T1, 1024);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
}

static void testNeqWhenNotNeq(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "neq t0 t1");
    MtmcSetRegisterValue(&emu, T0, 1024);
    MtmcSetRegisterValue(&emu, T1, 2);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) != 0);
}

static void testGtWhenGt(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "gt t0 t1");
    MtmcSetRegisterValue(&emu, T0, 2);
    MtmcSetRegisterValue(&emu, T1, 1);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) != 0);
}

static void testGtWhenNotGt(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "gt t0 t1");
    MtmcSetRegisterValue(&emu, T0, 1);
    MtmcSetRegisterValue(&emu, T1, 2);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
}

static void testGteWhenGt(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "gte t0 t1");
    MtmcSetRegisterValue(&emu, T0, 2);
    MtmcSetRegisterValue(&emu, T1, 1);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) != 0);
}

static void testGteWhenE(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "gte t0 t1");
    MtmcSetRegisterValue(&emu, T0, 2);
    MtmcSetRegisterValue(&emu, T1, 2);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) != 0);
}

static void testGteWhenNotGte(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "gte t0 t1");
    MtmcSetRegisterValue(&emu, T0, 1);
    MtmcSetRegisterValue(&emu, T1, 2);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
}

static void testLtWhenLt(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "lt t0 t1");
    MtmcSetRegisterValue(&emu, T0, 1);
    MtmcSetRegisterValue(&emu, T1, 2);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) != 0);
}

static void testLtWhenNotLt(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "lt t0 t1");
    MtmcSetRegisterValue(&emu, T0, 2);
    MtmcSetRegisterValue(&emu, T1, 1);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
}

static void testLteWhenLt(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "lte t0 t1");
    MtmcSetRegisterValue(&emu, T0, 1);
    MtmcSetRegisterValue(&emu, T1, 2);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) != 0);
}

static void testLteWhenE(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "lte t0 t1");
    MtmcSetRegisterValue(&emu, T0, 2);
    MtmcSetRegisterValue(&emu, T1, 2);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) != 0);
}

static void testLteWhenNotLte(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "lte t0 t1");
    MtmcSetRegisterValue(&emu, T0, 2);
    MtmcSetRegisterValue(&emu, T1, 1);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
}

static void testEqiWhenEq(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "eqi t0 1");
    MtmcSetRegisterValue(&emu, T0, 1);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) != 0);
}

static void testEqiWhenNotEq(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "eqi t0 1");
    MtmcSetRegisterValue(&emu, T0, 1024);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
}

static void testNeqiWhenNeq(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "neqi t0 1");
    MtmcSetRegisterValue(&emu, T0, 1);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
}

static void testNeqiWhenNotNeq(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "neqi t0 1");
    MtmcSetRegisterValue(&emu, T0, 2);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) != 0);
}

static void testGtiWhenGt(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "gti t0 1");
    MtmcSetRegisterValue(&emu, T0, 2);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) != 0);
}

static void testGtiWhenNotGt(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "gti t0 1");
    MtmcSetRegisterValue(&emu, T0, 1);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
}

static void testGteiWhenGt(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "gtei t0 1");
    MtmcSetRegisterValue(&emu, T0, 2);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) != 0);
}

static void testGteiWhenE(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "gtei t0 2");
    MtmcSetRegisterValue(&emu, T0, 2);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) != 0);
}

static void testGteiWhenNotGte(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "gtei t0 2");
    MtmcSetRegisterValue(&emu, T0, 1);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
}

static void testLtiWhenLt(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "lti t0 2");
    MtmcSetRegisterValue(&emu, T0, 1);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) != 0);
}

static void testLtiWhenNotLt(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "lti t0 2");
    MtmcSetRegisterValue(&emu, T0, 2);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
}

static void testLteiWhenLt(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "ltei t0 2");
    MtmcSetRegisterValue(&emu, T0, 1);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) != 0);
}

static void testLteiWhenE(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "ltei t0 2");
    MtmcSetRegisterValue(&emu, T0, 2);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) != 0);
}

static void testLteiWhenNotLte(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "ltei t0 2");
    MtmcSetRegisterValue(&emu, T0, 3);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
    MtmcRun(&emu);
    assert(MtmcIsFlagTestBitSet(&emu) == 0);
}

static void testLoadWord(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "lw t0 10");
    MtmcWriteWordToMemory(&emu, 10, 20);
    assert(0 == MtmcGetRegisterValue(&emu, T0));
    MtmcRun(&emu);
    assert(20 == MtmcGetRegisterValue(&emu, T0));
}

static void testLoadWordOffset(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "lwo t0 t1 10");
    MtmcSetRegisterValue(&emu, T1, 5);
    MtmcWriteWordToMemory(&emu, 15, 20);
    assert(0 == MtmcGetRegisterValue(&emu, T0));
    MtmcRun(&emu);
    assert(20 == MtmcGetRegisterValue(&emu, T0));
}

static void testLoadByte(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "lb t0 10");
    MtmcWriteByteToMemory(&emu, 10,  20);
    assert(0 == MtmcGetRegisterValue(&emu, T0));
    MtmcRun(&emu);
    assert(20 == MtmcGetRegisterValue(&emu, T0));
}

static void testLoadByteOffset(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "lbo t0 t1 10");
    MtmcSetRegisterValue(&emu, T1, 5);
    MtmcWriteByteToMemory(&emu, 15,  20);
    assert(0 == MtmcGetRegisterValue(&emu, T0));
    MtmcRun(&emu);
    assert(20 == MtmcGetRegisterValue(&emu, T0));
}

static void testSaveWord(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sw t0 10");
    MtmcSetRegisterValue(&emu, T0, 20);
    assert(0 == MtmcFetchWordFromMemory(&emu, 10));
    MtmcRun(&emu);
    assert(20 == MtmcFetchWordFromMemory(&emu, 10));
}

static void testSaveWordWithOffset(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "swo t0 t1 10");
    MtmcSetRegisterValue(&emu, T0, 20);
    MtmcSetRegisterValue(&emu, T1, 10);
    assert(0 == MtmcFetchWordFromMemory(&emu, 20));
    MtmcRun(&emu);
    assert(20 == MtmcFetchWordFromMemory(&emu, 20));
}

static void testSaveByte(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sb t0 10");
    MtmcSetRegisterValue(&emu, T0, 20);
    assert(0 == MtmcFetchByteFromMemory(&emu, 10));
    MtmcRun(&emu);
    assert(20 == MtmcFetchByteFromMemory(&emu, 10));
}

static void testSaveByteWithOffset(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sbo t0 t1 10");
    MtmcSetRegisterValue(&emu, T0, 20);
    MtmcSetRegisterValue(&emu, T1, 10);
    assert(0 == MtmcFetchByteFromMemory(&emu, 20));
    MtmcRun(&emu);
    assert(20 == MtmcFetchByteFromMemory(&emu, 20));
}

static void testLoadImmediate(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "li t0 20");
    assert(0 == MtmcGetRegisterValue(&emu, T0));
    MtmcRun(&emu);
    assert(20 == MtmcGetRegisterValue(&emu, T0));
}

static void testLoadRegisterWord(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "lwr t0 t1 t2");
    MtmcSetRegisterValue(&emu, T1, 100);
    MtmcSetRegisterValue(&emu, T2, 0);
    MtmcWriteWordToMemory(&emu, 100, (short) 10); // value written to memory
    MtmcRun(&emu);
    assert(10 == MtmcGetRegisterValue(&emu, T0)); // should be loaded into t0
}

static void testLoadWordRegisterWithOffset(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "lwr t0 t1 t2");
    MtmcSetRegisterValue(&emu, T1, 100);
    MtmcSetRegisterValue(&emu, T2, 5);
    MtmcWriteWordToMemory(&emu, 105, (short) 10); // value written to memory
    MtmcRun(&emu);
    assert(10 == MtmcGetRegisterValue(&emu, T0)); // should be loaded into t0
}

static void testLoadByteRegister(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "lbr t0 t1 t2");
    MtmcSetRegisterValue(&emu, T1, 100);
    MtmcSetRegisterValue(&emu, T2, 0);
    MtmcWriteByteToMemory(&emu, 100,  10); // value written to memory
    MtmcRun(&emu);
    assert(10 == MtmcGetRegisterValue(&emu, T0)); // should be loaded into t0
}

static void testLoadByteRegisterWithOffset(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "lbr t0 t1 t2");
    MtmcSetRegisterValue(&emu, T1, 100);
    MtmcSetRegisterValue(&emu, T2, 5);
    MtmcWriteByteToMemory(&emu, 105,  10); // value written to memory
    MtmcRun(&emu);
    assert(10 == MtmcGetRegisterValue(&emu, T0)); // should be loaded into t0
}

static void testSaveRegisterWord(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "swr t0 t1 t2");
    MtmcSetRegisterValue(&emu, T0, 10);
    MtmcSetRegisterValue(&emu, T1, 100);
    MtmcSetRegisterValue(&emu, T2, 0);
    MtmcRun(&emu);
    assert(10 == MtmcFetchWordFromMemory(&emu, 100));
}

static void testSaveWordRegisterWithOffset(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "swr t0 t1 t2");
    MtmcSetRegisterValue(&emu, T0, 10);
    MtmcSetRegisterValue(&emu, T1, 100);
    MtmcSetRegisterValue(&emu, T2, 5);
    MtmcRun(&emu);
    assert(10 == MtmcFetchWordFromMemory(&emu, 105));
}

static void testSaveByteRegister(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sbr t0 t1 t2");
    MtmcSetRegisterValue(&emu, T0, 10);
    MtmcSetRegisterValue(&emu, T1, 100);
    MtmcSetRegisterValue(&emu, T2, 0);
    MtmcRun(&emu);
    assert(10 == MtmcFetchByteFromMemory(&emu, 100));
}

static void testSaveByteRegisterWithOffset(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "sbr t0 t1 t2");
    MtmcSetRegisterValue(&emu, T0, 10);
    MtmcSetRegisterValue(&emu, T1, 100);
    MtmcSetRegisterValue(&emu, T2, 5);
    MtmcRun(&emu);
    assert(10 == MtmcFetchByteFromMemory(&emu, 105));
}

static void testJumpRegister(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "jr t0");
    MtmcSetRegisterValue(&emu, T0, 50);
    _TestStep(&emu);
    //MtmcRun(&emu);
    assert(50 == MtmcGetRegisterValue(&emu, PC));
}

static void testRet(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "ret");
    MtmcSetRegisterValue(&emu, RA, 50);
    _TestStep(&emu);
    assert(50 == MtmcGetRegisterValue(&emu, PC));
}

static void testJump(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgramAt(&emu, "j 16", 50);
    MtmcSetRegisterValue(&emu, PC, 50);
    _TestStep(&emu);
    assert(16 == MtmcGetRegisterValue(&emu, PC));
}

static void testJumpZeroIfFlagBitSetToZero(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "jz 16");
    MtmcSetFlagTestBit(&emu, 0);
    _TestStep(&emu);
    assert(16 == MtmcGetRegisterValue(&emu, PC));
}

static void testJumpZeroIfFlagBitSetToOne(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "jz 16");
    MtmcSetFlagTestBit(&emu, 1);
    _TestStep(&emu);
    assert(2 == MtmcGetRegisterValue(&emu, PC));
}

static void testJumpNotZeroIfFlagBitSetToZero(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "jnz 16");
    MtmcSetFlagTestBit(&emu, 0);
    _TestStep(&emu);
    assert(2 == MtmcGetRegisterValue(&emu, PC));
}

static void testJumpNotZeroIfFlagBitSetToOne(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgram(&emu, "jnz 16");
    MtmcSetFlagTestBit(&emu, 1);
    _TestStep(&emu);
    assert(16 == MtmcGetRegisterValue(&emu, PC));
}

static void testJumpAndLink(void) {
    struct MtmcEmu emu = {0};
    _TestLoadProgramAt(&emu, "jal 16", 50);
    MtmcSetRegisterValue(&emu, PC, 50);
    _TestStep(&emu);
    assert(16 == MtmcGetRegisterValue(&emu, PC));
    assert(52 == MtmcGetRegisterValue(&emu, RA));
}

int main(int argc, const char* argv[]) {
    testSysCall();
    testMov();
    testInc();
    testInc3();
    testDec();
    testDec3();
    testSeti();
    // testNoOp();
    testAdd();
    testImmediateAdd();
    testSub();
    testImmediateSub();
    testMul();
    testImmediateMul();
    testDiv();
    testImmediateDiv();
    testMod();
    testImmediateMod();
    testAnd();
    testImmediateAnd();
    testOr();
    testImmediateOr();
    testXor();
    testImmediateXor();
    testShiftLeft();
    testImmediateShiftLeft();
    testShiftRight();
    testImmediateShiftRight();
    testMin();
    testMin2();
    testImmediateMin();
    testMax();
    testMax2();
    testImmediateMax();
    testBitwiseNot();
    testNotWithZero();
    testNotWithNonZeros1();
    testNotWithNonZeros10();
    testNegate();
    testImmediateInstruction();
    testPushSP();
    testPush();
    testPopSP();
    testPop();
    testDuplicateSP();
    testDuplicate();
    testSwapSP();
    testSwap();
    testDropSP();
    testDrop();
    testOverSP();
    testOver();
    testRotSP();
    testRot();
    testSopAddSP();
    testSopAdd();
    testSopSubSP();
    testSopSub();
    testSopMulSP();
    testSopMul();
    testSopDivSP();
    testSopDiv();
    testSopModSP();
    testSopMod();
    testSopAndSP();
    testSopAnd();
    testSopOrSP();
    testSopOr();
    testSopXorSP();
    testSopXor();
    testSopShlSP();
    testSopShl();
    testSopShrSP();
    testSopShr();
    testSopMinSP();
    testSopMin();
    testSopMin2SP();
    testSopMin2();
    testSopMaxSP();
    testSopMax();
    testSopMax2SP();
    testSopMax2();
    testSopNotSP();
    testSopNot();
    testSopLnotWhenNonZeroSP();
    testSopLnotWhenNonZero();
    testSopLnotWhenZeroSP();
    testSopLnotWhenZero();
    testSopNegSP();
    testSopNeg();
    testPushImmediateSP();
    testPushImmediate();
    testEqWhenEq();
    testEqWhenNotEq();
    testNeqWhenNeq();
    testNeqWhenNotNeq();
    testGtWhenGt();
    testGtWhenNotGt();
    testGteWhenGt();
    testGteWhenE();
    testGteWhenNotGte();
    testLtWhenLt();
    testLtWhenNotLt();
    testLteWhenLt();
    testLteWhenE();
    testLteWhenNotLte();
    testEqiWhenEq();
    testEqiWhenNotEq();
    testNeqiWhenNeq();
    testNeqiWhenNotNeq();
    testGtiWhenGt();
    testGtiWhenNotGt();
    testGteiWhenGt();
    testGteiWhenE();
    testGteiWhenNotGte();
    testLtiWhenLt();
    testLtiWhenNotLt();
    testLteiWhenLt();
    testLteiWhenE();
    testLteiWhenNotLte();
    testLoadWord();
    testLoadWordOffset();
    testLoadByte();
    testLoadByteOffset();
    testSaveWord();
    testSaveWordWithOffset();
    testSaveByte();
    testSaveByteWithOffset();
    testLoadImmediate();
    testLoadRegisterWord();
    testLoadWordRegisterWithOffset();
    testLoadByteRegister();
    testLoadByteRegisterWithOffset();
    testSaveRegisterWord();
    testSaveWordRegisterWithOffset();
    testSaveByteRegister();
    testSaveByteRegisterWithOffset();
    testJumpRegister();
    testRet();
    testJump();
    testJumpZeroIfFlagBitSetToZero();
    testJumpZeroIfFlagBitSetToOne();
    testJumpNotZeroIfFlagBitSetToZero();
    testJumpNotZeroIfFlagBitSetToOne();
    testJumpAndLink();
    return 0;
}
