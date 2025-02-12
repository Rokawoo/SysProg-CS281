#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "dshlib.h"

// Dragon print data
static const char CHARS[] = {' ', '%', '@', '\n'};
const dragon_run_t DRAGON_DATA[] = {
    {56<<2|0}, {4<<2|2}, {1<<2|1}, {1<<2|3},
    {53<<2|0}, {6<<2|1}, {1<<2|3},
    {52<<2|0}, {6<<2|1}, {1<<2|3},
    {49<<2|0}, {1<<2|1}, {1<<2|0}, {7<<2|1}, {11<<2|0}, {1<<2|2}, {1<<2|3},
    {48<<2|0}, {10<<2|1}, {8<<2|0}, {7<<2|1}, {1<<2|3},
    {39<<2|0}, {7<<2|1}, {1<<2|0}, {4<<2|1}, {1<<2|2}, {9<<2|0}, {10<<2|1}, {4<<2|2}, {1<<2|1}, {1<<2|3},
    {34<<2|0}, {24<<2|1}, {6<<2|0}, {16<<2|1}, {1<<2|3},
    {32<<2|0}, {26<<2|1}, {3<<2|0}, {15<<2|1}, {1<<2|3},
    {31<<2|0}, {27<<2|1}, {1<<2|0}, {19<<2|1}, {5<<2|0}, {3<<2|1}, {1<<2|3},
    {29<<2|0}, {27<<2|1}, {1<<2|2}, {1<<2|0}, {1<<2|2}, {18<<2|1}, {8<<2|0}, {2<<2|1}, {1<<2|3},
    {28<<2|0}, {29<<2|1}, {1<<2|0}, {20<<2|1}, {1<<2|3},
    {28<<2|0}, {36<<2|1}, {1<<2|3},
    {28<<2|0}, {35<<2|1}, {1<<2|2}, {6<<2|1}, {1<<2|2}, {1<<2|3},
    {6<<2|0}, {8<<2|1}, {1<<2|2}, {11<<2|0}, {12<<2|1}, {8<<2|0}, {20<<2|1}, {6<<2|0}, {2<<2|1}, {1<<2|3},
    {4<<2|0}, {11<<2|1}, {9<<2|0}, {11<<2|1}, {11<<2|0}, {11<<2|1}, {6<<2|0}, {2<<2|1}, {1<<2|0}, {1<<2|2}, {1<<2|1}, {1<<2|3},
    {2<<2|0}, {10<<2|1}, {3<<2|0}, {8<<2|1}, {1<<2|0}, {10<<2|1}, {12<<2|0}, {20<<2|1}, {1<<2|3},
    {1<<2|0}, {9<<2|1}, {7<<2|0}, {1<<2|1}, {9<<2|0}, {11<<2|1}, {13<<2|0}, {1<<2|1}, {1<<2|2}, {9<<2|1}, {1<<2|3},
    {9<<2|1}, {1<<2|2}, {15<<2|0}, {11<<2|1}, {12<<2|0}, {1<<2|2}, {17<<2|1}, {1<<2|3},
    {8<<2|1}, {1<<2|2}, {17<<2|0}, {10<<2|1}, {12<<2|0}, {1<<2|2}, {20<<2|1}, {1<<2|3},
    {7<<2|1}, {1<<2|2}, {19<<2|0}, {11<<2|1}, {11<<2|0}, {25<<2|1}, {1<<2|3},
    {10<<2|1}, {18<<2|0}, {11<<2|1}, {10<<2|0}, {27<<2|1}, {6<<2|0}, {4<<2|1}, {1<<2|3},
    {9<<2|1}, {1<<2|2}, {19<<2|0}, {1<<2|2}, {10<<2|1}, {9<<2|0}, {9<<2|1}, {4<<2|0}, {17<<2|1}, {3<<2|0}, {8<<2|1}, {1<<2|3},
    {10<<2|1}, {18<<2|0}, {11<<2|1}, {8<<2|0}, {11<<2|1}, {6<<2|0}, {19<<2|1}, {9<<2|1}, {1<<2|3},
    {9<<2|1}, {1<<2|2}, {1<<2|1}, {1<<2|2}, {16<<2|0}, {12<<2|1}, {1<<2|2}, {7<<2|0}, {12<<2|1}, {5<<2|0}, {22<<2|1}, {2<<2|1}, {1<<2|3},
    {1<<2|0}, {10<<2|1}, {18<<2|0}, {1<<2|1}, {10<<2|1}, {1<<2|2}, {8<<2|0}, {12<<2|1}, {3<<2|0}, {23<<2|1}, {2<<2|1}, {1<<2|3},
    {2<<2|0}, {12<<2|1}, {2<<2|0}, {1<<2|2}, {11<<2|0}, {12<<2|1}, {8<<2|0}, {32<<2|1}, {3<<2|1}, {1<<2|3},
    {3<<2|0}, {13<<2|1}, {1<<2|0}, {2<<2|1}, {2<<2|0}, {1<<2|1}, {1<<2|2}, {1<<2|0}, {12<<2|1}, {10<<2|0}, {31<<2|1}, {4<<2|0}, {3<<2|1}, {1<<2|3},
    {4<<2|0}, {19<<2|1}, {12<<2|0}, {11<<2|0}, {1<<2|2}, {23<<2|1}, {4<<2|0}, {7<<2|1}, {1<<2|3},
    {5<<2|0}, {27<<2|1}, {14<<2|0}, {20<<2|1}, {8<<2|0}, {3<<2|1}, {1<<2|3},
    {6<<2|0}, {1<<2|2}, {27<<2|1}, {18<<2|0}, {17<<2|1}, {1<<2|3},
    {8<<2|0}, {24<<2|1}, {22<<2|0}, {11<<2|1}, {7<<2|0}, {1<<2|3},
    {11<<2|0}, {20<<2|1}, {27<<2|0}, {11<<2|1}, {2<<2|0}, {1<<2|2}, {9<<2|1}, {1<<2|3},
    {14<<2|0}, {16<<2|1}, {11<<2|0}, {1<<2|2}, {1<<2|1}, {1<<2|2}, {1<<2|1}, {1<<2|0}, {18<<2|0}, {1<<2|2}, {16<<2|1}, {3<<2|0}, {3<<2|1}, {1<<2|3},
    {18<<2|0}, {11<<2|1}, {8<<2|0}, {8<<2|1}, {20<<2|0}, {11<<2|1}, {4<<2|0}, {1<<2|1}, {1<<2|3},
    {16<<2|0}, {24<<2|1}, {22<<2|0}, {12<<2|1}, {1<<2|3},
    {16<<2|0}, {26<<2|1}, {2<<2|0}, {4<<2|1}, {1<<2|0}, {3<<2|1}, {21<<2|0}, {10<<2|1}, {1<<2|0}, {3<<2|1}, {1<<2|2}, {1<<2|3},
    {21<<2|0}, {19<<2|1}, {1<<2|0}, {6<<2|1}, {1<<2|0}, {2<<2|1}, {26<<2|0}, {1<<2|1}, {1<<2|2}, {1<<2|3},
    {49<<2|0}, {7<<2|1}, {1<<2|3}
};

// Dragon print function
void print_dragon(void) {
    for (size_t i = 0; i < sizeof(DRAGON_DATA)/sizeof(dragon_run_t); i++) {
        unsigned char count = DRAGON_DATA[i].data >> 2;
        unsigned char chr_idx = DRAGON_DATA[i].data & 0x3;
        for (unsigned char j = 0; j < count; j++) {
            putchar(CHARS[chr_idx]);
        }
    }
}

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