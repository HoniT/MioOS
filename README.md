# MioOS

MioOS is an Operating System designed for educational reasons.

- [License](LICENSE)


## Table of contents
- [Installation](#installation)
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)
- [Documentation](#documentation)


# Installation


1. Clone the repository:
    ``` bash
    git clone https://www.github.com/HoniT/MioOS
    ```

2. Install dependencies:
    For Windows users it is better to download WSL (Windows Subsystem for Linux) so you can use linux commands!

    - On Linux:
        Installing QEMU
        ``` bash
        sudo apt install qemu qemu-system-i386
        ```

        Installing compilers and tools
        ``` bash
        sudo apt install nasm gcc g++ gdb grub-common
        ```
    
    - On Windows:
        You will need to use package managers such as MinGW or Cygwin.
        To install NASM and QEMU, visit they're site and follow instructions.

3. Install the GCC cross-compiler:
    Follow instructions on the [OSDevOrg](https://www.wiki.osdev.org/GCC_Cross-Compiler).

# Usage

To use this project simply use the bash script located in tools. To do so:

    ``` bash
    bash ./scripts/build_run.sh
    ```

## Features

- 32-bit Operating System
- Kernel written in C++
- 32-bit paging
(More will come in future)


# Contributing

Contributioners are welcome to submit there version of my code on my [GitHub](https://www.github.com/HoniT/MioOS).


# License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.


# Documentation

To boot this OS onto real hardware follow [BootingOnHardware](documentation/BootingOnHardware.md).


## Credits
- Thanks to [Michael Petch](https://www.github.com/mpetch) for contributing to this project.
- Project author: [Ioane Baidoshvili](https://www.github.com/HoniT).
