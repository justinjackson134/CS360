// The echo client client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>

#include <sys/socket.h>
#include <netdb.h>

#define MAX 256

// Define variables
struct hostent *hp;              
struct sockaddr_in  server_addr; 

int server_sock, r;
int SERVER_IP, SERVER_PORT; 


// clinet initialization code

int client_init(char *argv[])
{
  printf("======= clinet init ==========\n");

  printf("1 : get server info\n");
  hp = gethostbyname(argv[1]);
  if (hp==0){
     printf("unknown host %s\n", argv[1]);
     exit(1);
  }

  SERVER_IP   = *(long *)hp->h_addr;
  SERVER_PORT = atoi(argv[2]);

  printf("2 : create a TCP socket\n");
  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock<0){
     printf("socket call failed\n");
     exit(2);
  }

  printf("3 : fill server_addr with server's IP and PORT#\n");
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = SERVER_IP;
  server_addr.sin_port = htons(SERVER_PORT);

  // Connect to server
  printf("4 : connecting to server ....\n");
  r = connect(server_sock,(struct sockaddr *)&server_addr, sizeof(server_addr));
  if (r < 0){
     printf("connect failed\n");
     exit(1);
  }

  printf("5 : connected OK to \007\n"); 
  printf("---------------------------------------------------------\n");
  printf("hostname=%s  IP=%s  PORT=%d\n", 
          hp->h_name, inet_ntoa(SERVER_IP), SERVER_PORT);
  printf("---------------------------------------------------------\n");

  printf("========= init done ==========\n");
}

main(int argc, char *argv[ ])
{
  int n;
  char line[MAX], ans[MAX];

  if (argc < 3){
     printf("Usage : client ServerName SeverPort\n");
     exit(1);
  }

  client_init(argv);
  // sock <---> server
  printf("********  processing loop  *********\n");
  while (1){
    printf("input a line : ");
    bzero(line, MAX);                // zero out line[ ]
    fgets(line, MAX, stdin);         // get a line (end with \n) from stdin

    line[strlen(line)-1] = 0;        // kill \n at end
    if (line[0]==0)                  // exit if NULL line
       exit(0);

    
    // Send ENTIRE line to server
    n = write(server_sock, line, MAX);
    printf("client: wrote n=%d bytes; line=(%s)\n", n, line);




    char *token = strtok(line, " ");
    char *pathName = strtok(NULL, " ");
    struct stat fileStat;
    
    if(strcmp(token, "get") == 0) {
      printf("IN GET\n");

      printf("WAITING FOR SIZE OR BAD...\n");
      n = read(server_sock, ans, MAX);
      printf("client: read  n=%d bytes; echo=(%s)\n",n, ans);
      if(strcmp(ans, "BAD") != 0) {
	//VALID FILE!
	int fp = open("/home/justin/Downloads/CopiedFile.txt", O_CREAT|O_WRONLY);
	int size = atoi(ans);
	int count = 0;

	printf("BEFORE WHILE LOOP\n");
	
	while (count < size) {
	  printf("BEFORE n=read\n");
	  n = read(server_sock, ans, MAX);
	  printf("AFTER n=read : %d\n", n);
	  printf("client: read  n=%d bytes; echo=(%s)\n",n, ans);

	  if (count + n > size) {
	    write(fp, ans, (size-count));
	  }
	  else {
	    write(fp, ans, n);
	  }
	  count += n;
	}
	
	close(fp);
      }
      else {
	printf("ERROR: SERVER SAYS BAD FILE\n");
      }
    }
    else if(strcmp(token, "put") == 0) {
      
      
      if(stat(pathName, &fileStat) < 0) {
	sprintf(line, "ERROR: FILE COULD NOT BE OPENED");
      }
      else {
	
      }
    }





    
    // Read a line from sock and show it
    n = read(server_sock, ans, MAX);
    printf("client: read  n=%d bytes; echo=(%s)\n",n, ans);
  }
}
