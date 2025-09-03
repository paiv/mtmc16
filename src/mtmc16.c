#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>
#define PAIV_JSON_IMPLEMENTATION
#include "paiv_json.h"
#define PAIV_MTMC_IMPLEMENTATION
#include "paiv_mtmc16.h"
#define PAIV_MTMCASM_IMPLEMENTATION
#include "paiv_mtmc16asm.h"
#include "platform.c"


static const char _usage[] =
    "usage: mtmc16 [-h] {run,asm,disasm,img} ...\n";

static const char _help_page[] =
    "usage: mtmc16 [-h] {run,asm,disasm} ...\n"
    "\n"
    "MTMC-16 The Montana Mini-Computer\n"
    "(paiv port)\n"
    "\n"
    "positional arguments:\n"
    "  {run,asm,disasm}\n"
    "    run          execute binary\n"
    "    asm          assemble binary\n"
    "    disasm       disassemble binary\n"
    "    img          preprocess graphics\n"
    "\n"
    "options:\n"
    "  -h, --help            show this help\n"
    ;

static const char _run_usage[] =
    "usage: mtmc16 run [-h] [-s SPEED] [-t TRACE] [-x SCALE] FILE [arg]\n";

static const char _run_help_page[] =
    "usage: mtmc16 run [-h] [-s SPEED] [-t TRACE] [-x SCALE] FILE [arg]\n"
    "\n"
    "positional arguments:\n"
    "  FILE                  executable binary\n"
    "  arg                   argument to the executable binary\n"
    "\n"
    "options:\n"
    "  -s, --speed SPEED     CPU speed in cycles per second\n"
    "  -t, --trace TRACE     tracing level\n"
    "  -x, --scale SCALE     scale window\n"
    "  -h, --help            show this help\n"
    ;

static const char _asm_usage[] =
    "usage: mtmc16 asm [-h] [-o OUT] FILE\n";

static const char _asm_help_page[] =
    "usage: mtmc16 asm [-h] [-o OUT] FILE\n"
    "\n"
    "positional arguments:\n"
    "  FILE                  assembly source file\n"
    "\n"
    "options:\n"
    "  -h, --help            show this help\n"
    "  -o, --output OUT      output filename\n"
    ;


static const char _disasm_usage[] =
    "usage: mtmc16 disasm [-h] [-b] [-g] [-o OUT] FILE\n";

static const char _disasm_help_page[] =
    "usage: mtmc16 disasm [-h] [-b] [-g] [-o OUT] FILE\n"
    "\n"
    "positional arguments:\n"
    "  FILE                  binary file\n"
    "\n"
    "options:\n"
    "  -b, --bytes           print code bytes\n"
    "  -g, --graphics        extract graphics\n"
    "  -h, --help            show this help\n"
    "  -o, --output OUT      output filename\n"
    ;


static const char _img_usage[] =
    "usage: mtmc16 img [-h] [-o OUT] FILE\n";

static const char _img_help_page[] =
    "usage: mtmc16 img [-h] [-o OUT] FILE\n"
    "\n"
    "Convert image to MTMC-16 4-bit graphics\n"
    "\n"
    "positional arguments:\n"
    "  FILE                  image file\n"
    "\n"
    "options:\n"
    "  -h, --help            show this help\n"
    "  -o, --output OUT      output filename\n"
    ;


enum AppMode {
    AppMode_none,
    AppMode_run,
    AppMode_asm,
    AppMode_disasm,
    AppMode_img,
};


struct AppArgs {
    int needs_help;
    enum AppMode app_mode;
    int run_needs_help;
    int run_speed;
    int run_trace_level;
    int run_window_scale;
    int asm_needs_help;
    int disasm_needs_help;
    int disasm_code_bytes;
    int disasm_graphics;
    int img_needs_help;
    const char* input;
    const char* input_arg;
    const char* output;
    FILE* input_file;
    FILE* output_file;
};


static void
arg_error(const char* usage, const char* message, ...) {
    FILE* file = stderr;
    fputs(usage, file);
    fprintf(file, "error: ");
    va_list args;
    va_start(args, message);
    vfprintf(file, message, args);
    va_end(args);
    fputs("\n", file);
}


static int
parse_args(int argc, const char* argv[], struct AppArgs* args) {
    *args = (struct AppArgs) {};
    int state = 0;

    for (int i = 1; i < argc; ++i) {
        switch (state) {
            case 0:
                if (strcmp(argv[i], "-h") == 0 ||
                    strcmp(argv[i], "--help") == 0) {
                    args->needs_help = 1;
                }
                else if (strcmp(argv[i], "run") == 0) {
                    args->app_mode = AppMode_run;
                    state = 1;
                }
                else if (strcmp(argv[i], "asm") == 0) {
                    args->app_mode = AppMode_asm;
                    state = 2;
                }
                else if (strcmp(argv[i], "disasm") == 0) {
                    args->app_mode = AppMode_disasm;
                    state = 3;
                }
                else if (strcmp(argv[i], "img") == 0) {
                    args->app_mode = AppMode_img;
                    state = 4;
                }
                else if (strncmp(argv[i], "-", 1) == 0) {
                    arg_error(_usage, "the following arguments are required: {run,asm,disasm,img}");
                    return 1;
                }
                else {
                    arg_error(_usage, "argument {run,asm,disasm,img}: invalid choice: '%s'", argv[i]);
                    return 1;
                }
                break;

            case 1:
                if (strcmp(argv[i], "-h") == 0 ||
                    strcmp(argv[i], "--help") == 0) {
                    args->run_needs_help = 1;
                }
                else if (strcmp(argv[i], "-s") == 0 ||
                    strcmp(argv[i], "--speed") == 0) {
                    state = 11;
                }
                else if (strcmp(argv[i], "-t") == 0 ||
                    strcmp(argv[i], "--trace") == 0) {
                    state = 12;
                }
                else if (strcmp(argv[i], "-x") == 0 ||
                    strcmp(argv[i], "--scale") == 0) {
                    state = 13;
                }
                else if (strncmp(argv[i], "-", 1) == 0) {
                    arg_error(_run_usage, "unrecognized arguments: %s", argv[i]);
                    return 1;
                }
                else {
                    args->input = argv[i];
                    state = 19;
                }
                break;

            case 11: {
                char* end = NULL;
                long x = strtol(argv[i], &end, 10);
                if (end != argv[i] + strlen(argv[i])) {
                    arg_error(_run_usage, "invalid integer value: %s", argv[i]);
                    return 1;
                }
                args->run_speed = x > 0 ? x : Mtmc_DEFAULT_SPEED;
                state = 1;
                break;
            }

            case 12: {
                char* end = NULL;
                long x = strtol(argv[i], &end, 10);
                if (end != argv[i] + strlen(argv[i])) {
                    arg_error(_run_usage, "invalid integer value: %s", argv[i]);
                    return 1;
                }
                args->run_trace_level = x >= 0 ? x : 0;
                state = 1;
                break;
            }

            case 13: {
                char* end = NULL;
                long x = strtol(argv[i], &end, 10);
                if (end != argv[i] + strlen(argv[i])) {
                    arg_error(_run_usage, "invalid integer value: %s", argv[i]);
                    return 1;
                }
                args->run_window_scale = x > 0 ? x : 1;
                state = 1;
                break;
            }

            case 19:
                args->input_arg = argv[i];
                break;

            case 2:
                if (strcmp(argv[i], "-h") == 0 ||
                    strcmp(argv[i], "--help") == 0) {
                    args->asm_needs_help = 1;
                }
                else if (strcmp(argv[i], "-o") == 0 ||
                    strcmp(argv[i], "--output") == 0) {
                    state = 21;
                }
                else if (strncmp(argv[i], "-", 1) == 0) {
                    arg_error(_asm_usage, "unrecognized arguments: %s", argv[i]);
                    return 1;
                }
                else {
                    args->input = argv[i];
                    state = 29;
                }
                break;

            case 21:
                args->output = argv[i];
                state = 2;
                break;

            case 29:
                arg_error(_asm_usage, "extra arguments: %s", argv[i]);
                break;

            case 3:
                if (strcmp(argv[i], "-h") == 0 ||
                    strcmp(argv[i], "--help") == 0) {
                    args->disasm_needs_help = 1;
                }
                else if (strcmp(argv[i], "-o") == 0 ||
                    strcmp(argv[i], "--output") == 0) {
                    state = 31;
                }
                else if (strcmp(argv[i], "-b") == 0 ||
                    strcmp(argv[i], "--bytes") == 0) {
                    args->disasm_code_bytes = 1;
                }
                else if (strcmp(argv[i], "-g") == 0 ||
                    strcmp(argv[i], "--graphics") == 0) {
                    args->disasm_graphics = 1;
                }
                else if (strncmp(argv[i], "-", 1) == 0) {
                    arg_error(_disasm_usage, "unrecognized arguments: %s", argv[i]);
                    return 1;
                }
                else {
                    args->input = argv[i];
                    state = 39;
                }
                break;

            case 31:
                args->output = argv[i];
                state = 3;
                break;

            case 39:
                arg_error(_disasm_usage, "extra arguments: %s", argv[i]);
                break;

            case 4:
                if (strcmp(argv[i], "-h") == 0 ||
                    strcmp(argv[i], "--help") == 0) {
                    args->img_needs_help = 1;
                }
                else if (strcmp(argv[i], "-o") == 0 ||
                    strcmp(argv[i], "--output") == 0) {
                    state = 41;
                }
                else if (strncmp(argv[i], "-", 1) == 0) {
                    arg_error(_img_usage, "unrecognized arguments: %s", argv[i]);
                    return 1;
                }
                else {
                    args->input = argv[i];
                    state = 49;
                }
                break;

            case 41:
                args->output = argv[i];
                state = 4;
                break;

            case 49:
                arg_error(_img_usage, "extra arguments: %s", argv[i]);
                break;
        }
    }

    if (args->needs_help != 0 ||
        args->run_needs_help != 0 ||
        args->asm_needs_help != 0 ||
        args->disasm_needs_help != 0 ||
        args->img_needs_help != 0) {
        return 0;
    }

    switch (state) {
        case 11:
            arg_error(_run_usage, "argument -s/--speed: expected a value");
            return 1;
        case 12:
            arg_error(_run_usage, "argument -t/--trace: expected a value");
            return 1;
        case 13:
            arg_error(_run_usage, "argument -x/--scale: expected a value");
            return 1;
        case 21:
            arg_error(_asm_usage, "argument -o/--output: expected a value");
            return 1;
        case 31:
            arg_error(_disasm_usage, "argument -o/--output: expected a value");
            return 1;
        case 41:
            arg_error(_img_usage, "argument -o/--output: expected a value");
            return 1;
    }

    switch (args->app_mode) {
        case AppMode_none:
            arg_error(_usage, "the following arguments are required: {run,asm,disasm,img}");
            return 1;

        case AppMode_run:
            if (args->input == NULL) {
                arg_error(_run_usage, "the following arguments are required: FILE");
                return 1;
            }
            break;

        case AppMode_asm:
            if (args->input == NULL) {
                arg_error(_asm_usage, "the following arguments are required: FILE");
                return 1;
            }
            break;

        case AppMode_disasm:
            if (args->input == NULL) {
                arg_error(_disasm_usage, "the following arguments are required: FILE");
                return 1;
            }
            break;

        case AppMode_img:
            if (args->input == NULL) {
                arg_error(_img_usage, "the following arguments are required: FILE");
                return 1;
            }
            break;
    }

    return 0;
}


static int
args_open_file(const char* filename, const char* mode, FILE** file) {
    if (*file != NULL) {
        return 0;
    }

    if (filename == NULL || strcmp(filename, "-") == 0) {
        if (strncmp(mode, "r", 1) == 0) {
            *file = stdin;
            return 0;
        }
        else if (strncmp(mode, "w", 1) == 0) {
            *file = stdout;
            return 0;
        }
    }

    FILE* fp = fopen(filename, mode);
    if (fp == NULL) {
        perror("fopen");
        fprintf(stderr, "file: '%s'\n", filename);
        return 1;
    }

    *file = fp;
    return 0;
}


static void
args_close_files(struct AppArgs* args) {
    if (args->input_file != stdin) {
        fclose(args->input_file);
    }
    if (args->output_file != stdout) {
        fclose(args->output_file);
    }
}


static int
app_run(FILE* file, const char* arg, int speed, int trace_level, int scale) {
    struct Platform platform = {
        .screen_width = MtmcDisplay_width,
        .screen_height = MtmcDisplay_height,
        .screen_scale = scale,
    };
    int res = PlatformInit(&platform);
    if (res != 0) { return res; }
    PlatformRandomSeed(&platform, time(NULL));

    res = MtmcPlatformRun(&platform, file, arg, speed, trace_level);

    PlatformDeinit(&platform);
    return res;
}


static int
app_asm(FILE* source, FILE* output, const char* source_filename) {
    int res = MtmcAssemble(source, output, source_filename);
    return res;
}


static int
app_disasm(FILE* input, FILE* output, const char* input_filename,
    int code_bytes, int graphics) {
    int res = MtmcDisassemble(input, output, input_filename,
        code_bytes, graphics);
    return res;
}


static int
app_img(FILE* input, FILE* output) {
    struct MtmcGraphic graphic;
    int res = MtmcGraphicLoad(&graphic, input);
    if (res != 0) { return res; }
    res = MtmcGraphicWrite(&graphic, output);
    if (res != 0) { return res; }
    return 0;
}


int main(int argc, const char* argv[]) {
    setlocale(LC_ALL, "");

    struct AppArgs args = {};
    int res = parse_args(argc, argv, &args);
    if (res != 0) { return res; }

    if (args.needs_help) {
        puts(_help_page);
        return 0;
    }

    if (args.run_needs_help) {
        puts(_run_help_page);
        return 0;
    }

    if (args.asm_needs_help) {
        puts(_asm_help_page);
        return 0;
    }

    if (args.disasm_needs_help) {
        puts(_disasm_help_page);
        return 0;
    }

    if (args.img_needs_help) {
        puts(_img_help_page);
        return 0;
    }

    res = args_open_file(args.input, "rb", &args.input_file);
    if (res != 0) { return res; }

    switch (args.app_mode) {
        case AppMode_none:
            break;

        case AppMode_run:
            res = app_run(args.input_file, args.input_arg,
                args.run_speed,
                args.run_trace_level,
                args.run_window_scale);
            break;

        case AppMode_asm:
            res = args_open_file(args.output, "wb", &args.output_file);
            if (res != 0) { return res; }
            res = app_asm(args.input_file, args.output_file, args.input);
            break;

        case AppMode_disasm:
            res = args_open_file(args.output, "wb", &args.output_file);
            if (res != 0) { return res; }
            res = app_disasm(args.input_file, args.output_file,
                args.input,
                args.disasm_code_bytes,
                args.disasm_graphics);
            break;

        case AppMode_img:
            res = args_open_file(args.output, "wb", &args.output_file);
            if (res != 0) { return res; }
            res = app_img(args.input_file, args.output_file);
            break;
    }

    args_close_files(&args);
    return res;
}

