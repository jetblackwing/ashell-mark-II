# A-Shell Architecture Documentation

## Overview

A-Shell is an educational Linux shell implementation demonstrating core shell concepts including command parsing, execution, and word expansion. The project is organized in progressive complexity (part2-3 → part4 → part5), with part5 being the most feature-complete.

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Main Event Loop (main.c)                 │
│  • Prompt display                                           │
│  • Command reading (with history support)                   │
│  • Parser/Executor dispatch                                 │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                  Parser (parser.c)                          │
│  • Tokenizes input (scanner.c)                              │
│  • Builds AST of command nodes (node.c)                     │
│  • Simple grammar: command + arguments                      │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│            Word Expansion (wordexp.c, strings.c)            │
│  1. Quote removal                                           │
│  2. Variable substitution ($VAR expansion)                  │
│  3. Command substitution ($(cmd) or `cmd`)                 │
│  4. Arithmetic expansion ($((expr)))                        │
│  5. Pathname globbing (*.txt expansion)                     │
│  6. Field splitting (IFS-based)                             │
│  Result: List of expanded words                             │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│               Execution (executor.c)                         │
│  • Check if builtin (builtins/builtins.c)                   │
│  • Search PATH for executable (executor.c:search_path)      │
│  • Fork & exec or run builtin                               │
│  • Wait for child process (Unix)                            │
└─────────────────────────────────────────────────────────────┘
```

## Core Components

### 1. Input/Output Management

**Files**: `main.c`, `prompt.c`, `source.c`, `source.h`

- **read_cmd()**: Reads user command with terminal control
  - Unix: Raw mode, ANSI arrow key support, history navigation
  - Windows: Simple fgets() wrapper
  - History stored in circular buffer (100 entries max)
  
- **Source abstraction** (`source_s` struct):
  - Buffer-based input stream
  - Position tracking for cursor
  - Used by scanner for character-by-character parsing

### 2. Lexical Analysis (Scanning)

**Files**: `scanner.c`, `scanner.h`

- **Tokenizer** (`tokenize()`):
  - Breaks input into tokens
  - Handles quoted strings (single, double, backticks)
  - Recognizes escape sequences
  - Dynamic buffer with exponential growth
  
- **Token Structure**:
  ```c
  struct token_s {
      struct source_s *src;       // Reference to source
      int text_len;               // Length of token
      char *text;                 // Token text
  };
  ```

### 3. Syntactic Analysis (Parsing)

**Files**: `parser.c`, `parser.h`, `node.c`, `node.h`

- **Simple Parser** (`parse_simple_command()`):
  - Very limited grammar (no pipes, redirections, or compound commands)
  - Builds AST where:
    - Root node = command name
    - Child nodes = arguments
    - Sibling links = argument sequence
    
- **AST Node Structure**:
  ```c
  struct node_s {
      enum node_type_e type;           // NODE_COMMAND, NODE_VAR, etc.
      enum val_type_e val_type;        // VAL_SINT, VAL_STR, etc.
      union symval_u val;              // Actual value
      int children;                    // Child count
      struct node_s *first_child;      // First child node
      struct node_s *next_sibling;     // Sibling pointers
      struct node_s *prev_sibling;
  };
  ```

### 4. Word Expansion Pipeline

**Files**: `wordexp.c`, `strings.c`, `pattern.c`, `shunt.c`

The expansion happens in multiple passes (order matters):

1. **Quote removal**: Remove quotes, process escapes
2. **Variable expansion**: `$VAR`, `$NNN` (positional params)
3. **Command substitution**: ``cmd`` or `$(cmd)`
4. **Arithmetic expansion**: `$((expr))` with Shunting-yard parser
5. **Pathname expansion (globbing)**: `*.txt`, `[abc]` patterns
6. **Field splitting**: Split on IFS whitespace

**Result**: `word_s` linked list of expanded words
```c
struct word_s {
    char  *data;                // Word text
    int    len;                 // Word length
    struct word_s *next;        // Next word in list
};
```

### 5. Command Execution

**Files**: `executor.c`, `executor.h`, `builtins/builtins.c`

- **Execution Flow**:
  1. Collect arguments by expanding each AST node
  2. NULL-terminate argv array
  3. Check builtin command table (linear search)
  4. If builtin: call handler function, return
  5. If external:
     - Unix: fork() → execv() in child
     - Windows: CreateProcess() wrapper
  6. Parent waits for child completion

- **PATH Search** (`search_path()`):
  - Splits PATH on `:` (Unix) or `;` (Windows)
  - Tests each directory for executable file
  - Returns first match or NULL
  
- **Builtin Commands**:
  - Table-based dispatch
  - Current: `dump` (symbol table dump)
  - Extensible via `struct builtin_s` array

### 6. Symbol Table (Variable Storage)

**Files**: `symtab/symtab.c`, `symtab/symtab.h`

- **Symbol Table Stack** (max 256 scopes):
  - Global scope at base
  - Function-local scopes for nesting
  
- **Symbol Entry**:
  ```c
  struct symtab_entry_s {
      char *name;                       // Variable/function name
      enum symbol_type_e val_type;      // STR or FUNC
      char *val;                        // String value
      unsigned int flags;               // FLAG_EXPORT, etc.
      struct node_s *func_body;         // For functions
      struct symtab_entry_s *next;      // Hash chain (linear)
  };
  ```
- **Lookup**: Linear search through entries
- **Scope**: New scope per function call

## Configuration & Constants

**File**: `constants.h`

Central location for all configurable limits:
- `MAX_INPUT_BUFFER`: 4096 bytes
- `INITIAL_TOK_BUFFER`: 1024 bytes (doubled as needed)
- `MAX_HISTORY_SIZE`: 256 commands
- `MAX_PATH_COMPONENT`: 1024 bytes
- `MAX_SYMTAB_STACK`: 256 scopes
- And more...

## Error Handling

**File**: `error.h`, `error.c`

Centralized error reporting macros:
- `SHELL_ERROR()`: Standard error message
- `SHELL_PERROR()`: Error with strerror() suffix
- `SHELL_WARN()`: Warning message
- `SHELL_DEBUG()`: Debug message (if DEBUG_MODE enabled)
- `shell_fatal_error()`: Exit with message

## Utility Functions

**File**: `utils.h`, `utils.c`

Safe wrappers for common operations:
- `shell_malloc()`, `shell_realloc()`: Memory with error checking
- `shell_strdup_bounded()`: Safe string duplication with size limit
- `shell_strlcpy()`, `shell_strlcat()`: Bounds-checking string functions
- `is_executable()`: Check file for execute permission
- `has_path_separator()`: Check if path is absolute

## Build System

**File**: `Makefile`

- GCC with `-Wall -Wextra -g` flags
- Builds into `build/` directory
- Links subdirectory object files (builtins, symtab)
- Target: `shell` executable

## Limitations & Future Work

### Known Limitations
1. **Parser**: No pipe (`|`), redirection (`>`, `<`), or compound commands
2. **No job control**: Blocking wait only, no background jobs
3. **Limited builtins**: Only `dump` and shell metadata commands
4. **Single-threaded**: Not thread-safe (global scanner state)
5. **Partial POSIX compliance**: Educational scope

### Planned Enhancements
1. **Extended parser**: Support pipes and redirections
2. **More builtins**: `cd`, `export`, `alias`, `unset`
3. **Compound commands**: `if`/`while`/`for` loops
4. **Job control**: Background jobs, `fg`/`bg` commands
5. **Command caching**: Cache PATH lookups for performance
6. **Debugging**: Trace/step mode for execution
7. **Full POSIX compliance**: Longer-term goal

## Testing

Run the shell:
```bash
$ cd part5
$ make
$ ./shell
ashell> ls -la
ashell> echo hello world
ashell> exit
```

## Code Quality Notes

- **Safety**: Buffer overflow protections, bounds checking
- **Maintainability**: Centralized constants, consistent error handling
- **Performance**: Token buffer reuse, efficient symbol lookup (linear, but cached)
- **Portability**: Windows/Unix abstraction layer (process creation)
- **Documentation**: GPL headers, function comments, this architecture guide
