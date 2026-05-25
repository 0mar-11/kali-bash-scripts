#  myShell - Custom Linux Shell Implementation

A lightweight, POSIX-compliant command-line shell written in **C**. 
This project demonstrates a deep understanding of Operating System concepts including process creation, signal handling, and Inter-Process Communication (IPC).

---

## 🚀 Features
*   ✅ **Process Management:** Uses `fork()` and `execvp()` to execute system commands.
*   🔄 **Pipelines:** Supports command chaining using `|` (e.g., `ls | grep text`).
*   📂 **I/O Redirection:** Handles input `<` and output `>` redirection.
*   ️ **Background Execution:** Supports running processes in the background using `&`.
*   🛡️ **Signal Handling:** 
    *   Gracefully handles `Ctrl+C` (`SIGINT`) without terminating the shell.
    *   Prevents "Zombie Processes" using `SIGCHLD` and `waitpid()`.
*   📜 **Built-in Commands:** Includes `cd`, `exit`, `pwd`, and `history`.
*    **Memory Management:** Proper dynamic allocation for command history and strict memory leak prevention.

---

## 🛠️ Tech Stack
*   **Language:** C (C99 / POSIX.1-2008)
*   **System Calls:** `fork`, `execvp`, `waitpid`, `signal`, `pipe`, `dup2`
*   **Build Tool:** Make

---

##  Installation & Usage

### 1. Prerequisites
Ensure you have `gcc` and `make` installed on your system.
```bash
sudo apt update
sudo apt install gcc make

## 1.Use the provided Makefile to compile the project:[make ]
## 2. Running
Start your custom shell:[./myShell]
## 3. Cleaning
Remove compiled files:[make clean]
