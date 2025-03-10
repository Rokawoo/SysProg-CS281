#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

static char child_stack[1048576];

static int child_fn() {
    printf("Child process UID: %d, GID: %d\n", getuid(), getgid());
    printf("Child's PID: %d\n", getpid());
    
    // Show capabilities
    system("capsh --print");
    

    return 0;
}

static void update_map(char *mapping, char *map_file) {
    int fd = open(map_file, O_RDWR);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    int map_len = strlen(mapping);
    if (write(fd, mapping, map_len) != map_len) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    close(fd);
}

int main() {
    pid_t child_pid;
    
    printf("Parent's PID: %d\n", getpid());
    printf("Parent process UID: %d, GID: %d\n", getuid(), getgid());
    
    // Create a new user namespace with a child process
    child_pid = clone(child_fn, child_stack + sizeof(child_stack),
                      CLONE_NEWUSER | SIGCHLD, NULL);
    
    if (child_pid == -1) {
        perror("clone");
        return 1;
    }
    
    printf("Child PID: %d\n", child_pid);
    
    // UID mapping: map child's UID 0 to parent's real UID, with a range of 1
    char uid_map[100];
    snprintf(uid_map, sizeof(uid_map), "0 %d 1", getuid());
    char map_path[100];
    snprintf(map_path, sizeof(map_path), "/proc/%d/uid_map", child_pid);
    update_map(uid_map, map_path);
    
    // GID mapping: map child's GID 0 to parent's real GID, with a range of 1
    char gid_map[100];
    snprintf(gid_map, sizeof(gid_map), "0 %d 1", getgid());
    snprintf(map_path, sizeof(map_path), "/proc/%d/gid_map", child_pid);
    update_map(gid_map, map_path);
    
    // Wait for the child to exit
    waitpid(child_pid, NULL, 0);
    
    return 0;
}