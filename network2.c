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

    // 循环查找每一个 Key
    while ((p = strchr(p, '"')) != NULL) {
        // 1. 提取 Key
        char *key_start = p + 1;
        char *key_end = strchr(key_start, '"');
        if (!key_end) break; 

        int key_len = key_end - key_start;
        
        // 移动指针去找冒号
        p = strchr(key_end, ':');
        if (!p) break;

        // 2. 找 Value 的起点（跳过冒号和空格）
        char *val_start = p + 1;
        while (*val_start && isspace(*val_start)) val_start++;

        char *val_end = NULL;
        
        // 3. 核心修改：判断 Value 类型
        if (*val_start == '"') {
            // Case A: Value 是字符串 (例如 "recv")
            val_start++; // 跳过开头的引号
            val_end = strchr(val_start, '"'); // 找结尾的引号
            
            if (val_end) {
                 // 打印字符串值
                printf("%-20.*s %.*s\n", key_len, key_start, (int)(val_end - val_start), val_start);
                // 准备处理下一个字段：指针跳过当前的结束引号
                p = val_end + 1;
            }
        } else {
            // Case B: Value 是数字 (例如 2, 1228)
            // 数字没有引号，它的结束符是 逗号(,) 或者 花括号(})
            val_end = val_start;
            while (*val_end && *val_end != ',' && *val_end != '}') {
                val_end++;
            }
            
            // 打印数字值
            printf("%-20.*s %.*s\n", key_len, key_start, (int)(val_end - val_start), val_start);
            
            // 准备处理下一个字段：指针直接停在这里即可，循环开头的 strchr 会负责找下一个 Key 的引号
            p = val_end;
        }

        // 安全检查
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

