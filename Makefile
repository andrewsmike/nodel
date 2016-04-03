#!/usr/bin/make

all_proxy: all

VERSION=0.0.1

# Toolchain programs.
CC=gcc
LD=ld
AR=ar
LN=ln


# Directories.
SRC=src
INC=src/include

# Source and header files.
SRC_OBJS=test nodepool
INC_OBJS=node

OBJ_PATHS=$(addprefix $(SRC)/, $(addsuffix .o, $(SRC_OBJS)))
SRC_PATHS=$(addprefix $(SRC)/, $(addsuffix .c, $(SRC_OBJS)))
INC_PATHS=$(addprefix $(INC)/, $(addsuffix .h, $(INC_OBJS)))



# Compiler flags.
CCFLAGS=-I$(INC) -Wall -Wextra -Wno-unused-parameter -Wformat -Wpedantic -O2
DEBUG=#-pg -g

# Default rule for compiling object files.
%.o: %.c $(INC_PATHS)
	$(CC) $(CCFLAGS) $(DEBUG) -c $< -o $@

# Rule for compiling main executable.
nodel: $(OBJ_PATHS)
	$(CC) $(DEBUG) $^ -o $@

run: nodel
	./nodel

# Main rule.
all: nodel

# Clean repo.
clean:
	rm -f $(OBJ_PATHS) nodel

.PHONY: all_proxy all clean
