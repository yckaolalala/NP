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
#include <sys/stat.h>

#define	SA	struct sockaddr
#define	MAXLINE		1024	/* max text line length */
#define MAXBUFF     6000
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
    char buff[MAXLINE+16];
};

int check_end(int total){
    for(int i=0;i<total;i++)
        if(check[i]==0)
            return 0;
    return 1;
}

void dg_cli(int sockfd, const SA *pservaddr, socklen_t servlen,char *name)
{
    int     n,nrecv;
    char    sendline[MAXLINE], recvline[MAXLINE + 1];
    FILE* fp=fopen(name,"r");
    char Fin[16];
    struct Frame data[MAXBUFF];
    struct Header fin;
    struct Header ack;
    fin.fin=1;
    memcpy(Fin,&fin,sizeof(fin));

    if(fp){
        int file_fd=fileno(fp);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 50000;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        int index=0;
        while(1){//read file
            memset(&sendline,0,MAXLINE);
            n=read(file_fd,sendline,MAXLINE);
            data[index].ptr.fin=0;
            data[index].ptr.index=index;
            data[index].ptr.total=0;
            data[index].ptr.size=n;
            // memcpy(data[index].buff,&data[index].ptr,sizeof(data[index].ptr));
            memcpy(data[index].buff+sizeof(data[index].ptr),&sendline,n);
            index++;
            if(n==0)
                break;
        }

        for(int i=0;i<index;i++){//add total 
            data[i].ptr.total=index;
            memcpy(data[i].buff,&data[i].ptr,sizeof(data[i].ptr));
        }

        while(!check_end(index)){
            for(int i=0;i<index;i++){
                if(check[i]!=1)
                    write(sockfd,data[i].buff,data[i].ptr.size+sizeof(data[i].ptr));
                if(index%1000==0)
                    usleep(1);

            }
            while(1){
                nrecv = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
                if(nrecv<0){
                    if(errno==EWOULDBLOCK){
                        break;
                    }
                }
                memcpy(&ack,recvline,nrecv);
                check[ack.index]=1;
        //        printf("get ack %d\n",ack.index);
            }

        }

        while(1){//send fin
            sendto(sockfd,Fin,sizeof(Fin),0,pservaddr,servlen);
            sendto(sockfd,Fin,sizeof(Fin),0,pservaddr,servlen);
            sendto(sockfd,Fin,sizeof(Fin),0,pservaddr,servlen);
            nrecv = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
            if(nrecv<0){
                if(errno==EWOULDBLOCK){
                    continue;
                }
            }
            recvline[nrecv] = 0;    
            break;
        }

        fclose(fp);
    }

}
int main(int argc, char **argv)
{
    int     sockfd;
    struct sockaddr_in servaddr;

    if(argc != 4){
        printf("usage: <IPaddress> <port> <filename>\n");
        return 0;
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(connect(sockfd,(SA *) &servaddr,sizeof(servaddr))==-1){
        printf("Connect error\n");
        exit(1);
    }

    for(int i=0;i<MAXBUFF;i++)
        check[i]=0;
    dg_cli(sockfd, (SA *) &servaddr, sizeof(servaddr),argv[3]);

    return 0;
}
