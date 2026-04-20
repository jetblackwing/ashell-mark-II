/* 
 *    A-Shell Project - Refactored
 *    file: constants.h
 *    
 *    Centralized configuration constants for the shell.
 *    This file consolidates all magic numbers and hardcoded limits
 *    for easier maintenance and configuration.
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

/* ==========================
   Input/Output Buffer Sizes
   ========================== */

/* Maximum size for a single command line input */
#define MAX_INPUT_BUFFER     4096

/* Initial size for scanner token buffer (doubles on overflow) */
#define INITIAL_TOK_BUFFER   1024

/* Maximum size for a single token */
#define MAX_TOKEN_SIZE       2048

/* Windows command line buffer (execv workaround on Windows) */
#define MAX_CMDLINE_BUFFER   8192

/* ==========================
   History & Session
   ========================== */

/* Maximum number of commands in history */
#define MAX_HISTORY_SIZE     256

/* ==========================
   Symbol Table & Functions
   ========================== */

/* Maximum nesting depth for symbol table scopes */
#define MAX_SYMTAB_STACK     256

/* ==========================
   Arithmetic & Operators
   ========================== */

/* Maximum operator stack depth for arithmetic expansion */
#define MAX_OPSTACK_DEPTH    64

/* ==========================
   Pathname & Path Lookup
   ========================== */

/* Maximum path component length */
#define MAX_PATH_COMPONENT   1024

/* PATH separator (: on Unix, ; on Windows) */
#ifdef _WIN32
    #define PATH_SEPARATOR   ';'
#else
    #define PATH_SEPARATOR   ':'
#endif

/* ==========================
   Error Codes & Exit Status
   ========================== */

/* Standard exit code for command not found */
#define SHELL_EX_NOTFOUND    127

/* Standard exit code for not executable */
#define SHELL_EX_NOEXEC      126

/* ==========================
   Feature Flags
   ========================== */

/* Enable debugging output (0 = off, 1 = on) */
#define DEBUG_MODE           0

/* Enable memory leak detection (requires Valgrind/ASAN) */
#define DEBUG_MEMORY         0

#endif /* CONSTANTS_H */
