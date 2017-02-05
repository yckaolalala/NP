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
#include "file.h"

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
    char list [MAXLINE][50];
    int index;
    int id;
};
struct cli_list{
    char list [MAXLINE][50];
    int index;
    int id;
};

struct  cli_list List[FD_SETSIZE];
struct  cli_data cli[FD_SETSIZE];
int List_size=0;
int isnumber(const char *str);
int Split(char **argv, char *str, const char* token);
void cloud(int index,int fd,char *readline,char **argv,int argc);

int main(int argc, char **argv)
{
    struct sockaddr_in	servaddr;
    struct sockaddr_in  sin;//cliaddr
    socklen_t len; // chiaddr len

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(atoi(argv[1]));	/* daytime server */

    bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

    listen(listenfd, LISTENQ);

    maxfd =listenfd;
    for(i=0;i<FD_SETSIZE;i++){
        cli[i].client=-1;
        cli[i].id=0;
        cli[i].index=0;
        List[i].id=0;
        List[i].index=0;
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


            for (i = 0; i < FD_SETSIZE; i++){
                if (cli[i].client < 0) {
                    cli[i].client=connfd;
                    cli[i].index=0;
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


                    cli[i].client = -1;
                    cli[i].index=0;
                    cli[i].id=0;
                } else{
                    //
                    char *Argv[MAXLINE];
                    int Argc;
                    cloud(i,sockfd,buff,Argv,Argc);
                    for(int j=0;j<Argc;j++)
                        free(Argv[j]);
                }

                if (--nready <= 0)
                    break;              /* no more readable descriptors */
            }
        }


        //close(connfd);
    }
}


void cloud(int index,int fd,char *readline,char **argv,int argc){
    char send[MAXLINE];
    argc=Split(argv,readline," \n");

    if(strcmp(argv[0],"GET")==0 && argc==3 ){
        if(cli[index].id==0){
            snprintf(send,sizeof(send),"-1");
            write(fd,send,strlen(send));
            //shutdown(fd,SHUT_WR);
        }else{
            char path[100];
            memset(&path,0,sizeof(path));
            snprintf(path,sizeof(path),"./%d/%s",cli[index].id,argv[1]);

            FILE* test=fopen(path,"r");
            if(test){
                struct sockaddr_in ftp_addr;
                int port=9870;
                memset(&ftp_addr,0,sizeof(ftp_addr));
                ftp_addr.sin_family = AF_INET;
                ftp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                ftp_addr.sin_port = htons(port);

                int ftp_fd;
                ftp_fd = socket(AF_INET,SOCK_STREAM,0);

                while(bind(ftp_fd,(struct sockaddr *)&ftp_addr,sizeof(ftp_addr))!=0){
                    port++;
                    ftp_addr.sin_port = htons(port);
                    ftp_fd = socket(AF_INET,SOCK_STREAM,0);
                }
                // printf("%d\n",port);
                snprintf(send,sizeof(send),"%d\n",port);
                write(fd,send,strlen(send));

                int flag=0;
                listen(ftp_fd,LISTENQ);
                while(flag==0){
                    struct sockaddr_in ftp_client_addr;
                    int ftp_sock_len=sizeof(ftp_client_addr); 
                    int ftp_connfd=accept(ftp_fd,(struct sockaddr *)&ftp_client_addr,&ftp_sock_len);
                    flag=send_file(ftp_connfd,path);
                    // printf("%d\n",flag);
                }
                close(ftp_fd);
            }
            else{
                snprintf(send,sizeof(send),"-2");
                write(fd,send,strlen(send));

            }
        }
    }
    else if(strcmp(argv[0],"PUT")==0 && argc ==3 ){
        if(cli[index].id==0){
            snprintf(send,sizeof(send),"-1");
            write(fd,send,strlen(send));
            //shutdown(fd,SHUT_WR);
        }
        else{



            for(int i=0;i<List_size;i++){
                if(List[i].id==cli[index].id){
                    strcpy(List[i].list[List[i].index++],argv[2]);
                    break;
                }
            }

            struct sockaddr_in ftp_addr;
            int port=8787;
            memset(&ftp_addr,0,sizeof(ftp_addr));
            ftp_addr.sin_family = AF_INET;
            ftp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
            ftp_addr.sin_port = htons(port);

            int ftp_fd;
            ftp_fd = socket(AF_INET,SOCK_STREAM,0);

            while(bind(ftp_fd,(struct sockaddr *)&ftp_addr,sizeof(ftp_addr))!=0){
                port++;
                ftp_addr.sin_port = htons(port);
                ftp_fd = socket(AF_INET,SOCK_STREAM,0);
            }
            //  printf("%d\n",port);
            snprintf(send,sizeof(send),"%d\n",port);
            write(fd,send,strlen(send));

            int flag=0;
            listen(ftp_fd,LISTENQ);
            while(flag==0){
                struct sockaddr_in ftp_client_addr;
                int ftp_sock_len=sizeof(ftp_client_addr); 
                int ftp_connfd=accept(ftp_fd,(struct sockaddr *)&ftp_client_addr,&ftp_sock_len);
                char path[100];
                memset(&path,0,sizeof(path));
                snprintf(path,sizeof(path),"./%d/%s",cli[index].id,argv[2]);
                flag=receive_file(ftp_connfd,path);
                // printf("%d\n",flag);
            }
            close(ftp_fd);
        }
    }
    else if(strcmp(argv[0],"LIST")==0&& cli[index].id!=0){
        if(cli[index].id==0){
            snprintf(send,sizeof(send),"-1");
            write(fd,send,strlen(send));
            shutdown(fd,SHUT_WR);
        }else{

            for(int i=0;i<List_size;i++){
                if(List[i].id==cli[index].id){
                    for(int j=0;j<List[i].index;j++){
                        snprintf(send,sizeof(send),"%s\n",List[i].list[j]);
                        write(fd,send,strlen(send));
                    }
                    break;
                }
            }
            snprintf(send,sizeof(send),"LIST succeeded\n");
            write(fd,send,strlen(send));
        }
    }
    else if(strcmp(argv[0],"CLIENTID")==0 && argc==2  && cli[index].id==0){
        //   printf("%d %d\n",atoi(argv[1]),cli[index].id);
        if(atoi(argv[1])<10000 || atoi(argv[1])>=100000){
            snprintf(send,sizeof(send),"CLIENTID must be five-digit number\n");
            write(fd,send,strlen(send));
            //shutdown(fd,SHUT_WR);
        }
        else{
            int flag=0;
            cli[i].id=atoi(argv[1]);
            for(int i=0;i<List_size;i++){
                if(List[i].id==cli[index].id){
                    flag=1;
                    break;
                }
            }
            if(flag==0){
                List[List_size++].id=cli[i].id;
                char array[40];
                memset(&array,0,sizeof(array));
                sprintf(array,"%d",cli[i].id);
                mkdir(array,0755);
            }
        }
    }
    else if(strcmp(argv[0],"EXIT")==0){
        shutdown(fd,SHUT_WR);
    }
    else{
        snprintf(send,sizeof(send),"ERROR\n");
        write(fd,send,strlen(send));

    }

    for(int i=0;i<argc;i++)      free(argv[i]);

}



int Split(char **argv,char *str,const char* token){
    char *arg;
    int argc=0;

    arg=strtok(str,token);
    while(arg!=NULL){

        argv[argc]=(char*)malloc (strlen(arg)+1);
        strcpy(argv[argc],arg);
        argc++;
        arg = strtok(NULL,token);
    }
    return argc;
}
int isnumber(const char *str){
    for(int i=0;i<(int)strlen(str);i++)
        if(!isdigit(str[i])) return 0;
    return 1;
}
