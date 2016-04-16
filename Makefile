#!/usr/bin/make

all_proxy: all

VERSION=0.0.9

# Toolchain programs.
CC=gcc
LD=ld
AR=ar
LN=ln


# Directories.
SRC=src
INC=src/include

# Source and header files.
SRC_OBJS=graph node asm runtime slab hashtable slabheap rehashtable nodepool eval opcodes
INC_OBJS=graph node asm runtime

OBJ_PATHS=$(addprefix $(SRC)/, $(addsuffix .o, $(SRC_OBJS)))
SRC_PATHS=$(addprefix $(SRC)/, $(addsuffix .c, $(SRC_OBJS)))
INC_PATHS=$(addprefix $(INC)/, $(addsuffix .h, $(INC_OBJS)))

LIBS=m

# Compiler flags.
CCFLAGS=-std=gnu11 -I$(INC) -Wall -Wextra -Wno-unused-parameter -Wformat -Wpedantic -Wconversion -Wmissing-prototypes -Werror
DEBUG= -g #-pg -Ofast
LIBFLAGS=$(addprefix -l, $(LIBS))

# Default rule for compiling object files.
%.o: %.c $(INC_PATHS) $(SRC)/*.h
	$(CC) $(CCFLAGS) $(DEBUG) -c $< -o $@

# Rule for compiling main executable.
nodel: $(OBJ_PATHS) $(SRC)/nodel.o
	$(CC) $(DEBUG) $(LIBFLAGS) $^ -o $@

# Rule for compiling main executable.
nodelasm: $(OBJ_PATHS) $(SRC)/nodelasm.o
	$(CC) $(DEBUG) $(LIBFLAGS) $^ -o $@

# Rule for compiling main executable.
test: $(OBJ_PATHS) $(SRC)/test.o
	$(CC) $(DEBUG) $(LIBFLAGS) $^ -o $@

run: nodel
	./nodel

# Main rule.
all: nodel test nodelasm

# Clean repo.
clean:
	rm -f $(OBJ_PATHS) nodel test

.PHONY: all_proxy all clean
