# Directories
BUILD = $(CURDIR)/build
SRC = $(CURDIR)/src

# HPP file includes
INCLUDE = -I $(SRC)/kernel/include \
          -I $(SRC)/kernel/include/arch

# Flags and tools
NASM = nasm

CXX = i686-elf-g++
CXX_FLAGS = $(INCLUDE) -g -Wall -O2 -ffreestanding -mgeneral-regs-only \
            -fno-use-cxa-atexit -fno-exceptions -fno-rtti -fno-pic \
            -fno-asynchronous-unwind-tables -m32 -march=i386

LD = i686-elf-ld
OBJCOPY = i686-elf-objcopy

# Output files
OS_ELF = $(CURDIR)/iso/boot/mio_os.elf

# Find all .cpp and .asm files
CPP_SOURCES = $(shell find $(SRC) -name "*.cpp")
ASM_SOURCES = $(shell find $(SRC) -name "*.asm")

# Convert sources to objects
CPP_OBJECTS = $(patsubst $(SRC)/%.cpp,$(BUILD)/%.o,$(CPP_SOURCES))
ASM_OBJECTS = $(patsubst $(SRC)/%.asm,$(BUILD)/%.o,$(ASM_SOURCES))

# Default target
all: $(OS_ELF)

# Link the kernel
$(OS_ELF): $(ASM_OBJECTS) $(CPP_OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) -m elf_i386 -T $(SRC)/kernel/linker.ld -o $@ $(ASM_OBJECTS) $(CPP_OBJECTS)

# Compile .cpp files
$(BUILD)/%.o: $(SRC)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXX_FLAGS) -std=c++23 -c $< -o $@

# Assemble .asm files
$(BUILD)/%.o: $(SRC)/%.asm
	@mkdir -p $(dir $@)
	$(NASM) -f elf32 -g $< -o $@

# Clean
clean:
	rm -rf $(BUILD)
	mkdir -p $(BUILD)
	rm -f $(OS_ELF) iso/mio_os.iso

.PHONY: all clean
