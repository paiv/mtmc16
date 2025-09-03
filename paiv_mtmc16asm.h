#ifndef _PAIV_MTMC_INCLUDE_MTMCASM_H_
#define _PAIV_MTMC_INCLUDE_MTMCASM_H_


#ifdef __cplusplus
extern "C" {
#endif


enum {
    MtmcAsm_identifier_max = 64,
    MtmcAsm_symbols_max = 1024,
};


int MtmcAssemble(FILE* source, FILE* output, const char* source_filename);
int MtmcDisassemble(FILE* input, FILE* output, const char* input_filename,
    int code_bytes, int graphics);


#ifdef __cplusplus
}
#endif


#endif /* _PAIV_MTMC_INCLUDE_MTMCASM_H_ */


#ifdef PAIV_MTMCASM_IMPLEMENTATION


struct _GraphicsImport {
    char filename[PATH_MAX];
};


struct MtmcExeObject {
    enum MtmcExecutableFormat format;
    size_t codesize;
    u8 code[Mtmc_MEMORY_SIZE];
    size_t datasize;
    u8 data[Mtmc_MEMORY_SIZE];
    size_t graphics_count;
    struct _GraphicsImport graphics[MtmcGraphics_max];
};


int MtmcAssemblerCompileSource(FILE* source, struct MtmcExeObject* exe,
    const char* source_path);
int MtmcAssemblerLinkExecutable(struct MtmcExeObject* exe, FILE* output);
int MtmcDecompileExecutable(struct MtmcExecutable* exe, FILE* output,
    int code_bytes);


enum AsmTokenType {
    AsmTokenType_invalid,
    AsmTokenType_eof,
    AsmTokenType_newline,
    AsmTokenType_number,
    AsmTokenType_string,
    AsmTokenType_strchunk,
    AsmTokenType_identifier,
    AsmTokenType_label,
    AsmTokenType_directive,
};


struct AsmToken {
    enum AsmTokenType type;
    int line;
    int col;
    size_t size;
    char text[MtmcAsm_identifier_max];
    char _null;
};


enum _TokenizerStatus {
    _TokState_none,
    _TokState_comment,
    _TokState_number,
    _TokState_string,
    _TokState_escape,
    _TokState_identifier,
    _TokState_directive,
};


enum _AssemblerStatus {
    _AsmState_none,
    _AsmState_data,
    _AsmState_data_chunk,
    _AsmState_data_bytes,
    _AsmState_data_words,
    _AsmState_data_image,
    _AsmState_code,
    _AsmState_instruction,
};


struct _SymTable {
    size_t datasize;
    char data[MtmcAsm_symbols_max * (MtmcAsm_identifier_max + 1)];
    size_t symcount;
    char* symbols[MtmcAsm_symbols_max];
};


struct _AsmLabel {
    u8 isdata;
    u16 addr;
    const char* symbol;
};


struct _ForwardRef {
    int line;
    int col;
    u16 addr;
    const char* symbol;
};


struct AssemblerState {
    FILE* source;
    struct MtmcExeObject* exe;
    int line;
    int col;
    enum _TokenizerStatus tokstate;
    enum _AssemblerStatus status;
    int argcount;
    struct _SymTable symtable;
    size_t labelcount;
    struct _AsmLabel labels[Mtmc_MEMORY_SIZE];
    size_t forwardsize;
    struct _ForwardRef forwardrefs[Mtmc_MEMORY_SIZE+1];
};


static const char* _AsmTokenType(enum AsmTokenType type) {
    static const char* const names[] = {
        "invalid",
        "eof",
        "newline",
        "number",
        "string",
        "strchunk",
        "identifier",
        "label",
        "directive",
    };
    return names[type];
}


#define _TokenizerError(msg, ...) {\
    fprintf(stderr, ":%d:%d: " msg "\n", state->line, state->col, __VA_ARGS__);\
    return 1; }


int MtmcAssemblerReadToken(struct AssemblerState* state, struct AsmToken* token) {
    *token = (struct AsmToken) {
        .line = state->line,
        .col = state->col,
    };
    for (;;) {
        int c = fgetc(state->source);
        state->col += 1;
        if (c == EOF) {
            switch (state->tokstate) {
                case _TokState_none:
                    *token = (struct AsmToken) {
                        .type = AsmTokenType_eof,
                        .line = state->line,
                        .col = state->col,
                    };
                    return 0;
                case _TokState_number:
                case _TokState_identifier:
                case _TokState_directive:
                    state->col -= 1;
                    state->tokstate = _TokState_none;
                    return 0;
                default:
                    _TokenizerError("unexpected EOF%s", "");
            }
        }
        if (c == '\n') {
            switch (state->tokstate) {
                case _TokState_none:
                case _TokState_comment:
                    *token = (struct AsmToken) {
                        .type = AsmTokenType_newline,
                        .line = state->line,
                        .col = state->col,
                    };
                    state->line += 1;
                    state->col = 0;
                    state->tokstate = _TokState_none;
                    return 0;
                case _TokState_number:
                case _TokState_identifier:
                case _TokState_directive:
                    ungetc(c, state->source);
                    state->col -= 1;
                    state->tokstate = _TokState_none;
                    return 0;
                default:
                    _TokenizerError("unexpected EOL%s", "");
            }
        }
        switch (state->tokstate) {
            case _TokState_none:
                if (isspace(c) == 0) {
                    *token = (struct AsmToken) {
                        .line = state->line,
                        .col = state->col,
                    };
                    switch (c) {
                        case '#':
                            state->tokstate = _TokState_comment;
                            break;
                        case '-':
                        case '0' ... '9':
                            token->type = AsmTokenType_number;
                            token->size = 1;
                            token->text[0] = c;
                            state->tokstate = _TokState_number;
                            break;
                        case '"':
                            token->type = AsmTokenType_string;
                            state->tokstate = _TokState_string;
                            break;
                        case '.':
                            token->type = AsmTokenType_directive;
                            token->size = 1;
                            token->text[0] = c;
                            state->tokstate = _TokState_directive;
                            break;
                        default:
                            if (isalpha(c) != 0 || c == '_') {
                                token->type = AsmTokenType_identifier;
                                token->size = 1;
                                token->text[0] = c;
                                state->tokstate = _TokState_identifier;
                            }
                            else {
                                _TokenizerError("unexpected symbol '%c'", c);
                            }
                            break;
                    }
                }
                break;

            case _TokState_comment:
                if (c == '\n') {
                    state->tokstate = _TokState_none;
                }
                break;

            case _TokState_number:
                if (c == '#') {
                    state->tokstate = _TokState_comment;
                    return 0;
                }
                else if (isdigit(c) != 0) {
                    if (token->size >= sizeof(token->text)) {
                        _TokenizerError("invalid number '%s'", token->text);
                    }
                    token->text[token->size++] = c;
                }
                else if (token->size == 1 && token->text[0] == '0' &&
                        (c == 'x' || c == 'X' || c == 'b')) {
                    token->text[token->size++] = c;
                }
                else if (c == '_') {
                }
                else if (isspace(c) != 0) {
                    state->tokstate = _TokState_none;
                    return 0;
                }
                else {
                    _TokenizerError("number '%s' invalid symbol '%c'", token->text, c);
                }
                break;

            case _TokState_string:
                if (c == '\\') {
                    if (token->size >= sizeof(token->text)) {
                        ungetc(c, state->source);
                        token->type = AsmTokenType_strchunk;
                        return 0;
                    }
                    state->tokstate = _TokState_escape;
                }
                else if (c == '"') {
                    token->type = AsmTokenType_string;
                    state->tokstate = _TokState_none;
                    return 0;
                }
                else {
                    if (token->size >= sizeof(token->text)) {
                        ungetc(c, state->source);
                        token->type = AsmTokenType_strchunk;
                        return 0;
                    }
                    token->text[token->size++] = c;
                }
                break;

            case _TokState_escape:
                switch (c) {
                    case 'b': c = '\b'; break;
                    case 't': c = '\t'; break;
                    case 'n': c = '\n'; break;
                    case 'f': c = '\f'; break;
                    case 'r': c = '\r'; break;
                }
                token->text[token->size++] = c;
                state->tokstate = _TokState_string;
                break;

            case _TokState_identifier:
                if (c == '#') {
                    state->tokstate = _TokState_comment;
                    return 0;
                }
                else if (c == ':') {
                    token->type = AsmTokenType_label;
                    state->tokstate = _TokState_none;
                    return 0;
                }
                else if (isalnum(c) != 0 || c == '_') {
                    if (token->size >= sizeof(token->text)) {
                        _TokenizerError("invalid identifier '%s'", token->text);
                    }
                    token->text[token->size++] = c;
                }
                else if (isspace(c) != 0) {
                    state->tokstate = _TokState_none;
                    return 0;
                }
                else {
                    _TokenizerError("identifier '%s' invalid symbol '%c'", token->text, c);
                }
                break;

            case _TokState_directive:
                if (c == '#') {
                    state->tokstate = _TokState_comment;
                    return 0;
                }
                #if 0
                else if (c == ':' && token->size > 1) {
                    fprintf(stderr, ":%d:%d: warning: extra ':' after directive '%*s'\n",
                        token->line, token->col, (int) token->size, token->text);
                    state->tokstate = _TokState_none;
                    return 0;
                }
                #endif
                else if (isspace(c) == 0) {
                    if (token->size >= sizeof(token->text)) {
                        _TokenizerError("invalid directive '%s'", token->text);
                    }
                    token->text[token->size++] = c;
                }
                else {
                    state->tokstate = _TokState_none;
                    return 0;
                }
                break;
        }
    }
    FatalError();
}


static int _AssemblerError(struct AsmToken* token, const char* msg, ...) {
    fprintf(stderr, ":%d:%d: ", token->line, token->col);
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    fprintf(stderr, " [#%d %s] \"%*s\"\n",
        token->type, _AsmTokenType(token->type),
        (int) token->size, token->text);
    return 1;
}


struct _AsmInstr {
    enum MtmcInstructionType type;
    u8 code;
    const char* name;
    u8 arity;
    u8 arity_max;
    u8 dword;
};


static const struct _AsmInstr _Instructions[] = {
    { MtmcInstructionType_MISC, MtmcInstructionMisc_sys, "sys", .arity = 1 },
    { MtmcInstructionType_MISC, MtmcInstructionMisc_mov, "mov", .arity = 2 },
    { MtmcInstructionType_MISC, MtmcInstructionMisc_inc, "inc", .arity = 1, 2 },
    { MtmcInstructionType_MISC, MtmcInstructionMisc_dec, "dec", .arity = 1, 2 },
    { MtmcInstructionType_MISC, MtmcInstructionMisc_seti, "seti", .arity = 2 },
    { MtmcInstructionType_MISC, MtmcInstructionMisc_mcp, "mcp", .arity = 3, .dword = 1 },
    { MtmcInstructionType_MISC, MtmcInstructionMisc_debug, "debug", .arity = 1 },
    { MtmcInstructionType_MISC, MtmcInstructionMisc_nop, "nop", .arity = 0 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_add, "add", .arity = 2 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_sub, "sub", .arity = 2 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_mul, "mul", .arity = 2 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_div, "div", .arity = 2 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_mod, "mod", .arity = 2 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_and, "and", .arity = 2 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_or, "or", .arity = 2 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_xor, "xor", .arity = 2 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_shl, "shl", .arity = 2 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_shr, "shr", .arity = 2 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_min, "min", .arity = 2 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_max, "max", .arity = 2 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_not, "not", .arity = 1 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_lnot, "lnot", .arity = 1 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_neg, "neg", .arity = 1 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_imm, "imm", .arity = 3, .dword = 1 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_imm, "addi", .arity = 2, .dword = 1 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_imm, "subi", .arity = 2, .dword = 1 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_imm, "muli", .arity = 2, .dword = 1 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_imm, "divi", .arity = 2, .dword = 1 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_imm, "modi", .arity = 2, .dword = 1 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_imm, "andi", .arity = 2, .dword = 1 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_imm, "ori", .arity = 2, .dword = 1 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_imm, "xori", .arity = 2, .dword = 1 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_imm, "shli", .arity = 2, .dword = 1 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_imm, "shri", .arity = 2, .dword = 1 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_imm, "mini", .arity = 2, .dword = 1 },
    { MtmcInstructionType_ALU, MtmcInstructionAlu_imm, "maxi", .arity = 2, .dword = 1 },
    { MtmcInstructionType_STACK, MtmcInstructionStack_push, "push", .arity = 1, 2 },
    { MtmcInstructionType_STACK, MtmcInstructionStack_pop, "pop", .arity = 1, 2 },
    { MtmcInstructionType_STACK, MtmcInstructionStack_dup, "dup", .arity = 0, 1 },
    { MtmcInstructionType_STACK, MtmcInstructionStack_swap, "swap", .arity = 0, 1 },
    { MtmcInstructionType_STACK, MtmcInstructionStack_drop, "drop", .arity = 0, 1 },
    { MtmcInstructionType_STACK, MtmcInstructionStack_over, "over", .arity = 0, 1 },
    { MtmcInstructionType_STACK, MtmcInstructionStack_rot, "rot", .arity = 0, 1 },
    { MtmcInstructionType_STACK, MtmcInstructionStack_sop, "sop", .arity = 1, 2 },
    { MtmcInstructionType_STACK, MtmcInstructionStack_pushi, "pushi", .arity = 1, 2, .dword = 1 },
    { MtmcInstructionType_TEST, MtmcInstructionTest_eq, "eq", .arity = 2 },
    { MtmcInstructionType_TEST, MtmcInstructionTest_neq, "neq", .arity = 2 },
    { MtmcInstructionType_TEST, MtmcInstructionTest_gt, "gt", .arity = 2 },
    { MtmcInstructionType_TEST, MtmcInstructionTest_gte, "gte", .arity = 2 },
    { MtmcInstructionType_TEST, MtmcInstructionTest_lt, "lt", .arity = 2 },
    { MtmcInstructionType_TEST, MtmcInstructionTest_lte, "lte", .arity = 2 },
    { MtmcInstructionType_TEST, MtmcInstructionTest_eqi, "eqi", .arity = 2 },
    { MtmcInstructionType_TEST, MtmcInstructionTest_neqi, "neqi", .arity = 2 },
    { MtmcInstructionType_TEST, MtmcInstructionTest_gti, "gti", .arity = 2 },
    { MtmcInstructionType_TEST, MtmcInstructionTest_gtei, "gtei", .arity = 2 },
    { MtmcInstructionType_TEST, MtmcInstructionTest_lti, "lti", .arity = 2 },
    { MtmcInstructionType_TEST, MtmcInstructionTest_ltei, "ltei", .arity = 2 },
    { MtmcInstructionType_LWR, 0, "lwr", .arity = 3 },
    { MtmcInstructionType_LBR, 0, "lbr", .arity = 3 },
    { MtmcInstructionType_SWR, 0, "swr", .arity = 3 },
    { MtmcInstructionType_SBR, 0, "sbr", .arity = 3 },
    { MtmcInstructionType_LOAD, MtmcInstructionLoad_lw, "lw", .arity = 2, .dword = 1 },
    { MtmcInstructionType_LOAD, MtmcInstructionLoad_lwo, "lwo", .arity = 3, .dword = 1 },
    { MtmcInstructionType_LOAD, MtmcInstructionLoad_lb, "lb", .arity = 2, .dword = 1 },
    { MtmcInstructionType_LOAD, MtmcInstructionLoad_lbo, "lbo", .arity = 3, .dword = 1 },
    { MtmcInstructionType_LOAD, MtmcInstructionLoad_sw, "sw", .arity = 2, .dword = 1 },
    { MtmcInstructionType_LOAD, MtmcInstructionLoad_swo, "swo", .arity = 3, .dword = 1 },
    { MtmcInstructionType_LOAD, MtmcInstructionLoad_sb, "sb", .arity = 2, .dword = 1 },
    { MtmcInstructionType_LOAD, MtmcInstructionLoad_sbo, "sbo", .arity = 3, .dword = 1 },
    { MtmcInstructionType_LOAD, MtmcInstructionLoad_li, "li", .arity = 2, .dword = 1 },
    { MtmcInstructionType_LOAD, MtmcInstructionLoad_la, "la", .arity = 2, .dword = 1 },
    { MtmcInstructionType_JUMPREG, 0, "jr", .arity = 1 },
    { MtmcInstructionType_RET, 0, "ret", .arity = 0 },
    { MtmcInstructionType_JUMP, 0, "j", .arity = 1 },
    { MtmcInstructionType_JUMPZ, 0, "jz", .arity = 1 },
    { MtmcInstructionType_JUMPNZ, 0, "jnz", .arity = 1 },
    { MtmcInstructionType_JUMPAL, 0, "jal", .arity = 1 },
};


static int _MtmcAssemblerGetInstruction(struct AsmToken* token,
    const struct _AsmInstr** instr) {
    size_t n = sizeof(_Instructions) / sizeof(_Instructions[0]);
    const struct _AsmInstr* p = &_Instructions[0];
    for (size_t i = 0; i < n; ++i, ++p) {
        if (strcmp(token->text, p->name) == 0) {
            *instr = p;
            return 0;
        }
    }
    return _AssemblerError(token, "invalid instruction");
}


static int _MtmcAssemblerTryParseRegister(struct AsmToken* token, i16* reg) {
    int res =
        (token->size != 2) ? -1 :
        (strcmp(token->text, "t0") == 0) ? T0 :
        (strcmp(token->text, "t1") == 0) ? T1 :
        (strcmp(token->text, "t2") == 0) ? T2 :
        (strcmp(token->text, "t3") == 0) ? T3 :
        (strcmp(token->text, "t4") == 0) ? T4 :
        (strcmp(token->text, "t5") == 0) ? T5 :
        (strcmp(token->text, "a0") == 0) ? A0 :
        (strcmp(token->text, "a1") == 0) ? A1 :
        (strcmp(token->text, "a2") == 0) ? A2 :
        (strcmp(token->text, "a3") == 0) ? A3 :
        (strcmp(token->text, "rv") == 0) ? RV :
        (strcmp(token->text, "ra") == 0) ? RA :
        (strcmp(token->text, "fp") == 0) ? FP :
        (strcmp(token->text, "sp") == 0) ? SP :
        (strcmp(token->text, "bp") == 0) ? BP :
        (strcmp(token->text, "pc") == 0) ? PC :
        -1;
    if (res < 0) { return 1; }
    *reg = res;
    return 0;
}


static int _MtmcAssemblerParseRegister(struct AsmToken* token, i16* reg) {
    int res = _MtmcAssemblerTryParseRegister(token, reg);
    if (res == 0) { return 0; }
    return _AssemblerError(token, "invalid register");
}


static int _MtmcAssemblerTryParseNumber(struct AsmToken* token, i16* value) {
    int state = 0;
    int x = 0;
    int sign = 1;
    const char* p = token->text;
    for (size_t i = 0; i < token->size; ++i) {
        char c = *p++;
        switch (state) {
            case 0:
                switch (c) {
                    case '-':
                        sign = -1;
                        state = 3;
                        break;
                    case '0':
                        state = 2;
                        break;
                    case '1' ... '9':
                        x = c - '0';
                        state = 3;
                        break;
                    default:
                        return 1;
                }
                break;
            case 2:
                switch (c) {
                    case 'x':
                    case 'X':
                        state = 5;
                        break;
                    case 'b':
                        state = 7;
                        break;
                    case '0' ... '9':
                        x = c - '0';
                        state = 3;
                        break;
                    default:
                        return 1;
                }
                break;
            case 3:
                switch (c) {
                    case '0' ... '9':
                        x = x * 10 + (c - '0');
                        break;
                    default:
                        return 1;
                }
                break;
            case 5:
                switch (c) {
                    case '0' ... '9':
                        x = x * 16 + (c - '0');
                        break;
                    case 'a' ... 'f':
                        x = x * 16 + (c - 'a');
                        break;
                    case 'A' ... 'F':
                        x = x * 16 + (c - 'A');
                        break;
                    default:
                        return 1;
                }
                break;
            case 7:
                switch (c) {
                    case '0':
                        x *= 2;
                        break;
                    case '1':
                        x = x * 2 + 1;
                        break;
                    default:
                        return 1;
                }
                break;
        }
    }
    switch (state) {
        case 0:
            return 1;
        case 2:
            break;
        case 3:
            x *= sign;
            if (x < -32768 || x > 32767) {
                return 1;
            }
            break;
        case 5:
        case 7:
            if (x < 0 || x > 65535) {
                return 1;
            }
            break;
    }
    *value = x;
    return 0;
}


static int _MtmcAssemblerParseNumber(struct AsmToken* token, i16* value) {
    int res = _MtmcAssemblerTryParseNumber(token, value);
    if (res == 0) { return 0; }
    return _AssemblerError(token, "invalid number");
}


static int _MtmcAssemblerTryParseSysCall(struct AsmToken* token, i16* addr) {
    if (token->size < 3) { return 1; }
    int res = -1;
    switch (token->text[0]) {
        case 'a':
            res =
                (strncmp("atoi", token->text, token->size) == 0) ? MtosSysCall_atoi :
                -1;
            break;
        case 'c':
            res =
                (strncmp("cwd", token->text, token->size) == 0) ? MtosSysCall_cwd :
                (strncmp("chdir", token->text, token->size) == 0) ? MtosSysCall_chdir :
                -1;
            break;
        case 'd':
            res =
                (strncmp("dirent", token->text, token->size) == 0) ? MtosSysCall_dirent :
                (strncmp("dfile", token->text, token->size) == 0) ? MtosSysCall_dfile :
                (strncmp("drawimg", token->text, token->size) == 0) ? MtosSysCall_drawimg :
                -1;
            break;
        case 'e':
            res =
                (strncmp("exit", token->text, token->size) == 0) ? MtosSysCall_exit :
                (strncmp("error", token->text, token->size) == 0) ? MtosSysCall_error :
                -1;
            break;
        case 'f':
            res =
                (strncmp("fbreset", token->text, token->size) == 0) ? MtosSysCall_fbreset :
                (strncmp("fbstat", token->text, token->size) == 0) ? MtosSysCall_fbstat :
                (strncmp("fbset", token->text, token->size) == 0) ? MtosSysCall_fbset :
                (strncmp("fbline", token->text, token->size) == 0) ? MtosSysCall_fbline :
                (strncmp("fbrect", token->text, token->size) == 0) ? MtosSysCall_fbrect :
                (strncmp("fbflush", token->text, token->size) == 0) ? MtosSysCall_fbflush :
                -1;
            break;
        case 'j':
            res =
                (strncmp("joystick", token->text, token->size) == 0) ? MtosSysCall_joystick :
                -1;
            break;
        case 'm':
            res =
                (strncmp("memcopy", token->text, token->size) == 0) ? MtosSysCall_memcopy :
                -1;
            break;
        case 'p':
            res =
                (strncmp("printf", token->text, token->size) == 0) ? MtosSysCall_printf :
                -1;
            break;
        case 'r':
            res =
                (strncmp("rint", token->text, token->size) == 0) ? MtosSysCall_rint :
                (strncmp("rstr", token->text, token->size) == 0) ? MtosSysCall_rstr :
                (strncmp("rchr", token->text, token->size) == 0) ? MtosSysCall_rchr :
                (strncmp("rfile", token->text, token->size) == 0) ? MtosSysCall_rfile :
                (strncmp("rnd", token->text, token->size) == 0) ? MtosSysCall_rnd :
                -1;
            break;
        case 's':
            res =
                (strncmp("sleep", token->text, token->size) == 0) ? MtosSysCall_sleep :
                (strncmp("scolor", token->text, token->size) == 0) ? MtosSysCall_scolor :
                -1;
            break;
        case 't':
            res =
                (strncmp("timer", token->text, token->size) == 0) ? MtosSysCall_timer :
                -1;
            break;
        case 'w':
            res =
                (strncmp("wint", token->text, token->size) == 0) ? MtosSysCall_wint :
                (strncmp("wchr", token->text, token->size) == 0) ? MtosSysCall_wchr :
                (strncmp("wstr", token->text, token->size) == 0) ? MtosSysCall_wstr :
                (strncmp("wfile", token->text, token->size) == 0) ? MtosSysCall_wfile :
                -1;
            break;
    }
    if (res < 0) { return 1; }
    *addr = res;
    return 0;
}


static int _MtmcAssemblerParseSysCall(struct AsmToken* token, i16* addr) {
    int res = _MtmcAssemblerTryParseSysCall(token, addr);
    if (res == 0) { return 0; }
    return _AssemblerError(token, "invalid syscall");
}


static const char* _SymTableAppendSymbol(struct _SymTable* table, const char* name) {
    int n = strlen(name) + 1;
    if (table->datasize + n > sizeof(table->data)) {
        return NULL;
    }
    if (table->symcount + 1 > sizeof(table->symbols)/sizeof(table->symbols[0])) {
        return NULL;
    }
    char* p = &table->data[table->datasize];
    strcpy(p, name);
    table->datasize += n;
    table->symbols[table->symcount++] = p;
    return p;
}


static const char* _SymTableResolveSymbol(struct _SymTable* table, const char* name) {
    char* const * p = &table->symbols[0];
    for (size_t i = 0; i < table->symcount; ++i, ++p) {
        if (strcmp(*p, name) == 0) {
            return *p;
        }
    }
    return _SymTableAppendSymbol(table, name);
}


static int _MtmcAssemblerWriteDataByte(struct MtmcExeObject* exe, u8 value) {
    if (exe->codesize + exe->datasize + 1 > sizeof(exe->code)) {
        fprintf(stderr, "asm error: data is too large\n");
        return 1;
    }
    exe->data[exe->datasize++] = value;
    return 0;
}


static int _MtmcAssemblerWriteDataWord(struct MtmcExeObject* exe, i16 value) {
    int res = _MtmcAssemblerWriteDataByte(exe, (u16)value >> 8);
    if (res != 0) { return res; }
    res = _MtmcAssemblerWriteDataByte(exe, (u16)value & 0xFF);
    if (res != 0) { return res; }
    return 0;
}


static int _MtmcAssemblerEmitDataWord(struct AssemblerState* state,
    struct AsmToken* token) {
    i16 value;
    int res = _MtmcAssemblerParseNumber(token, &value);
    if (res != 0) { return res; }
    res = _MtmcAssemblerWriteDataWord(state->exe, value);
    if (res != 0) { return res; }
    return 0;
}


static int _MtmcAssemblerEmitDataByteArray(struct AssemblerState* state,
    struct AsmToken* token, u8 value) {
    i16 count;
    int res = _MtmcAssemblerParseNumber(token, &count);
    if (res != 0) { return res; }
    if (count < 0) {
        return _AssemblerError(token, "invalid number");
    }
    for (i16 i = 0; i < count; ++i) {
        int res = _MtmcAssemblerWriteDataByte(state->exe, value);
        if (res != 0) { return res; }
    }
    return 0;
}


static int _MtmcAssemblerEmitDataWordArray(struct AssemblerState* state,
    struct AsmToken* token, i16 value) {
    i16 count;
    int res = _MtmcAssemblerParseNumber(token, &count);
    if (res != 0) { return res; }
    if (count < 0) {
        return _AssemblerError(token, "invalid number");
    }
    for (i16 i = 0; i < count; ++i) {
        int res = _MtmcAssemblerWriteDataWord(state->exe, value);
        if (res != 0) { return res; }
    }
    return 0;
}


static int _MtmcAssemblerEmitDataString(struct AssemblerState* state,
    struct AsmToken* token, int chunk) {
    for (size_t i = 0; i < token->size; ++i) {
        int res = _MtmcAssemblerWriteDataByte(state->exe, token->text[i]);
        if (res != 0) { return res; }
    }
    if (chunk == 0) {
        int res = _MtmcAssemblerWriteDataByte(state->exe, '\0');
        if (res != 0) { return res; }
    }
    return 0;
}


static int _MtmcAssemblerEmitDataLabel(struct AssemblerState* state,
    struct AsmToken* token) {
    const char* symbol = _SymTableResolveSymbol(&state->symtable, token->text);
    u16 addr = state->exe->datasize;
    state->labels[state->labelcount++] = (struct _AsmLabel) {
        .isdata = 1,
        .addr = addr,
        .symbol = symbol,
    };
    return 0;
}


static int _MtmcAssemblerEmitCodeLabel(struct AssemblerState* state,
    struct AsmToken* token) {
    const char* symbol = _SymTableResolveSymbol(&state->symtable, token->text);
    u16 addr = state->exe->codesize;
    state->labels[state->labelcount++] = (struct _AsmLabel) {
        .isdata = 0,
        .addr = addr,
        .symbol = symbol,
    };
    return 0;
}


static int _MtmcAssemblerEmitGraphicsImport(struct AssemblerState* state,
    struct AsmToken* token, const char* source_path) {
    struct MtmcExeObject* exe = state->exe;
    if (exe->graphics_count + 1 > sizeof(exe->graphics)/sizeof(exe->graphics[0])) {
        return _AssemblerError(token, "too many images");
    }
    size_t i = exe->graphics_count++;
    char* s = &exe->graphics[i].filename[0];
    s[0] = '\0';
    if (source_path != NULL) {
        strcpy(s, source_path);
        strcat(s, "/");
    }
    else if (token->text[0] == '/') {
        strcpy(s, ".");
    }
    size_t n = strlen(s) + token->size + 1;
    if (n > sizeof(exe->graphics[0].filename)) {
        return _AssemblerError(token, "filename is too long");
    }
    strncat(s, token->text, token->size);
    return 0;
}


static void _MtmcAssemblerAddForwardReference(struct AssemblerState* state,
    struct AsmToken* token) {
    u16 pc = state->exe->codesize;
    const char* symbol = _SymTableResolveSymbol(&state->symtable, token->text);
    state->forwardrefs[state->forwardsize++] = (struct _ForwardRef) {
        .line = token->line,
        .col = token->col,
        .addr = pc,
        .symbol = symbol,
    };
}


static int _MtmcAssemblerPatchInstruction(struct AssemblerState* state,
    struct _ForwardRef* ref, struct _AsmLabel* label) {
    u16 opcode = (u16)state->exe->code[ref->addr] << 8;
    opcode |= state->exe->code[ref->addr + 1];
    u16 addr = label->addr;
    if (label->isdata != 0) {
        addr += state->exe->codesize;
    }
    switch ((enum MtmcInstructionType) ((opcode >> 12) & 0xF)) {
        case MtmcInstructionType_LOAD:
            state->exe->code[ref->addr + 2] = addr >> 8;
            state->exe->code[ref->addr + 3] = addr & 0xFF;
            break;
        case MtmcInstructionType_JUMP:
        case MtmcInstructionType_JUMPZ:
        case MtmcInstructionType_JUMPNZ:
        case MtmcInstructionType_JUMPAL:
            opcode = (opcode & 0xF000) | addr;
            state->exe->code[ref->addr] = opcode >> 8;
            state->exe->code[ref->addr + 1] = opcode & 0xFF;
            break;
        default:
            FatalErrorFmt("could not patch instruction %04X", opcode);
    }
    return 0;
}


static int _MtmcAssemblerPatchForwardReference(struct AssemblerState* state,
    struct _ForwardRef* ref) {
    struct _AsmLabel* label = &state->labels[0];
    for (size_t i = 0; i < state->labelcount; ++i, ++label) {
        if (label->symbol == ref->symbol) {
            int res = _MtmcAssemblerPatchInstruction(state, ref, label);
            if (res != 0) { return res; }
            return 0;
        }
    }
    fprintf(stderr, ":%d:%d: undefined label '%s'\n", ref->line, ref->col, ref->symbol);
    return 1;
}


static int _MtmcAssemblerTryResolveAddress(struct AssemblerState* state,
    struct AsmToken* token, i16* addr) {
    switch (token->type) {
        case AsmTokenType_number: {
            i16 value;
            int err = _MtmcAssemblerParseNumber(token, &value);
            if (err != 0) { return err; }
            if (value < 0 || value >= Mtmc_MEMORY_SIZE) {
                return _AssemblerError(token, "invalid address");
            }
            *addr = value;
            return 0;
        }
        case AsmTokenType_identifier:
            _MtmcAssemblerAddForwardReference(state, token);
            *addr = 0;
            return 0;
        default:
            return _AssemblerError(token, "invalid address");
    }
}


static int _MtmcAssemblerWriteCodeWord(struct MtmcExeObject* exe, i16 value) {
    if (exe->codesize + exe->datasize + 2 > sizeof(exe->code)) {
        fprintf(stderr, "asm error: program is too large\n");
        return 1;
    }
    exe->code[exe->codesize++] = ((u16)value >> 8) & 0xFF;
    exe->code[exe->codesize++] = ((u16)value) & 0xFF;
    return 0;
}


static int _MtmcAssemblerEmitInstruction(struct AssemblerState* state,
    const struct _AsmInstr* instr, struct AsmToken* token, struct AsmToken* args) {
    if (state->argcount < instr->arity) {
        return _AssemblerError(token, "incomplete instruction");
    }
    i16 nib0 = 0, nib1 = 0;
    i16 nib2 = instr->code;
    i16 word1 = 0;
    switch (instr->type) {

        case MtmcInstructionType_MISC: {
            switch ((enum MtmcInstructionMisc) instr->code) {
                case MtmcInstructionMisc_sys: {
                    i16 sys;
                    int res = _MtmcAssemblerParseSysCall(&args[0], &sys);
                    if (res != 0) { return res; }
                    nib1 = ((u16)sys >> 4) & 0xF;
                    nib0 = ((u16)sys) & 0xF;
                    break;
                }
                case MtmcInstructionMisc_mov: {
                    int res = _MtmcAssemblerParseRegister(&args[0], &nib1);
                    if (res != 0) { return res; }
                    res = _MtmcAssemblerParseRegister(&args[1], &nib0);
                    if (res != 0) { return res; }
                    break;
                }
                case MtmcInstructionMisc_inc:
                case MtmcInstructionMisc_dec:
                case MtmcInstructionMisc_seti: {
                    nib0 = 1;
                    int res = _MtmcAssemblerParseRegister(&args[0], &nib1);
                    if (res != 0) { return res; }
                    if (state->argcount > 1) {
                        int res = _MtmcAssemblerParseNumber(&args[1], &nib0);
                        if (res != 0) { return res; }
                        if (nib0 < 0 || nib0 > 15) {
                            return _AssemblerError(&args[1], "invalid number argument");
                        }
                    }
                    break;
                }
                case MtmcInstructionMisc_mcp:
                case MtmcInstructionMisc_debug:
                    return _AssemblerError(token, "unhandled MISC instruction");

                case MtmcInstructionMisc_nop:
                    nib0 = 0xF;
                    nib1 = 0xF;
                    break;
            }
            break;
        }

        case MtmcInstructionType_ALU: {
            if (instr->code == MtmcInstructionAlu_imm) {
                const struct _AsmInstr* ii;
                if (instr->arity == 2) {
                    struct AsmToken itok = *token;
                    if (itok.text[itok.size - 1] == 'i') {
                        itok.size -= 1;
                        itok.text[itok.size] = '\0';
                    }
                    int res = _MtmcAssemblerGetInstruction(&itok, &ii);
                    if (res != 0) { return 1; }
                    nib0 = ii->code;
                    res = _MtmcAssemblerParseRegister(&args[0], &nib1);
                    if (res != 0) { return res; }
                    res = _MtmcAssemblerParseNumber(&args[1], &word1);
                    if (res != 0) { return res; }
                }
                else {
                    int res = _MtmcAssemblerGetInstruction(&args[0], &ii);
                    if (res != 0) { return 1; }
                    nib0 = ii->code;
                    res = _MtmcAssemblerParseRegister(&args[1], &nib1);
                    if (res != 0) { return res; }
                    res = _MtmcAssemblerParseNumber(&args[2], &word1);
                    if (res != 0) { return res; }
                }
            }
            else if (instr->arity == 2) {
                int res = _MtmcAssemblerParseRegister(&args[0], &nib1);
                if (res != 0) { return res; }
                res = _MtmcAssemblerParseRegister(&args[1], &nib0);
                if (res != 0) { return res; }
            }
            else if (instr->arity == 1) {
                int res = _MtmcAssemblerParseRegister(&args[0], &nib1);
                if (res != 0) { return res; }
            }
            break;
        }

        case MtmcInstructionType_STACK: {
            nib0 = SP;
            switch ((enum MtmcInstructionStack) instr->code) {
                case MtmcInstructionStack_push:
                case MtmcInstructionStack_pop: {
                    int res = _MtmcAssemblerParseRegister(&args[0], &nib1);
                    if (res != 0) { return res; }
                    if (state->argcount > 1) {
                        int res = _MtmcAssemblerParseRegister(&args[1], &nib0);
                        if (res != 0) { return res; }
                    }
                    break;
                }
                case MtmcInstructionStack_dup:
                case MtmcInstructionStack_swap:
                case MtmcInstructionStack_drop:
                case MtmcInstructionStack_over:
                case MtmcInstructionStack_rot: {
                    if (state->argcount > 0) {
                        int res = _MtmcAssemblerParseRegister(&args[0], &nib0);
                        if (res != 0) { return res; }
                    }
                    break;
                }
                case MtmcInstructionStack_sop: {
                    const struct _AsmInstr* ii;
                    int res = _MtmcAssemblerGetInstruction(&args[0], &ii);
                    if (res != 0) { return 1; }
                    nib1 = ii->code;
                    if (state->argcount > 1) {
                        int res = _MtmcAssemblerParseRegister(&args[1], &nib0);
                        if (res != 0) { return res; }
                    }
                    break;
                }
                case MtmcInstructionStack_pushi: {
                    int res = _MtmcAssemblerParseNumber(&args[0], &word1);
                    if (res != 0) { return res; }
                    if (state->argcount > 1) {
                        int res = _MtmcAssemblerParseRegister(&args[1], &nib0);
                        if (res != 0) { return res; }
                    }
                    break;
                }
            }
            break;
        }

        case MtmcInstructionType_TEST: {
            int res = _MtmcAssemblerParseRegister(&args[0], &nib1);
            if (res != 0) { return res; }
            switch ((enum MtmcInstructionTest) instr->code) {
                case MtmcInstructionTest_eq:
                case MtmcInstructionTest_neq:
                case MtmcInstructionTest_gt:
                case MtmcInstructionTest_gte:
                case MtmcInstructionTest_lt:
                case MtmcInstructionTest_lte:
                    res = _MtmcAssemblerParseRegister(&args[1], &nib0);
                    if (res != 0) { return res; }
                    break;
                case MtmcInstructionTest_eqi:
                case MtmcInstructionTest_neqi:
                case MtmcInstructionTest_gti:
                case MtmcInstructionTest_gtei:
                case MtmcInstructionTest_lti:
                case MtmcInstructionTest_ltei:
                    res = _MtmcAssemblerParseNumber(&args[1], &nib0);
                    if (res != 0) { return res; }
                    if (nib0 < 0 || nib0 > 15) {
                        return _AssemblerError(&args[1], "invalid number argument");
                    }
                    break;
            }
            break;
        }

        case MtmcInstructionType_LWR:
        case MtmcInstructionType_LBR:
        case MtmcInstructionType_SWR:
        case MtmcInstructionType_SBR: {
            int res = _MtmcAssemblerParseRegister(&args[0], &nib2);
            if (res != 0) { return res; }
            res = _MtmcAssemblerParseRegister(&args[1], &nib1);
            if (res != 0) { return res; }
            res = _MtmcAssemblerParseRegister(&args[2], &nib0);
            if (res != 0) { return res; }
            break;
        }

        case MtmcInstructionType_LOAD: {
            int ioff = 2;
            int res = _MtmcAssemblerParseRegister(&args[0], &nib1);
            if (res != 0) { return res; }
            switch ((enum MtmcInstructionLoad) instr->code) {
                case MtmcInstructionLoad_lw:
                case MtmcInstructionLoad_lb:
                case MtmcInstructionLoad_sw:
                case MtmcInstructionLoad_sb: {
                    int res = _MtmcAssemblerTryResolveAddress(state, &args[1], &word1);
                    if (res != 0) { return res; }
                    break;
                }
                case MtmcInstructionLoad_lwo:
                case MtmcInstructionLoad_lbo:
                case MtmcInstructionLoad_swo:
                case MtmcInstructionLoad_sbo: {
                    int res = _MtmcAssemblerTryParseRegister(&args[ioff], &nib0);
                    if (res != 0) {
                        res = _MtmcAssemblerParseRegister(&args[ioff-1], &nib0);
                        if (res != 0) { return res; }
                        ioff -= 1;
                    }
                    struct AsmToken* argToken = &args[ioff == 2 ? 1 : 2];
                    res = _MtmcAssemblerTryResolveAddress(state, argToken, &word1);
                    if (res != 0) { return res; }
                    break;
                }
                case MtmcInstructionLoad_li: {
                    int res = _MtmcAssemblerTryParseNumber(&args[1], &word1);
                    if (res != 0) {
                        res = _MtmcAssemblerTryResolveAddress(state, &args[1], &word1);
                        if (res != 0) { return res; }
                    }
                    break;
                }
            }
            break;
        }

        case MtmcInstructionType_JUMPREG:
        //case MtmcInstructionType_RET:
            nib0 = RA;
            if (instr->arity > 0) {
                int res = _MtmcAssemblerParseRegister(&args[0], &nib0);
                if (res != 0) { return res; }
            }
            break;

        case MtmcInstructionType_JUMP:
        case MtmcInstructionType_JUMPZ:
        case MtmcInstructionType_JUMPNZ:
        case MtmcInstructionType_JUMPAL: {
            struct AsmToken* argToken = &args[0];
            i16 arg = 0;
            int res = _MtmcAssemblerTryResolveAddress(state, argToken, &arg);
            if (res != 0) { return res; }
            nib2 = ((u16)arg >> 8) & 0xF;
            nib1 = ((u16)arg >> 4) & 0xF;
            nib0 = ((u16)arg) & 0xF;
            break;
        }

        default:
            FatalErrorFmt("unhandled instruction (%x%x) %s",
                instr->type, instr->code, instr->name);
    }

    u16 opcode = ((u16)instr->type << 12) | ((u16)nib2 << 8) |
        ((u16)nib1 << 4) | (u16)nib0;
    int res = _MtmcAssemblerWriteCodeWord(state->exe, opcode);
    if (res != 0) { return res; }
    if (instr->dword != 0) {
        int res = _MtmcAssemblerWriteCodeWord(state->exe, word1);
        if (res != 0) { return res; }
    }
    return 0;
}


int MtmcAssemblerCompileSource(FILE* source, struct MtmcExeObject* exe,
    const char* source_path) {
    struct AssemblerState ams = (struct AssemblerState) {
        .source = source,
        .line = 1,
        .col = 0,
        .exe = exe,
    };
    struct AsmToken token = {};
    const struct _AsmInstr* instr = NULL;
    struct AsmToken instrToken;
    struct AsmToken instrArgs[3];

    for (; token.type != AsmTokenType_eof;) {
        int res = MtmcAssemblerReadToken(&ams, &token);
        if (res != 0) { return res; }
        for (u8 consumed = 0; consumed == 0; ) {
            consumed = 1;
            switch (ams.status) {

                case _AsmState_none:
                    switch (token.type) {
                        case AsmTokenType_eof:
                        case AsmTokenType_newline:
                            break;
                        case AsmTokenType_directive:
                            if (strncmp(".data", token.text, token.size) == 0) {
                                ams.status = _AsmState_data;
                            }
                            else if (strncmp(".text", token.text, token.size) == 0) {
                                ams.status = _AsmState_code;
                            }
                            else {
                                return _AssemblerError(&token, "invalid directive");
                            }
                            break;
                        case AsmTokenType_label:
                        case AsmTokenType_identifier:
                            consumed = 0;
                            ams.status = _AsmState_code;
                            break;
                        default:
                            return _AssemblerError(&token, "unexpected token");
                    }
                    break;

                case _AsmState_data:
                    switch (token.type) {
                        case AsmTokenType_eof:
                        case AsmTokenType_newline:
                            break;
                        case AsmTokenType_directive:
                            if (strncmp(".text", token.text, token.size) == 0) {
                                ams.status = _AsmState_code;
                            }
                            else if (strncmp(".data", token.text, token.size) == 0) {
                                return _AssemblerError(&token, "unexpected token");
                            }
                            else if (strncmp(".byte", token.text, token.size) == 0) {
                                ams.status = _AsmState_data_bytes;
                            }
                            else if (strncmp(".int", token.text, token.size) == 0) {
                                ams.status = _AsmState_data_words;
                            }
                            else if (strncmp(".image", token.text, token.size) == 0) {
                                ams.status = _AsmState_data_image;
                            }
                            else {
                                return _AssemblerError(&token, "invalid directive");
                            }
                            break;
                        case AsmTokenType_number: {
                            int res = _MtmcAssemblerEmitDataWord(&ams, &token);
                            if (res != 0) { return res; }
                            break;
                        }
                        case AsmTokenType_string: {
                            int res = _MtmcAssemblerEmitDataString(&ams, &token, 0);
                            if (res != 0) { return res; }
                            break;
                        }
                        case AsmTokenType_strchunk: {
                            int res = _MtmcAssemblerEmitDataString(&ams, &token, 1);
                            if (res != 0) { return res; }
                            ams.status = _AsmState_data_chunk;
                            break;
                        }
                        case AsmTokenType_label: {
                            int res = _MtmcAssemblerEmitDataLabel(&ams, &token);
                            if (res != 0) { return res; }
                            break;
                        }
                        default:
                            return _AssemblerError(&token, "unexpected token");
                    }
                    break;

                case _AsmState_data_chunk:
                    switch (token.type) {
                        case AsmTokenType_string: {
                            int res = _MtmcAssemblerEmitDataString(&ams, &token, 0);
                            if (res != 0) { return res; }
                            ams.status = _AsmState_data;
                            break;
                        }
                        case AsmTokenType_strchunk: {
                            int res = _MtmcAssemblerEmitDataString(&ams, &token, 1);
                            if (res != 0) { return res; }
                            break;
                        }
                        default:
                            return _AssemblerError(&token, "unexpected token");
                    }
                    break;

                case _AsmState_data_bytes:
                    switch (token.type) {
                        case AsmTokenType_number: {
                            int res = _MtmcAssemblerEmitDataByteArray(&ams, &token, 0);
                            if (res != 0) { return res; }
                            ams.status = _AsmState_data;
                            break;
                        }
                        default:
                            return _AssemblerError(&token, "unexpected token");
                    }
                    break;

                case _AsmState_data_words:
                    switch (token.type) {
                        case AsmTokenType_number: {
                            int res = _MtmcAssemblerEmitDataWordArray(&ams, &token, 0);
                            if (res != 0) { return res; }
                            ams.status = _AsmState_data;
                            break;
                        }
                        default:
                            return _AssemblerError(&token, "unexpected token");
                    }
                    break;

                case _AsmState_data_image:
                    switch (token.type) {
                        case AsmTokenType_string: {
                            int res = _MtmcAssemblerWriteDataWord(ams.exe, ams.exe->graphics_count);
                            if (res != 0) { return res; }
                            res = _MtmcAssemblerEmitGraphicsImport(&ams, &token,
                                source_path);
                            if (res != 0) { return res; }
                            ams.status = _AsmState_data;
                            break;
                        }
                        default:
                            return _AssemblerError(&token, "unexpected token");
                    }
                    break;

                case _AsmState_code:
                    switch (token.type) {
                        case AsmTokenType_eof:
                        case AsmTokenType_newline:
                            break;
                        case AsmTokenType_label: {
                            int res = _MtmcAssemblerEmitCodeLabel(&ams, &token);
                            if (res != 0) { return res; }
                            break;
                        }
                        case AsmTokenType_identifier: {
                            int res = _MtmcAssemblerGetInstruction(&token, &instr);
                            if (res != 0) { return 1; }
                            instrToken = token;
                            ams.argcount = 0;
                            ams.status = _AsmState_instruction;
                            break;
                        }
                        default:
                            return _AssemblerError(&token, "unexpected token");
                    }
                    break;

                case _AsmState_instruction:
                    switch (token.type) {
                        case AsmTokenType_eof:
                        case AsmTokenType_newline: {
                            int res = _MtmcAssemblerEmitInstruction(&ams, instr,
                                &instrToken, &instrArgs[0]);
                            if (res != 0) { return res; }
                            ams.status = _AsmState_code;
                            break;
                        }
                        case AsmTokenType_number:
                        case AsmTokenType_identifier: {
                            u8 amax = instr->arity_max == 0 ? instr->arity : instr->arity_max;
                            if (ams.argcount >= amax) {
                                return _AssemblerError(&token, "unexpected token");
                            }
                            instrArgs[ams.argcount++] = token;
                            break;
                        }
                        default:
                            return _AssemblerError(&token, "unexpected token");
                    }
                    break;
            }
        }
    }
    switch (ams.status) {
        case _AsmState_data_chunk:
        case _AsmState_data_bytes:
        case _AsmState_data_words:
            return _AssemblerError(&token, "unexpected EOF");
        case _AsmState_instruction: {
            int res = _MtmcAssemblerEmitInstruction(&ams, instr,
                &instrToken, &instrArgs[0]);
            if (res != 0) { return res; }
            break;
        }
        default:
            break;
    }
    for (size_t i = 0; i < ams.forwardsize; ++i) {
        int res = _MtmcAssemblerPatchForwardReference(&ams, &ams.forwardrefs[i]);
        if (res != 0) { return res; }
    }
    return 0;
}


#ifdef PAIV_JSON_NUMBER_BACKEND_TYPE


#define _assert_json_ok(code, msg) {\
    if (code != JsonError_ok) {\
    fprintf(stderr, msg ": error %d\n", code);\
    return 1; }}


int _json_writer_write_pair_str_str(JSON* context, const char* key,
    const char* value) {
    JsonError err = json_writer_write_string(context, key);
    _assert_json_ok(err, "json_writer_write_string");

    err = json_writer_write_object_key_separator(context);
    _assert_json_ok(err, "json_writer_write_object_key_separator");

    err = json_writer_write_string(context, value);
    _assert_json_ok(err, "json_writer_write_string");
    return JsonError_ok;
}


int _json_writer_write_pair_str_arr(JSON* context, const char* key,
    const u8* buf, size_t bufsize) {
    JsonError err = json_writer_write_string(context, key);
    _assert_json_ok(err, "json_writer_write_string");

    err = json_writer_write_object_key_separator(context);
    _assert_json_ok(err, "json_writer_write_object_key_separator");

    JSON ar;
    err = json_writer_open_array(context, &ar);
    _assert_json_ok(err, "json_writer_open_array");

    for (size_t i = 0; i < bufsize; ++i) {
        err = json_writer_write_array_value_separator(&ar);
        _assert_json_ok(err, "json_writer_write_array_value_separator");

        err = json_writer_write_numberi(&ar, (i8) buf[i]);
        _assert_json_ok(err, "json_writer_write_numberi");
    }

    err = json_writer_close_array(&ar);
    _assert_json_ok(err, "json_writer_close_array");
    return JsonError_ok;
}


static JsonError _json_writer_write_array(JSON* context,
    const u8* buf, size_t bufsize) {
    JSON ar;
    JsonError err = json_writer_open_array(context, &ar);
    _assert_json_ok(err, "json_writer_open_array");
    const u8* p = buf;
    for (size_t i = 0; i < bufsize; ++i, ++p) {
        err = json_writer_write_array_value_separator(&ar);
        _assert_json_ok(err, "json_writer_write_array_value_separator");
        err = json_writer_write_numberi(&ar, (i8)*p);
        _assert_json_ok(err, "json_writer_write_numberi");
    }
    err = json_writer_close_array(&ar);
    _assert_json_ok(err, "json_writer_close_array");
    return JsonError_ok;
}


static int _MtmcGraphicFilter(const char* filename, FILE* output) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        perror("fopen");
        fprintf(stderr, "file: '%s'\n", filename);
        return 1;
    }
    struct MtmcGraphic graphic = {};
    int res = MtmcGraphicLoad(&graphic, file);
    fclose(file);
    if (res != 0) { return res; }
    res = MtmcGraphicWrite(&graphic, output);
    if (res != 0) { return res; }
    return 0;
}


static JsonError _MtmcAssemblerEmbedGraphicFile(JSON* context,
    const char* filename) {
    u8 buf[MtmcGraphics_bytes_max];
    FILE* bufile = fmemopen(buf, sizeof(buf), "wb");
    if (bufile == NULL) {
        perror("fmemopen");
        return JsonError_invalid;
    }
    int res = _MtmcGraphicFilter(filename, bufile);
    long n = ftell(bufile);
    fclose(bufile);
    if (res != 0) { return JsonError_invalid; }
    if (n < 0) { return JsonError_invalid; }
    JsonError err = _json_writer_write_array(context, buf, n);
    return err;
}


static JsonError _MtmcAssemblerLinkGraphics(JSON* context,
    const char* key, struct MtmcExeObject* exe) {
    JsonError err = json_writer_write_string(context, key);
    _assert_json_ok(err, "json_writer_write_string");

    err = json_writer_write_object_key_separator(context);
    _assert_json_ok(err, "json_writer_write_object_key_separator");

    JSON ar;
    err = json_writer_open_array(context, &ar);
    _assert_json_ok(err, "json_writer_open_array");

    for (size_t i = 0; i < exe->graphics_count; ++i) {
        err = json_writer_write_array_value_separator(&ar);
        _assert_json_ok(err, "json_writer_write_array_value_separator");

        err = _MtmcAssemblerEmbedGraphicFile(&ar, exe->graphics[i].filename);
        if (err != JsonError_ok) { return err; }
    }

    err = json_writer_close_array(&ar);
    _assert_json_ok(err, "json_writer_close_array");
    return JsonError_ok;
}


int MtmcAssemblerLinkExecutable(struct MtmcExeObject* exe, FILE* output) {
    JSON context, obj;
    JsonError err = json_writer_init(&context, output);
    _assert_json_ok(err, "json_writer_init");
    err = json_writer_open_object(&context, &obj);
    _assert_json_ok(err, "json_writer_open_object");

    err = json_writer_write_object_value_separator(&context);
    _assert_json_ok(err, "json_writer_write_object_value_separator");

    switch (exe->format) {
        case MtmcExecutableFormat_orc1:
            err = _json_writer_write_pair_str_str(&obj,
                "format", MtmcFormatOrc1);
            if (err != JsonError_ok) { return err; }
            break;
        default:
            fprintf(stderr, "unknown exe format %d\n", exe->format);
            return 1;
    }

    err = json_writer_write_object_value_separator(&context);
    _assert_json_ok(err, "json_writer_write_object_value_separator");

    err = _json_writer_write_pair_str_arr(&obj,
        "code", exe->code, exe->codesize);
    if (err != JsonError_ok) { return err; }

    err = json_writer_write_object_value_separator(&context);
    _assert_json_ok(err, "json_writer_write_object_value_separator");

    err = _json_writer_write_pair_str_arr(&obj,
        "data", exe->data, exe->datasize);
    if (err != JsonError_ok) { return err; }

    if (exe->graphics_count > 0) {
        err = json_writer_write_object_value_separator(&context);
        _assert_json_ok(err, "json_writer_write_object_value_separator");

        err = _MtmcAssemblerLinkGraphics(&context, "graphics", exe);
        if (err != JsonError_ok) { return err; }
    }

    err = json_writer_close_object(&obj);
    _assert_json_ok(err, "json_writer_close_object");
    return 0;
}

#endif /* PAIV_JSON_ */


int MtmcAssemble(FILE* source, FILE* output, const char* source_filename) {
    struct MtmcExeObject exe = {
        .format = MtmcExecutableFormat_default,
    };
    char path[PATH_MAX] = {};
    char* sep = strrchr(source_filename, '/');
    if (sep == NULL) {
        strcpy(path, ".");
    }
    else {
        strncpy(path, source_filename, sep - source_filename);
    }

    int res = MtmcAssemblerCompileSource(source, &exe, path);
    if (res != 0) { return res; }
    res = MtmcAssemblerLinkExecutable(&exe, output);
    if (res != 0) { return res; }
    return 0;
}


int MtmcDisassemble(FILE* input, FILE* output, const char* input_filename,
    int code_bytes, int graphics) {
    const char* name = "";
    if (input_filename != NULL) {
        char* sep = strrchr(input_filename, '/');
        if (sep != NULL) {
            name = sep + 1;
        }
    }
    struct MtmcExecutable exe;
    int res = MtmcExecutableLoad(input, &exe);
    if (res != 0) { return res; }
    res = MtmcDecompileExecutable(&exe, output, code_bytes);
    if (res != 0) { return res; }
    if (graphics != 0) {
        char filename[PATH_MAX];
        for (size_t i = 0; i < exe.graphics_count; ++i) {
            snprintf(filename, sizeof(filename), "%s_graphic%zu.png", name, i);
            fprintf(stderr, "%s\n", filename);
            FILE* file = fopen(filename, "wb");
            if (file == NULL) {
                perror("fopen");
                return 1;
            }
            int res = MtmcGraphicWrite(&exe.graphics[i], file);
            fclose(file);
            if (res != 0) { return res; }
        }
    }
    return 0;
}


static int _MtmcDasmTryGetInstruction(i16 opcode,
    const struct _AsmInstr** instr) {
    size_t n = sizeof(_Instructions) / sizeof(_Instructions[0]);
    const struct _AsmInstr* p = &_Instructions[0];
    u8 type = ((u16)opcode >> 12) & 0xF;
    u8 code = ((u16)opcode >> 8) & 0xF;
    switch (type) {
        case MtmcInstructionType_LWR:
        case MtmcInstructionType_LBR:
        case MtmcInstructionType_SWR:
        case MtmcInstructionType_SBR:
        case MtmcInstructionType_JUMP:
        case MtmcInstructionType_JUMPZ:
        case MtmcInstructionType_JUMPNZ:
        case MtmcInstructionType_JUMPAL:
            for (size_t i = 0; i < n; ++i, ++p) {
                if (p->type == type) {
                    *instr = p;
                    return 0;
                }
            }
            break;
        // case MtmcInstructionType_RET:
        case MtmcInstructionType_JUMPREG: {
            int isret = ((u16)opcode & 0xF) == RA ? 0 : 1;
            for (size_t i = 0; i < n; ++i, ++p) {
                if (p->type == type && p->arity == isret) {
                    *instr = p;
                    return 0;
                }
            }
            break;
        }
        default:
            for (size_t i = 0; i < n; ++i, ++p) {
                if (p->type == type && p->code == code) {
                    *instr = p;
                    return 0;
                }
            }
    }
    return 1;
}


static const char* _MtmcDasmGetRegisterName(enum MtmcRegister reg) {
    switch (reg) {
        case T0: return "t0";
        case T1: return "t1";
        case T2: return "t2";
        case T3: return "t3";
        case T4: return "t4";
        case T5: return "t5";
        case A0: return "a0";
        case A1: return "a1";
        case A2: return "a2";
        case A3: return "a3";
        case RV: return "rv";
        case RA: return "ra";
        case FP: return "fp";
        case SP: return "sp";
        case BP: return "bp";
        case PC: return "pc";
        default:
            FatalError();
    }
    return NULL;
}


static const char* _MtmsDasmGetAluOpName(enum MtmcInstructionAlu code) {
    switch (code) {
    case MtmcInstructionAlu_add: return "add";
    case MtmcInstructionAlu_sub: return "sub";
    case MtmcInstructionAlu_mul: return "mul";
    case MtmcInstructionAlu_div: return "div";
    case MtmcInstructionAlu_mod: return "mod";
    case MtmcInstructionAlu_and: return "and";
    case MtmcInstructionAlu_or: return "or";
    case MtmcInstructionAlu_xor: return "xor";
    case MtmcInstructionAlu_shl: return "shl";
    case MtmcInstructionAlu_shr: return "shr";
    case MtmcInstructionAlu_min: return "min";
    case MtmcInstructionAlu_max: return "max";
    case MtmcInstructionAlu_not: return "not";
    case MtmcInstructionAlu_lnot: return "lnot";
    case MtmcInstructionAlu_neg: return "neg";
    case MtmcInstructionAlu_imm: return "imm";
    default:
        FatalError();
    }
    return NULL;
}


struct _AsmSysCall {
    enum MtosSysCall code;
    const char* name;
};


static const char* _MtmcDasmGetSysCallName(enum MtosSysCall syscall) {
    switch (syscall) {
        case MtosSysCall_exit: return "exit";
        case MtosSysCall_rint: return "rint";
        case MtosSysCall_wint: return "wint";
        case MtosSysCall_rstr: return "rstr";
        case MtosSysCall_wchr: return "wchr";
        case MtosSysCall_rchr: return "rchr";
        case MtosSysCall_wstr: return "wstr";
        case MtosSysCall_atoi: return "atoi";
        case MtosSysCall_printf: return "printf";
        case MtosSysCall_rfile: return "rfile";
        case MtosSysCall_wfile: return "wfile";
        case MtosSysCall_cwd: return "cwd";
        case MtosSysCall_chdir: return "chdir";
        case MtosSysCall_dirent: return "dirent";
        case MtosSysCall_dfile: return "dfile";
        case MtosSysCall_rnd: return "rnd";
        case MtosSysCall_sleep: return "sleep";
        case MtosSysCall_timer: return "timer";
        case MtosSysCall_fbreset: return "fbreset";
        case MtosSysCall_fbstat: return "fbstat";
        case MtosSysCall_fbset: return "fbset";
        case MtosSysCall_fbline: return "fbline";
        case MtosSysCall_fbrect: return "fbrect";
        case MtosSysCall_fbflush: return "fbflush";
        case MtosSysCall_joystick: return "joystick";
        case MtosSysCall_scolor: return "scolor";
        case MtosSysCall_memcopy: return "memcopy";
        case MtosSysCall_drawimg: return "drawimg";
        case MtosSysCall_drawimgsz: return "drawimgsz";
        case MtosSysCall_drawimgclip: return "drawimgclip";
        case MtosSysCall_error: return "error";
    }
    return NULL;
};


static int _MtmcDasmPrintBytes(FILE* output, const u8* buf, size_t bufsize,
    size_t off) {
    size_t j = off;
    for (; j < bufsize; ++j) {
        if (buf[j] != 0) { break;}
    }
    int n = j - off;
    if (n < 2) { return 0; }
    if (n == 2) {
        fputs("0", output);
    }
    else {
        n -= n & 1;
        fprintf(output, ".byte %d", n);
    }
    return n;
}


static int _MtmcDasmPrintString(FILE* output, const u8* buf, size_t bufsize,
    size_t off) {
    u8 c = buf[off];
    switch (c) {
        case '\t':
        case '\n':
        case '\r':
        case ' ' ... '~':
            break;
        default:
            return 0;
    }
    size_t j = off;
    for (; j < bufsize; ++j) {
        u8 c = buf[j];
        if (c == '\0') { ++j; break; }
        if (c == '\t' || c == '\n' || c == '\r') { continue; }
        if (c < ' ' || c > '~') { break; }
    }
    int n = j - off;
    fputc('"', output);
    for (int i = 0; i < n; ++i) {
        u8 c = buf[off + i];
        switch (c) {
            case '"': fputc('\\', output); fputc('"', output); break;
            case '\\': fputc('\\', output); fputc('\\', output); break;
            case '\b': fputc('\\', output); fputc('b', output); break;
            case '\t': fputc('\\', output); fputc('t', output); break;
            case '\n': fputc('\\', output); fputc('n', output); break;
            case '\f': fputc('\\', output); fputc('f', output); break;
            case '\r': fputc('\\', output); fputc('r', output); break;
            case '\0': break;
            default:
                fputc(c, output);
        }
    }
    fputc('"', output);
    return n;
}


int MtmcDecompileExecutable(struct MtmcExecutable* exe, FILE* output,
    int code_bytes) {
    for (size_t pc = 0; pc < exe->codesize;) {
        u16 addr = pc;
        u16 opcode = (u16)exe->code[pc++] << 8;
        opcode |= (u16)exe->code[pc++];
        fprintf(output, "%04X: ", addr);
        if (code_bytes != 0) {
            fprintf(output, "%04x  ", opcode);
        }
        const struct _AsmInstr* instr;
        int res = _MtmcDasmTryGetInstruction(opcode, &instr);
        if (res == 0) {
            switch (instr->type) {

                case MtmcInstructionType_MISC:
                    fputs(instr->name, output);
                    switch ((enum MtmcInstructionMisc) instr->code) {
                        case MtmcInstructionMisc_mcp:
                        case MtmcInstructionMisc_mov: {
                            u8 reg = (opcode >> 4) & 0xF;
                            const char* name = _MtmcDasmGetRegisterName(reg);
                            fprintf(output, " %s", name);
                            reg = opcode & 0xF;
                            name = _MtmcDasmGetRegisterName(reg);
                            fprintf(output, " %s", name);
                            break;
                        }
                        case MtmcInstructionMisc_inc:
                        case MtmcInstructionMisc_dec: {
                            u8 reg = (opcode >> 4) & 0xF;
                            const char* name = _MtmcDasmGetRegisterName(reg);
                            fprintf(output, " %s", name);
                            int arg = opcode & 0xF;
                            if (arg != 1) {
                                fprintf(output, " %d", arg);
                            }
                            break;
                        }
                        case MtmcInstructionMisc_seti: {
                            u8 reg = (opcode >> 4) & 0xF;
                            const char* name = _MtmcDasmGetRegisterName(reg);
                            fprintf(output, " %s", name);
                            fprintf(output, " %d", (int)(opcode & 0xF));
                            break;
                        }
                        case MtmcInstructionMisc_sys: {
                            u16 syscall = opcode & 0xFF;
                            const char* name = _MtmcDasmGetSysCallName(syscall);
                            if (name != NULL) {
                                fprintf(output, " %s", name);
                            }
                            else {
                                fprintf(output, " 0x%02X", (int)syscall);
                            }
                            break;
                        }
                        case MtmcInstructionMisc_debug: {
                            u16 addr = opcode & 0xFF;
                            fprintf(output, " 0x%04x  # data[%d]", (int)addr,
                                (int)(addr - exe->codesize));
                            break;
                        }
                        case MtmcInstructionMisc_nop:
                            break;
                    }
                    break;

                case MtmcInstructionType_ALU: {
                    u8 reg = (opcode >> 4) & 0xF;
                    const char* name = _MtmcDasmGetRegisterName(reg);
                    switch ((enum MtmcInstructionAlu) instr->code) {
                        case MtmcInstructionAlu_not:
                        case MtmcInstructionAlu_lnot:
                        case MtmcInstructionAlu_neg: {
                            fputs(instr->name, output);
                            fprintf(output, " %s", name);
                            break;
                        }
                        case MtmcInstructionAlu_imm: {
                            const char* opname = _MtmsDasmGetAluOpName(opcode & 0xF);
                            fputs(opname, output);
                            fprintf(output, "i %s", name);
                            break;
                        }
                        default:
                            fputs(instr->name, output);
                            fprintf(output, " %s", name);
                            name = _MtmcDasmGetRegisterName(opcode & 0xF);
                            fprintf(output, " %s", name);
                            break;
                    }
                    break;
                }

                case MtmcInstructionType_STACK: {
                    fputs(instr->name, output);
                    switch ((enum MtmcInstructionStack) instr->code) {
                        case MtmcInstructionStack_push:
                        case MtmcInstructionStack_pop: {
                            u8 reg = (opcode >> 4) & 0xF;
                            const char* name = _MtmcDasmGetRegisterName(reg);
                            fprintf(output, " %s", name);
                            break;
                        }
                        case MtmcInstructionStack_sop: {
                            const char* opname = _MtmsDasmGetAluOpName((opcode >> 4) & 0xF);
                            fprintf(output, " %s", opname);
                            break;
                        }
                        default:
                            break;
                    }
                    u16 stack = opcode & 0xF;
                    if (stack != SP) {
                        const char* name = _MtmcDasmGetRegisterName(stack);
                        fprintf(output, " %s", name);
                    }
                    break;
                }

                case MtmcInstructionType_TEST: {
                    fputs(instr->name, output);
                    u8 reg = (opcode >> 4) & 0xF;
                    const char* name = _MtmcDasmGetRegisterName(reg);
                    fprintf(output, " %s", name);
                    switch ((enum MtmcInstructionTest) instr->code) {
                        case MtmcInstructionTest_eq:
                        case MtmcInstructionTest_neq:
                        case MtmcInstructionTest_gt:
                        case MtmcInstructionTest_gte:
                        case MtmcInstructionTest_lt:
                        case MtmcInstructionTest_lte:
                            name = _MtmcDasmGetRegisterName(opcode & 0xF);
                            fprintf(output, " %s", name);
                            break;
                        case MtmcInstructionTest_eqi:
                        case MtmcInstructionTest_neqi:
                        case MtmcInstructionTest_gti:
                        case MtmcInstructionTest_gtei:
                        case MtmcInstructionTest_lti:
                        case MtmcInstructionTest_ltei:
                            fprintf(output, " %d", (int)(opcode & 0xF));
                            break;
                    }
                    break;
                }

                case MtmcInstructionType_JUMP:
                case MtmcInstructionType_JUMPZ:
                case MtmcInstructionType_JUMPNZ:
                case MtmcInstructionType_JUMPAL:
                    fputs(instr->name, output);
                    fprintf(output, " 0x%04x", (int)(opcode &0xFFF));
                    break;

                case MtmcInstructionType_LOAD: {
                    fputs(instr->name, output);
                    u8 reg = (opcode >> 4) & 0xF;
                    const char* name = _MtmcDasmGetRegisterName(reg);
                    fprintf(output, " %s", name);
                    break;
                }

                case MtmcInstructionType_LWR:
                case MtmcInstructionType_LBR:
                case MtmcInstructionType_SWR:
                case MtmcInstructionType_SBR: {
                    fputs(instr->name, output);
                    const char* name = _MtmcDasmGetRegisterName((opcode >> 8) & 0xF);
                    fprintf(output, " %s", name);
                    name = _MtmcDasmGetRegisterName((opcode >> 4) & 0xF);
                    fprintf(output, " %s", name);
                    name = _MtmcDasmGetRegisterName(opcode & 0xF);
                    fprintf(output, " %s", name);
                    break;
                }

                default:
                    fputs(instr->name, output);
                    break;
            }
            if (instr->dword != 0) {
                i16 word = (u16)exe->code[pc++] << 8;
                word |= (u16)exe->code[pc++];
                if (word >= (int)exe->codesize && word < (int)(exe->codesize + exe->datasize)) {
                    fprintf(output, " 0x%04x  # data[%d]", (int)word,
                        (int)(word - exe->codesize));
                }
                else {
                    fprintf(output, " %d", (int)word);
                }
            }
        }
        else {
            fputs("(unknown)", output);
        }
        fputc('\n', output);
    }
    size_t pc = exe->codesize;
    for (size_t di = 0; di < exe->datasize;) {
        fprintf(output, "%04X: ", (int)(pc + di));
        if (code_bytes != 0) {
            fputs("      ", output);
        }
        int n = _MtmcDasmPrintString(output, exe->data, exe->datasize, di);
        if (n == 0) {
            n = _MtmcDasmPrintBytes(output, exe->data, exe->datasize, di);
        }
        if (n == 0) {
            u16 x = exe->data[di++];
            if (di < exe->datasize) {
                x = (x << 8) | exe->data[di++];
            }
            fprintf(output, "%d", (int)(i16)x);
        }
        else {
            di += n;
        }
        fputc('\n', output);
    }
    return 0;
}


#endif /* PAIV_MTMCASM_IMPLEMENTATION */
