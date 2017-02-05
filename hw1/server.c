#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <ctype.h>

#define	SA	struct sockaddr
#define	MAXLINE		4096	/* max text line length */
#define LISTENQ 20
    int					i,maxfd,listenfd, connfd, sockfd;
    int                 nready,client[FD_SETSIZE];
    ssize_t             n;
    fd_set              rset,allset;
    char				buff[MAXLINE];

    struct cli_data{
        int client;
        char name[MAXLINE];
        char ip[MAXLINE];
        int port;
    };

    struct  cli_data cli[FD_SETSIZE];

    void service(int fd,char *read);
    void some_quit(int fd);
    int Split(char **argv, char *str, const char* token);
    int isallalpha(const char *str);
    int isexist(const char *str);
int main(int argc, char **argv)
{
    struct sockaddr_in	servaddr;
    struct sockaddr_in  sin;//cliaddr
    socklen_t len; // chiaddr len

 	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(7890);	/* daytime server */

	bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

	listen(listenfd, LISTENQ);

    maxfd =listenfd;
    char anon[]="anonymous";
    for(i=0;i<FD_SETSIZE;i++){
        cli[i].client=-1;
        strcpy(cli[i].name,anon);
    }

    FD_ZERO(&allset);
    FD_SET(listenfd,&allset);
	for ( ; ; ) {
        rset=allset;
        nready=select(maxfd+1,&rset,NULL,NULL,NULL);

        if(FD_ISSET(listenfd,&rset)){
            len = sizeof(sin);
            char str[INET_ADDRSTRLEN];
            connfd = accept(listenfd, (SA *)&sin,&len);
            inet_ntop(AF_INET, &sin.sin_addr, str, INET_ADDRSTRLEN);
            snprintf(buff, sizeof(buff), "[Server] Hello, anonymous! From: <%s>/<%d>\n",str,ntohs(sin.sin_port));
            write(connfd, buff, strlen(buff));

         	for (i = 0; i < FD_SETSIZE; i++){
                if (cli[i].client < 0) {
                    cli[i].client=connfd;
                    strcpy(cli[i].name,anon);//anonymous
                    strcpy(cli[i].ip,str);
                    cli[i].port=ntohs(sin.sin_port);
                    break;
                }
            }

            if (i == FD_SETSIZE)
                printf("too many clients");

            FD_SET(connfd, &allset);    /* add new descriptor to set */
            if (connfd > maxfd)
                maxfd = connfd;         /* for select */

        	for(i = 0; i <= maxfd; i++) {   // new come 
                if ( (sockfd = cli[i].client) < 0) 
                    continue;  
                if(sockfd != connfd){
                    snprintf(buff, sizeof(buff), "[Server] Someone coming!\n");
                    write(sockfd, buff, strlen(buff));
                }
            }

            if (--nready <= 0)
                continue;               /* no more readable descriptors */

    	}


		for(i = 0; i <= maxfd; i++) {   /* check all clients for data */
            if ( (sockfd = cli[i].client) < 0)
                continue;
            if (FD_ISSET(sockfd, &rset)) {
                memset(&buff, 0, MAXLINE); 
                if ( (n = read(sockfd, buff, MAXLINE)) == 0) {
                        /* connection closed by client */
                    close(sockfd);
                    FD_CLR(sockfd, &allset);

                    some_quit(sockfd);// [Server] someone offline
                    
                    cli[i].client = -1;
                } else{
                    service(sockfd,buff);
                }

                if (--nready <= 0)
                    break;              /* no more readable descriptors */
            }
        }
	 

		//close(connfd);
	}
}

void service(int fd,char *read){
    char    *argv[MAXLINE];
    int argc;
    argc=Split(argv,read," \n");
  //  buff[0]='\0';
    if(strcmp(argv[0],"who")==0 && argc==1){
        for(int j = 0; j < maxfd ; j++){ 
            if(cli[j].client < 0) continue; 
                if(fd == cli[j].client)
                    snprintf(buff, sizeof(buff), "[Server] %s %s/%d ->me\n", cli[j].name, cli[j].ip, cli[j].port);
                else 
                    snprintf(buff, sizeof(buff), "[Server] %s %s/%d\n", cli[j].name, cli[j].ip, cli[j].port);
                write(fd, buff, strlen(buff));
        }
    }    
    else if(strcmp(argv[0],"name")==0){
       if(argc  != 2 ){
            snprintf(buff,sizeof(buff),"[Server] ERROR: Username can only consists of 2~12 English letters.\n");
            write(fd, buff, strlen(buff));
       }
       else if(strlen(argv[1]) > 12 || strlen(argv[1]) < 2 || !isallalpha(argv[1])){
            snprintf(buff,sizeof(buff),"[Server] ERROR: Username can only consists of 2~12 English letters.\n");
            write(fd, buff, strlen(buff));
       }
       else if(strcmp(argv[1],"anonymous")==0){
            snprintf(buff,sizeof(buff),"[Server] ERROR: Username cannot be anonymous.\n");
            write(fd, buff, strlen(buff));
       }
       else{
            int flag=0;
            for(int j = 0; j < maxfd ; j++){ 
                if(cli[j].client < 0) 
                    continue;
                if(strcmp(argv[1],cli[j].name)==0 && cli[j].client != fd)
                    flag=1;
            } 
            if(flag==1){
                snprintf(buff, sizeof(buff), "[Server] ERROR: <%s> has been used by others.\n",argv[1]);
                write(fd, buff, strlen(buff));
            }
            else{
                int index;
                for(index = 0; index < maxfd; index++)
                    if(fd == cli[index].client) break;

                for(int j = 0; j < maxfd ; j++){ 
                    if((sockfd = cli[j].client )< 0) continue; 
                    if(sockfd == fd)
                        snprintf(buff, sizeof(buff), "[Server] You're now known as <%s>.\n", argv[1]);
                    else 
                        snprintf(buff, sizeof(buff), "[Server] <%s> is now known as <%s>.\n", cli[index].name,argv[1]);
                    write(sockfd, buff, strlen(buff));
                 }
                //cli[index].name=argv[1];
                strcpy(cli[index].name,argv[1]); //will change all 
            }
        }
    }
    else if(strcmp(argv[0],"tell")==0 && argc==3){
       int index;
        for(index = 0; index < maxfd; index++)
            if(fd == cli[index].client) break;
        

        if(strcmp(cli[index].name,"anonymous")==0){
            snprintf(buff, sizeof(buff), "[Server] ERROR: You are anonymous.\n");
            write(sockfd, buff, strlen(buff));
        }
        else if(strcmp(argv[1],"anonymous")==0){
            snprintf(buff, sizeof(buff), "[Server] ERROR: The client to which you sent is anonymous.\n");
            write(sockfd, buff, strlen(buff));
        }
        else if(!isexist(argv[1])){
            snprintf(buff, sizeof(buff), "[Server] ERROR: The receiver doesn't exist.\n");
            write(sockfd, buff, strlen(buff));
        }
        else{
            for(int j = 0; j < maxfd ; j++){ 
                if((sockfd = cli[j].client )< 0) continue; 
                if(strcmp(argv[1],cli[j].name)==0){// if send itself both show success and message
                    snprintf(buff, sizeof(buff), "[Server] %s tell you %s\n",cli[index].name,argv[2]);
                    write(sockfd, buff, strlen(buff));
                }
                if(sockfd == fd){
                    snprintf(buff, sizeof(buff), "[Server] SUCCESS: Your message has been sent.\n");
                    write(sockfd, buff, strlen(buff));
                }

             }
        }
    }
    else if(strcmp(argv[0],"yell")==0){
        int index;
        for(index = 0; index < maxfd; index++)
            if(fd == cli[index].client) break;

        for(int j = 0; j < maxfd ; j++){ 
            if((sockfd = cli[j].client )< 0) continue; 
                snprintf(buff, sizeof(buff), "[Server] %s yell %s\n", cli[index].name,argv[1]);
                write(sockfd, buff, strlen(buff));
        }
    }    
    else{
        snprintf(buff, sizeof(buff), "[Server] ERROR: Error command.\n");
        write(fd, buff, strlen(buff));
    }
    
    for(int j=0;j<argc;j++)
        free(argv[j]);
   // free(argv);
}

void some_quit(int fd){
    int index;
    for(index = 0; index < maxfd; index++)
        if(fd == cli[index].client) break;

    for( int j = 0; j < maxfd; j++){
        if((sockfd = cli[j].client) < 0) continue;
        snprintf(buff, sizeof(buff), "[Server] <%s> is offline.\n", cli[index].name);
        write(sockfd,buff,strlen(buff));
    }
}

int Split(char **argv,char *str, const char* token){
    char *arg;
    int argc=0;
    arg = strtok(str,token);//command
    if(strcmp(arg,"tell")==0){
            argv[argc] = (char*)malloc (strlen(arg) + 1);
            strcpy (argv[argc], arg);//tell
            argc++;
            arg = strtok(NULL,token);
        if(arg!=NULL){
            argv[1] = (char*)malloc (strlen(arg) + 1);
            strcpy (argv[argc], arg);//<usrname>
            argc++;
            arg = strtok(NULL,"\n");
        }
        if(arg!=NULL){//<message>
            argv[argc] = (char*)malloc (strlen(arg) + 1);
            strcpy (argv[argc], arg);
            argc++;
        }
        else{//if tell <username> null
            argv[argc] = (char*)malloc (2);
            strcpy (argv[argc],"\0");
            argc++;
        }
      
    }
    else if(strcmp(arg,"yell")==0){
        argv[argc] = (char*)malloc (strlen(arg) + 1);
        strcpy (argv[argc], arg);
        argc++;
        arg = strtok(NULL,"\n");
        if(arg!=NULL){
            argv[argc] = (char*)malloc (strlen(arg) + 1);
            strcpy (argv[argc], arg);
            argc++;
        }
        else{//if yell null
            argv[argc] = (char*)malloc (2);
            strcpy (argv[argc],"\0");
            argc++;
        }
    }
    else{
        while(arg != NULL){
            argv[argc] = (char*)malloc (strlen(arg) + 1);
            strcpy (argv[argc], arg);
            argc++;
            arg = strtok(NULL, token);
        }
    }
   //     free(arg);
        return argc;
}

int isallalpha(const char *str){
    for(int i=0;i<(int)strlen(str);i++)
        if(!isalpha(str[i])) return 0;
    return 1;
}
int isexist(const char *str){
    for(int i=0;i<maxfd;i++){
        if(cli[i].client < 0) continue;
        if(strcmp(cli[i].name,str)==0) 
            return 1;
    }
    return 0;
}
