#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    char *hostname;
    struct hostent *host_info;
    struct in_addr **addr_list;
    int i;
    
    if (argc < 2) {
        printf("Usage: %s <hostname>\n", argv[0]);
        return 1;
    }
    
    hostname = argv[1];
    
    // Get host information
    host_info = gethostbyname(hostname);
    if (host_info == NULL) {
        printf("Failed to resolve hostname: %s\n", hostname);
        return 1;
    }
    
    // Cast the h_addr_list to in_addr since h_addr_list also has the IP address in long format
    addr_list = (struct in_addr **) host_info->h_addr_list;
    
    // Print all IP addresses for this hostname
    printf("IP addresses for %s:\n", hostname);
    for (i = 0; addr_list[i] != NULL; i++) {
        printf("%s\n", inet_ntoa(*addr_list[i]));
    }
    
    return 0;
}