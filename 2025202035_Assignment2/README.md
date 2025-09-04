# Custom Shell (`mysh`)

This project implements a **mini-shell in C++** that supports various UNIX-like shell features such as built-in commands, pipelines, redirections, history, job control signals, and auto-completion.

---

## Features and Approach

### 1. **Prompt**
- Displays in the format:
user@host:path>
- Colored for readability.
- Path is displayed using `tildeify()`:
- If inside the **launch directory** (shell home), it shows relative `~`.
- Otherwise, the absolute path is shown.

---

### 2. **Home Directory**
- **Shell home** = the directory where the shell was launched from.
- Used in prompt tilde substitution.
- **Special Note:**  
- `cd ~` takes the shell to the **system root (`/`)**, not the shell home.  
- This is a design choice to differentiate between the shell’s launch directory and the actual system root.

---

### 3. **Built-in Commands**
Implemented directly in the shell without forking:
- `cd`  
- Handles `.`, `..`, `-`, and `~`.
- Tracks `OLDPWD` for `cd -`.
- Updates `PWD` internally so that prompt stays in sync.
- `pwd`  
- Prints the absolute current working directory.
- `echo`  
- Prints arguments with spacing.
- `ls`  
- Supports `-a` and `-l` flags.
- Displays tabular format like real `ls`.
- `pinfo`  
- Shows process information (PID, status, memory, and executable path).
- Adds `+` when process is foreground and running.
- `search`  
- Recursively searches from current directory for a file/directory.
- `history`  
- Maintains last 20 commands in `.mysh_history_child`.

---

### 4. **External Commands**
- For non-builtin commands, `execvp()` is used in forked child processes.
- Supports execution of editors like `vi`, `emacs`, or custom binaries.

---

### 5. **Pipelines and Redirection**
- Multiple stages are connected using pipes.
- `<`, `>`, `>>` redirections are supported.
- Redirection applied before execution using `dup2`.

---

### 6. **History & Navigation**
- Stores last 20 commands.
- Persistent between sessions using `~/.mysh_history_child`.
- Implemented via GNU `readline`:
- **Arrow keys**: navigate history inline.
- **Tab completion**: completes builtins, executables, and filenames.

---

### 7. **Signals**
- Custom handlers for:
- `Ctrl+C` (`SIGINT`) → Kills foreground process if running, ignored otherwise.
- `Ctrl+Z` (`SIGTSTP`) → Stops foreground process and pushes it to background, ignored if none running.
- Shell itself never terminates or suspends due to these keys.

---

### 8. **Exit Commands**
- `exit` → closes the shell.
- `exitall` → prints termination message and exits.

---

## Feature-to-File Mapping

| **File**          | **Responsibility / Features**                                                                 |
|--------------------|-----------------------------------------------------------------------------------------------|
| `main.cpp`         | Shell entrypoint, main loop, integrates all modules, loads/saves history.                     |
| `prompt.cpp/.h`    | Builds and formats the colored prompt. Handles user/host/path display and tilde substitution. |
| `parser.cpp/.h`    | Splits input lines by `;`, tokenizes commands, handles pipelines and arguments.               |
| `exec.cpp/.h`      | Executes commands. Builtins run in parent, externals via `execvp`. Handles pipes & redirs.    |
| `builtins.cpp/.h`  | Implements built-in commands: `cd`, `pwd`, `echo`, `ls`, `pinfo`, `search`, `history`.        |
| `signals.cpp/.h`   | Signal handlers (`SIGINT`, `SIGTSTP`). Tracks foreground PID group (`FG_PGID`).               |
| `arrow.cpp/.h`     | Input handling via GNU Readline. Provides history navigation with arrows and autocomplete.    |
| `common.cpp/.h`    | Shared helpers (trimming, string split, tildeify, global vars like `SHELL_HOME`).             |

---

## Design Choices
- **Shell Home:** launch directory (`getcwd()` at startup).
- **System Root:** `/`. Used when `cd ~` is run.
- History persistence limited to last **20 commands** for simplicity.
- Builtins are run in **parent shell process** to ensure state changes (like `cd`) are reflected.

---

## Compilation & Run
```bash
make clean && make / make
make run


