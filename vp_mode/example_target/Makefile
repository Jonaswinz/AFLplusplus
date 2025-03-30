SRCDIR           ?= $(dir $(firstword $(MAKEFILE_LIST)))

O                ?= $(PWD)/build
P                ?= $(O)/install

ITERATIONS 	     := 30000
HAS_FLOAT        := 0
CC               := aarch64-none-elf-gcc
CXX              := aarch64-none-elf-g++
LD               := aarch64-none-elf-g++
OBJCPY           := aarch64-none-elf-objcopy

CFLAGS           := -ffreestanding -O0 -I./
LFLAGS           := -nostartfiles -T link.ld -Wl,--build-id=none
CXXFLAGS         := $(CFLAGS)

PROGRAM          := $(O)/main.elf
BIN_PROGRAM      := $(O)/main.bin
OBJECTS          := $(O)/boot.o  \
                    $(O)/main.o  \

.PHONY: all clean install install_dirs install_program

all: $(PROGRAM)

install: $(PROGRAM)
	$(S) echo -e "  INSTALL\t$(notdir $(PROGRAM))"
	$(S) install -m 755 $(PROGRAM) $(P)
	$(S) echo -e "  INSTALL\t$(notdir $(BIN_PROGRAM))"
	$(S) install -m 755 $(BIN_PROGRAM) $(P)

clean:
	$(S) echo -e "  CLEAN"
	$(S) $(RM) $(OBJECTS) $(PROGRAM) $(BIN_PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(S) echo -e "  LINK\t$(notdir $@)"
	$(S) $(LD) $(LFLAGS) -o $@ $^
	$(S) $(OBJCPY) -O binary $(PROGRAM) $(BIN_PROGRAM)

$(O)/%.o: %.cpp
	$(S) echo -e "  CC\t$(notdir $@)"
	$(S) mkdir -p $(dir $@)
	$(S) $(CXX) $(CXXFLAGS) -c -o $@ $<

$(O)/%.o: %.S
	$(S) echo -e "  CC\t$(notdir $@)"
	$(S) mkdir -p $(dir $@)
	$(S) $(CC) $(CFLAGS) -c -o $@ $<