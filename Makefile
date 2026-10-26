CC := gcc
AR := ar

CFLAGS ?= -O2
INCDIR := -Iinclude
ARFLAGS ?= rcs

LIB_TARGET := libm.a
LIB_OBJS := src/path.o \
			src/stack.o \
			src/dynarr.o

all: $(LIB_TARGET)

$(LIB_TARGET): $(LIB_OBJS)
	@echo "AR $@"
	@$(AR) $(ARFLAGS) $@ $^

%.o: %.c
	@echo "CC $@"
	@$(CC) $(CFLAGS) $(INCDIR) -MMD -MP -c $< -o $@

clean:
	find . -type f -name '*.[oad]' -delete
	rm -f mbuild compile_commands.json
