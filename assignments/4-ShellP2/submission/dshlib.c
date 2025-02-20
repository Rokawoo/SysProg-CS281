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
int exec_local_cmd_loop() {
    char cmd_buff[SH_CMD_MAX];
    cmd_buff_t cmd;
    int rc = 0;

    while(1) {
        printf("%s", SH_PROMPT);
        if (fgets(cmd_buff, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }
        //remove the trailing \n from cmd_buff
        cmd_buff[strcspn(cmd_buff,"\n")] = '\0';

        // Skip empty commands
        if (strlen(cmd_buff) == 0) {
            printf("%s\n", CMD_WARN_NO_CMD);
            continue;
        }

        // Handle exit command
        if (strncmp(cmd_buff, EXIT_CMD, strlen(EXIT_CMD)) == 0) {
            break;
        }

        // Handle dragon command (extra credit from previous assignment)
        if (strncmp(cmd_buff, DRAGON_CMD, strlen(DRAGON_CMD)) == 0) {
            print_dragon();
            continue;
        }

        // Initialize cmd structure
        memset(&cmd, 0, sizeof(cmd_buff_t));
        cmd._cmd_buffer = strdup(cmd_buff);
        if (!cmd._cmd_buffer) {
            return ERR_MEMORY;
        }

        // Parse command and arguments
        char *input = cmd._cmd_buffer;
        int arg_idx = 0;
        bool in_quotes = false;
        char *start = input;
        
        // Skip leading whitespace
        while (*start && isspace(*start)) start++;
        
        char *p = start;
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
                    if (arg_idx < CMD_ARGV_MAX) {
                        cmd.argv[arg_idx++] = token_start;
                    }
                }
                
                // Skip multiple spaces
                while (*(p+1) && isspace(*(p+1)) && !in_quotes) p++;
                
                token_start = p + 1;
            }
            p++;
        }
        
        // Add the last argument if there is one
        if (p > token_start && arg_idx < CMD_ARGV_MAX) {
            cmd.argv[arg_idx++] = token_start;
        }
        
        cmd.argc = arg_idx;
        
        // Handle empty command after parsing
        if (cmd.argc == 0) {
            free(cmd._cmd_buffer);
            printf("%s\n", CMD_WARN_NO_CMD);
            continue;
        }

        // Process quoted strings within arguments
        for (int i = 0; i < cmd.argc; i++) {
            char *arg = cmd.argv[i];
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

        // Handle built-in cd command
        if (strcmp(cmd.argv[0], "cd") == 0) {
            if (cmd.argc > 1) {
                if (chdir(cmd.argv[1]) != 0) {
                    perror("cd");
                }
            }
            free(cmd._cmd_buffer);
            continue;
        }

        // Execute external command using fork/exec
        pid_t pid = fork();
        
        if (pid < 0) {
            // Fork failed
            perror("fork");
            free(cmd._cmd_buffer);
            continue;
        } else if (pid == 0) {
            // Child process
            execvp(cmd.argv[0], cmd.argv);
            // If execvp returns, it failed
            printf("%s\n", ERR_EXEC_CMD);
            exit(1);
        } else {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
            // Could extract exit status with WEXITSTATUS(status) for extra credit
        }

        free(cmd._cmd_buffer);
    }

    return OK;
}

    /*
    */
    // TODO IMPLEMENT MAIN LOOP

    // TODO IMPLEMENT parsing input to cmd_buff_t *cmd_buff

    // TODO IMPLEMENT if built-in command, execute builtin logic for exit, cd (extra credit: dragon)
    // the cd command should chdir to the provided directory; if no directory is provided, do nothing

    // TODO IMPLEMENT if not built-in command, fork/exec as an external command
    // for example, if the user input is "ls -l", you would fork/exec the command "ls" with the arg "-l"

    //return OK;

