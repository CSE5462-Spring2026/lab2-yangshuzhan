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
    printf("%-20s %s\n", "Key", "Value"); // 表头改成通用的 Key/Value
    printf("************************************************\n");

    char *p = json;
    
    // 循环查找每一个 Key
    while ((p = strchr(p, '"')) != NULL) {
        // 1. 提取 Key
        char *key_start = p + 1;
        char *key_end = strchr(key_start, '"');
        if (!key_end) break; // 格式错误

        int key_len = key_end - key_start;
        
        // 移动指针去找冒号
        p = strchr(key_end, ':');
        if (!p) break;

        // 2. 找 Value 的起点（跳过冒号和空格）
        char *val_start = p + 1;
        while (*val_start && isspace(*val_start)) val_start++;

        char *val_end = NULL;
        
        // 3. 判断 Value 类型
        if (*val_start == '"') {
            // Case A: Value 是字符串 (例如 "DAVE")
            val_start++; // 跳过开头的引号
            val_end = strchr(val_start, '"'); // 找结尾的引号
        } else {
            // Case B: Value 是数字或布尔值 (例如 1228, true)
            // 找逗号 ',' 或者大括号 '}' 作为结尾
            val_end = val_start;
            while (*val_end && *val_end != ',' && *val_end != '}') {
                val_end++;
            }
        }

        if (val_end) {
            int val_len = val_end - val_start;
            
            // 4. 打印结果
            // "%.*s" 允许我们打印指定长度的字符串，不需要手动加 \0
            printf("%-20.*s %.*s\n", key_len, key_start, val_len, val_start);
            
            // 指针移动到当前 Value 后面，准备下一轮循环
            p = val_end;
        } else {
            break; 
        }
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

