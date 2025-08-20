#include <dirent.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>
#include "shaders.c"


typedef uint64_t u64;


struct Platform {
    i16 screen_width;
    i16 screen_height;
    i16 buttons;
    int screen_scale;
    GLFWwindow* window;
    GLubyte* canvas;
    GLuint glprogram;
    GLuint glarray;
    GLuint glcanvas;
    size_t glcanvassize;
    u8 closed;
    i16 color;
    u16 randstate[4];
    struct timespec timer;
    char cwd[PATH_MAX];
};


enum {
    _Padding = 1,
    _DefaultScale = 4,
};


static void _PlatformDrawWindow(PlatformState state) {
    int w, h;
    glfwGetFramebufferSize(state->window, &w, &h);
    glViewport(0, 0, w, h);

    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(state->glprogram);
    glBindVertexArray(state->glarray);
    float s = state->screen_scale > 3 ? 0.9 : 1;
    glPointSize((float)w / (2 * _Padding + state->screen_width) * s);
    glDrawArrays(GL_POINTS, 0, state->screen_width * state->screen_height);
    glfwSwapBuffers(state->window);
}


static void _PlatformRunloop(PlatformState state) {
    glfwPollEvents();
    if (glfwWindowShouldClose(state->window) != GLFW_FALSE) {
        state->closed = 1;
    }
}


static void _HandleError(int code, const char* description) {
    fprintf(stderr, "glfw error: %d %s\n", code, description);
}


static void _HandleFramebufferSizeChange(GLFWwindow* window,
    int width, int height) {
    glViewport(0, 0, width, height);
}


static void _FlipVertically(u8* pixels, int width, int height) {
    size_t nw = width * 3 / sizeof(u64);
    size_t nq = width * 3 % sizeof(u64);
    for (int j = 0; j < height / 2; ++j) {
        u64* s = (u64*)(&pixels[j * width * 3]);
        u64* t = (u64*)(&pixels[(height - j - 1) * width * 3]);
        for (size_t i = 0; i < nw; ++i) {
            u64 x = s[i];
            s[i] = t[i];
            t[i] = x;
        }
        u8* sb = (u8*)&s[nw];
        u8* tb = (u8*)&t[nw];
        for (size_t i = 0; i < nq; ++i) {
            u8 x = sb[i];
            sb[i] = tb[i];
            tb[i] = x;
        }
    }
}


static void _TakeScreenshot(PlatformState state) {
    time_t now = time(NULL);
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, &viewport[0]);
    int x = viewport[0], y = viewport[1];
    int width = viewport[2], height = viewport[3];
    u8* pixels = malloc(width * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    _FlipVertically(pixels, width, height);
    char filename[50];
    strftime(filename, sizeof(filename),
        "screenshot-%Y%m%d_%H%M%S.ppm", localtime(&now));
    FILE* fp = fopen(filename, "wb");
    if (fp == NULL) {
        perror("fopen");
        free(pixels);
        return;
    }
    char ts[40];
    strftime(ts, sizeof(ts), "%Y %b %d %H:%M:%S", localtime(&now));
    fprintf(fp, "P6\n# %s\n%d %d\n255\n", ts, width, height);
    fwrite(pixels, 3, width * height, fp);
    fclose(fp);
    free(pixels);
    fprintf(stderr, "%s\n", filename);
}


static void _SetButton(i16* state, u16 mask, int action, int mods) {
    int filter = GLFW_MOD_CONTROL | GLFW_MOD_ALT | GLFW_MOD_SUPER;
    if ((mods & filter) != 0) {
        return;
    }
    if (action == GLFW_RELEASE) {
        *state &= ~mask;
    }
    else {
        *state |= mask;
    }
}


static void _HandleKeyboard(GLFWwindow* window, int key, int scancode,
    int action, int mods) {
    PlatformState state = glfwGetWindowUserPointer(window);
    switch (key) {

        case GLFW_KEY_ESCAPE:
            if (action == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            break;

        case GLFW_KEY_UP:
            _SetButton(&state->buttons, 0x80, action, mods); break;
        case GLFW_KEY_DOWN:
            _SetButton(&state->buttons, 0x40, action, mods); break;
        case GLFW_KEY_LEFT:
            _SetButton(&state->buttons, 0x20, action, mods); break;
        case GLFW_KEY_RIGHT:
            _SetButton(&state->buttons, 0x10, action, mods); break;
        case GLFW_KEY_L:
            _SetButton(&state->buttons, 0x08, action, mods); break;
        case GLFW_KEY_SPACE:
            _SetButton(&state->buttons, 0x04, action, mods); break;
        case GLFW_KEY_A:
            _SetButton(&state->buttons, 0x02, action, mods); break;
        case GLFW_KEY_S:
            if ((mods & (GLFW_MOD_CONTROL | GLFW_MOD_SUPER)) != 0) {
                if (action == GLFW_PRESS) {
                    _TakeScreenshot(state); break;
                }
            }
            _SetButton(&state->buttons, 0x01, action, mods); break;
    }
}


/*
static void _HandleGamepad(GLFWwindow* window, int jid) {
    PlatformState state = glfwGetWindowUserPointer(window);
    GLFWgamepadstate gamepad;
    if (glfwGetGamepadState(jid, &gamepad) == GLFW_FALSE) {
        return;
    }
    static const int buttons[8] = {
        GLFW_GAMEPAD_BUTTON_A,
        GLFW_GAMEPAD_BUTTON_B,
        GLFW_GAMEPAD_BUTTON_GUIDE,
        GLFW_GAMEPAD_BUTTON_START,
        GLFW_GAMEPAD_BUTTON_DPAD_RIGHT,
        GLFW_GAMEPAD_BUTTON_DPAD_LEFT,
        GLFW_GAMEPAD_BUTTON_DPAD_DOWN,
        GLFW_GAMEPAD_BUTTON_DPAD_UP,
    };
    for (size_t i = 0; i < 8; ++i) {
        int action = gamepad.buttons[buttons[i]];
        _SetButton(&state->buttons, (1 << i), action, 0);
    }
}


static void _PlatformPollGamepads(PlatformState state) {
    for (int i = 0; i <= GLFW_JOYSTICK_LAST; ++i) {
        if (glfwJoystickIsGamepad(i) != GLFW_FALSE) {
            _HandleGamepad(state->window, i);
        }
    }
}
*/


static int _CheckShaderStatus(GLuint shader, const char* message) {
    GLint status, len;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    if (len > 0) {
        GLchar text[len];
        glGetShaderInfoLog(shader, sizeof(text), NULL, text);
        fprintf(stderr, "%s: %s", message, text);
    }
    if (status == GL_FALSE) {
        return 1;
    }
    return 0;
}


static int _CheckProgramStatus(GLuint program, const char* message) {
    GLint status, len;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
    if (len > 0) {
        GLchar text[len];
        glGetProgramInfoLog(program, sizeof(text), NULL, text);
        fprintf(stderr, "%s: %s", message, text);
    }
    if (status == GL_FALSE) {
        return 1;
    }
    return 0;
}


static int _CompileShaders(GLuint* program) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertexShader, 1, &_vertex_glsl, NULL);
    glCompileShader(vertexShader);
    int res = _CheckShaderStatus(vertexShader, "vertex");
    if (res != 0) { return res; }

    glShaderSource(fragmentShader, 1, &_fragment_glsl, NULL);
    glCompileShader(fragmentShader);
    res = _CheckShaderStatus(fragmentShader, "fragment");
    if (res != 0) { return res; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vertexShader);
    glAttachShader(prog, fragmentShader);
    glLinkProgram(prog);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    res = _CheckProgramStatus(prog, "program");
    if (res != 0) { return res; }

    *program = prog;
    return 0;
}


static int _PlatformCreateWindow(PlatformState state) {
    state->color = MtmcDisplayColor_LIGHTEST;
    if (state->screen_scale <= 0) {
        state->screen_scale = _DefaultScale;
    }

    glfwSetErrorCallback(_HandleError);

    if (glfwInit() == GLFW_FALSE) {
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_TRUE);

    state->window = glfwCreateWindow(
        state->screen_scale * (2 * _Padding + state->screen_width),
        state->screen_scale * (2 * _Padding + state->screen_height),
        "MTMC-16",
        NULL,
        NULL);
    if (state->window == NULL) {
        return 1;
    }

    glfwSetWindowUserPointer(state->window, state);
    glfwSetFramebufferSizeCallback(state->window, _HandleFramebufferSizeChange);
    glfwSetKeyCallback(state->window, _HandleKeyboard);

    glfwMakeContextCurrent(state->window);
    glfwSetWindowAspectRatio(state->window, state->screen_width, state->screen_height);

    #if 0
    glfwSwapInterval(1);
    #endif

    glClearColor(0, 0, 0, 1);

    glGenVertexArrays(1, &state->glarray);
    glBindVertexArray(state->glarray);

    float w = state->screen_width + 2 * _Padding;
    float h = state->screen_height + 2 * _Padding;
    GLfloat vertex[2 * state->screen_width * state->screen_height];
    for (int j = 0, col = 0; col < state->screen_width; ++col) {
        for (int row = 0; row < state->screen_height; ++row) {
            vertex[j++] = (col + _Padding + 0.5) / w * 2 - 1;
            vertex[j++] = (row + _Padding + 0.5) / h * 2 - 1;
        }
    }

    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);

    size_t canvas_size = state->screen_width * state->screen_height *
        sizeof(state->canvas[0]);
    state->canvas = calloc(state->screen_width * state->screen_height,
        sizeof(state->canvas[0]));
    GLuint canvasid;
    glGenBuffers(1, &canvasid);
    glBindBuffer(GL_ARRAY_BUFFER, canvasid);
    glBufferData(GL_ARRAY_BUFFER, canvas_size, state->canvas, GL_DYNAMIC_COPY);
    state->glcanvas = canvasid;
    state->glcanvassize = canvas_size;

    int res = _CompileShaders(&state->glprogram);
    if (res != 0) { return res; }

    glUseProgram(state->glprogram);

    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const GLvoid *) 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, canvasid);
    glVertexAttribPointer(1, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(GLubyte), (const GLvoid *) 0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    return 0;
}


int PlatformInit(PlatformState state) {
    return 0;
}


void PlatformDeinit(PlatformState state) {
    if (state->window != NULL) {
        glfwTerminate();
        free(state->canvas);
    }
}


u8 PlatformIsClosed(PlatformState state) {
    return state->closed;
}


static struct timespec TimeNow(void) {
    struct timespec now;
    int res = clock_gettime(CLOCK_MONOTONIC, &now);
    if (res != 0) {
        perror("clock_gettime");
        FatalError();
    }
    return now;
}


static struct timespec TimeAdd(struct timespec t,
    time_t sec, long nsec) {
    t.tv_sec += sec;
    t.tv_nsec += nsec;
    if (t.tv_nsec < 0) {
        t.tv_sec -= 1;
        t.tv_nsec += 1000000000;
    }
    else if (t.tv_nsec >= 1000000000) {
        t.tv_sec += 1;
        t.tv_nsec -= 1000000000;
    }
    return t;
}


static struct timespec TimeDiff(struct timespec* since,
    struct timespec* upto) {
    return TimeAdd(*upto, -since->tv_sec, -since->tv_nsec);
}


static double TimeElapsed(struct timespec* since) {
    struct timespec now = TimeNow();
    struct timespec diff = TimeDiff(since, &now);
    return (double)diff.tv_sec + (double)diff.tv_nsec / 1e9;
}


void PlatformSleep(PlatformState state, i16 millis) {
    if (millis <= 0) { return; }

    struct timespec start = TimeNow();
    double total = millis / 1000.0;

    for (double timeout = total; timeout > 0.0005; ) {
        glfwWaitEventsTimeout(timeout);
        glfwPollEvents();
        timeout = total - TimeElapsed(&start);
    }
}


void PlatformResetFrame(PlatformState state) {
    if (state->window == NULL) {
        _PlatformCreateWindow(state);
    }
    memset(state->canvas, MtmcDisplayColor_LIGHTEST, state->glcanvassize);
    state->color = MtmcDisplayColor_DARK;
}


void PlatformDrawFrame(PlatformState state) {
    if (state->window == NULL) {
        _PlatformCreateWindow(state);
    }
    glBindBuffer(GL_ARRAY_BUFFER, state->glcanvas);
    glBufferData(GL_ARRAY_BUFFER, state->glcanvassize, state->canvas, GL_DYNAMIC_COPY);
    _PlatformDrawWindow(state);
    _PlatformRunloop(state);
}


void PlatformSetColor(PlatformState state, i16 color) {
    state->color = color;
}


void PlatformFillRect(PlatformState state, i16 x, i16 y,
    i16 width, i16 height) {
    if (state->window == NULL) {
        _PlatformCreateWindow(state);
    }
    for (i16 i = 0; i < width; ++i) {
        if ((i + x >= state->screen_width) || (i + x < 0)) { continue; }
        for (i16 j = 0; j < height; ++j) {
            if ((j + y >= state->screen_height) || (j + y < 0)) { continue; }
            size_t n = state->screen_height * (i + x + 1) - (j + y + 1);
            state->canvas[n] = state->color;
        }
    }
}


void PlatformDrawImage(PlatformState state, struct MtmcGraphic* image,
    i16 x, i16 y) {
    for (int i = 0; i < image->width; ++i) {
        if ((i + x >= state->screen_width) || (i + x < 0)) { continue; }
        for (int j = 0; j < image->height; ++j) {
            if ((j + y >= state->screen_height) || (j + y < 0)) { continue; }
            u8 mask = image->mask[(image->width + 7) / 8 * j + i / 8] >> (i % 8);
            if ((mask & 1) == 1) { continue; }
            size_t n = state->screen_height * (i + x + 1) - (j + y + 1);
            u8 c = (image->data[(image->width + 3) / 4 * j + i / 4] >> (i % 4 * 2)) & 3;
            state->canvas[n] = c;
        }
    }
}


i16 PlatformGetJoystick(PlatformState state) {
    if (state->window == NULL) {
        _PlatformCreateWindow(state);
    }
    _PlatformRunloop(state);
    // _PlatformPollGamepads(state);
    return state->buttons;
}


char PlatformGetChar(PlatformState state) {
    char buf[4];
    char* res = fgets(buf, sizeof(buf), stdin);
    return (res == NULL) ? 0 : buf[0];
}


void PlatformPutChar(PlatformState state, char c) {
    fputc(c, stdout);
}


void PlatformPutString(PlatformState state, const char* s) {
    fputs(s, stdout);
}


void PlatformPutWord(PlatformState state, i16 n) {
    fprintf(stdout, "%d", (int) n);
}


i16 PlatformParseWord(PlatformState state, const char* s) {
    int x = atoi(s);
    if (x >= -32768 && x <= 32767) {
        return x;
    }
    return 0;
}


i16 PlatformReadWord(PlatformState state) {
    char buf[24];
    char* res = fgets(buf, sizeof(buf), stdin);
    if (res != NULL) {
        return PlatformParseWord(state, buf);
    }
    return 0;
}


void PlatformRandomSeed(PlatformState state, int seed) {
    u64 s = ((u64)seed << 1) | 1;
    state->randstate[0] = (s) & 0xFFFF;
    state->randstate[1] = (s >> 16) & 0xFFFF;
    state->randstate[2] = (s >> 32) & 0xFFFF;
    state->randstate[3] = (s >> 48) & 0xFFFF;
}


i16 PlatformRandom(PlatformState state, i16 start, i16 stop) {
    i16 m = stop - start;
    if (m <= 0) { return start; }
    u64 x = nrand48(&state->randstate[0]);
    i16 res = start + ((((u64)1 << 48) - x) % m);
    return res;
}


i16 PlatformSetTimer(PlatformState state, i16 millis) {
    struct timespec now = TimeNow();
    if (millis > 0) {
        state->timer = TimeAdd(now, 0, millis * 1000000);
    }
    struct timespec diff = TimeDiff(&now, &state->timer);
    if (diff.tv_sec < 0 || diff.tv_nsec < 0) {
        return 0;
    }
    i16 dt = diff.tv_sec * 1000 + diff.tv_nsec / 1e6;
    return dt;
}


static int _PlatformGetDiskPath(PlatformState state,
    char* buf, int bufsize) {
    char* res = realpath("./disk", buf);
    if (res == NULL) { perror("realpath"); return 1; }
    return 0;
}


static int _PlatformTryResolvePath(PlatformState state, const char* filename,
    char* buf, int bufsize) {
    char disk[PATH_MAX];
    char path[PATH_MAX];
    char resolved[PATH_MAX];

    int res = _PlatformGetDiskPath(state, disk, sizeof(disk));
    if (res != 0) { return res; }

    if (strlen(state->cwd) == 0) {
        strcpy(state->cwd, "/");
    }

    res = snprintf(path, sizeof(path), "./disk%s%s",
        (*filename == '/' ? "" : state->cwd), filename);
    if (res <= 0) { perror("snprintf"); return 1; }

    char* pres = realpath(path, resolved);
    if (pres == NULL) {
        return 2;
    }

    int n = strlen(disk);
    if (strncmp(disk, resolved, n) != 0) {
        return 3;
    }

    strncpy(buf, resolved, bufsize);
    buf[bufsize-1] = '\0';
    return 0;
}


static int _PlatformResolvePath(PlatformState state, const char* filename,
    char* buf, int bufsize) {
    int res = _PlatformTryResolvePath(state, filename, buf, bufsize);
    switch (res) {
        case 0: return 0;
        case 2: perror("realpath"); break;
        case 3: fputs("not on disk\n", stderr); break;
    }
    return 1;
}


static int _PlatformFileReadCells(FILE* fp, u8* buf, int maxcol, int maxrow) {
    maxcol = (maxcol + 7) / 8;
    int row = 0, col = 0;
    u8* p = buf;
    u8 value = 0;
    u8 ix = 0;
    int state = 0;
    for (; row < maxrow;) {
        int c = fgetc(fp);
        if (c == EOF) { break; }
        switch (state) {
            case 0:
                if (isspace(c) != 0) {
                }
                else if (c == '!') {
                    state = 9;
                }
                else {
                    value = (c == 'O') ? 1 : 0;
                    ix = 1;
                    state = 1;
                }
                break;
            case 1:
                if (c == '\n') {
                    for (; col < maxcol; ++col) {
                        *p++ = value;
                        value = 0;
                    }
                    row += 1;
                    col = 0;
                    value = 0;
                    ix = 0;
                }
                else {
                    value |= ((c == 'O') ? 1 : 0) << ix;
                    if (ix < 7) {
                        ix += 1;
                    }
                    else {
                        *p++ = value;
                        col += 1;
                        value = 0;
                        ix = 0;
                        if (col >= maxcol) {
                            row += 1;
                            col = 0;
                            state = 8;
                        }
                    }
                }
                break;
            case 8:
                if (c == '\n') {
                    state = 1;
                }
                break;
            case 9:
                if (c == '\n') {
                    state = 0;
                }
                break;
        }
    }
    for (; col < maxcol; ++col) {
        *p++ = value;
        value = 0;
    }
    for (; row < maxrow; ++row) {
        for (int i = 0; i < maxcol; ++i) {
            *p++ = 0;
        }
    }
    return 0;
}


i16 PlatformFileRead(PlatformState state, const char* filename,
    u8* buf, i16 bufsize, i16 maxlines) {
    char path[PATH_MAX];
    int res = _PlatformResolvePath(state, filename, path, sizeof(path));
    if (res != 0) {
        fprintf(stderr, "%s\n", filename);
        return 1;
    }
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) {
        perror("fopen");
        fprintf(stderr, "%s\n", filename);
        return 1;
    }
    int n = strlen(path);
    if (strcmp(&path[n - 6], ".cells") == 0) {
        int res = _PlatformFileReadCells(fp, buf, bufsize, maxlines);
        if (res != 0) { fclose(fp); return -1; }
    }
    else {
        size_t n = fread(buf, 1, bufsize, fp);
        if (n == 0) { perror("fread"); fclose(fp); return -1; }
    }
    fclose(fp);
    return 0;
}


i16 PlatformGetCurrentDir(PlatformState state, char* buf, size_t bufsize) {
    char path[PATH_MAX];
    _PlatformResolvePath(state, "/", path, sizeof(path));
    int res = strlen(state->cwd);
    if (res + 1 <= (int)bufsize) {
        strcpy(buf, state->cwd);
    }
    else {
        res = bufsize - 1;
        strncpy(buf, state->cwd, bufsize);
        buf[bufsize - 1] = '\0';
    }
    return res;
}


i16 PlatformSetCurrentDir(PlatformState state, const char* name) {
    char disk[PATH_MAX];
    char path[PATH_MAX];
    int res = _PlatformResolvePath(state, name, path, sizeof(path));
    if (res != 0) { return res; }
    res = _PlatformGetDiskPath(state, disk, sizeof(disk));
    if (res != 0) { return res; }
    int n = strlen(disk);
    strcpy(state->cwd, &path[n]);
    if (strlen(state->cwd) == 0) {
        strcpy(state->cwd, "/");
    }
    return 0;
}


i16 PlatformDirGetSize(PlatformState state, const char* name) {
    char path[PATH_MAX];
    int res = _PlatformTryResolvePath(state, name, path, sizeof(path));
    if (res != 0) { return -1; }
    struct dirent** entries;
    int n = scandir(path, &entries, NULL, alphasort);
    if (n < 0) { perror("scandir"); return -1; }
    res = n;
    for (int i = 0; i < n; ++i) {
        if (strcmp(".", entries[i]->d_name) == 0 ||
            strcmp("..", entries[i]->d_name) == 0) {
            res -= 1;
        }
    }
    free(entries);
    return res;
}


i16 PlatformDirReadEntry(PlatformState state, const char* name,
    i16 index, i16* flags, char* buf, size_t bufsize) {
    char path[PATH_MAX];
    int res = _PlatformResolvePath(state, name, path, sizeof(path));
    if (res != 0) { return -1; }
    if (index++ < 0) { return -1; }

    struct dirent** entries;
    int n = scandir(path, &entries, NULL, alphasort);
    if (n < 0) {
        perror("scandir");
        fprintf(stderr, "path: %s\n", path);
        return -1;
    }

    const char* sname = NULL;
    for (int i = 0; i < n && index > 0; ++i) {
        sname = entries[i]->d_name;
        if (strcmp(".", sname) == 0 ||
            strcmp("..", sname) == 0) {
            continue;
        }
        index -= 1;
    }
    if (index != 0) { free(entries); return -1; }

    strcat(path, "/");
    strcat(path, sname);

    *flags = 0;
    struct stat st;
    res = stat(path, &st);
    if (res == 0) {
        *flags |= S_ISDIR(st.st_mode) == 0 ? 0 : 1;
    }
    else {
        perror("stat");
        fprintf(stderr, "path: %s\n", path);
    }

    res = strlen(sname);
    if (res + 1 <= (int)bufsize) {
        strcpy(buf, sname);
    }
    else {
        res = bufsize - 1;
        strncpy(buf, sname, res + 1);
        buf[res + 1] = '\0';
    }
    free(entries);
    return res;
}
