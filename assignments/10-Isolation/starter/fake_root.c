#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

// Modified update_map function with better error handling
static int update_map(char *mapping, char *map_file) {
    int fd = open(map_file, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "Error opening %s: %s\n", map_file, strerror(errno));
        return -1;
    }

    size_t map_len = strlen(mapping);
    ssize_t written = write(fd, mapping, map_len);
    
    if (written != map_len) {
        fprintf(stderr, "Error writing to %s: %s\n", map_file, strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

// Set up proper procedures for writing to uid/gid map
static int setup_user_mapping(pid_t child_pid) {
    char mapping[100];
    char map_path[100];
    
    // First, disable setgroups which is required before writing to gid_map
    snprintf(map_path, sizeof(map_path), "/proc/%d/setgroups", child_pid);
    int fd = open(map_path, O_WRONLY);
    if (fd >= 0) {
        if (write(fd, "deny", 4) != 4) {
            fprintf(stderr, "Failed to disable setgroups: %s\n", strerror(errno));
            close(fd);
            return -1;
        }
        close(fd);
    }
    
    // UID mapping: map child's UID 0 to parent's real UID
    snprintf(mapping, sizeof(mapping), "0 %d 1", getuid());
    snprintf(map_path, sizeof(map_path), "/proc/%d/uid_map", child_pid);
    if (update_map(mapping, map_path) != 0) {
        return -1;
    }
    
    // GID mapping: map child's GID 0 to parent's real GID
    snprintf(mapping, sizeof(mapping), "0 %d 1", getgid());
    snprintf(map_path, sizeof(map_path), "/proc/%d/gid_map", child_pid);
    if (update_map(mapping, map_path) != 0) {
        return -1;
    }
    
    return 0;
}

// Simple function for child to execute
static int child_function(void *arg) {
    char **argv = (char **)arg;
    
    // Give parent time to set up the mappings
    usleep(100000);
    
    // Print the UID information
    printf("Running as UID: %d, GID: %d (should be 0 for both)\n", getuid(), getgid());
    
    // Try a simple namespace operation that doesn't require additional capabilities
    if (unshare(CLONE_NEWUTS) == 0) {
        sethostname("namespace-test", 14);
        printf("Successfully set hostname in new UTS namespace\n");
    } else {
        printf("Limited namespace capabilities: %s\n", strerror(errno));
    }
    
    // Execute the command
    execvp(argv[0], argv);
    
    // If we get here, execvp failed
    perror("execvp");
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s COMMAND [ARGS...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // Prepare the child stack
    const int STACK_SIZE = 1024 * 1024;
    char *stack = malloc(STACK_SIZE);
    if (!stack) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    
    // Create flags for the new user namespace
    int clone_flags = CLONE_NEWUSER | SIGCHLD;
    
    printf("Parent process: UID=%d, GID=%d, PID=%d\n", getuid(), getgid(), getpid());
    
    // Clone a new process with a new user namespace
    pid_t child_pid = clone(child_function, stack + STACK_SIZE, clone_flags, &argv[1]);
    
    if (child_pid == -1) {
        perror("clone");
        free(stack);
        return 1;
    }
    
    printf("Created child process with PID: %d\n", child_pid);
    
    // Set up UID/GID mappings for the child
    if (setup_user_mapping(child_pid) != 0) {
        fprintf(stderr, "Failed to set up user namespace mappings\n");
        // Continue anyway to see what happens
    }
    
    // Wait for the child to exit
    int status;
    waitpid(child_pid, &status, 0);
    
    if (WIFEXITED(status)) {
        printf("Child exited with status: %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("Child terminated by signal: %d\n", WTERMSIG(status));
    }
    
    free(stack);
    return 0;
}