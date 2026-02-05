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

/**********************************************************************/
/* This is the first program in the series. It is meant to teach 1    */
/* things. First, you will learn/recall how to sendto/recvfrom with   */
/* with datagrams.                                                    */
/**********************************************************************/
char* keys[]={"File_Name","File_Size","File_Type","Date_Created","Description"};
typedef struct{
  char File_Name[50];
  char File_Size[50];
 	char File_Type[50];
 	char Date_Created[50];
 	char Description[100];
}json;
json linetojson(char *line){
  json currentFile;
  sscanf(line, "File_Name:\"%[^\"]\" File_Size:%s File_Type:%s Date_Created:%s Description:%s",
     currentFile.File_Name,currentFile.File_Size, currentFile.File_Type,currentFile.Date_Created,currentFile.Description);
  return currentFile;
}
void jsontostring(json *data, char *buffer) {
    sprintf(buffer, 
        "{\n"
        "  \"File_Name\": \"%s\",\n"
        "  \"File_Size\": \"%s\",\n"
        "  \"File_Type\": \"%s\",\n"
        "  \"Date_Created\": \"%s\",\n"
        "  \"Description\": \"%s\"\n"
        "}", 
        data->File_Name, 
        data->File_Size, 
        data->File_Type, 
        data->Date_Created, 
        data->Description
    );
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
    printf("生成的 JSON 是:\n%s\n", buffer);
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
