#ifndef FILE_H_   /* Include guard */
#define FILE_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

size_t getFilesize(const char* filename) {
    struct stat st;
    if(stat(filename, &st) != 0) {
        return 0;
    }
    return st.st_size;   
}

int send_file(int fd,char *name){
    FILE* fp=fopen(name,"r");
    int flag=0;
    if(fp){
       int file_fd=fileno(fp);
       int bytes=getFilesize(name);
       char *sendbuff = (char*)malloc (bytes);
       int n=read(file_fd,sendbuff,bytes);
//        printf("send %s\n",sendbuff);
       if(n!=0){
           write(fd,sendbuff,n) ;
           flag=-1;
       } 
    }
	close(fd);
    fclose(fp);
    return flag;
};

int receive_file(int fd,char *name){
    FILE* fp=fopen(name,"w");
    char buff[4096];
	int flag=0;
    //printf("name %s\n",name);
    if(fp){
       int file_fd=fileno(fp);
       int n;
       while(1){
			memset(&buff,0,4096);
            n=read(fd,buff,4096);
   //         printf("recive %d\n",n);
            if(n==0){
				close(fd);
                break;
            }
            else{
      //          printf("recive %s\n",buff);
                write(file_fd,buff,n);
				if(n>0)
    				flag=1;
            }
       }
    }
	else{
		close(fd);
		flag=-1;
	}
    fclose(fp);
	return flag;

};


#endif
