#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>

int main(){
    pid_t pid1;
    char string[] ="hello,pipeline";

    char readBuffer[80]={0};

    int fd[2];
    int *read_fd=fd;
    int *writer_df=fd+1;

    int result=pipe(fd);

    if (result==-1){
        printf("failed to create pipe line\n");
    }

    pid1=fork();

    if (pid1==-1){
        printf("failed to create son process\n");
    }

    if (pid1==0){
        close(*read_fd);
        result=write(*writer_df,string,strlen(string));
        printf("this is father\n");
        return 0;
    } else{
        close(*writer_df);
        int nbytes=read(*read_fd,readBuffer, sizeof(readBuffer)-1);
        printf("this is son read ---->  %s",readBuffer);
    }

    return 0;
}