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
TEST=$(SRC)/test
INC=$(SRC)/include

# Source and header files.
SRC_OBJS=graph node asm runtime vector slab hashtable slabheap rehashtable nodepool eval opcodes
INC_OBJS=graph node asm runtime

OBJ_PATHS=$(addprefix $(SRC)/, $(addsuffix .o, $(SRC_OBJS)))
SRC_PATHS=$(addprefix $(SRC)/, $(addsuffix .c, $(SRC_OBJS)))

TEST_OBJ_PATHS=$(addprefix $(TEST)/, $(addsuffix .o, $(SRC_OBJS)))
TEST_PATHS=$(addprefix $(TEST)/, $(addsuffix .c, $(SRC_OBJS)))

INC_PATHS=$(addprefix $(INC)/, $(addsuffix .h, $(INC_OBJS)))

LIBS=m

# Compiler flags.
CCFLAGS=-std=gnu11 -I$(INC) -I$(SRC) -Wall -Wextra -Wno-unused-parameter -Wformat -Wpedantic -Wconversion -Wmissing-prototypes -Werror
DEBUG= -g #-pg -Ofast
LIBFLAGS=$(addprefix -l, $(LIBS))

# Default rule for compiling object files.
%.o: %.c $(INC_PATHS) $(SRC)/*.h
	$(CC) $(CCFLAGS) $(DEBUG) -c $< -o $@

# Rule for compiling main executable.
ndlrun: $(OBJ_PATHS) $(SRC)/nodel.o
	$(CC) $(DEBUG) $(LIBFLAGS) $^ -o $@

# Rule for compiling main executable.
ndlasm: $(OBJ_PATHS) $(SRC)/nodelasm.o
	$(CC) $(DEBUG) $(LIBFLAGS) $^ -o $@

# Rule for compiling main executable.
main: $(OBJ_PATHS) $(SRC)/main.o
	$(CC) $(DEBUG) $(LIBFLAGS) $^ -o $@

# Rule for compiling testing executable.
ndltest: $(OBJ_PATHS) $(TEST_OBJ_PATHS) $(SRC)/test.o
	$(CC) $(DEBUG) $(LIBFLAGS) $^ -o $@

run: ndlrun
	./ndlrun

# Main rule.
all: ndlrun main ndlasm ndltest

# Clean repo.
clean:
	rm -f $(OBJ_PATHS) $(TEST_OBJ_PATHS) \
	ndlrun ndlasm main ndltest $(SRC)/nodel.o $(SRC)/nodelasm.o $(SRC)/main.o $(SRC)/test.o

.PHONY: all_proxy all clean
