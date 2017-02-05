#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <time.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>

#define	SA	struct sockaddr
#define	MAXLINE		4096	/* max text line length */
#define LISTENQ 20

struct timeval t={1,0};
int Split(char **argv,char *str, const char* token);

int main(int argc, char **argv)
{
	int					sockfd, n;
	char				recvline[MAXLINE + 1],sendline[MAXLINE + 1];
	struct sockaddr_in	servaddr;
    char *argvv[MAXLINE];
    int argcc;

	if (argc != 3)
		printf("usage: a.out <IPaddress> <port>");

	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		printf("socket error");

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port   = htons(atoi(argv[2]));	/* daytime server */
	if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
		printf("inet_pton error for %s", argv[1]);

	if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
		printf("connect error");

//    sleep(1);
    fd_set rset;
    FD_ZERO(&rset);
    int stdineof=0;
    while(1){
        if(stdineof==0)
            FD_SET(fileno(stdin), &rset);

        FD_SET(sockfd,&rset);
        if(select(sockfd+1,&rset,NULL,NULL,NULL) < 0)
            exit(1);
		
		if(FD_ISSET(sockfd,&rset)){
 			/* Read from the server */
            memset(&recvline, 0, MAXLINE); 
            n = read(sockfd, recvline, MAXLINE);
			recvline[n] = 0;	// null terminate 
            if(n == 0){
                if(stdineof==1)
                    return 0;
                else{
                    printf("Server close the connection\n");
                    return 0;
                }
            }
            else if(recvline < 0){
                printf("ERROR: Read from the server error\n");
                return 0;
            }
            else {
                printf("%s", recvline);
            }
		}        

		if(FD_ISSET(fileno(stdin), &rset)){ /* input is readable */
            /* Read message from the stdin, and send it to the server */
            memset(&sendline, 0, MAXLINE); 
            if(fgets(sendline, sizeof(sendline), stdin) == NULL){
                stdineof=1;
                shutdown(sockfd, SHUT_WR); /* send FIN */
                FD_CLR(fileno(stdin), &rset);
                continue;
            }
            argcc=Split(argvv,sendline," \n");
            if(argcc==1 && strcmp(argvv[0],"exit")==0)
                return 0;
            else
             write(sockfd, sendline, strlen(sendline)); 
            
           for(int i=0;i<argcc;i++) 
               free(argvv[i]);
        }
            
    }
	if (n < 0)
		printf("read error");

	exit(0);
}

int Split(char **argv,char *str, const char* token){
    char *arg;
    int argc=0;
    char str2[MAXLINE];
    strcpy(str2,str);
    arg = strtok(str2,token);//command
    while(arg != NULL){
        argv[argc] = (char*)malloc (strlen(arg) + 1);
        strcpy (argv[argc], arg);
        argc++;
        arg = strtok(NULL, token);
    }    
    return argc;
}
