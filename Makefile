BUILD = $(CURDIR)/build
SRC   = $(CURDIR)/src

INCLUDE = -I $(SRC)/kernel/include

CXX_FLAGS = $(INCLUDE) -g -Wall -O2 -ffreestanding \
            -fno-use-cxa-atexit -fno-exceptions -fno-rtti -fno-pic \
            -fno-asynchronous-unwind-tables -m64 -mcmodel=kernel \
            -mno-red-zone -mgeneral-regs-only
DEP_FLAGS = -MMD -MP

CXX = x86_64-elf-g++
LD  = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy
NASM = nasm

CPP_SOURCES = $(shell find $(SRC) -name "*.cpp")
ASM_SOURCES = $(shell find $(SRC) -name "*.asm")

OS_ELF = $(CURDIR)/iso/boot/mio_os.elf

CPP_OBJECTS = $(patsubst $(SRC)/%.cpp,$(BUILD)/%.o,$(CPP_SOURCES))
ASM_OBJECTS = $(patsubst $(SRC)/%.asm,$(BUILD)/%.o,$(ASM_SOURCES))

DEPS = $(CPP_OBJECTS:.o=.d)

all: $(OS_ELF)

$(OS_ELF): $(ASM_OBJECTS) $(CPP_OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -T $(SRC)/kernel/linker.ld -o $@ \
		$(ASM_OBJECTS) $(CPP_OBJECTS)

$(BUILD)/%.o: $(SRC)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXX_FLAGS) $(DEP_FLAGS) -std=c++23 -c $< -o $@

$(BUILD)/%.o: $(SRC)/%.asm
	@mkdir -p $(dir $@)
	$(NASM) -f elf64 -g $< -o $@

clean:
	rm -rf $(BUILD)
	mkdir -p $(BUILD)
	rm -f $(OS_ELF) iso/mio_os.iso

-include $(DEPS)

.PHONY: all clean