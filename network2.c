/*Shuzhan Yang
2026/1/23*/
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
void extract_and_print(char *json, char *key_display, char *json_key) {
    // 1. 先只查找 Key 的名字 (比如 "File_Name")
    char *key_ptr = strstr(json, json_key);
    
    if (key_ptr) {
        // 2. 从 Key 的位置往后找冒号 ':'
        char *colon_ptr = strchr(key_ptr, ':');
        
        if (colon_ptr) {
            // 3. 从冒号往后找第一个双引号 '"' (这是值的开始)
            char *val_start = strchr(colon_ptr, '"');
            
            if (val_start) {
                // 指针移到引号里面
                val_start++; 
                
                // 4. 找结束的引号
                char *val_end = strchr(val_start, '"');
                
                if (val_end) {
                    int len = val_end - val_start;
                    // 打印
                    printf("%-20s %.*s\n", key_display, len, val_start);
                    return;
                }
            }
        }
    }
    // 没找到
    printf("%-20s (Unknown)\n", key_display);
}

// --- 修改：针对 JSON 格式完全重写 ---
void format_message(char *buf) {
    printf("************************************************\n");
    printf("%-20s %s\n", "Name", "Value");
    printf("************************************************\n");

    // 直接去 JSON 里“抓”我们要的数据
    // 参数2是显示的名字，参数3是JSON里的Key名
    extract_and_print(buf, "File Name",    "File_Name");
    extract_and_print(buf, "File Size",    "File_Size");
    extract_and_print(buf, "File Type",    "File_Type");
    extract_and_print(buf, "Date Created", "Date_Created");
    extract_and_print(buf, "Description",  "Description");

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

