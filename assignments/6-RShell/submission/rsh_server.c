
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>

//INCLUDES for extra credit
#include <signal.h>
#include <pthread.h>
//------------------------- NOTE: EXTRA CREDIT IMPLEMENTED

#include "dshlib.h"
#include "rshlib.h"

// Function prototype for handle_client
void *handle_client(void *arg);

// Global variables for threaded server
int g_is_threaded = 0;  // Flag indicating if server is threaded
pthread_mutex_t g_client_mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex for thread safety
int g_active_clients = 0;  // Count of active client connections
volatile int g_server_should_exit = 0;  // Flag to signal server shutdown

/*
 * start_server(ifaces, port, is_threaded)
 *      ifaces:  a string in ip address format, indicating the interface
 *              where the server will bind.  In almost all cases it will
 *              be the default "0.0.0.0" which binds to all interfaces.
 *              note the constant RDSH_DEF_SVR_INTFACE in rshlib.h
 * 
 *      port:   The port the server will use.  Note the constant 
 *              RDSH_DEF_PORT which is 1234 in rshlib.h.  If you are using
 *              tux you may need to change this to your own default, or even
 *              better use the command line override -s implemented in dsh_cli.c
 *              For example ./dsh -s 0.0.0.0:5678 where 5678 is the new port  
 * 
 *      is_threded:  Used for extra credit to indicate the server should implement
 *                   per thread connections for clients  
 * 
 *      This function basically runs the server by: 
 *          1. Booting up the server
 *          2. Processing client requests until the client requests the
 *             server to stop by running the `stop-server` command
 *          3. Stopping the server. 
 * 
 *      This function is fully implemented for you and should not require
 *      any changes for basic functionality.  
 * 
 *      IF YOU IMPLEMENT THE MULTI-THREADED SERVER FOR EXTRA CREDIT YOU NEED
 *      TO DO SOMETHING WITH THE is_threaded ARGUMENT HOWEVER.  
 */
int start_server(char *ifaces, int port, int is_threaded) {
    int svr_socket;
    int rc;

    // Set up threading mode if requested
    g_is_threaded = is_threaded;
    if (g_is_threaded) {
        printf("Starting server in threaded mode\n");
    } else {
        printf("Starting server in single-threaded mode\n");
    }

    svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0) {
        int err_code = svr_socket;
        return err_code;
    }

    rc = process_cli_requests(svr_socket);

    stop_server(svr_socket);

    return rc;
}

/*
 * stop_server(svr_socket)
 *      svr_socket: The socket that was created in the boot_server()
 *                  function. 
 * 
 *      This function simply returns the value of close() when closing
 *      the socket.  
 */
int stop_server(int svr_socket) {
    // Clean up threading resources
    if (g_is_threaded) {
        pthread_mutex_destroy(&g_client_mutex);
    }
    
    return close(svr_socket);
}

/*
 * boot_server(ifaces, port)
 *      ifaces & port:  see start_server for description.  They are passed
 *                      as is to this function.   
 * 
 *      This function "boots" the rsh server.  It is responsible for all
 *      socket operations prior to accepting client connections.  Specifically: 
 * 
 *      1. Create the server socket using the socket() function. 
 *      2. Calling bind to "bind" the server to the interface and port
 *      3. Calling listen to get the server ready to listen for connections.
 * 
 *      after creating the socket and prior to calling bind you might want to 
 *      include the following code:
 * 
 *      int enable=1;
 *      setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
 * 
 *      when doing development you often run into issues where you hold onto
 *      the port and then need to wait for linux to detect this issue and free
 *      the port up.  The code above tells linux to force allowing this process
 *      to use the specified port making your life a lot easier.
 * 
 *  Returns:
 * 
 *      server_socket:  Sockets are just file descriptors, if this function is
 *                      successful, it returns the server socket descriptor, 
 *                      which is just an integer.
 * 
 *      ERR_RDSH_COMMUNICATION:  This error code is returned if the socket(),
 *                               bind(), or listen() call fails. 
 * 
 */
int boot_server(char *ifaces, int port) {
    int svr_socket;
    int ret;
    struct sockaddr_in addr;

    // Create a socket
    svr_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_socket < 0) {
        perror("socket");
        return ERR_RDSH_COMMUNICATION;
    }

    // Set socket to reuse address
    int enable = 1;
    setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    // Set up the server address structure
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, ifaces, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    // Bind the socket to the address
    if (bind(svr_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    // Listen for incoming connections
    ret = listen(svr_socket, 20);
    if (ret == -1) {
        perror("listen");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    return svr_socket;
}

/*
 * process_cli_requests(svr_socket)
 *      svr_socket:  The server socket that was obtained from boot_server()
 *   
 *  This function handles managing client connections.  It does this using
 *  the following logic
 * 
 *      1.  Starts a while(1) loop:
 *  
 *          a. Calls accept() to wait for a client connection. Recall that 
 *             the accept() function returns another socket specifically
 *             bound to a client connection. 
 *          b. Calls exec_client_requests() to handle executing commands
 *             sent by the client. It will use the socket returned from
 *             accept().
 *          c. Loops back to the top (step 2) to accept connecting another
 *             client.  
 * 
 *          note that the exec_client_requests() return code should be
 *          negative if the client requested the server to stop by sending
 *          the `stop-server` command.  If this is the case step 2b breaks
 *          out of the while(1) loop. 
 * 
 *      2.  After we exit the loop, we need to cleanup.  Dont forget to 
 *          free the buffer you allocated in step #1.  Then call stop_server()
 *          to close the server socket. 
 * 
 *  Returns:
 * 
 *      OK_EXIT:  When the client sends the `stop-server` command this function
 *                should return OK_EXIT. 
 * 
 *      ERR_RDSH_COMMUNICATION:  This error code terminates the loop and is
 *                returned from this function in the case of the accept() 
 *                function failing. 
 * 
 *      OTHERS:   See exec_client_requests() for return codes.  Note that positive
 *                values will keep the loop running to accept additional client
 *                connections, and negative values terminate the server. 
 * 
 */
int process_cli_requests(int svr_socket) {
    int cli_socket;
    int rc = OK;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t thread_id;

    while (1) {
        // Check if we should stop the server
        if (g_server_should_exit) {
            return OK_EXIT;
        }
        
        // Accept connection from client
        cli_socket = accept(svr_socket, (struct sockaddr *)&client_addr, &client_len);
        if (cli_socket < 0) {
            // If we were interrupted by a signal, check if server should exit
            if (errno == EINTR && g_server_should_exit) {
                return OK_EXIT;
            }
            perror("accept");
            return ERR_RDSH_COMMUNICATION;
        }
        
        if (g_is_threaded) {
            // Handle client in a new thread
            int *client_sock = malloc(sizeof(int));
            if (client_sock == NULL) {
                perror("malloc");
                close(cli_socket);
                continue;
            }
            
            *client_sock = cli_socket;
            
            // Update active client count
            pthread_mutex_lock(&g_client_mutex);
            g_active_clients++;
            pthread_mutex_unlock(&g_client_mutex);
            
            // Create new thread to handle client
            if (pthread_create(&thread_id, NULL, handle_client, client_sock) != 0) {
                perror("pthread_create");
                close(cli_socket);
                free(client_sock);
                
                pthread_mutex_lock(&g_client_mutex);
                g_active_clients--;
                pthread_mutex_unlock(&g_client_mutex);
                
                continue;
            }
            
            // Detach thread so it cleans up itself when done
            pthread_detach(thread_id);
        } else {
            // Handle client in main thread (non-threaded mode)
            rc = exec_client_requests(cli_socket);
            close(cli_socket);
            
            if (rc == OK_EXIT) {
                printf(RCMD_MSG_SVR_STOP_REQ);
                return OK_EXIT;
            } else if (rc == OK) {
                printf(RCMD_MSG_CLIENT_EXITED);
            } else {
                printf(CMD_ERR_RDSH_ITRNL, rc);
                return rc;
            }
        }
    }

    return rc;
}

// Thread handler function for client requests
void *handle_client(void *arg) {
    int cli_socket = *((int *)arg);
    int rc;
    
    // Free the socket pointer allocated in process_cli_requests
    free(arg);
    
    // Execute client requests
    rc = exec_client_requests(cli_socket);
    
    // Close the client socket
    close(cli_socket);
    
    // Update active client count
    pthread_mutex_lock(&g_client_mutex);
    g_active_clients--;
    
    // Check if server should exit
    if (rc == OK_EXIT) {
        // Signal the main thread to stop the server
        printf(RCMD_MSG_SVR_STOP_REQ);
        g_server_should_exit = 1;
    } else if (rc == OK) {
        printf(RCMD_MSG_CLIENT_EXITED);
    }
    
    pthread_mutex_unlock(&g_client_mutex);
    
    pthread_exit(NULL);
    return NULL;
}

/*
 * exec_client_requests(cli_socket)
 *      cli_socket:  The server-side socket that is connected to the client
 *   
 *  This function handles accepting remote client commands. The function will
 *  loop and continue to accept and execute client commands.  There are 2 ways
 *  that this ongoing loop accepting client commands ends:
 * 
 *      1.  When the client executes the `exit` command, this function returns
 *          to process_cli_requests() so that we can accept another client
 *          connection. 
 *      2.  When the client executes the `stop-server` command this function
 *          returns to process_cli_requests() with a return code of OK_EXIT
 *          indicating that the server should stop. 
 * 
 *  Note that this function largely follows the implementation of the
 *  exec_local_cmd_loop() function that you implemented in the last 
 *  shell program deliverable. The main difference is that the command will
 *  arrive over the recv() socket call rather than reading a string from the
 *  keyboard. 
 * 
 *  This function also must send the EOF character after a command is
 *  successfully executed to let the client know that the output from the
 *  command it sent is finished.  Use the send_message_eof() to accomplish 
 *  this. 
 * 
 *  Of final note, this function must allocate a buffer for storage to 
 *  store the data received by the client. For example:
 *     io_buff = malloc(RDSH_COMM_BUFF_SZ);
 *  And since it is allocating storage, it must also properly clean it up
 *  prior to exiting.
 * 
 *  Returns:
 * 
 *      OK:       The client sent the `exit` command.  Get ready to connect
 *                another client. 
 *      OK_EXIT:  The client sent `stop-server` command to terminate the server
 * 
 *      ERR_RDSH_COMMUNICATION:  A catch all for any socket() related send
 *                or receive errors. 
 */
int exec_client_requests(int cli_socket) {
    int io_size;
    command_list_t cmd_list;
    int rc;
    int cmd_rc;
    char *io_buff;

    // Allocate input/output buffer
    io_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (io_buff == NULL) {
        return ERR_RDSH_SERVER;
    }

    while (1) {
        // Clear buffer
        memset(io_buff, 0, RDSH_COMM_BUFF_SZ);
        
        // Receive command from client
        io_size = recv(cli_socket, io_buff, RDSH_COMM_BUFF_SZ - 1, 0);
        
        // Check for errors or closed connection
        if (io_size <= 0) {
            if (io_size < 0) {
                perror("recv");
            }
            free(io_buff);
            return ERR_RDSH_COMMUNICATION;
        }
        
        // Make sure buffer is null-terminated
        io_buff[io_size] = '\0';
        
        // Print received command for debugging
        printf(RCMD_MSG_SVR_EXEC_REQ, io_buff);
        
        // Check for exit command
        if (strcmp(io_buff, EXIT_CMD) == 0) {
            char *exit_msg = "exiting...\n";
            send_message_string(cli_socket, exit_msg);
            free(io_buff);
            return OK;
        }
        
        // Check for stop-server command
        if (strcmp(io_buff, "stop-server") == 0) {
            char *stop_msg = "stopping server...\n";
            send_message_string(cli_socket, stop_msg);
            free(io_buff);
            return OK_EXIT;
        }
        
        // Build command list from input
        memset(&cmd_list, 0, sizeof(command_list_t));
        rc = build_cmd_list(io_buff, &cmd_list);
        
        // Handle parsing errors
        if (rc == WARN_NO_CMDS) {
            send_message_string(cli_socket, CMD_WARN_NO_CMD);
            continue;
        } else if (rc == ERR_TOO_MANY_COMMANDS) {
            char error_msg[100];
            sprintf(error_msg, CMD_ERR_PIPE_LIMIT, CMD_MAX);
            send_message_string(cli_socket, error_msg);
            continue;
        } else if (rc != OK) {
            char error_msg[100];
            sprintf(error_msg, "Error parsing command: %d\n", rc);
            send_message_string(cli_socket, error_msg);
            continue;
        }
        
        // Execute command pipeline
        cmd_rc = rsh_execute_pipeline(cli_socket, &cmd_list);
        
        // Send EOF to indicate command completion
        rc = send_message_eof(cli_socket);
        if (rc != OK) {
            printf(CMD_ERR_RDSH_COMM);
            free(io_buff);
            free_cmd_list(&cmd_list);
            return ERR_RDSH_COMMUNICATION;
        }
        
        // Free command list resources
        free_cmd_list(&cmd_list);
        
        // Check for special built-in command results
        if (cmd_rc == EXIT_SC) {
            free(io_buff);
            return OK_EXIT;
        }
    }

    // Should never reach here, but clean up just in case
    free(io_buff);
    return OK;
}

/*
 * send_message_eof(cli_socket)
 *      cli_socket:  The server-side socket that is connected to the client

 *  Sends the EOF character to the client to indicate that the server is
 *  finished executing the command that it sent. 
 * 
 *  Returns:
 * 
 *      OK:  The EOF character was sent successfully. 
 * 
 *      ERR_RDSH_COMMUNICATION:  The send() socket call returned an error or if
 *           we were unable to send the EOF character. 
 */
int send_message_eof(int cli_socket){
    int send_len = (int)sizeof(RDSH_EOF_CHAR);
    int sent_len;
    sent_len = send(cli_socket, &RDSH_EOF_CHAR, send_len, 0);

    if (sent_len != send_len){
        return ERR_RDSH_COMMUNICATION;
    }
    return OK;
}


/*
 * send_message_string(cli_socket, char *buff)
 *      cli_socket:  The server-side socket that is connected to the client
 *      buff:        A C string (aka null terminated) of a message we want
 *                   to send to the client. 
 *   
 *  Sends a message to the client.  Note this command executes both a send()
 *  to send the message and a send_message_eof() to send the EOF character to
 *  the client to indicate command execution terminated. 
 * 
 *  Returns:
 * 
 *      OK:  The message in buff followed by the EOF character was 
 *           sent successfully. 
 * 
 *      ERR_RDSH_COMMUNICATION:  The send() socket call returned an error or if
 *           we were unable to send the message followed by the EOF character. 
 */
int send_message_string(int cli_socket, char *buff) {
    ssize_t sent_len;
    size_t msg_len;
    
    if (buff == NULL) {
        return ERR_RDSH_COMMUNICATION;
    }
    
    // Get message length
    msg_len = strlen(buff);
    
    // Send the message
    sent_len = send(cli_socket, buff, msg_len, 0);
    if (sent_len < 0 || (size_t)sent_len != msg_len) {
        printf(CMD_ERR_RDSH_SEND, (int)sent_len, (int)msg_len);
        return ERR_RDSH_COMMUNICATION;
    }
    
    // Send EOF character
    return send_message_eof(cli_socket);
}


/*
 * rsh_execute_pipeline(int cli_sock, command_list_t *clist)
 *      cli_sock:    The server-side socket that is connected to the client
 *      clist:       The command_list_t structure that we implemented in
 *                   the last shell. 
 *   
 *  This function executes the command pipeline.  It should basically be a
 *  replica of the execute_pipeline() function from the last deliverable. 
 *  The only thing different is that you will be using the cli_sock as the
 *  main file descriptor on the first executable in the pipeline for STDIN,
 *  and the cli_sock for the file descriptor for STDOUT, and STDERR for the
 *  last executable in the pipeline.  See picture below:  
 * 
 *      
 *┌───────────┐                                                    ┌───────────┐
 *│ cli_sock  │                                                    │ cli_sock  │
 *└─────┬─────┘                                                    └────▲──▲───┘
 *      │   ┌──────────────┐     ┌──────────────┐     ┌──────────────┐  │  │    
 *      │   │   Process 1  │     │   Process 2  │     │   Process N  │  │  │    
 *      │   │              │     │              │     │              │  │  │    
 *      └───▶stdin   stdout├─┬──▶│stdin   stdout├─┬──▶│stdin   stdout├──┘ │    
 *          │              │ │   │              │ │   │              │     │    
 *          │        stderr├─┘   │        stderr├─┘   │        stderr├─────┘    
 *          └──────────────┘     └──────────────┘     └──────────────┘   
 *                                                      WEXITSTATUS()
 *                                                      of this last
 *                                                      process to get
 *                                                      the return code
 *                                                      for this function       
 * 
 *  Returns:
 * 
 *      EXIT_CODE:  This function returns the exit code of the last command
 *                  executed in the pipeline.  If only one command is executed
 *                  that value is returned.  Remember, use the WEXITSTATUS()
 *                  macro that we discussed during our fork/exec lecture to
 *                  get this value. 
 */
int rsh_execute_pipeline(int cli_sock, command_list_t *clist) {
    int pipes[clist->num - 1][2];  // Array of pipes
    pid_t pids[clist->num];
    int pids_st[clist->num];      // Array to store process status
    Built_In_Cmds bi_cmd;
    int exit_code;

    // Check for empty command list
    if (clist == NULL || clist->num == 0) {
        return WARN_NO_CMDS;
    }
    
    // For single command (no pipeline)
    if (clist->num == 1) {
        // Check for built-in commands
        bi_cmd = rsh_built_in_cmd(&clist->commands[0]);
        
        if (bi_cmd == BI_CMD_EXIT) {
            return EXIT_SC;
        } else if (bi_cmd == BI_CMD_STOP_SVR) {
            return STOP_SERVER_SC;
        } else if (bi_cmd == BI_EXECUTED) {
            // Built-in command was executed
            return OK;
        }
        
        // Non-built-in command, execute using fork/exec
        pid_t pid = fork();
        
        if (pid < 0) {
            // Fork failed
            perror("fork");
            return ERR_RDSH_CMD_EXEC;
        } else if (pid == 0) {
            // Child process
            
            // Set up redirection
            if (clist->commands[0].input_file == NULL) {
                // No input redirection, use socket for stdin
                dup2(cli_sock, STDIN_FILENO);
            } else {
                // Input redirection from file
                int fd = open(clist->commands[0].input_file, O_RDONLY);
                if (fd < 0) {
                    perror(clist->commands[0].input_file);
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            
            if (clist->commands[0].output_file == NULL) {
                // No output redirection, use socket for stdout and stderr
                dup2(cli_sock, STDOUT_FILENO);
                dup2(cli_sock, STDERR_FILENO);
            } else {
                // Output redirection to file
                int flags = O_WRONLY | O_CREAT;
                if (clist->commands[0].append_mode) {
                    flags |= O_APPEND;
                } else {
                    flags |= O_TRUNC;
                }
                
                int fd = open(clist->commands[0].output_file, flags, 0644);
                if (fd < 0) {
                    perror(clist->commands[0].output_file);
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
                
                // Redirect stderr to socket
                dup2(cli_sock, STDERR_FILENO);
            }
            
            // Execute command
            execvp(clist->commands[0].argv[0], clist->commands[0].argv);
            
            // If we get here, execvp failed
            perror(clist->commands[0].argv[0]);
            exit(EXIT_FAILURE);
        } else {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
            
            return WEXITSTATUS(status);
        }
    }

    // Multiple commands - need pipes
    
    // Create all necessary pipes
    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return ERR_RDSH_CMD_EXEC;
        }
    }

    // Fork processes for each command
    for (int i = 0; i < clist->num; i++) {
        // Check if first command is built-in
        if (i == 0) {
            bi_cmd = rsh_match_command(clist->commands[i].argv[0]);
            if (bi_cmd != BI_NOT_BI) {
                // Built-in commands don't support piping
                char error_msg[] = "Built-in commands don't support piping\n";
                write(cli_sock, error_msg, strlen(error_msg));
                
                // Close all pipes
                for (int j = 0; j < clist->num - 1; j++) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
                
                return ERR_RDSH_CMD_EXEC;
            }
        }
        
        // Fork child process
        pids[i] = fork();
        
        if (pids[i] < 0) {
            // Fork failed
            perror("fork");
            
            // Close all pipes
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Kill any already created children
            for (int j = 0; j < i; j++) {
                kill(pids[j], SIGTERM);
                waitpid(pids[j], NULL, 0);
            }
            
            return ERR_RDSH_CMD_EXEC;
        } else if (pids[i] == 0) {
            // Child process
            
            // Set up stdin from previous pipe or socket (for first command)
            if (i == 0) {
                // First command gets input from socket
                dup2(cli_sock, STDIN_FILENO);
            } else {
                // Other commands get input from previous pipe
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            
            // Set up stdout to next pipe or socket (for last command)
            if (i == clist->num - 1) {
                // Last command outputs to socket
                dup2(cli_sock, STDOUT_FILENO);
                dup2(cli_sock, STDERR_FILENO);
            } else {
                // Other commands output to next pipe
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            
            // Close all pipe file descriptors
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Execute command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            
            // If execvp returns, there was an error
            perror(clist->commands[i].argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // Parent process: close all pipe ends
    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all children
    for (int i = 0; i < clist->num; i++) {
        waitpid(pids[i], &pids_st[i], 0);
    }

    // Get exit code from last command
    exit_code = WEXITSTATUS(pids_st[clist->num - 1]);
    
    // Check for special exit codes
    for (int i = 0; i < clist->num; i++) {
        if (WEXITSTATUS(pids_st[i]) == EXIT_SC) {
            exit_code = EXIT_SC;
        }
    }
    
    return exit_code;
}

/**************   OPTIONAL STUFF  ***************/
/****
 **** NOTE THAT THE FUNCTIONS BELOW ALIGN TO HOW WE CRAFTED THE SOLUTION
 **** TO SEE IF A COMMAND WAS BUILT IN OR NOT.  YOU CAN USE A DIFFERENT
 **** STRATEGY IF YOU WANT.  IF YOU CHOOSE TO DO SO PLEASE REMOVE THESE
 **** FUNCTIONS AND THE PROTOTYPES FROM rshlib.h
 **** 
 */

/*
 * rsh_match_command(const char *input)
 *      cli_socket:  The string command for a built-in command, e.g., dragon,
 *                   cd, exit-server
 *   
 *  This optional function accepts a command string as input and returns
 *  one of the enumerated values from the BuiltInCmds enum as output. For
 *  example:
 * 
 *      Input             Output
 *      exit              BI_CMD_EXIT
 *      dragon            BI_CMD_DRAGON
 * 
 *  This function is entirely optional to implement if you want to handle
 *  processing built-in commands differently in your implementation. 
 * 
 *  Returns:
 * 
 *      BI_CMD_*:   If the command is built-in returns one of the enumeration
 *                  options, for example "cd" returns BI_CMD_CD
 * 
 *      BI_NOT_BI:  If the command is not "built-in" the BI_NOT_BI value is
 *                  returned. 
 */
Built_In_Cmds rsh_match_command(const char *input)
{
    if (strcmp(input, "exit") == 0)
        return BI_CMD_EXIT;
    if (strcmp(input, "dragon") == 0)
        return BI_CMD_DRAGON;
    if (strcmp(input, "cd") == 0)
        return BI_CMD_CD;
    if (strcmp(input, "stop-server") == 0)
        return BI_CMD_STOP_SVR;
    if (strcmp(input, "rc") == 0)
        return BI_CMD_RC;
    return BI_NOT_BI;
}

/*
 * rsh_built_in_cmd(cmd_buff_t *cmd)
 *      cmd:  The cmd_buff_t of the command, remember, this is the 
 *            parsed version fo the command
 *   
 *  This optional function accepts a parsed cmd and then checks to see if
 *  the cmd is built in or not.  It calls rsh_match_command to see if the 
 *  cmd is built in or not.  Note that rsh_match_command returns BI_NOT_BI
 *  if the command is not built in. If the command is built in this function
 *  uses a switch statement to handle execution if appropriate.   
 * 
 *  Again, using this function is entirely optional if you are using a different
 *  strategy to handle built-in commands.  
 * 
 *  Returns:
 * 
 *      BI_NOT_BI:   Indicates that the cmd provided as input is not built
 *                   in so it should be sent to your fork/exec logic
 *      BI_EXECUTED: Indicates that this function handled the direct execution
 *                   of the command and there is nothing else to do, consider
 *                   it executed.  For example the cmd of "cd" gets the value of
 *                   BI_CMD_CD from rsh_match_command().  It then makes the libc
 *                   call to chdir(cmd->argv[1]); and finally returns BI_EXECUTED
 *      BI_CMD_*     Indicates that a built-in command was matched and the caller
 *                   is responsible for executing it.  For example if this function
 *                   returns BI_CMD_STOP_SVR the caller of this function is
 *                   responsible for stopping the server.  If BI_CMD_EXIT is returned
 *                   the caller is responsible for closing the client connection.
 * 
 *   AGAIN - THIS IS TOTALLY OPTIONAL IF YOU HAVE OR WANT TO HANDLE BUILT-IN
 *   COMMANDS DIFFERENTLY. 
 */
Built_In_Cmds rsh_built_in_cmd(cmd_buff_t *cmd)
{
    Built_In_Cmds ctype = BI_NOT_BI;
    ctype = rsh_match_command(cmd->argv[0]);

    switch (ctype)
    {
    // case BI_CMD_DRAGON:
    //     print_dragon();
    //     return BI_EXECUTED;
    case BI_CMD_EXIT:
        return BI_CMD_EXIT;
    case BI_CMD_STOP_SVR:
        return BI_CMD_STOP_SVR;
    case BI_CMD_RC:
        return BI_CMD_RC;
    case BI_CMD_CD:
        chdir(cmd->argv[1]);
        return BI_EXECUTED;
    default:
        return BI_NOT_BI;
    }
}
