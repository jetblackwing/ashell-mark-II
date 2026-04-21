/* 
 *    A-Shell Project
 *    file: builtins/shell_builtins.h
 *    
 *    Declarations for additional shell builtin commands
 */

#ifndef SHELL_BUILTINS_H
#define SHELL_BUILTINS_H

/* Additional builtin command handlers */
int shell_cd(int argc, char **argv);
int shell_pwd(int argc, char **argv);
int shell_export(int argc, char **argv);
int shell_unset(int argc, char **argv);
int shell_echo(int argc, char **argv);
int shell_type(int argc, char **argv);
int shell_saveconfig(int argc, char **argv);

#endif /* SHELL_BUILTINS_H */
