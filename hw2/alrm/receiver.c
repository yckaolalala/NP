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
#include <signal.h>

#define	SA	struct sockaddr
#define	MAXLINE		1040	/* max text line length */
#define	MAXBUFF		6000	
#define LISTENQ 20

int check[MAXBUFF];
struct Header{
    int fin;
    int index;
    int total;
    int size;
};
struct Frame{
    struct Header ptr;
    char buff[MAXLINE];
};

static void sig_alrm(int signo){
        return;
}

int check_end(int total){
    for(int i=0;i<total;i++)
        if(check[i]==0)
            return 0;
    return 1;
}

void dg_write(int sockfd, SA *pcliaddr, socklen_t clilen,char *name)
{
    int     n;
    socklen_t len;
    char    mesg[MAXLINE];
    char buff[MAXLINE];
    char wbuff[MAXLINE];
    char prebuff[MAXLINE];
    char Ack[16];
    FILE* fp=fopen(name,"w");
    struct Frame data[MAXBUFF];
    struct Header header,ack;

    
    int total;
    if(fp){
        int file_fd=fileno(fp);
        while(1){
            len = clilen;
            memset(&buff,0,MAXLINE);
            n = recvfrom(sockfd, buff, MAXLINE, 0, pcliaddr, &len);
            memcpy(&header,buff,sizeof(header));
            if(header.fin==0){
                memcpy(&data[header.index].ptr,buff,sizeof(header));
                memcpy(data[header.index].buff,buff+sizeof(header),data[header.index].ptr.size);
                total=header.total;
                memset(&ack,0,16);
                ack.index=header.index;
                memcpy(Ack,&ack,sizeof(ack));
                sendto(sockfd, Ack,sizeof(Ack) , 0, pcliaddr, len);
            }
            if(header.fin==1 && check_end(total)){  
                for(int i=0;i<total;i++){
                   write(file_fd,data[i].buff,data[i].ptr.size);
                }

                signal(SIGALRM,sig_alrm);
                siginterrupt(SIGALRM,1);
               while(1){//wait_close
                    alarm(2);
                    memset(&ack,0,16);
                    ack.index=header.index;
                    memcpy(Ack,&ack,sizeof(ack));
                    sendto(sockfd, Ack,sizeof(Ack) , 0, pcliaddr, len);
					if((n = recvfrom(sockfd, buff, MAXLINE, 0, pcliaddr, &len)<0)){
                        exit(0);
					}
                    else{
                        alarm(0);
                    }
                }
            }
            else if(check[header.index]==0){
                check[header.index]=1;
            }
        }
        fclose(fp);
    }  
} 
int main(int argc, char **argv)
{
    int     sockfd;
    struct sockaddr_in servaddr, cliaddr;

    if(argc!=3){
        printf("usage:<port> <filename>\n");
        return 0;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    if(bind(sockfd, (SA *) &servaddr, sizeof(servaddr))==-1){
        printf("bind fail\n");
        exit(2);
    }
    
    for(int i=0;i<MAXBUFF;i++){
        check[i]=0;
    }
    dg_write(sockfd, (SA *) &cliaddr, sizeof(cliaddr),argv[2]);
    close(sockfd);
    return 0;
}


