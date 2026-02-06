#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
int sendStuff(char *buffer, int sd, struct sockaddr_in server_address);
void makeSocket(int *sd, char *argv[], struct sockaddr_in *server_address);
FILE *  openFile();
char *rtrim(char *s);

#define MAX_FIELDS 20
#define MAX_LEN 100

typedef struct {
    char keys[MAX_FIELDS][MAX_LEN];   // 存字段名
    char values[MAX_FIELDS][MAX_LEN]; // 存字段值
    int count;                        // 记录存了多少个字段
} json;

/* 2. 保持函数名 linetojson
    功能：解析 "Key:Value Key2:Value2" 格式的字符串
*/
json linetojson(char *line) {
    json currentJson;
    currentJson.count = 0;

    char *ptr = line;

    // 循环解析直到字符串结束
    while (*ptr != '\0' && currentJson.count < MAX_FIELDS) {
        
        // 1. 跳过前导空格 (Trim leading spaces)
        while (*ptr == ' ' || *ptr == '\n' || *ptr == '\r') ptr++;
        if (*ptr == '\0') break; // 如果全是空格，结束

        // 2. 找冒号 (Key 的结束)
        char *colon = strchr(ptr, ':');
        if (!colon) break; // 没找到冒号，格式不对，退出

        // 3. 提取 Key
        int keyLen = colon - ptr;
        if (keyLen >= MAX_LEN) keyLen = MAX_LEN - 1;
        strncpy(currentJson.keys[currentJson.count], ptr, keyLen);
        currentJson.keys[currentJson.count][keyLen] = '\0';

        // 4. 定位 Value 的起点
        char *valStart = colon + 1;
        char *valEnd = NULL;
        int isQuoted = 0;

        if (*valStart == '"') {
            // --- 情况 A: 值被引号包围 (例如 msg:" hello world") ---
            isQuoted = 1;
            valStart++; // 跳过开头的引号
            valEnd = strchr(valStart, '"'); // 找下一个引号
            
            if (!valEnd) {
                // 如果没找到结束引号，就读到行尾
                valEnd = valStart + strlen(valStart);
            }
        } else {
            // --- 情况 B: 普通值，读到空格为止 (例如 time:1228) ---
            // 找下一个空格
            valEnd = strchr(valStart, ' ');
            
            // 如果没找到空格，说明是最后一个字段 (可能带换行符)
            if (!valEnd) {
                valEnd = valStart + strlen(valStart);
            }
        }

        // 5. 提取 Value
        int valLen = valEnd - valStart;
        
        // 去掉 Value 里的换行符 (针对最后一个字段如 myName:DAVE\n)
        while (valLen > 0 && (valStart[valLen-1] == '\n' || valStart[valLen-1] == '\r')) {
            valLen--;
        }

        if (valLen >= MAX_LEN) valLen = MAX_LEN - 1;
        strncpy(currentJson.values[currentJson.count], valStart, valLen);
        currentJson.values[currentJson.count][valLen] = '\0';

        // 6. 成功存入一组
        currentJson.count++;

        // 7. 移动指针，准备下一轮
        if (isQuoted) {
            ptr = valEnd + 1; // 跳过结束引号
        } else {
            ptr = valEnd; // 从刚才停下的地方继续
        }
    }

    return currentJson;
}

/* 3. 保持函数名 jsontostring
    功能：把存好的动态 Key-Value 拼成 JSON 格式
*/
void jsontostring(json *data, char *buffer) {
    strcpy(buffer, "{\n"); // 开始

    char temp[256];
    
    for (int i = 0; i < data->count; i++) {
        // 拼装一行: "Key": "Value"
        sprintf(temp, "  \"%s\": \"%s\"", data->keys[i], data->values[i]);
        strcat(buffer, temp);

        // 如果不是最后一行，加逗号
        if (i < data->count - 1) {
            strcat(buffer, ",\n");
        } else {
            strcat(buffer, "\n");
        }
    }

    strcat(buffer, "}"); // 结束
}

int main(int argc, char *argv[])
{
  int sd; /* the socket descriptor */
  struct sockaddr_in server_address;  /* structures for addresses */
  char * lineFromFile = NULL;
  FILE * fptr;
  int rc;
  size_t lengthRead = 0;
  
  /* checck to see if the right number of parameters was entered */
  if (argc < 3){
    printf ("usage is client <ipaddr> <portnumber>\n");
    exit(1); /* just leave if wrong number entered */
  }

  /* call the function to make the socket and fill in server address */
  makeSocket(&sd, argv, &server_address);
  fptr = openFile(); // open the file with the data to send

  /* now we will loop until the end of file, sending one line */
  /* at a time.                                               */
  
  for (;;){

    rc = getline(&lineFromFile, &lengthRead, fptr);
    if (rc <=0){
      printf ("done with file!\n");
      fclose (fptr);
      break; // exit the forever loop
    }
    char buffer[1024];
    json currentStruct = linetojson(lineFromFile);
    jsontostring(&currentStruct,buffer);
    printf ("I am sending '%s'", lineFromFile);
    printf("parsed JSON data:\n%s\n", buffer);
    printf ("the length of the string is %lu bytes\n", strlen(lineFromFile));
    sendStuff(buffer, sd, server_address);
  }


  return 0 ; 

}

/******************************************************************/
/* this function actually does the sending of the data            */
/******************************************************************/
int sendStuff(char *buffer, int sd, struct sockaddr_in server_address){

  int rc = 0;
  rc = sendto(sd, buffer, strlen(buffer), 0,
	      (struct sockaddr *) &server_address, sizeof(server_address));
  printf ("I think i sent %d bytes \n", rc);

  return (0); 
}

/******************************************************************/
/* this function will create a socket and fill in the address of  */
/*  the server                                                    */
/******************************************************************/
void makeSocket(int *sd, char *argv[], struct sockaddr_in *server_address){
  int i; // loop variable
  struct sockaddr_in inaddr; // use this as a temp value for checking validity
  int portNumber; // get this from command line
  char serverIP[50]; // overkill on size
  
  /* this code checks to see if the ip address is a valid ip address */
  /* meaning it is in dotted notation and has valid numbers          */
  if (!inet_pton(AF_INET, argv[1], &inaddr)){
    printf ("error, bad ip address\n");
    exit (1); /* just leave if is incorrect */
  }
  
  /* first create a socket */
  *sd = socket(AF_INET, SOCK_DGRAM, 0); /* create a socket */
  int reuse =1;
  setsockopt(*sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));


  /* always check for errors */
  if (*sd == -1){ /* means some kind of error occured */
    perror ("socket");
    exit(1); /* just leave if wrong number entered */
  }



 /* check that the port number is a number..... */

  for (i=0;i<strlen(argv[2]); i++){
    if (!isdigit(argv[2][i]))
      {
	printf ("The Portnumber isn't a number!\n");
	exit(1);
      }
  }

  portNumber = strtol(argv[2], NULL, 10); /* many ways to do this */
  /* exit if a bad port number is entered */
  if ((portNumber > 65535) || (portNumber < 0)){
    printf ("you entered an invalid socket number\n");
    exit (1);
  }
  /* now fill in the address data structure we use to sendto the server */  
  strcpy(serverIP, argv[1]); /* copy the ip address */

  server_address->sin_family = AF_INET; /* use AF_INET addresses */
  server_address->sin_port = htons(portNumber); /* convert port number */
  server_address->sin_addr.s_addr = inet_addr(serverIP); /* convert IP addr */

}

/******************************************************************/
/* this function will ask the user for the name of the input file */
/* it will then open that file and pass pack the file descriptor  */
/******************************************************************/
FILE * openFile (){
  FILE * fptr = NULL; 
  char fileName [100]; // this will be given by user
  while (1){
    memset (fileName, 0,100); // always blank the buffer
    printf ("What is the name of the messages file you would like to use? ");
    char *ptr = fgets(fileName, sizeof(fileName), stdin);
    if (ptr == NULL){
      perror ("fgets");
      exit (1);
    }

    ptr = rtrim(ptr);
    if (ptr == NULL){ // error occured                                                                  
      printf ("you didn't enter anything, try again.\n");
    }
    else{
      fptr = fopen (fileName, "r");
      if (fptr == NULL){
	printf ("error opening the file, try again\n");
	continue; //bad bad bad
      }
      return fptr;
      break; // stop looping
    }
  } // end of the forever loop!
}// end of the function

/* this trims characters from a string */
char *rtrim(char *s)
{
    char* back = s + strlen(s);
    while(isspace(*--back));
    *(back+1) = '\0';
    return s;
}
