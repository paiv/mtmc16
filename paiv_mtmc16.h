#ifndef _PAIV_MTMC_INCLUDE_MTMC_H_
#define _PAIV_MTMC_INCLUDE_MTMC_H_

#include <stdint.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef uint8_t u8;
typedef uint16_t u16;
typedef int8_t i8;
typedef int16_t i16;


enum {
    Mtmc_MEMORY_SIZE = 4096,
    Mtmc_DEFAULT_SPEED = 1000000,
    MtmcDisplay_width = 160,
    MtmcDisplay_height = 144,
    MtmcGraphics_max = 10,
    MtmcGraphics_bytes_max = 10000,
    MtmcGraphics_width_max = 500,
    MtmcGraphics_height_max = 200,
};


enum MtmcEmuStatus {
    MtmcEmuStatus_READY,
    MtmcEmuStatus_EXECUTING,
    MtmcEmuStatus_PERMANENT_ERROR,
    MtmcEmuStatus_FINISHED,
};


enum MtmcRegister {
    T0, T1, T2, T3, T4, T5,
    A0, A1, A2, A3,
    RV, RA, FP, SP, BP, PC,
    IR, DR, CB, DB, IO, FLAGS,
    _total_registers
};


enum MtmcDisplayColor {
    MtmcDisplayColor_DARK = 0,
    MtmcDisplayColor_MEDIUM = 1,
    MtmcDisplayColor_LIGHT = 2,
    MtmcDisplayColor_LIGHTEST = 3,
};


enum MtosSysCall {
    MtosSysCall_exit = 0x00,
    MtosSysCall_rint = 0x01,
    MtosSysCall_wint = 0x02,
    MtosSysCall_rstr = 0x03,
    MtosSysCall_wchr = 0x04,
    MtosSysCall_rchr = 0x05,
    MtosSysCall_wstr = 0x06,
    MtosSysCall_printf = 0x07,
    MtosSysCall_atoi = 0x08,

    MtosSysCall_rfile = 0x10,
    MtosSysCall_wfile = 0x11,
    MtosSysCall_cwd = 0x12,
    MtosSysCall_chdir = 0x13,
    MtosSysCall_dirent = 0x14,
    MtosSysCall_dfile = 0x15,

    MtosSysCall_rnd = 0x20,
    MtosSysCall_sleep = 0x21,
    MtosSysCall_timer = 0x22,

    MtosSysCall_fbreset = 0x30,
    MtosSysCall_fbstat = 0x31,
    MtosSysCall_fbset = 0x32,
    MtosSysCall_fbline = 0x33,
    MtosSysCall_fbrect = 0x34,
    MtosSysCall_fbflush = 0x35,
    MtosSysCall_joystick = 0x3A,
    MtosSysCall_scolor = 0x3B,

    MtosSysCall_memcopy = 0x40,

    MtosSysCall_drawimg = 0x50,
    MtosSysCall_drawimgsz = 0x51,
    MtosSysCall_drawimgclip = 0x52,

    MtosSysCall_error = 0xFF,
};


typedef struct Platform* PlatformState;


struct MtmcEmu {
    enum MtmcEmuStatus status;
    PlatformState platform;
    size_t speed;
    int trace_level;
    size_t graphics_count;
    struct MtmcGraphic* graphics;
    i16 registerFile[_total_registers];
    u8 memory[Mtmc_MEMORY_SIZE];
};


enum MtmcExecutableFormat {
    MtmcExecutableFormat_unknown,
    MtmcExecutableFormat_orc1,
    MtmcExecutableFormat_default = MtmcExecutableFormat_orc1,
};


#define MtmcFormatOrc1 "Orc1"


struct MtmcGraphic {
    i16 width;
    i16 height;
    /* 1 bit per pixel: 1 transparent, 0 opaque */
    u8 mask[(MtmcGraphics_width_max + 7) / 8 * MtmcGraphics_height_max];
    /* 2 bits per pixel */
    u8 data[(MtmcGraphics_width_max + 3) / 4 * MtmcGraphics_height_max];
};


struct MtmcExecutable {
    enum MtmcExecutableFormat format;
    size_t codesize;
    u8 code[Mtmc_MEMORY_SIZE];
    size_t datasize;
    u8 data[Mtmc_MEMORY_SIZE];
    size_t graphics_count;
    struct MtmcGraphic graphics[MtmcGraphics_max];
};


enum MtmcEmuStatus MtmcGetStatus(struct MtmcEmu* emu);

i16 MtmcGetRegisterValue(struct MtmcEmu* emu, i16 reg);
void MtmcSetRegisterValue(struct MtmcEmu* emu, i16 reg, i16 value);

u8 MtmcFetchByteFromMemory(struct MtmcEmu* emu, i16 addr);
void MtmcWriteByteToMemory(struct MtmcEmu* emu, i16 addr, u8 value);

i16 MtmcFetchWordFromMemory(struct MtmcEmu* emu, i16 addr);
void MtmcWriteWordToMemory(struct MtmcEmu* emu, i16 addr, i16 value);

u8 MtmcIsFlagTestBitSet(struct MtmcEmu* emu);
void MtmcSetFlagTestBit(struct MtmcEmu* emu, u8 value);
void MtmcSetFlagOverflowBit(struct MtmcEmu* emu, u8 value);

void MtmcExecInstruction(struct MtmcEmu* emu, i16 instr);

void MtmcInitMemory(struct MtmcEmu* emu);
void MtmcLoad(struct MtmcEmu* emu, struct MtmcExecutable* exe);
void MtmcSetArg(struct MtmcEmu* emu, const char* arg);
int MtmcRun(struct MtmcEmu* emu);
int MtmcPulse(struct MtmcEmu* emu, int pulse);
int MtmcExecutableLoad(FILE* file, struct MtmcExecutable* exe);

int MtmcGraphicLoad(struct MtmcGraphic* graphic, FILE* file);
int MtmcGraphicWrite(struct MtmcGraphic* graphic, FILE* file);
int MtmcGraphicHasAlpha(struct MtmcGraphic* graphic);


#if defined(EXIT_FAILURE)
#define FatalError() abort()
#else
#define FatalError() (*((volatile int*)0) = 0)
#endif

#define FatalErrorFmt(...) {\
    fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); FatalError(); }


#ifdef __cplusplus
}
#endif


#endif /* _PAIV_MTMC_INCLUDE_MTMC_H_ */


#ifdef PAIV_MTMC_IMPLEMENTATION

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>


int PlatformInit(PlatformState state);
void PlatformDeinit(PlatformState state);
u8 PlatformIsClosed(PlatformState state);
void PlatformSleep(PlatformState state, i16 millis);
void PlatformResetFrame(PlatformState state);
void PlatformDrawFrame(PlatformState state);
void PlatformSetColor(PlatformState state, i16 color);
void PlatformFillRect(PlatformState state, i16 x, i16 y, i16 width, i16 height);
void PlatformDrawImage(PlatformState state, struct MtmcGraphic* image, i16 x, i16 y);
i16 PlatformGetJoystick(PlatformState state);
char PlatformGetChar(PlatformState state);
void PlatformPutChar(PlatformState state, char c);
void PlatformPutString(PlatformState state, const char* s);
void PlatformPutWord(PlatformState state, i16 n);
i16 PlatformReadWord(PlatformState state);
i16 PlatformParseWord(PlatformState state, const char* s);
i16 PlatformRandom(PlatformState state, i16 start, i16 stop);
i16 PlatformSetTimer(PlatformState state, i16 millis);
i16 PlatformFileRead(PlatformState state, const char* filename, u8* buf, i16 bufsize, i16 maxlines);
i16 PlatformGetCurrentDir(PlatformState state, char* buf, size_t bufsize);
i16 PlatformSetCurrentDir(PlatformState state, const char* name);
i16 PlatformDirGetSize(PlatformState state, const char* name);
i16 PlatformDirReadEntry(PlatformState state, const char* name, i16 index, i16* flags, char* buf, size_t bufsize);


static const u8 _MtmcPalette[5][3] = {
    {42, 69, 59},
    {54, 93, 72},
    {87, 124, 68},
    {127, 134, 15},
    {0, 0, 0},
};


int MtmcGraphicHasAlpha(struct MtmcGraphic* graphic) {
    size_t n = (graphic->width + 7) / 8 * graphic->height;
    u8* p = &graphic->mask[0];
    for (size_t i = 0; i < n; ++i, ++p) {
        if (*p != 0) { return 1; }
    }
    return 0;
}


enum MtmcDirentCommand {
    MtmcDirent_count = 0x00,
    MtmcDirent_get_entry = 0x01,
};


void MtosHandleSysCall(struct MtmcEmu* emu, i16 number);
void _MtmcFetchCurrentInstruction(struct MtmcEmu* emu);
u8 _MtmcIsDoubleWordInstruction(i16 instr);
void _DumpMemory(FILE* file, const u8* data, size_t size);


enum MtmcEmuStatus MtmcGetStatus(struct MtmcEmu* emu) {
    return emu->status;
}


void MtmcSetErrorStatus(struct MtmcEmu* emu, enum MtmcEmuStatus status,
    const char* fmt, ...) {
    emu->status = status;
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}


i16 MtmcGetRegisterValue(struct MtmcEmu* emu, i16 reg) {
    return emu->registerFile[reg];
}


void MtmcSetRegisterValue(struct MtmcEmu* emu, i16 reg, i16 value) {
    emu->registerFile[reg] = value;
}


void MtmcUpdateFlagsWithValue(struct MtmcEmu* emu, int value) {
    i16 chk = value;
    MtmcSetFlagTestBit(emu, chk != 0 ? 1 : 0);
    MtmcSetFlagOverflowBit(emu, chk != value ? 1 : 0);
}


void MtmcSetRegisterValueChecked(struct MtmcEmu* emu, i16 reg, int value) {
    MtmcSetRegisterValue(emu, reg, value);
    MtmcUpdateFlagsWithValue(emu, value);
}


u8 MtmcFetchByteFromMemory(struct MtmcEmu* emu, i16 addr) {
    if (addr >= 0 && addr < (i16)sizeof(emu->memory)) {
        return emu->memory[addr];
    }
    else {
        MtmcSetErrorStatus(emu, MtmcEmuStatus_PERMANENT_ERROR,
            "bad memory address on read: %d (0x%04x)", addr, (u16)addr);
        return 0;
    }
}


void MtmcWriteByteToMemory(struct MtmcEmu* emu, i16 addr, u8 value) {
    if (addr >= 0 && addr < (i16)sizeof(emu->memory)) {
        emu->memory[addr] = value;
    }
    else {
        MtmcSetErrorStatus(emu, MtmcEmuStatus_PERMANENT_ERROR,
            "bad memory address on write: %d (0x%04x)", addr, (u16)addr);
    }
}


i16 MtmcFetchWordFromMemory(struct MtmcEmu* emu, i16 addr) {
    u8 hi = MtmcFetchByteFromMemory(emu, addr);
    u8 lo = MtmcFetchByteFromMemory(emu, addr + 1);
    return (hi << 8) | lo;
}


void MtmcWriteWordToMemory(struct MtmcEmu* emu, i16 addr, i16 value) {
    MtmcWriteByteToMemory(emu, addr, value >> 8);
    MtmcWriteByteToMemory(emu, addr + 1, value);
}


u8 MtmcIsFlagTestBitSet(struct MtmcEmu* emu) {
    i16 value = MtmcGetRegisterValue(emu, FLAGS);
    u8 flag = value & 1;
    return flag;
}


void MtmcSetFlagTestBit(struct MtmcEmu* emu, u8 flag) {
    i16 value = MtmcGetRegisterValue(emu, FLAGS);
    if (flag != 0) {
        value |= 1;
    }
    else {
        value &= -2;
    }
    MtmcSetRegisterValue(emu, FLAGS, value);
}


void MtmcSetFlagOverflowBit(struct MtmcEmu* emu, u8 value) {
}


void MtmcInitMemory(struct MtmcEmu* emu) {
    memset(emu->memory, 0, sizeof(emu->memory));
    MtmcSetRegisterValue(emu, SP, Mtmc_MEMORY_SIZE);
}


void MtmcLoad(struct MtmcEmu* emu, struct MtmcExecutable* exe) {
    emu->graphics_count = exe->graphics_count;
    emu->graphics = &exe->graphics[0];
    MtmcInitMemory(emu);

    size_t boundary = exe->codesize;
    if (boundary > sizeof(emu->memory)) {
        boundary = sizeof(emu->memory);
    }
    memcpy(emu->memory, exe->code, boundary);

    size_t datasize = exe->datasize;
    if (boundary + datasize > sizeof(emu->memory)) {
        datasize = sizeof(emu->memory) - boundary;
    }
    size_t total = boundary + datasize;

    memcpy(&emu->memory[boundary], exe->data, datasize);

    MtmcSetRegisterValue(emu, CB, boundary - 1);
    MtmcSetRegisterValue(emu, DB, total - 1);
    MtmcSetRegisterValue(emu, BP, total);
    _MtmcFetchCurrentInstruction(emu);
    emu->status = MtmcEmuStatus_READY;

    if (emu->trace_level > 0) {
        fputs("memory:\n", stderr);
        _DumpMemory(stderr, emu->memory, total);
    }
}


void MtmcSetArg(struct MtmcEmu* emu, const char* arg) {
    int n = strlen(arg);
    if (n <= 0) { return; }
    i16 bp = MtmcGetRegisterValue(emu, BP);
    memcpy(&emu->memory[bp], arg, n + 1);
    MtmcSetRegisterValue(emu, A0, bp);
    MtmcSetRegisterValue(emu, BP, bp + n + 1);
}


enum MtmcInstructionType {
    MtmcInstructionType_MISC = 0b0000,
    MtmcInstructionType_ALU = 0b0001,
    MtmcInstructionType_STACK = 0b0010,
    MtmcInstructionType_TEST = 0b0011,
    MtmcInstructionType_LWR = 0b0100,
    MtmcInstructionType_LBR = 0b0101,
    MtmcInstructionType_SWR = 0b0110,
    MtmcInstructionType_SBR = 0b0111,
    MtmcInstructionType_LOAD = 0b1000,
    MtmcInstructionType_JUMPREG = 0b1001,
    MtmcInstructionType_RET = MtmcInstructionType_JUMPREG,
    MtmcInstructionType_JUMP = 0b1100,
    MtmcInstructionType_JUMPZ = 0b1101,
    MtmcInstructionType_JUMPNZ = 0b1110,
    MtmcInstructionType_JUMPAL = 0b1111,
};


enum MtmcInstructionMisc {
    MtmcInstructionMisc_sys = 0b0000,
    MtmcInstructionMisc_mov = 0b0001,
    MtmcInstructionMisc_inc = 0b0010,
    MtmcInstructionMisc_dec = 0b0011,
    MtmcInstructionMisc_seti = 0b0100,
    MtmcInstructionMisc_mcp = 0b0101,
    MtmcInstructionMisc_debug = 0b1000,
    MtmcInstructionMisc_nop = 0b1111,
};


enum MtmcInstructionAlu {
    MtmcInstructionAlu_add = 0b0000,
    MtmcInstructionAlu_sub = 0b0001,
    MtmcInstructionAlu_mul = 0b0010,
    MtmcInstructionAlu_div = 0b0011,
    MtmcInstructionAlu_mod = 0b0100,
    MtmcInstructionAlu_and = 0b0101,
    MtmcInstructionAlu_or = 0b0110,
    MtmcInstructionAlu_xor = 0b0111,
    MtmcInstructionAlu_shl = 0b1000,
    MtmcInstructionAlu_shr = 0b1001,
    MtmcInstructionAlu_min = 0b1010,
    MtmcInstructionAlu_max = 0b1011,
    MtmcInstructionAlu_not = 0b1100,
    MtmcInstructionAlu_lnot = 0b1101,
    MtmcInstructionAlu_neg = 0b1110,
    MtmcInstructionAlu_imm = 0b1111,
};


enum MtmcInstructionStack {
    MtmcInstructionStack_push = 0b0000,
    MtmcInstructionStack_pop = 0b0001,
    MtmcInstructionStack_dup = 0b0010,
    MtmcInstructionStack_swap = 0b0011,
    MtmcInstructionStack_drop = 0b0100,
    MtmcInstructionStack_over = 0b0101,
    MtmcInstructionStack_rot = 0b0110,
    MtmcInstructionStack_sop = 0b0111,
    MtmcInstructionStack_pushi = 0b1111,
};


enum MtmcInstructionTest {
    MtmcInstructionTest_eq = 0b0000,
    MtmcInstructionTest_neq = 0b0001,
    MtmcInstructionTest_gt = 0b0010,
    MtmcInstructionTest_gte = 0b0011,
    MtmcInstructionTest_lt = 0b0100,
    MtmcInstructionTest_lte = 0b0101,
    MtmcInstructionTest_eqi = 0b1000,
    MtmcInstructionTest_neqi = 0b1001,
    MtmcInstructionTest_gti = 0b1010,
    MtmcInstructionTest_gtei = 0b1011,
    MtmcInstructionTest_lti = 0b1100,
    MtmcInstructionTest_ltei = 0b1101,
};


enum MtmcInstructionLoad {
    MtmcInstructionLoad_lw = 0b0000,
    MtmcInstructionLoad_lwo = 0b0001,
    MtmcInstructionLoad_lb = 0b0010,
    MtmcInstructionLoad_lbo = 0b0011,
    MtmcInstructionLoad_sw = 0b0100,
    MtmcInstructionLoad_swo = 0b0101,
    MtmcInstructionLoad_sb = 0b0110,
    MtmcInstructionLoad_sbo = 0b0111,
    MtmcInstructionLoad_li = 0b1111,
    MtmcInstructionLoad_la = 0b1111,
};


#define NIB3(i) (((u16)(i) >> 12) & 0xF)
#define NIB2(i) (((u16)(i) >> 8) & 0xF)
#define NIB1(i) (((u16)(i) >> 4) & 0xF)
#define NIB0(i) ((u16)(i) & 0xF)
#define HALF1(i) ((u8)(((u16)(i) >> 8) & 0xFF))
#define HALF0(i) ((u8)((u16)(i) & 0xFF))


void MtmcExecInstruction(struct MtmcEmu* emu, i16 instr) {
    switch ((enum MtmcInstructionType) NIB3(instr)) {

        case MtmcInstructionType_MISC:
            switch ((enum MtmcInstructionMisc) NIB2(instr)) {

                case MtmcInstructionMisc_sys:
                    MtosHandleSysCall(emu, HALF0(instr));
                    break;

                case MtmcInstructionMisc_mov:
                    MtmcSetRegisterValue(emu, NIB1(instr),
                        MtmcGetRegisterValue(emu, NIB0(instr)));
                    break;

                case MtmcInstructionMisc_inc:
                    MtmcSetRegisterValueChecked(emu, NIB1(instr),
                        MtmcGetRegisterValue(emu, NIB1(instr))
                        + NIB0(instr));
                    break;

                case MtmcInstructionMisc_dec:
                    MtmcSetRegisterValueChecked(emu, NIB1(instr),
                        MtmcGetRegisterValue(emu, NIB1(instr))
                        - NIB0(instr));
                    break;

                case MtmcInstructionMisc_seti:
                    MtmcSetRegisterValue(emu, NIB1(instr), NIB0(instr));
                    break;

                case MtmcInstructionMisc_mcp:
                    FatalError();
                    break;

                case MtmcInstructionMisc_debug:
                    FatalError();
                    break;

                case MtmcInstructionMisc_nop:
                    break;

                default:
                    FatalErrorFmt("unhandled MISC op %02x", NIB2(instr));
            }
            break;

        case MtmcInstructionType_ALU: {
            enum MtmcInstructionAlu op = NIB2(instr);
            i16 target = NIB1(instr);
            i16 source;
            if (op < MtmcInstructionAlu_not) {
                source = NIB0(instr);
            }
            else if (op < MtmcInstructionAlu_imm) {
                source = target;
            }
            else {
                source = DR;
                op = NIB0(instr);
            }

            i16 sourceValue = MtmcGetRegisterValue(emu, source);
            i16 targetValue = MtmcGetRegisterValue(emu, target);
            int result = 0;

            switch (op) {

                case MtmcInstructionAlu_add:
                    result = targetValue + sourceValue;
                    break;

                case MtmcInstructionAlu_sub:
                    result = targetValue - sourceValue;
                    break;

                case MtmcInstructionAlu_mul:
                    result = targetValue * sourceValue;
                    break;

                case MtmcInstructionAlu_div:
                    result = targetValue / sourceValue;
                    break;

                case MtmcInstructionAlu_mod:
                    result = targetValue % sourceValue;
                    break;

                case MtmcInstructionAlu_and:
                    result = targetValue & sourceValue;
                    break;

                case MtmcInstructionAlu_or:
                    result = targetValue | sourceValue;
                    break;

                case MtmcInstructionAlu_xor:
                    result = targetValue ^ sourceValue;
                    break;

                case MtmcInstructionAlu_shl:
                    result = targetValue << sourceValue;
                    break;

                case MtmcInstructionAlu_shr:
                    result = targetValue >> sourceValue;
                    break;

                case MtmcInstructionAlu_min:
                    result = targetValue < sourceValue ? targetValue : sourceValue;
                    break;

                case MtmcInstructionAlu_max:
                    result = sourceValue < targetValue ? targetValue : sourceValue;
                    break;

                case MtmcInstructionAlu_not:
                    result = ~targetValue;
                    break;

                case MtmcInstructionAlu_lnot:
                    result = targetValue != 0 ? 0 : 1;
                    break;

                case MtmcInstructionAlu_neg:
                    result = -targetValue;
                    break;

                default:
                    FatalErrorFmt("unhandled ALU op %02x", op);
            }

            MtmcSetRegisterValueChecked(emu, target, result);
            break;
        }

        case MtmcInstructionType_STACK: {
            enum MtmcInstructionStack op = NIB2(instr);
            i16 stack = NIB0(instr);
            i16 addr = MtmcGetRegisterValue(emu, stack);

            switch (op) {

                case MtmcInstructionStack_push: {
                    MtmcSetRegisterValue(emu, stack, addr - 2);
                    i16 addr = MtmcGetRegisterValue(emu, stack);
                    i16 value = MtmcGetRegisterValue(emu, NIB1(instr));
                    MtmcWriteWordToMemory(emu, addr, value);
                    break;
                }

                case MtmcInstructionStack_pop: {
                    i16 value = MtmcFetchWordFromMemory(emu, addr);
                    MtmcSetRegisterValue(emu, NIB1(instr), value);
                    MtmcSetRegisterValue(emu, stack, addr + 2);
                    break;
                }

                case MtmcInstructionStack_dup: {
                    i16 value = MtmcFetchWordFromMemory(emu, addr);
                    MtmcSetRegisterValue(emu, stack, addr - 2);
                    i16 addr = MtmcGetRegisterValue(emu, stack);
                    MtmcWriteWordToMemory(emu, addr, value);
                    break;
                }

                case MtmcInstructionStack_swap: {
                    i16 v1 = MtmcFetchWordFromMemory(emu, addr);
                    i16 v2 = MtmcFetchWordFromMemory(emu, addr + 2);
                    MtmcWriteWordToMemory(emu, addr, v2);
                    MtmcWriteWordToMemory(emu, addr + 2, v1);
                    break;
                }

                case MtmcInstructionStack_drop:
                    MtmcSetRegisterValue(emu, stack, addr + 2);
                    break;

                case MtmcInstructionStack_over: {
                    i16 v2 = MtmcFetchWordFromMemory(emu, addr + 2);
                    MtmcSetRegisterValue(emu, stack, addr - 2);
                    i16 addr = MtmcGetRegisterValue(emu, stack);
                    MtmcWriteWordToMemory(emu, addr, v2);
                    break;
                }

                case MtmcInstructionStack_rot: {
                    i16 v1 = MtmcFetchWordFromMemory(emu, addr);
                    i16 v2 = MtmcFetchWordFromMemory(emu, addr + 2);
                    i16 v3 = MtmcFetchWordFromMemory(emu, addr + 4);
                    MtmcWriteWordToMemory(emu, addr, v3);
                    MtmcWriteWordToMemory(emu, addr + 2, v1);
                    MtmcWriteWordToMemory(emu, addr + 4, v2);
                    break;
                }

                case MtmcInstructionStack_sop: {
                    enum MtmcInstructionAlu op = NIB1(instr);

                    if (op < MtmcInstructionAlu_not) {
                        i16 targetValue = MtmcFetchWordFromMemory(emu, addr + 2);
                        i16 sourceValue = MtmcFetchWordFromMemory(emu, addr);
                        int result = 0;
                        switch (op) {

                            case MtmcInstructionAlu_add:
                                result = targetValue + sourceValue;
                                break;

                            case MtmcInstructionAlu_sub:
                                result = targetValue - sourceValue;
                                break;

                            case MtmcInstructionAlu_mul:
                                result = targetValue * sourceValue;
                                break;

                            case MtmcInstructionAlu_div:
                                result = targetValue / sourceValue;
                                break;

                            case MtmcInstructionAlu_mod:
                                result = targetValue % sourceValue;
                                break;

                            case MtmcInstructionAlu_and:
                                result = targetValue & sourceValue;
                                break;

                            case MtmcInstructionAlu_or:
                                result = targetValue | sourceValue;
                                break;

                            case MtmcInstructionAlu_xor:
                                result = targetValue ^ sourceValue;
                                break;

                            case MtmcInstructionAlu_shl:
                                result = targetValue << sourceValue;
                                break;

                            case MtmcInstructionAlu_shr:
                                result = targetValue >> sourceValue;
                                break;

                            case MtmcInstructionAlu_min:
                                result = targetValue < sourceValue ? targetValue : sourceValue;
                                break;

                            case MtmcInstructionAlu_max:
                                result = sourceValue < targetValue ? targetValue : sourceValue;
                                break;

                           default:
                                FatalErrorFmt("unhandled ALU op %02x", op);
                        }
                        MtmcUpdateFlagsWithValue(emu, result);
                        MtmcSetRegisterValue(emu, stack, addr + 2);
                        addr = MtmcGetRegisterValue(emu, stack);
                        MtmcWriteWordToMemory(emu, addr, result);
                    }
                    else {
                        i16 targetValue = MtmcFetchWordFromMemory(emu, addr);
                        int result = 0;
                        switch (op) {
                            case MtmcInstructionAlu_not:
                                result = ~targetValue;
                                break;

                            case MtmcInstructionAlu_lnot:
                                result = targetValue != 0 ? 0 : 1;
                                break;

                            case MtmcInstructionAlu_neg:
                                result = -targetValue;
                                break;

                           default:
                                FatalErrorFmt("unhandled ALU op %02x", op);
                        }
                        MtmcUpdateFlagsWithValue(emu, result);
                        MtmcWriteWordToMemory(emu, addr, result);
                    }
                    break;
                }

                case MtmcInstructionStack_pushi: {
                    i16 value = MtmcGetRegisterValue(emu, DR);
                    MtmcSetRegisterValue(emu, stack, addr - 2);
                    i16 addr = MtmcGetRegisterValue(emu, stack);
                    MtmcWriteWordToMemory(emu, addr, value);
                    break;
                }

                default:
                    FatalErrorFmt("unhandled STACK op %02x", op);
            }
            break;
        }

        case MtmcInstructionType_TEST: {
            enum MtmcInstructionTest op = NIB2(instr);
            i16 targetValue = MtmcGetRegisterValue(emu, NIB1(instr));
            int result = 0;

            if (op < MtmcInstructionTest_eqi) {
                i16 sourceValue = MtmcGetRegisterValue(emu, NIB0(instr));
                switch (op) {
                    case MtmcInstructionTest_eq:
                        result = targetValue == sourceValue;
                        break;
                    case MtmcInstructionTest_neq:
                        result = targetValue != sourceValue;
                        break;
                    case MtmcInstructionTest_gt:
                        result = targetValue > sourceValue;
                        break;
                    case MtmcInstructionTest_gte:
                        result = targetValue >= sourceValue;
                        break;
                    case MtmcInstructionTest_lt:
                        result = targetValue < sourceValue;
                        break;
                    case MtmcInstructionTest_lte:
                        result = targetValue <= sourceValue;
                        break;
                    default:
                        FatalErrorFmt("unhandled TEST op %02x", op);
                }
            }
            else {
                i16 sourceValue = NIB0(instr);
                switch (op) {
                    case MtmcInstructionTest_eqi:
                        result = targetValue == sourceValue;
                        break;
                    case MtmcInstructionTest_neqi:
                        result = targetValue != sourceValue;
                        break;
                    case MtmcInstructionTest_gti:
                        result = targetValue > sourceValue;
                        break;
                    case MtmcInstructionTest_gtei:
                        result = targetValue >= sourceValue;
                        break;
                    case MtmcInstructionTest_lti:
                        result = targetValue < sourceValue;
                        break;
                    case MtmcInstructionTest_ltei:
                        result = targetValue <= sourceValue;
                        break;
                    default:
                        FatalErrorFmt("unhandled TEST op %02x", op);
                }
            }

            MtmcSetFlagTestBit(emu, result != 0 ? 1 : 0);
            break;
        }

        case MtmcInstructionType_LWR: {
            i16 addr = MtmcGetRegisterValue(emu, NIB1(instr));
            i16 offset = MtmcGetRegisterValue(emu, NIB0(instr));
            MtmcSetRegisterValue(emu, NIB2(instr),
                MtmcFetchWordFromMemory(emu, addr + offset));
            break;
        }

        case MtmcInstructionType_LBR: {
            i16 addr = MtmcGetRegisterValue(emu, NIB1(instr));
            i16 offset = MtmcGetRegisterValue(emu, NIB0(instr));
            MtmcSetRegisterValue(emu, NIB2(instr),
                MtmcFetchByteFromMemory(emu, addr + offset));
            break;
        }

        case MtmcInstructionType_SWR: {
            i16 addr = MtmcGetRegisterValue(emu, NIB1(instr));
            i16 offset = MtmcGetRegisterValue(emu, NIB0(instr));
            MtmcWriteWordToMemory(emu, addr + offset,
                MtmcGetRegisterValue(emu, NIB2(instr)));
            break;
        }

        case MtmcInstructionType_SBR: {
            i16 addr = MtmcGetRegisterValue(emu, NIB1(instr));
            i16 offset = MtmcGetRegisterValue(emu, NIB0(instr));
            MtmcWriteByteToMemory(emu, addr + offset,
                MtmcGetRegisterValue(emu, NIB2(instr)));
            break;
        }

        case MtmcInstructionType_LOAD: {
            enum MtmcInstructionLoad op = NIB2(instr);
            i16 target = NIB1(instr);
            i16 roffset = NIB0(instr);
            i16 addr = MtmcGetRegisterValue(emu, DR);

            if ((op & 0b0100) != 0 && op != MtmcInstructionLoad_li) {
                switch (op) {
                    case MtmcInstructionLoad_sw:
                        MtmcWriteWordToMemory(emu, addr,
                            MtmcGetRegisterValue(emu, target));
                        break;
                    case MtmcInstructionLoad_swo: {
                        i16 offset = MtmcGetRegisterValue(emu, roffset);
                        MtmcWriteWordToMemory(emu, addr + offset,
                            MtmcGetRegisterValue(emu, target));
                        break;
                    }
                    case MtmcInstructionLoad_sb:
                        MtmcWriteByteToMemory(emu, addr,
                            MtmcGetRegisterValue(emu, target));
                        break;
                    case MtmcInstructionLoad_sbo: {
                        i16 offset = MtmcGetRegisterValue(emu, roffset);
                        MtmcWriteByteToMemory(emu, addr + offset,
                            MtmcGetRegisterValue(emu, target));
                        break;
                    }
                    default:
                        FatalErrorFmt("unhandled LOAD op %02x", op);
                }
            }
            else {
                switch (op) {
                    case MtmcInstructionLoad_lw:
                        MtmcSetRegisterValue(emu, target,
                            MtmcFetchWordFromMemory(emu, addr));
                        break;
                    case MtmcInstructionLoad_lwo: {
                        i16 offset = MtmcGetRegisterValue(emu, roffset);
                        MtmcSetRegisterValue(emu, target,
                            MtmcFetchWordFromMemory(emu, addr + offset));
                        break;
                    }
                    case MtmcInstructionLoad_lb:
                        MtmcSetRegisterValue(emu, target,
                            MtmcFetchByteFromMemory(emu, addr));
                        break;
                    case MtmcInstructionLoad_lbo: {
                        i16 offset = MtmcGetRegisterValue(emu, roffset);
                        MtmcSetRegisterValue(emu, target,
                            MtmcFetchByteFromMemory(emu, addr + offset));
                        break;
                    }
                    case MtmcInstructionLoad_li:
                        MtmcSetRegisterValue(emu, target, addr);
                        break;
                    default:
                        FatalErrorFmt("unhandled LOAD op %02x", op);
                }
            }
            break;
        }

        case MtmcInstructionType_JUMPREG:
            MtmcSetRegisterValue(emu, PC,
                MtmcGetRegisterValue(emu, NIB0(instr)));
            break;

        case MtmcInstructionType_JUMP:
            MtmcSetRegisterValue(emu, PC, (u16)instr & 0x0FFF);
            break;

        case MtmcInstructionType_JUMPZ:
            if (MtmcIsFlagTestBitSet(emu) == 0) {
                MtmcSetRegisterValue(emu, PC, (u16)instr & 0x0FFF);
            }
            break;

        case MtmcInstructionType_JUMPNZ:
            if (MtmcIsFlagTestBitSet(emu) != 0) {
                MtmcSetRegisterValue(emu, PC, (u16)instr & 0x0FFF);
            }
            break;

        case MtmcInstructionType_JUMPAL:
            MtmcSetRegisterValue(emu, RA, MtmcGetRegisterValue(emu, PC));
            MtmcSetRegisterValue(emu, PC, (u16)instr & 0xFFF);
            break;

        default:
            FatalErrorFmt("unhandled instruction %04x", (u16)instr);
    }
}


u8 _MtmcIsDoubleWordInstruction(i16 instr) {
    switch ((enum MtmcInstructionType) NIB3(instr)) {
        case MtmcInstructionType_MISC:
            return NIB2(instr) == MtmcInstructionMisc_mcp ? 1 : 0;
        case MtmcInstructionType_ALU:
            return NIB2(instr) == MtmcInstructionAlu_imm ? 1 : 0;
        case MtmcInstructionType_STACK:
            return NIB2(instr) == MtmcInstructionStack_pushi ? 1 : 0;
        case MtmcInstructionType_LOAD:
            return 1;
        default:
            return 0;
    }
}


void _MtmcFetchCurrentInstruction(struct MtmcEmu* emu) {
    i16 addr = MtmcGetRegisterValue(emu, PC);
    i16 instr = MtmcFetchWordFromMemory(emu, addr);
    MtmcSetRegisterValue(emu, IR, instr);
    if (_MtmcIsDoubleWordInstruction(instr) == 0) {
        MtmcSetRegisterValue(emu, DR, 0);
    }
    else {
        i16 data = MtmcFetchWordFromMemory(emu, addr + 2);
        MtmcSetRegisterValue(emu, DR, data);
    }
}


void _MtmcTraceState(struct MtmcEmu* emu, FILE* file) {
    u16 pc = MtmcGetRegisterValue(emu, PC);
    u16 instr = MtmcFetchWordFromMemory(emu, pc);
    fprintf(file, "%04X: %04x\n", pc, instr);
}


void _DumpMemory(FILE* file, const u8* data, size_t size) {
    for (size_t n = 0; n < size; n += 8) {
        fprintf(file, "%04X:", (int)(n / 8 * 8));
        for (size_t j = 0, i = n; j < 8 && i < size; ++j, ++i) {
            fprintf(file, " %02x", data[i]);
        }
        if (size - n < 8) {
            for (size_t j = 8 - (size - n); j > 0; --j) {
                fputs("   ", file);
            }
        }
        fputs("  ", file);
        for (size_t j = 0, i = n; j < 8 && i < size; ++j, ++i) {
            char c = data[i];
            if (c < ' ' || c > '~') { c = '.'; }
            fprintf(file, "%c", c);
        }
        fputs("\n", file);
    }
}


void _MtmcFetchAndExecute(struct MtmcEmu* emu) {
    _MtmcFetchCurrentInstruction(emu);
    if (emu->trace_level > 0) {
        _MtmcTraceState(emu, stderr);
    }
    i16 instr = MtmcGetRegisterValue(emu, IR);
    i16 offset = _MtmcIsDoubleWordInstruction(instr) == 0 ? 2 : 4;
    MtmcSetRegisterValue(emu, PC,
        MtmcGetRegisterValue(emu, PC) + offset);
    MtmcExecInstruction(emu, instr);
}


int MtmcPulse(struct MtmcEmu* emu, int pulse) {
    int count = 0;
    for (; count < pulse && MtmcGetStatus(emu) == MtmcEmuStatus_EXECUTING; ++count) {
        _MtmcFetchAndExecute(emu);
    }
    return count;
}


int MtmcRun(struct MtmcEmu* emu) {
    if (emu->speed == 0) {
        emu->speed = Mtmc_DEFAULT_SPEED;
    }
    emu->status = MtmcEmuStatus_EXECUTING;
    size_t window = emu->speed > 1000 ? 1 : (1000 / emu->speed);
    size_t pulse = emu->speed * window / 1000;
    if (pulse == 0) { pulse = 1; }

    if (emu->trace_level > 0) {
        fputs("executing:\n", stderr);
    }

    while (emu->status == MtmcEmuStatus_EXECUTING) {
        MtmcPulse(emu, pulse);
        PlatformSleep(emu->platform, window);
    }

    if (emu->trace_level > 0) {
        fputs("stopped\n", stderr);
    }

    return 0;
}


i16 MtmcPrintFormat(struct MtmcEmu* emu, i16 addr, i16 stack) {
    i16 total = 0;
    int state = 0;
    u8* p = &emu->memory[addr];
    for (; *p != '\0';) {
        u8 c = *p++;
        switch (state) {
            case 0:
                if (c == '%') {
                    state = 1;
                }
                else {
                    PlatformPutChar(emu->platform, c);
                    total += 1;
                }
                break;
            case 1:
                switch (c) {
                    case 'd':
                    case 'c':
                    case 's':
                        FatalErrorFmt("not implemented: %%%c", c);
                        break;
                    default:
                        PlatformPutChar(emu->platform, '%');
                        PlatformPutChar(emu->platform, c);
                        total += 2;
                }
                break;
        }
    }
    switch (state) {
        case 1:
            PlatformPutChar(emu->platform, '%');
            total += 1;
    }
    return total;
}


void MtosHandleSysCall(struct MtmcEmu* emu, i16 number) {
    switch ((enum MtosSysCall)number) {
        case MtosSysCall_exit:
            emu->status = MtmcEmuStatus_FINISHED;
            break;

        case MtosSysCall_rint: {
            i16 x = PlatformReadWord(emu->platform);
            MtmcSetRegisterValue(emu, RV, x);
            break;
        }

        case MtosSysCall_wint: {
            i16 value = MtmcGetRegisterValue(emu, A0);
            PlatformPutWord(emu->platform, value);
            break;
        }

        case MtosSysCall_rstr:
            FatalErrorFmt("unhandled SYS CALL %02x", (u16)number);
            break;

        case MtosSysCall_wchr: {
            i16 value = MtmcGetRegisterValue(emu, A0);
            PlatformPutChar(emu->platform, value);
            break;
        }

        case MtosSysCall_rchr: {
            u8 c = PlatformGetChar(emu->platform);
            MtmcSetRegisterValue(emu, RV, c);
            break;
        }

        case MtosSysCall_wstr: {
            i16 addr = MtmcGetRegisterValue(emu, A0);
            PlatformPutString(emu->platform, (char*)&emu->memory[addr]);
            break;
        }

        case MtosSysCall_printf: {
            i16 addr = MtmcGetRegisterValue(emu, A0);
            i16 stack = MtmcGetRegisterValue(emu, A1);
            i16 res = MtmcPrintFormat(emu, addr, stack);
            MtmcSetRegisterValue(emu, RV, res);
            break;
        }

        case MtosSysCall_atoi: {
            i16 addr = MtmcGetRegisterValue(emu, A0);
            i16 res = PlatformParseWord(emu->platform, (char*) &emu->memory[addr]);
            MtmcSetRegisterValue(emu, RV, res);
            break;
        }

        case MtosSysCall_rfile: {
            i16 fname = MtmcGetRegisterValue(emu, A0);
            i16 addr = MtmcGetRegisterValue(emu, A1);
            i16 size = MtmcGetRegisterValue(emu, A2);
            i16 lines = MtmcGetRegisterValue(emu, A3);
            i16 res = PlatformFileRead(
                emu->platform,
                (char*)&emu->memory[fname],
                &emu->memory[addr],
                size, lines);
            MtmcSetRegisterValue(emu, RV, res);
            break;
        }

        case MtosSysCall_wfile:
            FatalErrorFmt("unhandled SYS CALL %02x", (u16)number);
            break;

        case MtosSysCall_cwd: {
            i16 addr = MtmcGetRegisterValue(emu, A0);
            i16 bufsize = MtmcGetRegisterValue(emu, A1);
            char path[PATH_MAX];
            int res = PlatformGetCurrentDir(emu->platform, path, sizeof(path));
            if (res > 0) {
                res += 1;
                if (res > bufsize) {
                    res = bufsize;
                }
                for (int i = 0; i < res; ++i) {
                    MtmcWriteByteToMemory(emu, addr + i, path[i]);
                }
            }
            else {
                res = 0;
            }
            MtmcSetRegisterValue(emu, RV, res);
            break;
        }

        case MtosSysCall_chdir: {
            i16 name = MtmcGetRegisterValue(emu, A0);
            i16 res = PlatformSetCurrentDir(emu->platform,
                (char*) &emu->memory[name]);
            MtmcSetRegisterValue(emu, RV, res);
            break;
        }

        case MtosSysCall_dirent: {
            i16 name = MtmcGetRegisterValue(emu, A0);
            i16 cmd = MtmcGetRegisterValue(emu, A1);
            switch ((enum MtmcDirentCommand)cmd) {
                case MtmcDirent_count: {
                    i16 size = PlatformDirGetSize(emu->platform,
                        (char*) &emu->memory[name]);
                    MtmcSetRegisterValue(emu, RV, size);
                    break;
                }
                case MtmcDirent_get_entry: {
                    i16 i = MtmcGetRegisterValue(emu, A2);
                    i16 addr = MtmcGetRegisterValue(emu, A3);
                    i16 bufsize = MtmcFetchWordFromMemory(emu, addr + 2);
                    char* buf = (char*) &emu->memory[addr + 4];
                    i16 flags = 0;
                    i16 res = PlatformDirReadEntry(emu->platform,
                        (char*) &emu->memory[name], i, &flags, buf, bufsize);
                    MtmcWriteWordToMemory(emu, addr, flags);
                    MtmcSetRegisterValue(emu, RV, res);
                    break;
                }
            }
            break;
        }

        case MtosSysCall_dfile:
            FatalErrorFmt("unhandled SYS CALL %02x", (u16)number);
            break;

        case MtosSysCall_rnd: {
            i16 start = MtmcGetRegisterValue(emu, A0);
            i16 stop = MtmcGetRegisterValue(emu, A1);
            if (start > stop) {
                i16 t = stop;
                stop = start;
                start = t;
            }
            i16 x = PlatformRandom(emu->platform, start, stop + 1);
            MtmcSetRegisterValue(emu, RV, x);
            break;
        }

        case MtosSysCall_sleep:
            PlatformSleep(emu->platform,
                MtmcGetRegisterValue(emu, A0));
            break;

        case MtosSysCall_timer: {
            i16 t = MtmcGetRegisterValue(emu, A0);
            i16 x = PlatformSetTimer(emu->platform, t);
            MtmcSetRegisterValue(emu, RV, x);
            break;
        }

        case MtosSysCall_fbreset:
            PlatformResetFrame(emu->platform);
            break;

        case MtosSysCall_fbstat:
        case MtosSysCall_fbset:
        case MtosSysCall_fbline:
            FatalErrorFmt("unhandled SYS CALL %02x", (u16)number);
            break;

        case MtosSysCall_fbrect:
            PlatformFillRect(emu->platform,
                MtmcGetRegisterValue(emu, A0),
                MtmcGetRegisterValue(emu, A1),
                MtmcGetRegisterValue(emu, A2),
                MtmcGetRegisterValue(emu, A3)
            );
            break;

        case MtosSysCall_fbflush:
            PlatformDrawFrame(emu->platform);
            break;

        case MtosSysCall_joystick: {
            i16 state = PlatformGetJoystick(emu->platform);
            MtmcSetRegisterValue(emu, IO, state);
            MtmcSetRegisterValue(emu, RV, MtmcGetRegisterValue(emu, IO));
            break;
        }

        case MtosSysCall_scolor: {
            i16 c = MtmcGetRegisterValue(emu, A0);
            PlatformSetColor(emu->platform, c);
            break;
        }

        case MtosSysCall_memcopy: {
            i16 s = MtmcGetRegisterValue(emu, A0);
            i16 t = MtmcGetRegisterValue(emu, A1);
            i16 n = MtmcGetRegisterValue(emu, A2);
            for (i16 i = 0; i < n; ++i) {
                u8 x = MtmcFetchByteFromMemory(emu, s + i);
                MtmcWriteByteToMemory(emu, t + i, x);
            }
            break;
        }

        case MtosSysCall_drawimg: {
            i16 i = MtmcGetRegisterValue(emu, A0);
            i16 x = MtmcGetRegisterValue(emu, A1);
            i16 y = MtmcGetRegisterValue(emu, A2);
            i16 res = 1;
            if (i >= 0 && i < (int)emu->graphics_count) {
                PlatformDrawImage(emu->platform, &emu->graphics[i], x, y);
                res = 0;
            }
            MtmcSetRegisterValue(emu, RV, res);
            break;
        }

        case MtosSysCall_drawimgsz:
        case MtosSysCall_drawimgclip:
            FatalErrorFmt("unhandled SYS CALL %02x", (u16)number);
            break;

        case MtosSysCall_error: {
            i16 addr = MtmcGetRegisterValue(emu, A0);
            emu->status = MtmcEmuStatus_PERMANENT_ERROR;
            fprintf(stderr, "program error: %s\n", (char*) &emu->memory[addr]);
            break;
        }

        default:
            FatalErrorFmt("unhandled SYS CALL %02x", (u16)number);
    }

    if (PlatformIsClosed(emu->platform) != 0) {
        emu->status = MtmcEmuStatus_FINISHED;
    }
}


#ifdef PNG_LIBPNG_VER


static const png_color _MtmcPngPalette[5] = {
    { _MtmcPalette[0][0], _MtmcPalette[0][1], _MtmcPalette[0][2] },
    { _MtmcPalette[1][0], _MtmcPalette[1][1], _MtmcPalette[1][2] },
    { _MtmcPalette[2][0], _MtmcPalette[2][1], _MtmcPalette[2][2] },
    { _MtmcPalette[3][0], _MtmcPalette[3][1], _MtmcPalette[3][2] },
    { _MtmcPalette[4][0], _MtmcPalette[4][1], _MtmcPalette[4][2] },
};


static void _png_error(png_structp png, const char* s) {
    fprintf(stderr, "PNG error: %s\n", s);
}


static void _png_warning(png_structp png, const char* s) {
    fprintf(stderr, "PNG warning: %s\n", s);
}

int MtmcGraphicLoad(struct MtmcGraphic* graphic, FILE* file) {
    png_structp pp = png_create_read_struct(PNG_LIBPNG_VER_STRING,
        NULL, _png_error, _png_warning);
    if (pp == NULL) { return 1; }

    png_infop info = png_create_info_struct(pp);
    if (info == NULL) {
        png_destroy_read_struct(&pp, &info, NULL);
        return 1;
    }

    png_set_user_limits(pp, MtmcGraphics_width_max, MtmcGraphics_height_max);
    png_init_io(pp, file);
    int transforms =
        PNG_TRANSFORM_PACKING |
        PNG_TRANSFORM_EXPAND;
    png_read_png(pp, info, transforms, NULL);

    png_uint_32 width = png_get_image_width(pp, info);
    png_uint_32 height = png_get_image_height(pp, info);
    png_byte colors = png_get_color_type(pp, info);
    png_bytepp rows = png_get_rows(pp, info);

    *graphic = (struct MtmcGraphic) {};
    graphic->width = width;
    graphic->height = height;

    int mw = (width + 7) / 8;
    int dw = (width + 3) / 4;
    int stride;
    if (colors == PNG_COLOR_TYPE_RGBA) {
        stride = 4;
    }
    else if (colors == PNG_COLOR_TYPE_RGB) {
        stride = 3;
    }
    else {
        fprintf(stderr, "PNG error: color type %d not supported\n", (int)colors);
        png_destroy_read_struct(&pp, &info, NULL);
        return 1;
    }

    for (size_t row = 0; row < height; ++row) {
        for (size_t col = 0; col < width; ++col) {
            int red = rows[row][col * stride];
            int grn = rows[row][col * stride + 1];
            int blu = rows[row][col * stride + 2];
            int alp = stride == 4 ? rows[row][col * stride + 3] : 255;
            u8 mask = ((alp < 127) ? 1 : 0) << (col % 8);
            u8 c = 0;
            int d = 0x7fffffff;
            for (int pi = 0; pi < 4; ++pi) {
                int r = (red - _MtmcPngPalette[pi].red);
                int g = (grn - _MtmcPngPalette[pi].green);
                int b = (blu - _MtmcPngPalette[pi].blue);
                int dist = r * r + g * g + b * b;
                if (dist < d) {
                    d = dist;
                    c = pi;
                }
            }
            u8 mc = c << (col % 4 * 2);
            graphic->mask[row * mw + col / 8] |= mask;
            graphic->data[row * dw + col / 4] |= mc;
        }
    }

    png_destroy_read_struct(&pp, &info, NULL);
    return 0;
}


static int _MtmcGraphicLoadPng(struct MtmcGraphic* graphic,
    const u8* data, size_t datasize) {
    FILE* fp = fmemopen((void*)data, datasize, "rb");
    if (fp == NULL) { perror("fmemopen"); return 1; }
    int res = MtmcGraphicLoad(graphic, fp);
    fclose(fp);
    return res;
}


int MtmcGraphicWrite(struct MtmcGraphic* graphic, FILE* file) {
    int hasalpha = MtmcGraphicHasAlpha(graphic);
    png_structp pp = png_create_write_struct(PNG_LIBPNG_VER_STRING,
        NULL, _png_error, _png_warning);
    if (pp == NULL) { return 1; }

    png_infop info = png_create_info_struct(pp);
    if (info == NULL) {
        png_destroy_write_struct(&pp, &info);
        return 1;
    }

    png_init_io(pp, file);
    png_set_IHDR(pp, info,
        graphic->width, graphic->height,
        8,
        PNG_COLOR_TYPE_PALETTE,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);

    png_set_PLTE(pp, info, _MtmcPngPalette, hasalpha + 4);
    if (hasalpha != 0) {
        static const png_byte trns[] = { 255, 255, 255, 255, 0 };
        png_set_tRNS(pp, info, trns, 5, NULL);
    }
    png_write_info(pp, info);
    png_set_packing(pp);

    u8 pixel[MtmcGraphics_width_max];
    size_t mw = (graphic->width + 7) / 8;
    size_t dw = (graphic->width + 3) / 4;
    for (int row = 0; row < graphic->height; ++row) {
        for (int col = 0; col < graphic->width; ++col) {
            u8 a = graphic->mask[row * mw + col / 8] >> (col % 8);
            u8 c = graphic->data[row * dw + col / 4] >> (col % 4 * 2);
            pixel[col] = ((a & 1) == 0) ? (c & 3) : 4;
        }
        png_write_row(pp, pixel);
    }

    png_write_end(pp, info);
    png_destroy_write_struct(&pp, &info);
    return 0;
}


#endif /* PNG_LIBPNG_VER */


#ifdef PAIV_JSON_NUMBER_BACKEND_TYPE


#define _assert_json_ok(code, msg) {\
    if (code != JsonError_ok) {\
    fprintf(stderr, msg ": error %d\n", code);\
    return 1; }}


static JsonError _json_reader_read_i8_array(JSON* context, u8* buf, size_t* bufsize) {
    size_t limit = *bufsize;
    *bufsize = 0;
    JSON ar;
    JsonValueType type;
    JsonError err = json_reader_open_array(context, &ar);
    _assert_json_ok(err, "json_reader_open_array");
    size_t count = 0;
    for (;;) {
        err = json_reader_read_array(&ar, &type);
        if (err == JsonError_not_found) { break; }
        _assert_json_ok(err, "json_reader_read_array");

        if (count >= limit) {
            fprintf(stderr, "error: array is over %zu bytes\n", limit);
            return JsonError_bufsize;
        }

        if (type != JsonValueType_number) {
            return JsonError_type_mismatch;
        }

        int x;
        err = json_reader_read_numberi(&ar, &x);
        _assert_json_ok(err, "json_reader_read_numberi");
        if (x < -128 || x > 127) {
            fprintf(stderr, "error: invalid byte value %d\n", x);
            return JsonError_invalid;
        }

        buf[count++] = x;
    }
    *bufsize = count;
    return JsonError_ok;
}


static JsonError _json_reader_read_graphics(JSON* context, struct MtmcGraphic* graphic) {
    u8 data[MtmcGraphics_bytes_max];
    size_t datasize = sizeof(data);
    JsonError err = _json_reader_read_i8_array(context, data, &datasize);
    _assert_json_ok(err, "json_reader_read_i8_array");
    int res = _MtmcGraphicLoadPng(graphic, data, datasize);
    if (res != 0) { return JsonError_invalid; }
    return JsonError_ok;
}


int MtmcExecutableLoad(FILE* file, struct MtmcExecutable* exe) {
    JSON json, content, ar;
    int err = json_reader_init(&json, file);
    _assert_json_ok(err, "json_reader_init");

    err = json_reader_open_object(&json, &content);
    _assert_json_ok(err, "json_reader_open_object");

    JsonValueType type;
    char key[20], svalue[100];
    size_t keysize, valsize;

    for (;;) {
        keysize = sizeof(key);
        int err = json_reader_read_object(&content, &keysize, key, &type);
        if (err == JsonError_not_found) { break; }
        _assert_json_ok(err, "json_reader_read_object");

        if (strcmp(key, "format") == 0) {
            valsize = sizeof(svalue);
            err = json_reader_read_string(&content, &valsize, svalue);
            _assert_json_ok(err, "json_reader_read_string");

            if (strcmp(svalue, MtmcFormatOrc1) == 0) {
                exe->format = MtmcExecutableFormat_orc1;
            }
            else {
                fprintf(stderr, "unexpected executable format '%s'\n", svalue);
                return 1;
            }
        }
        else if (strcmp(key, "code") == 0) {
            size_t codesize = 0;
            err = json_reader_open_array(&content, &ar);
            _assert_json_ok(err, "json_reader_open_array");
            for (;;) {
                err = json_reader_read_array(&ar, &type);
                if (err == JsonError_not_found) { break; }
                _assert_json_ok(err, "json_reader_read_array");

                if (codesize >= sizeof(exe->code)/sizeof(exe->code[0])) {
                    fprintf(stderr, "code: out of memory at %zd\n", codesize);
                    return 1;
                }

                if (type == JsonValueType_number) {
                    int x;
                    err = json_reader_read_numberi(&ar, &x);
                    _assert_json_ok(err, "json_reader_read_numberi");
                    exe->code[codesize++] = x;
                }
                else {
                    fprintf(stderr, "code: unexpected value type %d\n", type);
                    return 1;
                }
            }
            exe->codesize = codesize;
        }
        else if (strcmp(key, "data") == 0) {
            size_t datasize = 0;
            err = json_reader_open_array(&content, &ar);
            _assert_json_ok(err, "json_reader_open_array");
            for (;;) {
                err = json_reader_read_array(&ar, &type);
                if (err == JsonError_not_found) { break; }
                _assert_json_ok(err, "json_reader_read_array");

                if (datasize >= sizeof(exe->data)/sizeof(exe->data[0])) {
                    fprintf(stderr, "data: out of memory at %zd\n", datasize);
                    return 1;
                }

                if (type == JsonValueType_number) {
                    int x;
                    err = json_reader_read_numberi(&ar, &x);
                    _assert_json_ok(err, "json_reader_read_numberi");
                    exe->data[datasize++] = x;
                }
                else {
                    fprintf(stderr, "data: unexpected value type %d\n", type);
                    return 1;
                }
            }
            exe->datasize = datasize;
        }
        else if (strcmp(key, "graphics") == 0) {
            err = json_reader_open_array(&content, &ar);
            _assert_json_ok(err, "json_reader_open_array");
            for (;;) {
                err = json_reader_read_array(&ar, &type);
                if (err == JsonError_not_found) { break; }
                _assert_json_ok(err, "json_reader_read_array");

                if (exe->graphics_count >= MtmcGraphics_max) {
                    fprintf(stderr, "error: max graphics limit %d\n", MtmcGraphics_max);
                    err = json_reader_consume_value(&ar);
                    _assert_json_ok(err, "json_reader_consume_value");
                    continue;
                }

                if (type != JsonValueType_array) {
                    fprintf(stderr, "invalid graphics json type %d\n", type);
                    err = json_reader_consume_value(&ar);
                    _assert_json_ok(err, "json_reader_consume_value");
                    exe->graphics_count += 1;
                    continue;
                }

                err = _json_reader_read_graphics(&ar, &exe->graphics[exe->graphics_count]);
                _assert_json_ok(err, "json_reader_read_graphics");
                exe->graphics_count += 1;
            }
        }
        else {
            err = json_reader_consume_value(&content);
            _assert_json_ok(err, "json_reader_consume_value");
        }
    }

    return 0;
}


#endif /* PAIV_JSON_ */


int MtmcPlatformRun(PlatformState platform, FILE* file, const char* arg,
    int speed, int trace_level) {
    struct MtmcExecutable exe = {};
    int res = MtmcExecutableLoad(file, &exe);
    if (res != 0) { return res; }
    struct MtmcEmu emu = {
        .platform = platform,
        .speed = speed,
        .trace_level = trace_level,
        };
    MtmcLoad(&emu, &exe);
    if (arg != NULL) {
        MtmcSetArg(&emu, arg);
    }
    MtmcRun(&emu);
    return 0;
}


#endif /* PAIV_MTMC_IMPLEMENTATION */
