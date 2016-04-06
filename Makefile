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
SRC_OBJS=test node graph runtime asm slab nodepool eval opcodes
INC_OBJS=     node graph runtime asm

OBJ_PATHS=$(addprefix $(SRC)/, $(addsuffix .o, $(SRC_OBJS)))
SRC_PATHS=$(addprefix $(SRC)/, $(addsuffix .c, $(SRC_OBJS)))
INC_PATHS=$(addprefix $(INC)/, $(addsuffix .h, $(INC_OBJS)))

LIBS=m


# Compiler flags.
CCFLAGS=-I$(INC) -Wall -Wextra -Wno-unused-parameter -Wformat -Wpedantic -O2
DEBUG=#-g -pg
LIBFLAGS=$(addprefix -l, $(LIBS))

# Default rule for compiling object files.
%.o: %.c $(INC_PATHS)
	$(CC) $(CCFLAGS) $(DEBUG) -c $< -o $@

# Rule for compiling main executable.
nodel: $(OBJ_PATHS)
	$(CC) $(DEBUG) $(LIBFLAGS) $^ -o $@

run: nodel
	./nodel

# Main rule.
all: nodel

# Clean repo.
clean:
	rm -f $(OBJ_PATHS) nodel

.PHONY: all_proxy all clean
