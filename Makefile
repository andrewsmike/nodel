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
SUBS=container runtime core test

# Source and header files.
SRC_CORE_OBJS=graph node asm nodepool eval opcodes
SRC_CONTAINER_OBJS =heap vector hashtable slab slabheap rehashtable
SRC_RUNTIME_OBJS=runtime
SRC_OBJS=$(addprefix core/, $(SRC_CORE_OBJS)) \
         $(addprefix container/, $(SRC_CONTAINER_OBJS)) \
         $(addprefix runtime/, $(SRC_RUNTIME_OBJS))

OBJ_PATHS=$(addprefix $(SRC)/, $(addsuffix .o, $(SRC_OBJS)))
SRC_PATHS=$(addprefix $(SRC)/, $(addsuffix .c, $(SRC_OBJS)))
INC_PATHS=$(addprefix $(SRC)/, $(addsuffix .h, $(SRC_OBJS)))

TEST_OBJ_PATHS=$(addprefix $(TEST)/, $(addsuffix .o, $(SRC_OBJS)))
TEST_SRC_PATHS=$(addprefix $(TEST)/, $(addsuffix .c, $(SRC_OBJS)))

INC_SUBS=$(addprefix -I, $(addprefix $(SRC)/, $(SUBS)))

LIBS=m

# Compiler flags.
CCWARN=all extra no-unused-parameter format pedantic conversion missing-prototypes error
CCVERSION=gnu11
CCDEBUG= -g #-pg -Ofast

CCFLAGS=-std=$(CCVERSION) $(INC_SUBS) $(addprefix -W, $(CCWARN)) $(CCDEBUG)

CCLIBS=$(addprefix -l, $(LIBS))

# Default rule for compiling object files.
%.o: %.c $(INC_PATHS)
	$(CC) $(CCFLAGS) -c $< -o $@

# Rule for compiling main executable.
ndlrun: $(OBJ_PATHS) $(SRC)/nodelrun.o
	$(CC) $(CCDEBUG) $(CCLIBS) $^ -o $@

# Rule for compiling main executable.
ndlasm: $(OBJ_PATHS) $(SRC)/nodelasm.o
	$(CC) $(CCDEBUG) $(CCLIBS) $^ -o $@

# Rule for compiling main executable.
ndldump: $(OBJ_PATHS) $(SRC)/nodeldump.o
	$(CC) $(CCDEBUG) $(CCLIBS) $^ -o $@

# Rule for compiling main executable.
main: $(OBJ_PATHS) $(SRC)/main.o
	$(CC) $(CCDEBUG) $(CCLIBS) $^ -o $@

# Rule for compiling testing executable.
ndltest: $(OBJ_PATHS) $(TEST_OBJ_PATHS) $(SRC)/test.o
	$(CC) $(CCDEBUG) $(CCLIBS) $^ -o $@

# Main rule.
all: ndlrun main ndlasm ndltest ndldump

# Clean repo.
clean:
	rm -f $(OBJ_PATHS) $(TEST_OBJ_PATHS) \
	ndlrun ndlasm main ndltest $(SRC)/nodelrun.o $(SRC)/nodeldump.o $(SRC)/nodelasm.o $(SRC)/main.o $(SRC)/test.o

.PHONY: all_proxy all clean
