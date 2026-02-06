/*Shuzhan Yang
2026/2/5*/
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>


void format_message(char *json) {
    printf("************************************************\n");
    printf("%-20s %s\n", "Key", "Value");
    printf("************************************************\n");

    char *p = json;

    while ((p = strchr(p, '"')) != NULL) {
        // extract key
        char *key_start = p + 1;
        char *key_end = strchr(key_start, '"');
        if (!key_end) break; 

        int key_len = key_end - key_start;
        
        //look for :
        p = strchr(key_end, ':');
        if (!p) break;

        char *val_start = p + 1;
        while (*val_start && isspace(*val_start)) val_start++;

        char *val_end = NULL;
        
        if (*val_start == '"') {
            // if value is number
            val_start++; 
            val_end = strchr(val_start, '"'); 
            
            if (val_end) {
                 // print string
                printf("%-20.*s %.*s\n", key_len, key_start, (int)(val_end - val_start), val_start);
                p = val_end + 1;
            }
        } else {
            //if Value is number 
            val_end = val_start;
            while (*val_end && *val_end != ',' && *val_end != '}') {
                val_end++;
            }
            
            // print numbers
            printf("%-20.*s %.*s\n", key_len, key_start, (int)(val_end - val_start), val_start);
            
            p = val_end;
        }

        if (!val_end) break; 
    }
    printf("************************************************\n");
}
int main(){
    int socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    int reuse = 1; // 1 means on
    struct sockaddr_in src;
    socklen_t len = sizeof(src);

    struct sockaddr_in addr;
    struct ip_mreq mreq;
    memset(&addr, 0, sizeof(addr));      // initialize
    addr.sin_family = AF_INET;            // IPv4
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // address

    char mcast_ip[32];
    int port;

    printf("Enter multicast IP port: ");
    int result =scanf("%31s%d", mcast_ip,&port);
    if (result != 2) {//handel exception
            printf("Error: Invalid input format.\n");
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            continue;
        }

    mreq.imr_multiaddr.s_addr = inet_addr(mcast_ip); //multicast address 
    addr.sin_port = htons(port);         //port number

    setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    bind(socketfd, (struct sockaddr*)&addr, sizeof(addr));
    setsockopt(socketfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));//join
    char buf[1024];  // 1 KB cache

    while (1) {
    int n = recvfrom(socketfd, buf, sizeof(buf) - 1, 0,
                     (struct sockaddr*)&src, &len);
    if (n < 0) {
        perror("recvfrom");
        break;
    }

    buf[n] = 0; 
    printf("Raw JSON: %s\n", buf);
    format_message(buf);
}
}

