# A-Shell Development Guide

## Project Structure

```
ashell-mark-II/
├── part2-3/                    # Version 1: Basic shell
├── part4/                      # Version 2: With builtins
├── part5/                      # Version 3: Full-featured (main focus)
│   ├── constants.h             # Centralized configuration
│   ├── error.h/error.c         # Error handling
│   ├── utils.h/utils.c         # Utility functions
│   ├── cache.h/cache.c         # Command caching (OPTIMIZATION)
│   ├── main.c                  # Main event loop
│   ├── scanner.c/scanner.h     # Tokenization
│   ├── parser.c/parser.h       # Simple parsing
│   ├── executor.c/executor.h   # Command execution
│   ├── shell.h                 # Main declarations
│   ├── node.c/node.h           # AST nodes
│   ├── wordexp.c               # Word expansion
│   ├── strings.c               # String utilities
│   ├── pattern.c               # Glob matching
│   ├── shunt.c                 # Arithmetic parsing
│   ├── source.c/source.h       # Input source abstraction
│   ├── prompt.c                # Prompt display
│   ├── initsh.c                # Shell initialization
│   ├── builtins/
│   │   ├── builtins.c          # Builtin dispatcher
│   │   ├── dump.c              # dump command
│   │   └── shell_builtins.c    # cd, pwd, echo, etc.
│   ├── symtab/
│   │   ├── symtab.c
│   │   └── symtab.h            # Symbol table
│   ├── Makefile                # Build system
│   └── README.md               # Part 5 documentation
├── ARCHITECTURE.md             # Architecture guide
└── README.md                   # Project overview
```

## Code Style & Standards

### Naming Conventions

```c
/* Global constants: UPPER_CASE_WITH_UNDERSCORES */
#define MAX_BUFFER_SIZE    1024
#define PATHSEP            ':'

/* Global variables: lower_case_with_underscores (rare, static preferred) */
static char *global_buffer = NULL;

/* Functions: snake_case_with_underscores */
void init_shell(void);
char* search_path(const char *file);

/* Macros: UPPER_CASE */
#define SHELL_ERROR(fmt, ...)  fprintf(stderr, "ashell: error: " fmt "\n", ##__VA_ARGS__)

/* Types: CamelCase or snake_case_t suffix */
typedef struct {
    int field1;
} my_struct_t;

struct my_struct_s {
    char *name;
};
```

### File Organization

Every source file should have:

1. **License header** (GPL v3)
2. **File comment** describing purpose
3. **Includes** (system first, then local)
4. **Static globals** if any
5. **Function implementations** (public first, helpers last)

### Function Documentation

Use block comments for public functions:

```c
/**
 * Search for a command in PATH
 * 
 * Iterates through PATH directories looking for an executable file.
 * Returns a newly allocated string that must be freed by caller.
 * 
 * @param file - Command name to search for
 * @return Full path if found, NULL if not found (errno set to ENOENT)
 */
char *search_path(const char *file);
```

### Error Handling

Always use centralized error handling:

```c
/* WRONG */
if (malloc(size) == NULL) {
    fprintf(stderr, "memory error\n");
    return NULL;
}

/* CORRECT */
void *ptr = shell_malloc(size);
if (!ptr) {
    return NULL;
}

/* Also correct for perror-style */
if (chdir(dir) != 0) {
    SHELL_PERROR("failed to change directory to '%s'", dir);
    return 1;
}
```

### Memory Management

Follow these patterns:

```c
/* Allocation */
char *buf = shell_malloc(size);
if (!buf) return NULL;

/* Deallocation - always check before free */
if (ptr) {
    free(ptr);
    ptr = NULL;
}

/* String handling - use bounds-checking versions */
shell_strlcpy(dest, src, sizeof(dest));
shell_strlcat(dest, suffix, sizeof(dest));

/* Register cleanup on exit */
atexit(cleanup_function);
```

### Const Correctness

Mark parameters and variables as const when appropriate:

```c
/* GOOD */
char* search_path(const char *file);
void print_message(const char *msg);

/* BAD - loses const safety */
char* search_path(char *file);
```

## Adding New Features

### Adding a New Builtin Command

1. **Implement function** in `builtins/shell_builtins.c`:
```c
int shell_mycommand(int argc, char **argv)
{
    /* Implementation */
    return 0;  /* Exit code */
}
```

2. **Declare function** in `builtins/shell_builtins.h`:
```c
int shell_mycommand(int argc, char **argv);
```

3. **Register in `builtins/builtins.c`**:
```c
struct builtin_s builtins[] =
{
    /* ... existing commands ... */
    { "mycommand", shell_mycommand },
};
```

4. **Recompile**:
```bash
make clean && make
```

### Adding New Error Messages

1. **Define in error handling**:
```c
if (error_condition) {
    SHELL_ERROR("descriptive error message: %s", detail);
    return 1;
}
```

2. **For system errors**:
```c
if (system_call() != 0) {
    SHELL_PERROR("operation failed");
    return 1;
}
```

### Adding New Configuration

1. **Define constant** in `constants.h`:
```c
#define MY_NEW_LIMIT    512
```

2. **Document its purpose** in comments:
```c
/* Maximum size for widget buffers */
#define MY_NEW_LIMIT    512
```

### Optimization Guidelines

1. **Profile before optimizing**
2. **Use `cache_lookup()` for PATH searches** (if implemented)
3. **Consider token buffer reuse** for frequent operations
4. **Avoid string copies where possible**
5. **Use const parameters** to enable compiler optimizations

## Testing Guidelines

### Unit Testing Approach

Since this is educational code without a formal test framework:

```bash
# Test builtin command
$ echo "pwd" | ./shell

# Test word expansion
$ echo "echo $HOME" | ./shell

# Test error handling
$ echo "cd /nonexistent" | ./shell
```

### Manual Testing

```bash
# Interactive testing
$ ./shell
ashell> pwd
ashell> echo $HOME
ashell> export VAR=value
ashell> echo $VAR
ashell> type echo
ashell> exit
```

### Regression Testing

After making changes:

```bash
# Rebuild and test basic commands
$ make clean && make
$ echo -e "pwd\necho test\ntype ls\nexit" | ./shell

# Test word expansion
$ echo -e 'echo $HOME\necho $(pwd)\nexit' | ./shell

# Test error cases
$ echo -e "cd /bad\ntype nonexistent\nexit" | ./shell
```

## Common Tasks

### Debugging

Compile with debug info and use gdb:

```bash
$ make clean && make
$ gdb ./shell
(gdb) run
(gdb) break main
(gdb) continue
(gdb) next
```

### Performance Profiling

Use valgrind for memory analysis:

```bash
$ valgrind --leak-check=full --show-leak-kinds=all ./shell << EOF
pwd
echo test
exit
EOF
```

### Code Style Checking

Use automated tools if available:

```bash
$ clang-format -i *.c *.h
$ cppcheck --enable=all *.c
```

### Building on Different Platforms

**Linux/Unix:**
```bash
$ make
```

**Windows (with MinGW):**
```bash
$ make CC=i686-w64-mingw32-gcc
```

## Architecture Decisions

### Why Linear Search for Builtins?

- **Simple**: Easy to understand and modify
- **Sufficient**: Only 7 builtins (acceptable O(n) search)
- **Educational**: Demonstrates dynamic dispatch clearly

If this becomes >50 builtins, consider hash table.

### Why Simple Parser?

- **Educational**: Focus on execution, not parsing complexity
- **Intentional limitation**: Emphasizes single-command focus
- **Future extension**: Parser can be enhanced to support pipes/redirection

### Why Reusable Token Buffer?

- **Performance**: Reduces malloc/free overhead
- **Simplicity**: Single global buffer (not thread-safe, but acceptable for single-threaded shell)
- **Memory fragmentation**: Reuse reduces heap fragmentation

## Known Technical Debt

Items identified for future improvement:

1. **Parser is too simple** - No pipes or redirections
2. **Symbol table uses linear search** - Consider hash table for >100 variables
3. **No command caching** - PATH searches could be cached
4. **Not thread-safe** - Global state everywhere (intentional for educational scope)
5. **Limited POSIX compliance** - Focus on core features
6. **Windows port incomplete** - Mostly works but some Unix-isms remain

## Contributing

To contribute improvements:

1. **Follow code style** - Use established patterns
2. **Centralize constants** - Add to `constants.h`
3. **Use error macros** - SHELL_ERROR, SHELL_PERROR, etc.
4. **Test thoroughly** - Both success and error cases
5. **Document changes** - Update ARCHITECTURE.md if structure changes
6. **Maintain compatibility** - Don't break existing functionality

## Further Reading

- **POSIX Shell**: https://pubs.opengroup.org/onlinepubs/9699919799/utilities/sh.html
- **Bash Manual**: https://www.gnu.org/software/bash/manual/
- **C Programming**: K&R "The C Programming Language"
- **Unix**: Stevens/Rago "Advanced Programming in the UNIX Environment"

## License

All contributions to A-Shell are licensed under GNU General Public License v3.
