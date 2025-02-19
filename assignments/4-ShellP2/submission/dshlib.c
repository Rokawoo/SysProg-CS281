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
int exec_local_cmd_loop()
{
    char *cmd_buff;
    int rc = 0;
    cmd_buff_t cmd;

    
    while (1) {
        printf("%s", SH_PROMPT);
        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        // Handle exit command
        if (strcmp(cmd_buff, EXIT_CMD) == 0) {
            break;
        }

        // Handle dragon command
        if (strcmp(cmd_buff, DRAGON_CMD) == 0) { 
            print_dragon();
            continue;
        }

        // Parse and process command
        rc = build_cmd_list(cmd_buff, &clist);
        
        // Handle results
        switch (rc) {
            case OK:
                printf(CMD_OK_HEADER, clist.num);
                for (int i = 0; i < clist.num; i++) {
                    if (strlen(clist.commands[i].args) > 0) {
                        printf("<%d> %s [%s]\n", i+1, 
                               clist.commands[i].exe, 
                               clist.commands[i].args);
                    } else {
                        printf("<%d> %s\n", i+1, clist.commands[i].exe);
                    }
                }
                break;
            case WARN_NO_CMDS:
                printf(CMD_WARN_NO_CMD);
                break;
            case ERR_TOO_MANY_COMMANDS:
                printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
                break;
        }
    }
    
    return 0;
}

    // TODO IMPLEMENT MAIN LOOP

    // TODO IMPLEMENT parsing input to cmd_buff_t *cmd_buff

    // TODO IMPLEMENT if built-in command, execute builtin logic for exit, cd (extra credit: dragon)
    // the cd command should chdir to the provided directory; if no directory is provided, do nothing

    // TODO IMPLEMENT if not built-in command, fork/exec as an external command
    // for example, if the user input is "ls -l", you would fork/exec the command "ls" with the arg "-l"

    return OK;
}





/*
int build_cmd_list(char *cmd_line, command_list_t *clist) {
    if (!cmd_line || !clist) {
        return ERR_CMD_OR_ARGS_TOO_BIG;
    }

    // Initialize command list
    memset(clist, 0, sizeof(command_list_t));
    
    // Split commands by pipe character
    char *saveptr1;
    char *cmd = strtok_r(cmd_line, "|", &saveptr1);
    
    while (cmd) {
        // Check command limit
        if (clist->num >= CMD_MAX) {
            return ERR_TOO_MANY_COMMANDS;
        }

        // Skip leading whitespace
        while (isspace(*cmd)) cmd++;
        
        // Parse command and arguments
        char *saveptr2;
        char *token = strtok_r(cmd, " \t\n", &saveptr2);
        if (!token) continue;
        
        // Validate command length
        if (strlen(token) >= EXE_MAX) {
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }
        
        // Store command
        strcpy(clist->commands[clist->num].exe, token);
        
        // Process arguments
        char args[ARG_MAX] = "";
        token = strtok_r(NULL, " \t\n", &saveptr2);
        
        while (token) {
            size_t curr_len = strlen(args);
            size_t token_len = strlen(token);
            
            // Validate argument length
            if (curr_len + token_len + 2 >= ARG_MAX) {
                return ERR_CMD_OR_ARGS_TOO_BIG;
            }
            
            // Add space between arguments
            if (curr_len > 0) {
                strcat(args, " ");
            }
            strcat(args, token);
            token = strtok_r(NULL, " \t\n", &saveptr2);
        }
        
        // Store arguments and increment command count
        strcpy(clist->commands[clist->num].args, args);
        clist->num++;
        cmd = strtok_r(NULL, "|", &saveptr1);
    }
    
    return clist->num > 0 ? OK : WARN_NO_CMDS;
}
*/