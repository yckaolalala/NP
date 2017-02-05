#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "file.h"

#define CMD_MAX 1000
#define MAXLINE 4096
int wflag=0;

int Split(char **argv,char *str,const char* token);

int main(int argc,char *argv[]){
    if(argc!=3){
        fprintf(stderr,"Usage: %s <server_host> <port>\n",argv[0]);
        exit(1);
    }
    struct hostent *host = gethostbyname(argv[1]);
    if(host ==NULL){
        fprintf(stderr,"Cannot get host by %s\n",argv[1]);
        exit(2);
    }
    char recvline[MAXLINE+1],sendline[MAXLINE];
    int n;
    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    memcpy(&server_addr.sin_addr,host->h_addr,host->h_length);

    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd == -1){
        fputs("Fail to create socket.\n",stderr);
        exit(2);
    }

    if(connect(sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr))==-1){
        fputs("Fail to connect.\n",stderr);
        exit(2);
    }

    fd_set rset;
    FD_ZERO(&rset);
    int stdineof=0;
    int Argc=0;
    char *Argv[MAXLINE];
    int put_flag=0;
    int get_flag=0;
    char file_name[MAXLINE];
    while(1){

        if(stdineof==0)
            FD_SET(fileno(stdin),&rset);
        FD_SET(sockfd,&rset);
        if(select(sockfd+1,&rset,NULL,NULL,NULL)<0)
            exit(1);

        if(FD_ISSET(sockfd,&rset)){
            memset(&recvline,0,MAXLINE);
            n=read(sockfd,recvline,MAXLINE);
            recvline[n]=0;

            if(n==0){
                if(stdineof==1)
                    return 0;
                else{
                    //                    printf("Server close\n");
                    return 0;
                }
            }
            else if(recvline <0){
                printf("Erroe read from server\n");
                return 0;
            }
            else{
                if(atoi(recvline)==-1){
                    printf("Must Enter CLIENTID\n");
                }
                else if(atoi(recvline)==-2 || put_flag==-1){
                    printf("file not exist\n");
                }
                else{
                    if(put_flag==1){
                        char num[MAXLINE];
                        char *pch;
                        pch=strtok(recvline,"\n");          
                        strcpy(num,pch);
                        //printf("%d\n",atoi(num));
                        //
                        struct sockaddr_in ftp_addr;
                        memset(&ftp_addr,0,sizeof(ftp_addr));
                        ftp_addr.sin_family = AF_INET;
                        ftp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                        ftp_addr.sin_port = htons(atoi(num));

                        int ftp_fd;
                        ftp_fd = socket(AF_INET,SOCK_STREAM,0);
                        while(connect(ftp_fd,(struct sockaddr *)&ftp_addr,sizeof(ftp_addr))==-1){
                            printf("Fail to connec %d.\n",atoi(num));
                        }
                        send_file(ftp_fd,file_name);
                        char *arg=strtok(sendline,"\n");
                        printf("%s succeeded\n",arg);

                        put_flag=0;
                    }
                    else if(get_flag==1){
                        char num[MAXLINE];
                        char *pch;
                        pch=strtok(recvline,"\n");          
                        strcpy(num,pch);
                        //printf("%d\n",atoi(num));
                        //
                        struct sockaddr_in ftp_addr;
                        memset(&ftp_addr,0,sizeof(ftp_addr));
                        ftp_addr.sin_family = AF_INET;
                        ftp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                        ftp_addr.sin_port = htons(atoi(num));

                        int ftp_fd;
                        ftp_fd = socket(AF_INET,SOCK_STREAM,0);
                        while(connect(ftp_fd,(struct sockaddr *)&ftp_addr,sizeof(ftp_addr))==-1){
                            printf("Fail to connec %d.\n",atoi(num));
                        }
                        receive_file(ftp_fd,file_name);
                        char *arg=strtok(sendline,"\n");
                        printf("%s succeeded\n",arg);

                        get_flag=0;
                    }
                    else{
                        get_flag=0;
                        put_flag=0;
                        printf("%s",recvline);
                    }
                }

            }


        }

        if(FD_ISSET(fileno(stdin),&rset)){
            memset(&sendline,0,MAXLINE);
            if(fgets(sendline,sizeof(sendline),stdin)==NULL){
                stdineof=1;
                shutdown(sockfd,SHUT_WR);
                FD_CLR(fileno(stdin),&rset);
                continue;
            }

            //char cmd[MAXLINE];
            //strcmp(cmd,sendline);
            Argc=Split(Argv,sendline," \n");
            if(Argc!=0){
                put_flag=0;
                get_flag=0;
                
                if(strcmp(Argv[0],"PUT")==0 && Argc==3){
                    FILE* test=fopen(Argv[1],"r");
                    if(test){
                        put_flag=0;
                        fclose(test);
                    }
                    else{
                        put_flag=-1;
                        printf("file not exist\n");
                    }
                }
                    
                if(put_flag!=-1)
                    write(sockfd,sendline,strlen(sendline));

                if(strcmp(Argv[0],"PUT")==0 && Argc==3){
                    strcpy(file_name,Argv[1]);
                    put_flag=1;
                                    }
                else if(strcmp(Argv[0],"GET")==0 && Argc==3){
                    strcpy(file_name,Argv[2]);
                    get_flag=1;
                }
            }

        }

    }
}

int Split(char **argv,char *str,const char* token){
    char *arg;
    int argc=0;
    char str2[MAXLINE];
    strcpy(str2,str);

    arg=strtok(str2,token);
    while(arg!=NULL){

        argv[argc]=(char*)malloc (strlen(arg)+1);
        strcpy(argv[argc],arg);
        argc++;
        arg = strtok(NULL,token);
    }
    return argc;
}

//fread(buff,MAXLINE,sizeof(buff),fp);
//
