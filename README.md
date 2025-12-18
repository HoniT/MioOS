<div align="center">

<h1 style="font-size: 3em; font-weight: bold; margin: 0;">MioOS</h1>

**A 32-bit hobby operating system for learning low-level systems programming**

*Built with C++ and x86 assembly*

[Features](#features) • [Installation](#installation-and-usage) • [Building](#building-and-running) • [Documentation](#documentation) • [Contributing](#contributing)

---

</div>

# Features

- **GRUB Bootloader** - Standard multiboot-compliant bootloader
- **32-bit Kernel** - Written in modern C++
- **Memory Management**
  - Fragmentation handling PMM
  - Dynamic heap allocation
  - 32-bit paging and virtual memory manager
- **File Systems**
  - IRQ mode PIO ATA driver
  - Ext2 file system implementation
  - Virtual file system layer
- **Process Management** - Basic process scheduling
- **Hardware Drivers** - Keyboard input support

# Project Structure
```
MioOS/
├── boot/              # Bootloader and GRUB configuration
├── kernel/            # Core kernel implementation
│   ├── arch/          # Architecture-specific code
│   │   ├── interrupts/    # IDT and interrupt handling
│   │   └── sched/         # Architecture-specific scheduling
│   ├── drivers/       # Hardware drivers
│   ├── fs/            # File system implementations
│   ├── graphics/      # Graphics and visual output
│   ├── lib/           # Helper libraries and utilities
│   ├── mm/            # Memory management subsystems
│   ├── sched/         # Process scheduling and management
│   └── tests/         # Unit tests
├── docs/              # Additional documentation
└── scripts/           # Build and toolchain scripts
```

# Installation and Usage

**1. Clone the repository**
```bash
git clone https://github.com/HoniT/MioOS.git
cd MioOS
```

**2. Install the toolchain**

For Debian-based distributions:
```bash
bash ./scripts/toolchain/debian_build.sh
```

For Arch Linux:
```bash
bash ./scripts/toolchain/arch_build.sh
```

For other distributions, Windows, and macOS, follow the [GCC Cross-Compiler Guide](https://wiki.osdev.org/GCC_Cross-Compiler) and install dependencies manually.

# Building and Running

### Quick Start

Build and run with a single command:
```bash
bash ./scripts/build_run.sh
```

#### Build Only
```bash
bash ./scripts/build.sh
```

### Run with KVM Acceleration

Automatically uses KVM if available, falls back to standard QEMU otherwise:
```bash
bash ./scripts/run_kvm.sh
```

#### Run without KVM
```bash
bash ./scripts/run.sh
```

# Contributing

Contributors are welcome to submit improvements and bug fixes! Please visit the [GitHub repository](https://github.com/HoniT/MioOS) to:

- Report issues
- Submit pull requests
- Suggest new features
- Improve documentation

# Documentation

Additional documentation and technical details can be found in the `/docs/` directory.

# License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

# Credits

- **Project Author:** [Ioane Baidoshvili](https://github.com/HoniT)
- **Special Thanks:** [Michael Petch](https://github.com/mpetch) for contributing to this project

---

<div align="center">


*Star this repository if you find it interesting!*

</div>