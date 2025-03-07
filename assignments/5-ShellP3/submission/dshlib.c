#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>    /* Added for errno */
#include <signal.h>   /* Added for kill() */

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
    
    // Reset redirection fields (for extra credit)
    cmd_buff->input_file = NULL;
    cmd_buff->output_file = NULL;
    cmd_buff->append_mode = false;
    
    return OK;
}

/*
 * Builds a command buffer from a command line string with redirection support
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
    
    // Initialize argc and redirection fields
    cmd_buff->argc = 0;
    cmd_buff->input_file = NULL;
    cmd_buff->output_file = NULL;
    cmd_buff->append_mode = false;
    
    // Parse command and arguments
    bool in_quotes = false;
    char *p = cmd_buff->_cmd_buffer;
    char *token_start = p;
    
    while (*p) {
        // Handle redirection operators
        if (!in_quotes && (*p == '<' || *p == '>')) {
            // End current token if any
            if (p > token_start) {
                *p = '\0';
                if (cmd_buff->argc < CMD_ARGV_MAX) {
                    cmd_buff->argv[cmd_buff->argc++] = token_start;
                }
            }
            
            // Mark redirection type
            bool is_input = (*p == '<');
            bool is_append = (p[0] == '>' && p[1] == '>');
            
            // Skip the operator(s)
            if (is_append) p += 2;
            else p++;
            
            // Skip whitespace after operator
            while (*p && isspace(*p)) p++;
            
            // Find the file name
            token_start = p;
            
            // Find end of file name (next space or redirection operator)
            while (*p && !isspace(*p) && *p != '<' && *p != '>') {
                if (*p == '"') {
                    // Skip quoted string
                    p++;
                    while (*p && *p != '"') p++;
                    if (*p) p++; // Skip closing quote
                } else {
                    p++;
                }
            }
            
            // Terminate file name
            if (*p) {
                char saved = *p;
                *p = '\0';
                
                // Store file name in appropriate field
                if (is_input) {
                    cmd_buff->input_file = token_start;
                } else {
                    cmd_buff->output_file = token_start;
                    cmd_buff->append_mode = is_append;
                }
                
                *p = saved; // Restore character
            } else {
                // End of string
                if (is_input) {
                    cmd_buff->input_file = token_start;
                } else {
                    cmd_buff->output_file = token_start;
                    cmd_buff->append_mode = is_append;
                }
                break;
            }
            
            // Prepare for next token
            token_start = p;
            continue;
        }
        
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
    
    // Add the last argument if there is one and it's not a redirection file
    if (p > token_start && 
        token_start != cmd_buff->input_file && 
        token_start != cmd_buff->output_file && 
        cmd_buff->argc < CMD_ARGV_MAX) {
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
    
    // Process quoted strings in redirection file names
    if (cmd_buff->input_file && cmd_buff->input_file[0] == '"') {
        // Remove starting quote
        memmove(cmd_buff->input_file, cmd_buff->input_file + 1, strlen(cmd_buff->input_file));
        
        // Find and remove ending quote if it exists
        size_t len = strlen(cmd_buff->input_file);
        if (len > 0 && cmd_buff->input_file[len - 1] == '"') {
            cmd_buff->input_file[len - 1] = '\0';
        }
    }
    
    if (cmd_buff->output_file && cmd_buff->output_file[0] == '"') {
        // Remove starting quote
        memmove(cmd_buff->output_file, cmd_buff->output_file + 1, strlen(cmd_buff->output_file));
        
        // Find and remove ending quote if it exists
        size_t len = strlen(cmd_buff->output_file);
        if (len > 0 && cmd_buff->output_file[len - 1] == '"') {
            cmd_buff->output_file[len - 1] = '\0';
        }
    }
    
    // Ensure null termination of argv
    cmd_buff->argv[cmd_buff->argc] = NULL;
    
    return OK;
}

/*
 * Builds a command list from a command line containing pipes
 */
int build_cmd_list(char *cmd_line, command_list_t *clist) {
    if (cmd_line == NULL || clist == NULL) return ERR_MEMORY;
    
    // Initialize command list
    clist->num = 0;
    
    // Make a copy of the command line to work with
    char *cmd_line_copy = strdup(cmd_line);
    if (cmd_line_copy == NULL) {
        return ERR_MEMORY;
    }
    
    // Tokenize the command line by '|' while respecting quotes
    char *saveptr = cmd_line_copy;
    bool in_quotes = false;
    char *token_start = saveptr;
    char *p = saveptr;
    
    while (*p) {
        if (*p == '"') {
            in_quotes = !in_quotes;
        } else if (*p == '|' && !in_quotes) {
            // Found pipe operator outside of quotes
            *p = '\0';
            
            // Allocate memory for command buffer
            if (alloc_cmd_buff(&clist->commands[clist->num]) != OK) {
                // Clean up already allocated commands
                for (int i = 0; i < clist->num; i++) {
                    free_cmd_buff(&clist->commands[i]);
                }
                free(cmd_line_copy);
                return ERR_MEMORY;
            }
            
            // Process this command segment
            int result = build_cmd_buff(token_start, &clist->commands[clist->num]);
            if (result != OK) {
                if (result == WARN_NO_CMDS) {
                    // Skip empty commands
                    free_cmd_buff(&clist->commands[clist->num]);
                    token_start = p + 1;
                    p++;
                    continue;
                }
                
                // Clean up on error
                for (int i = 0; i < clist->num; i++) {
                    free_cmd_buff(&clist->commands[i]);
                }
                free_cmd_buff(&clist->commands[clist->num]);
                free(cmd_line_copy);
                return result;
            }
            
            // Move to next command
            clist->num++;
            if (clist->num >= CMD_MAX) {
                // Too many commands
                for (int i = 0; i < clist->num; i++) {
                    free_cmd_buff(&clist->commands[i]);
                }
                free(cmd_line_copy);
                return ERR_TOO_MANY_COMMANDS;
            }
            
            token_start = p + 1;
        }
        p++;
    }
    
    // Process the last command segment if there is one
    if (*token_start != '\0') {
        // Allocate memory for command buffer
        if (alloc_cmd_buff(&clist->commands[clist->num]) != OK) {
            // Clean up already allocated commands
            for (int i = 0; i < clist->num; i++) {
                free_cmd_buff(&clist->commands[i]);
            }
            free(cmd_line_copy);
            return ERR_MEMORY;
        }
        
        // Process this command segment
        int result = build_cmd_buff(token_start, &clist->commands[clist->num]);
        if (result != OK) {
            if (result != WARN_NO_CMDS) {
                // Clean up on error
                for (int i = 0; i < clist->num; i++) {
                    free_cmd_buff(&clist->commands[i]);
                }
                free_cmd_buff(&clist->commands[clist->num]);
                free(cmd_line_copy);
                return result;
            }
            // Skip last command if empty
            free_cmd_buff(&clist->commands[clist->num]);
        } else {
            // Valid last command
            clist->num++;
        }
    }
    
    free(cmd_line_copy);
    
    // Check if we found any valid commands
    if (clist->num == 0) {
        return WARN_NO_CMDS;
    }
    
    return OK;
}

/*
 * Frees all memory used by a command list
 */
int free_cmd_list(command_list_t *cmd_lst) {
    if (cmd_lst == NULL) return ERR_MEMORY;
    
    for (int i = 0; i < cmd_lst->num; i++) {
        free_cmd_buff(&cmd_lst->commands[i]);
    }
    
    return OK;
}

/*
 * Matches command to check if it's a built-in command
 */
Built_In_Cmds match_command(const char *input) {
    if (input == NULL) return BI_NOT_BI;
    
    if (strcmp(input, EXIT_CMD) == 0) {
        return BI_CMD_EXIT;
    } else if (strcmp(input, DRAGON_CMD) == 0) {
        return BI_CMD_DRAGON;
    } else if (strcmp(input, "cd") == 0) {
        return BI_CMD_CD;
    } else if (strcmp(input, "rc") == 0) {
        return BI_RC;
    }
    
    return BI_NOT_BI;
}

/*
 * Executes a built-in command
 */
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    if (cmd == NULL || cmd->argc == 0) return BI_NOT_BI;
    
    Built_In_Cmds cmd_type = match_command(cmd->argv[0]);
    
    switch (cmd_type) {
        case BI_CMD_EXIT:
            printf("exiting...\n");
            return BI_CMD_EXIT;
            
        case BI_CMD_DRAGON:
            print_dragon();
            last_return_code = 0;
            return BI_EXECUTED;
            
        case BI_CMD_CD:
            if (cmd->argc > 1) {
                if (chdir(cmd->argv[1]) != 0) {
                    perror("cd");
                    last_return_code = errno;
                } else {
                    last_return_code = 0;
                }
            } else {
                // CD to home directory if no argument
                const char *home = getenv("HOME");
                if (home && chdir(home) != 0) {
                    perror("cd");
                    last_return_code = errno;
                } else {
                    last_return_code = 0;
                }
            }
            return BI_EXECUTED;
            
        case BI_RC:
            printf("%d\n", last_return_code);
            return BI_EXECUTED;
            
        case BI_NOT_BI:
        default:
            return BI_NOT_BI;
    }
}

/*
 * Sets up file redirection for a command
 * Returns 0 on success, -1 on error
 */
int setup_redirection(cmd_buff_t *cmd) {
    // Handle input redirection
    if (cmd->input_file != NULL) {
        int fd = open(cmd->input_file, O_RDONLY);
        if (fd < 0) {
            perror(cmd->input_file);
            return -1;
        }
        
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2");
            close(fd);
            return -1;
        }
        
        close(fd);
    }
    
    // Handle output redirection
    if (cmd->output_file != NULL) {
        int flags = O_WRONLY | O_CREAT;
        if (cmd->append_mode) {
            flags |= O_APPEND;
        } else {
            flags |= O_TRUNC;
        }
        
        int fd = open(cmd->output_file, flags, 0644);
        if (fd < 0) {
            perror(cmd->output_file);
            return -1;
        }
        
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2");
            close(fd);
            return -1;
        }
        
        close(fd);
    }
    
    return 0;
}

/*
 * Executes a pipeline of commands
 */
int execute_pipeline(command_list_t *clist) {
    if (clist == NULL || clist->num == 0) return WARN_NO_CMDS;
    
    // For single command, no need for pipes
    if (clist->num == 1) {
        // Check if it's a built-in command
        Built_In_Cmds cmd_type = exec_built_in_cmd(&clist->commands[0]);
        
        if (cmd_type == BI_CMD_EXIT) {
            return OK_EXIT;
        } else if (cmd_type == BI_EXECUTED) {
            return OK;
        }
        
        // Handle file redirection for built-in commands
        if (clist->commands[0].input_file != NULL || clist->commands[0].output_file != NULL) {
            // Save original stdin/stdout
            int saved_stdin = dup(STDIN_FILENO);
            int saved_stdout = dup(STDOUT_FILENO);
            
            if (saved_stdin < 0 || saved_stdout < 0) {
                perror("dup");
                return ERR_EXEC_CMD;
            }
            
            // Set up redirection
            if (setup_redirection(&clist->commands[0]) < 0) {
                // Restore original stdin/stdout
                dup2(saved_stdin, STDIN_FILENO);
                dup2(saved_stdout, STDOUT_FILENO);
                close(saved_stdin);
                close(saved_stdout);
                return ERR_EXEC_CMD;
            }
            
            // Execute built-in command again with redirection
            cmd_type = exec_built_in_cmd(&clist->commands[0]);
            
            // Restore original stdin/stdout
            dup2(saved_stdin, STDIN_FILENO);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdin);
            close(saved_stdout);
            
            if (cmd_type == BI_CMD_EXIT) {
                return OK_EXIT;
            } else if (cmd_type == BI_EXECUTED) {
                return OK;
            }
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
            
            // Set up redirection
            if (setup_redirection(&clist->commands[0]) < 0) {
                exit(EXIT_FAILURE);
            }
            
            // Execute command
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
            exit(errno);
        } else {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
            
            if (WIFEXITED(status)) {
                last_return_code = WEXITSTATUS(status);
            } else {
                last_return_code = -1;
            }
            
            return OK;
        }
    }
    
    // Multiple commands - need pipes
    int pipe_fds[CMD_MAX - 1][2];
    pid_t child_pids[CMD_MAX];
    
    // Create all pipes
    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipe_fds[i]) < 0) {
            perror("pipe");
            
            // Close any already created pipes
            for (int j = 0; j < i; j++) {
                close(pipe_fds[j][0]);
                close(pipe_fds[j][1]);
            }
            
            last_return_code = errno;
            return ERR_EXEC_CMD;
        }
    }
    
    // Create processes and set up pipes
    for (int i = 0; i < clist->num; i++) {
        // Check if the first command is a built-in
        if (i == 0) {
            Built_In_Cmds cmd_type = match_command(clist->commands[i].argv[0]);
            if (cmd_type != BI_NOT_BI) {
                // Built-in commands don't support piping in this implementation
                fprintf(stderr, "Built-in commands don't support piping\n");
                
                // Close all pipes
                for (int j = 0; j < clist->num - 1; j++) {
                    close(pipe_fds[j][0]);
                    close(pipe_fds[j][1]);
                }
                
                last_return_code = 1;
                return ERR_EXEC_CMD;
            }
        }
        
        // Fork child process
        child_pids[i] = fork();
        
        if (child_pids[i] < 0) {
            // Fork failed
            perror("fork");
            
            // Close all pipes
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipe_fds[j][0]);
                close(pipe_fds[j][1]);
            }
            
            // Kill any already created children
            for (int j = 0; j < i; j++) {
                kill(child_pids[j], SIGTERM);
                waitpid(child_pids[j], NULL, 0);
            }
            
            last_return_code = errno;
            return ERR_EXEC_CMD;
        } else if (child_pids[i] == 0) {
            // Child process
            
            // Set up stdin from previous pipe (if not first command)
            if (i > 0) {
                if (dup2(pipe_fds[i - 1][0], STDIN_FILENO) < 0) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }
            
            // Set up stdout to next pipe (if not last command)
            if (i < clist->num - 1) {
                if (dup2(pipe_fds[i][1], STDOUT_FILENO) < 0) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }
            
            // Close all pipe file descriptors in child
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipe_fds[j][0]);
                close(pipe_fds[j][1]);
            }
            
            // Set up redirection (this will only affect the first and last commands in the pipeline)
            if (setup_redirection(&clist->commands[i]) < 0) {
                exit(EXIT_FAILURE);
            }
            
            // Execute command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            
            // If execvp returns, there was an error
            perror(clist->commands[i].argv[0]);
            exit(errno);
        }
    }
    
    // Parent process - close all pipe file descriptors
    for (int i = 0; i < clist->num - 1; i++) {
        close(pipe_fds[i][0]);
        close(pipe_fds[i][1]);
    }
    
    // Wait for all child processes
    int last_status = 0;
    for (int i = 0; i < clist->num; i++) {
        int status;
        waitpid(child_pids[i], &status, 0);
        
        // Save the exit status of the last command
        if (i == clist->num - 1) {
            if (WIFEXITED(status)) {
                last_status = WEXITSTATUS(status);
            } else {
                last_status = -1;
            }
        }
    }
    
    last_return_code = last_status;
    return OK;
}

/*
 * Main command execution loop
 */
int exec_local_cmd_loop() {
    char cmd_buff[SH_CMD_MAX];
    command_list_t cmd_list;
    int result;
    
    while (1) {
        // Prompt user for input
        printf("%s", SH_PROMPT);
        
        // Read input
        if (fgets(cmd_buff, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }
        
        // Remove trailing newline
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';
        
        // Check for empty command
        if (strlen(cmd_buff) == 0) {
            continue;
        }
        
        // Check for simple exit command
        if (strcmp(cmd_buff, EXIT_CMD) == 0) {
            printf("exiting...\n");
            break;
        }
        
        // Build command list
        memset(&cmd_list, 0, sizeof(command_list_t));
        result = build_cmd_list(cmd_buff, &cmd_list);
        
        // Handle parsing results
        if (result == WARN_NO_CMDS) {
            printf("%s", CMD_WARN_NO_CMD);
            continue;
        } else if (result == ERR_TOO_MANY_COMMANDS) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            continue;
        } else if (result != OK) {
            printf("Error parsing command: %d\n", result);
            continue;
        }
        
        // Execute pipeline
        result = execute_pipeline(&cmd_list);
        
        if (result == OK_EXIT) {
            printf("exiting...\n");
            free_cmd_list(&cmd_list);
            break;
        } else if (result != OK && result != WARN_NO_CMDS) {
            printf("Pipeline execution error: %d\n", result);
        }
        
        // Free command list
        free_cmd_list(&cmd_list);
    }
    
    return OK;
}