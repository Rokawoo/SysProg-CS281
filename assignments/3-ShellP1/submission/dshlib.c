#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "dshlib.h"

/*
 *  build_cmd_list
 *    cmd_line:     the command line from the user
 *    clist *:      pointer to clist structure to be populated
 *
 *  This function builds the command_list_t structure passed by the caller
 *  It does this by first splitting the cmd_line into commands by spltting
 *  the string based on any pipe characters '|'.  It then traverses each
 *  command.  For each command (a substring of cmd_line), it then parses
 *  that command by taking the first token as the executable name, and
 *  then the remaining tokens as the arguments.
 *
 *  NOTE your implementation should be able to handle properly removing
 *  leading and trailing spaces!
 *
 *  errors returned:
 *
 *    OK:                      No Error
 *    ERR_TOO_MANY_COMMANDS:   There is a limit of CMD_MAX (see dshlib.h)
 *                             commands.
 *    ERR_CMD_OR_ARGS_TOO_BIG: One of the commands provided by the user
 *                             was larger than allowed, either the
 *                             executable name, or the arg string.
 *
 *  Standard Library Functions You Might Want To Consider Using
 *      memset(), strcmp(), strcpy(), strtok(), strlen(), strchr()
 */
int build_cmd_list(char *cmd_line, command_list_t *clist) {
    if (!cmd_line || !clist) {
        return ERR_CMD_OR_ARGS_TOO_BIG;
    }

    memset(clist, 0, sizeof(command_list_t));
    
    char *saveptr1;
    char *cmd = strtok_r(cmd_line, "|", &saveptr1);
    
    while (cmd) {
        if (clist->num_commands >= CMD_MAX) {
            return ERR_TOO_MANY_COMMANDS;
        }

        // Skip leading whitespace
        while (isspace(*cmd)) cmd++;
        
        char *saveptr2;
        char *token = strtok_r(cmd, " \t\n", &saveptr2);
        if (!token) continue;
        
        // Check executable name length
        if (strlen(token) >= MAX_LINE) {
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }
        
        command_t *current = &clist->commands[clist->num_commands];
        strcpy(current->exec_name, token);
        
        // Build arguments string
        char args[MAX_LINE] = "";
        token = strtok_r(NULL, " \t\n", &saveptr2);
        
        while (token) {
            size_t curr_len = strlen(args);
            size_t token_len = strlen(token);
            
            // Check if adding token would exceed MAX_LINE
            if (curr_len + token_len + 2 >= MAX_LINE) {
                return ERR_CMD_OR_ARGS_TOO_BIG;
            }
            
            if (curr_len > 0) {
                strcat(args, " ");
            }
            strcat(args, token);
            token = strtok_r(NULL, " \t\n", &saveptr2);
        }
        
        strcpy(current->args, args);
        clist->num_commands++;
        cmd = strtok_r(NULL, "|", &saveptr1);
    }
    
    return OK;
}