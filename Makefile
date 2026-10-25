CC := gcc
AR := ar

CFLAGS ?= -O3 -Iinclude
ARFLAGS ?= rcs

LIB_TARGET := libm.a
LIB_OBJS := src/path.o \
			src/stack.o \

all: $(LIB_TARGET)

$(LIB_TARGET): $(LIB_OBJS)
	@echo "AR $@"
	@$(AR) $(ARFLAGS) $@ $^

%.o: %.c
	@echo "CC $@"
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

clean:
	find . -type f -name '*.[oad]' -delete
	rm -f mbuild compile_commands.json
