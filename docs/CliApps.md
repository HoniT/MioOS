# Kernel Command Line Interface (CLI)

To improve modularity, kernel terminal commands have been decoupled from their respective implementation files. Previously, commands such as `heapdump` were located within subsystem files like `src/kernel/mm/heap.cpp`.

### New Architecture
* **Location:** All terminal applications now reside in `src/kernel/apps/`.
* **Grouping:** Commands are grouped into classes based on functionality. For example, filesystem commands like `ls`, `cd`, and `mkdir` are located in `storage_cli.cpp`.
* **Implementation:** New commands should inherit from the `cli_app` base class. This class provides utility helper functions, such as `get_params`, to streamline the creation of kernel command line applications.

---

### Key File Structure
| Category | File Path | Examples |
| :--- | :--- | :--- |
| **Storage** | `src/kernel/apps/storage_cli.cpp` | `ls`, `cd`, `mkdir` |
| **Memory** | `src/kernel/apps/memory_cli.cpp` | `heapdump`, `heapinfo` |
| **System** | `src/kernel/apps/sys_cli.cpp` | `sysinfo`, `uptime` |
| **Base Class**| `src/kernel/apps/cli_app.hpp` | (Inheritance & Helpers) |
