EXE=mtmc16 testmtmc16 testmtmc16asm
DEP=glfw3 libpng

CFLAGS=-O2 -I..
CFLAGS+=-Wall -Wextra -Wno-unused-parameter

SDIR=$(patsubst %/,%, $(dir $(lastword $(MAKEFILE_LIST))))
OS=$(shell uname -s)

Darwin_CFLAGS=$(shell pkg-config --cflags ${DEP})
Darwin_LDFLAGS=-framework OpenGL $(shell pkg-config --libs ${DEP})

CFLAGS += $(${OS}_CFLAGS)
LDFLAGS += $(${OS}_LDFLAGS)

.PHONY: all
all: shaders.c ${EXE}

.PHONY: debug
debug: CFLAGS += -O0 -g -fsanitize=address
debug: all

shaders.c: $(wildcard *.glsl)
	${SDIR}/../tools/sres.py -c -o shaders.c $^

.PHONY: test
test: testmtmc16 testmtmc16asm
	${SDIR}/testmtmc16
	${SDIR}/testmtmc16asm

.PHONY: clean
clean:
	rm -f ${EXE}
	rm -rf *.dSYM
