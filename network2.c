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
void extract(char *json, char *key_display, char *json_key) {
    // look for name of key
    char *key_ptr = strstr(json, json_key);
    
    if (key_ptr) {
        //look for : after key
        char *colon_ptr = strchr(key_ptr, ':');
        
        if (colon_ptr) {
            //look for'"' after :
            char *val_start = strchr(colon_ptr, '"');
            
            if (val_start) {
                val_start++; 
                
                // find last "
                char *val_end = strchr(val_start, '"');
                
                if (val_end) {
                    int len = val_end - val_start;
                    // print
                    printf("%-20s %.*s\n", key_display, len, val_start);
                    return;
                }
            }
        }
    }
    // if not working
    printf("%-20s (Unknown)\n", key_display);
}

void format_message(char *buf) {
    printf("************************************************\n");
    printf("%-20s %s\n", "Name", "Value");
    printf("************************************************\n");

    extract(buf, "File Name",    "File_Name");
    extract(buf, "File Size",    "File_Size");
    extract(buf, "File Type",    "File_Type");
    extract(buf, "Date Created", "Date_Created");
    extract(buf, "Description",  "Description");

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
    scanf("%31s%d", mcast_ip,&port);
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

