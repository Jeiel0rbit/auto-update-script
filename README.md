# Cross-Platform Linux Update Script

This C++ command-line tool automates system updates and upgrades for **Debian/Ubuntu Linux** distributions. It performs:

- Validation of supported Linux distros (only Ubuntu and Debian).
- Dry-run simulation of update and upgrade commands to prevent unexpected errors.
- Real execution with real-time, colored output for logs, info, warnings, and errors.
- Clear ASCII separators between update and upgrade phases.
- Immediate abortion if errors are detected in the simulation phase to avoid unintended consequences.

---

## Features

- **Ubuntu/Debian support** using `apt-get`.
- Color-coded terminal output (yellow for info, green for logs, gray for warnings, red for errors).
- Safe: runs simulations before real operations.
- Stops execution if unsupported OS or distribution detected.

---

## Requirements

- C++ compiler (tested with `clang++` and `g++`)
- On Linux: `sudo` privileges to run `apt-get` commands.
  
---

## Compilation

```bash
clang++ up.cpp -o up
# or
g++ up.cpp -o up
```

---

Usage

Run the compiled executable from your terminal:

`./up`

The script will:

1. Detect if running on supported Linux.


2. Perform a dry-run simulation of update and upgrade commands.


3. If simulations succeed, perform the real update and upgrade.


4. Print color-coded output with progress and errors.


---

> [!note]
>
> Only Ubuntu and Debian distributions are supported on Linux.
The script aborts on any errors during simulations to prevent unexpected states.
Output warnings (e.g., apt CLI warnings) appear in gray.
Customize colors by modifying ANSI codes in the source if desired.

---

## License

MIT License


---

Feel free to open issues or contribute improvements!
