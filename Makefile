# Directories
BUILD = $(CURDIR)/build
SRC   = $(CURDIR)/src

# Include paths
INCLUDE = -I $(SRC)/kernel/include

# Tools
NASM = nasm

CXX = x86_64-elf-g++
LD  = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy

# Flags
CXX_FLAGS = $(INCLUDE) -g -Wall -O2 -ffreestanding -mgeneral-regs-only \
            -fno-use-cxa-atexit -fno-exceptions -fno-rtti -fno-pic \
            -fno-asynchronous-unwind-tables -m64 -mcmodel=kernel -mno-red-zone

DEP_FLAGS = -MMD -MP

# Output
OS_ELF = $(CURDIR)/iso/boot/mio_os.elf

# Sources
CPP_SOURCES = $(shell find $(SRC) -name "*.cpp")
ASM_SOURCES = $(shell find $(SRC) -name "*.asm")

# Objects
CPP_OBJECTS = $(patsubst $(SRC)/%.cpp,$(BUILD)/%.o,$(CPP_SOURCES))
ASM_OBJECTS = $(patsubst $(SRC)/%.asm,$(BUILD)/%.o,$(ASM_SOURCES))

DEPS = $(CPP_OBJECTS:.o=.d)

# Targets
all: $(OS_ELF)

# Link
$(OS_ELF): $(ASM_OBJECTS) $(CPP_OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -T $(SRC)/kernel/linker.ld -o $@ \
		$(ASM_OBJECTS) $(CPP_OBJECTS)

# Compile C++
$(BUILD)/%.o: $(SRC)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXX_FLAGS) $(DEP_FLAGS) -std=c++23 -c $< -o $@

# Assemble ASM
$(BUILD)/%.o: $(SRC)/%.asm
	@mkdir -p $(dir $@)
	$(NASM) -f elf64 -g $< -o $@

# Clean
clean:
	rm -rf $(BUILD)
	mkdir -p $(BUILD)
	rm -f $(OS_ELF) iso/mio_os.iso
	rm -f hdd_main.img

# Dependency includes
-include $(DEPS)

.PHONY: all clean