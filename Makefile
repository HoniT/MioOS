BUILD = $(CURDIR)/build
SRC   = $(CURDIR)/src

INCLUDE = -I $(SRC)/kernel/include

ARCH ?= x86_64

CXX_FLAGS = $(INCLUDE) -g -Wall -O2 -ffreestanding \
            -fno-use-cxa-atexit -fno-exceptions -fno-rtti -fno-pic \
            -fno-asynchronous-unwind-tables
DEP_FLAGS = -MMD -MP

ifeq ($(ARCH), x86_64)
    PREFIX = x86_64-elf-
    CXX_FLAGS += -m64 -mcmodel=kernel -mno-red-zone -mgeneral-regs-only
    LD_FLAGS = -m elf_x86_64
    ASM_FORMAT = elf64
endif

CXX = $(PREFIX)g++
LD  = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
NASM = nasm

# Exclude arch folder from core sources to prevent compiling wrong architectures
CORE_CPP_SOURCES = $(shell find $(SRC)/kernel -name "*.cpp" ! -path "*/arch/*")
ARCH_CPP_SOURCES = $(shell find $(SRC)/kernel/arch/$(ARCH) -name "*.cpp" 2>/dev/null)

ARCH_ASM_SOURCES = \
    $(shell find $(SRC)/boot/arch/$(ARCH) -name "*.asm" 2>/dev/null) \
    $(shell find $(SRC)/kernel/arch/$(ARCH) -name "*.asm" 2>/dev/null)

CPP_SOURCES = $(CORE_CPP_SOURCES) $(ARCH_CPP_SOURCES)
ASM_SOURCES = $(ARCH_ASM_SOURCES)

OS_ELF = $(CURDIR)/iso/boot/mio_os.elf

CPP_OBJECTS = $(patsubst $(SRC)/%.cpp,$(BUILD)/%.o,$(CPP_SOURCES))
ASM_OBJECTS = $(patsubst $(SRC)/%.asm,$(BUILD)/%.o,$(ASM_SOURCES))

DEPS = $(CPP_OBJECTS:.o=.d)

all: $(OS_ELF)

$(OS_ELF): $(ASM_OBJECTS) $(CPP_OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) $(LD_FLAGS) -T $(SRC)/kernel/arch/$(ARCH)/linker.ld -o $@ \
		$(ASM_OBJECTS) $(CPP_OBJECTS)

$(BUILD)/%.o: $(SRC)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXX_FLAGS) $(DEP_FLAGS) -std=c++23 -c $< -o $@

$(BUILD)/%.o: $(SRC)/%.asm
	@mkdir -p $(dir $@)
	$(NASM) -f $(ASM_FORMAT) -g $< -o $@

clean:
	rm -rf $(BUILD)
	mkdir -p $(BUILD)
	rm -f $(OS_ELF) iso/mio_os.iso

-include $(DEPS)

.PHONY: all clean