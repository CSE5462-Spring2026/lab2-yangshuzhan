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

    // 1. 先找到第一个 Key 的开始引号
    // 只要能找到引号，就说明还有数据
    while ((p = strchr(p, '"')) != NULL) {
        
        // --- 解析 Key ---
        char *key_start = p + 1; // 跳过 Key 的开头引号
        char *key_end = strchr(key_start, '"'); // 找 Key 的结尾引号
        if (!key_end) break; // 格式坏了，退出

        int key_len = key_end - key_start;

        // --- 解析 Value ---
        // 从 Key 的结尾往后找冒号
        char *colon = strchr(key_end, ':');
        if (!colon) break;

        // 从冒号往后找 Value 的开头引号
        char *val_start = strchr(colon, '"');
        if (!val_start) break;
        
        val_start++; // 跳过 Value 的开头引号
        
        char *val_end = strchr(val_start, '"'); // 找 Value 的结尾引号
        if (!val_end) break;

        int val_len = val_end - val_start;

        // --- 打印 ---
        printf("%-20.*s %.*s\n", key_len, key_start, val_len, val_start);

        // --- 🔴 关键修复在这里 🔴 ---
        // 之前的 bug 是 p = val_end; 导致下一次 strchr 又找到了同一个引号
        // 现在我们将 p 指向 val_end 的下一个字符 (+1)
        // 这样它就会跳过当前的 Value，去寻找下一个 Key 的引号了
        p = val_end + 1;
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

