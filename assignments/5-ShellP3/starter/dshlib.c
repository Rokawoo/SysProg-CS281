#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "dshlib.h"

/*
 * Implement your exec_local_cmd_loop function by building a loop that prompts the 
 * user for input.  Use the SH_PROMPT constant from dshlib.h and then
 * use fgets to accept user input.
 * 
 *      while(1){
 *        printf("%s", SH_PROMPT);
 *        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL){
 *           printf("\n");
 *           break;
 *        }
 *        //remove the trailing \n from cmd_buff
 *        cmd_buff[strcspn(cmd_buff,"\n")] = '\0';
 * 
 *        //IMPLEMENT THE REST OF THE REQUIREMENTS
 *      }
 * 
 *   Also, use the constants in the dshlib.h in this code.  
 *      SH_CMD_MAX              maximum buffer size for user input
 *      EXIT_CMD                constant that terminates the dsh program
 *      SH_PROMPT               the shell prompt
 *      OK                      the command was parsed properly
 *      WARN_NO_CMDS            the user command was empty
 *      ERR_TOO_MANY_COMMANDS   too many pipes used
 *      ERR_MEMORY              dynamic memory management failure
 * 
 *   errors returned
 *      OK                     No error
 *      ERR_MEMORY             Dynamic memory management failure
 *      WARN_NO_CMDS           No commands parsed
 *      ERR_TOO_MANY_COMMANDS  too many pipes used
 *   
 *   console messages
 *      CMD_WARN_NO_CMD        print on WARN_NO_CMDS
 *      CMD_ERR_PIPE_LIMIT     print on ERR_TOO_MANY_COMMANDS
 *      CMD_ERR_EXECUTE        print on execution failure of external command
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 1+)
 *      malloc(), free(), strlen(), fgets(), strcspn(), printf()
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 2+)
 *      fork(), execvp(), exit(), chdir()
 */
// Track last command's return code
int last_return_code = 0;

/*
 * Allocates memory for a command buffer and initializes it
 */
int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff == NULL) return ERR_MEMORY;
    
    cmd_buff->_cmd_buffer = (char *)malloc(SH_CMD_MAX);
    if (cmd_buff->_cmd_buffer == NULL) return ERR_MEMORY;
    
    clear_cmd_buff(cmd_buff);
    return OK;
}

/*
 * Frees memory used by a command buffer
 */
int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff == NULL) return ERR_MEMORY;
    
    if (cmd_buff->_cmd_buffer != NULL) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    
    return OK;
}

/*
 * Resets the command buffer to its initial state
 */
int clear_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff == NULL) return ERR_MEMORY;
    
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    
    if (cmd_buff->_cmd_buffer != NULL) {
        cmd_buff->_cmd_buffer[0] = '\0';
    }
    
    return OK;
}

/*
 * Builds a command buffer from a command line string
 */
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    if (cmd_line == NULL || cmd_buff == NULL) return ERR_MEMORY;
    
    // Skip leading whitespace
    while (*cmd_line && isspace(*cmd_line)) cmd_line++;
    
    // Check if there's any command
    if (*cmd_line == '\0') {
        return WARN_NO_CMDS;
    }
    
    // Copy command line to buffer
    cmd_buff->_cmd_buffer = strdup(cmd_line);
    if (cmd_buff->_cmd_buffer == NULL) {
        return ERR_MEMORY;
    }
    
    // Initialize argc
    cmd_buff->argc = 0;
    
    // Parse command and arguments
    bool in_quotes = false;
    char *p = cmd_buff->_cmd_buffer;
    char *token_start = p;
    
    while (*p) {
        if (*p == '"') {
            // Toggle quote state
            in_quotes = !in_quotes;
            p++;
            continue;
        }
        
        if (isspace(*p) && !in_quotes) {
            // End of token
            if (p > token_start) {
                *p = '\0';
                if (cmd_buff->argc < CMD_ARGV_MAX) {
                    cmd_buff->argv[cmd_buff->argc++] = token_start;
                }
            }
            
            // Skip multiple spaces
            while (*(p+1) && isspace(*(p+1)) && !in_quotes) p++;
            
            token_start = p + 1;
        }
        p++;
    }
    
    // Add the last argument if there is one
    if (p > token_start && cmd_buff->argc < CMD_ARGV_MAX) {
        cmd_buff->argv[cmd_buff->argc++] = token_start;
    }
    
    // Process quoted strings inside arguments
    for (int i = 0; i < cmd_buff->argc; i++) {
        char *arg = cmd_buff->argv[i];
        // If argument starts with a quote
        if (arg[0] == '"') {
            // Remove starting quote
            memmove(arg, arg + 1, strlen(arg));
            
            // Find and remove ending quote if it exists
            size_t len = strlen(arg);
            if (len > 0 && arg[len - 1] == '"') {
                arg[len - 1] = '\0';
            }
        }
    }
    
        // Handle our cd command
        if (strcmp(cmd.argv[0], "cd") == 0) {
            if (cmd.argc > 1) {
                if (chdir(cmd.argv[1]) != 0) {
                    perror("cd");
                    last_return_code = errno;
                } else {
                    last_return_code = 0;
                }
            }
            free(cmd._cmd_buffer);
            continue;
        }
        
        // Execute the command using fork/exec
        pid_t pid = fork();
        
        if (pid < 0) {
            // Fork failed
            perror("fork");
            last_return_code = errno;
            return ERR_EXEC_CMD;
        } else if (pid == 0) {
            // Child process
            execvp(clist->commands[0].argv[0], clist->commands[0].argv);
            
            // If we get here, execvp failed
            switch (errno) {
                case ENOENT:
                    printf("Command not found in PATH\n");
                    break;
                case EACCES:
                    printf("Permission denied\n");
                    break;
                case ENOMEM:
                    printf("Out of memory\n");
                    break;
                case E2BIG:
                    printf("Argument list too long\n");
                    break;
                default:
                    printf("%s\n", CMD_ERR_EXECUTE);
            }
            exit(errno); // Return errno as status code
        } else {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
            
            if (WIFEXITED(status)) {
                last_return_code = WEXITSTATUS(status);
            } else {
                last_return_code = -1;
            }
        }
        
        // Free command list
        free_cmd_list(&cmd_list);
    }
    
    return OK;
}