# A-Shell Part 5 - Enhanced Shell Implementation

## Overview

Part 5 is the most advanced version of A-Shell, featuring:

- **Complete word expansion pipeline** (variables, command substitution, globbing)
- **Symbol table** for variable storage
- **Arithmetic expression evaluation** using Shunting-yard algorithm
- **Extended builtin commands** (cd, pwd, export, unset, echo, type, dump)
- **Robust error handling** with centralized messaging
- **Safety improvements**: Buffer overflow protections, bounds checking, VLA elimination
- **Cross-platform support**: Unix/Linux and Windows

## Features

### Supported Command Types

1. **External Commands**: Any executable in PATH
   ```bash
   $ ls -la /tmp
   $ grep "pattern" file.txt
   $ ./custom_script.sh
   ```

2. **Builtin Commands**:
   - `cd [dir]` - Change directory (defaults to HOME)
   - `pwd` - Print working directory
   - `export VAR=value` - Export environment variables
   - `unset VAR` - Unset variables
   - `echo [args...]` - Print arguments
   - `type cmd` - Show command type (builtin or external)
   - `dump` - Display symbol table contents

### Word Expansion

The shell supports comprehensive word expansion in this order:

1. **Quote Processing**:
   ```bash
   $ echo "hello world"      # Double quotes preserve spaces
   $ echo 'no expansion'     # Single quotes prevent expansion
   $ echo hello\ world       # Backslash escapes
   ```

2. **Variable Expansion**:
   ```bash
   $ export MY_VAR="value"
   $ echo $MY_VAR             # Simple expansion
   $ echo ${MY_VAR}           # Braced expansion
   $ echo $?                  # Last exit status
   ```

3. **Command Substitution**:
   ```bash
   $ echo $(date)             # Modern syntax
   $ echo `date`              # Legacy syntax
   ```

4. **Arithmetic Expansion**:
   ```bash
   $ result=$((2 + 3 * 4))
   $ echo $result             # Outputs: 14
   ```

5. **Pathname Globbing**:
   ```bash
   $ ls *.txt                 # Match .txt files
   $ echo [abc]*              # Match files starting with a, b, or c
   ```

6. **Field Splitting**:
   ```bash
   $ IFS=:
   $ echo $PATH               # Splits on colons
   ```

### History Navigation

On Unix/Linux, use arrow keys for command history:

```bash
$ ashell> ls -la
$ ashell> [UP ARROW]          # Show previous command
$ ashell> [DOWN ARROW]        # Show next command
```

History buffer holds up to 256 commands.

## Building

### Requirements

- GCC or compatible C compiler
- Make
- Unix/Linux or Windows system

### Compilation

```bash
cd part5
make              # Builds the shell executable
make clean        # Removes build artifacts
make distclean     # Removes build artifacts and shell executable
```

### Build Output

Executable: `./shell`

### Compiler Flags

- `-Wall -Wextra`: Enable all warnings
- `-g`: Include debug symbols
- Automatic platform detection (Windows vs Unix)

## Running the Shell

### Interactive Mode

```bash
$ ./shell
ashell> pwd
/home/user/ashell-mark-II/part5
ashell> echo $HOME
/home/user
ashell> cd /tmp
ashell> pwd
/tmp
ashell> exit
```

### Piping Commands (Limited)

```bash
$ echo "hello" | ./shell
ashell> # Now in shell with "hello" in stdin
```

Note: Pipes within commands are not yet supported. This is a limitation of the simple parser.

## Architecture

See [ARCHITECTURE.md](../ARCHITECTURE.md) for detailed architecture documentation.

### Key Components

- **main.c**: Main loop, command reading, history management
- **scanner.c**: Tokenization with quote and escape handling
- **parser.c**: Simple command parser
- **executor.c**: Command execution with PATH search
- **wordexp.c**: Complete word expansion pipeline
- **symtab/symtab.c**: Symbol table for variables
- **builtins/**: Builtin command implementations
- **constants.h**: Centralized configuration constants
- **error.h/error.c**: Unified error handling
- **utils.h/utils.c**: Safe utility functions

## Performance Characteristics

### Optimizations

- **Token buffer reuse**: Avoids repeated allocations
- **Fixed buffer for file paths**: Reduces malloc overhead in hot paths
- **Symbol table linear search**: Acceptable for educational scope (typically <100 vars)
- **Direct builtin dispatch**: O(n) linear search in builtin table (7 builtins max)

### Known Bottlenecks

1. **PATH search**: Linear scan, no caching (could optimize with cache)
2. **Symbol table**: Linear search (could use hash table)
3. **Glob expansion**: Uses glob() from libc (standard performance)

## Limitations

### Known Limitations

1. **No pipes or redirections**: Parser is intentionally simple
2. **No compound commands**: No if/while/for loops
3. **No job control**: No background jobs, no fg/bg
4. **Simple builtins only**: Limited to basic shell operations
5. **No command substitution in builtins**: Builtins receive literal args
6. **Single-threaded**: Not thread-safe (educational scope)

### POSIX Compliance

This shell implements a small subset of POSIX shell features. It is educational software, not a production shell.

## Error Handling

Errors are reported with context:

```bash
$ ashell> /nonexistent/command
ashell: error: command not found: /nonexistent/command

$ ashell> cd /no/such/dir
ashell: error: cannot change to directory '/no/such/dir': No such file or directory
```

All errors go to stderr, preserving stdout for command output.

## Testing

### Test Basic Operations

```bash
$ ./shell
ashell> pwd
ashell> cd /tmp
ashell> pwd
ashell> echo $HOME
ashell> export MY_VAR="test"
ashell> echo $MY_VAR
ashell> type ls
ashell> type echo
ashell> exit
```

### Test Word Expansion

```bash
$ ./shell
ashell> echo "hello world"
ashell> echo $HOME
ashell> echo $(pwd)
ashell> touch test.txt
ashell> echo *.txt
ashell> rm *.txt
```

## Configuration

All configuration constants are defined in `constants.h`:

- `MAX_INPUT_BUFFER`: Maximum command line size (4096 bytes)
- `MAX_HISTORY_SIZE`: History buffer size (256 commands)
- `INITIAL_TOK_BUFFER`: Token buffer initial size (1024 bytes, doubles as needed)
- `MAX_PATH_COMPONENT`: Maximum path length (1024 bytes)
- And more...

To change limits, edit `constants.h` and rebuild.

## Safety Features

### Buffer Overflow Protection

- Dynamic buffers with bounds checking
- String functions use safe length-limited versions (`shell_strlcat`, `shell_strlcpy`)
- Maximum path and token sizes enforced
- Removed VLAs (Variable Length Arrays) that could cause stack overflow

### Error Recovery

- Centralized error handling prevents cascading failures
- All memory allocations checked
- Proper cleanup on exit via `atexit()` handlers
- Command execution isolated in child processes (Unix)

## Future Enhancements

Potential improvements for future versions:

1. **Parser improvements**: Support pipes, redirections, compound commands
2. **Command caching**: Cache PATH lookups for faster execution
3. **Hash table**: Replace linear searches with hash tables
4. **Readline integration**: GNU readline for better line editing
5. **More builtins**: alias, set, typeset, readonly, etc.
6. **Job control**: Background jobs, fg/bg, jobs commands
7. **Scripting support**: Execute shell scripts from files
8. **Better diagnostics**: Line numbers, command tracing, debugging mode

## License

This project is licensed under the GNU General Public License v3.
See the individual source files for copyright information.

## Resources

- Original tutorial: "Let's Build a Linux Shell" by jetblackwing [amaljk80@gmail.com]
- Architecture documentation: [ARCHITECTURE.md](../ARCHITECTURE.md)
- POSIX shell specification: https://pubs.opengroup.org/onlinepubs/9699919799/utilities/sh.html

## Development Notes

### Code Quality

The refactored code emphasizes:

- **Safety**: Bounds checking, error handling, memory safety
- **Maintainability**: Clear structure, documented functions, consistent style
- **Readability**: Descriptive variable names, helpful comments
- **Portability**: Platform abstraction for Windows/Unix

### Contributing

To extend the shell:

1. **Add builtins**: Implement function in `builtins/`, add to `builtins[]` array
2. **Add error messages**: Use `SHELL_ERROR()` macro from `error.h`
3. **Add utilities**: Extend `utils.h/utils.c` for common patterns
4. **Update constants**: Use `constants.h` for all hardcoded limits

## Troubleshooting

### Build Issues

```bash
# Clean and rebuild if errors persist
$ make distclean
$ make

# Check for compiler warnings
$ make clean
$ make 2>&1 | grep warning
```

### Runtime Issues

```bash
# Command not found
ashell: error: command not found: foo

# Solution: Ensure command is in PATH
$ echo $PATH
$ which ls

# Variable not set
ashell: error: no such variable: MYVAR

# Solution: Use export to set variables
$ export MYVAR=value
```

## Contact & Support

For issues, questions, or suggestions related to this refactored version, refer to the architecture documentation and inline code comments.
