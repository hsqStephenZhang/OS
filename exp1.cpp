#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
using namespace std;

int pipefd[2];
int num=1;
int MAX_NUM=10;
int receive_num1=0,receive_num2=0;
pid_t s1,s2;

void my_func(int);//全部杀死
void kill_s1(int);//s1 kill
void kill_s2(int);//s2 kill

int main() {
    signal(SIGINT,my_func);//父进程控制进程终止
    char buffer[128];//管道缓冲区
    pipe(pipefd);//管道初始化


    s1 = fork();//子进程s1创建

    if(s1 == 0){//s1子进程运行
        char receive1[128]={0};
        signal(SIGINT,SIG_IGN);
        signal(SIGUSR1,kill_s1);
        while(1) {
            close(pipefd[1]);
            read(pipefd[0], receive1, 128);
            cout << " s1--->"<< receive1 ;
            receive_num1++;
        }
    }
    
    else {//父进程执行
        s2 = fork();//s2子进程创建
        
        if(s2 == 0){//s2子进程

            char receive2[128]={0};
            signal(SIGINT,SIG_IGN);
            signal(SIGUSR2,kill_s2);

            while(1) {
                close(pipefd[1]);
                read(pipefd[0], receive2, 128);
                cout << " s2--->"<< receive2 ;
                receive_num2++;
            }
        }
        
        else {//父进程执行

            signal(SIGINT,my_func);

            while(num<=MAX_NUM){//写入缓冲区,通过管道读取
                sprintf(buffer, "I send you %d times\n", num);
                close(pipefd[0]);
                write(pipefd[1] , buffer , 128);
                sleep(1);//间隔一秒
                num++;
            }
            pid_t  p;

            kill(getpid(),SIGINT);

            waitpid(s1, nullptr,0);
            waitpid(s2, nullptr,0);
            close(pipefd[0]);
            close(pipefd[1]);
        }
    }
   
}

void my_func(int sig_no)
{
    signal(SIGINT,SIG_DFL);
    kill(s1,SIGUSR1);
    kill(s2,SIGUSR2);

    cout<<"Parent Process is Killed"<<endl;
    cout << "total send : "<< num-1 <<" times"<<endl;
}
void kill_s1(int sig_no) {
    cout << "Child Process 1 is Killed by Parent!" << endl;
    cout << "pro1 receive : "<< receive_num1 <<" times"<<endl;
    exit(1);
}
void kill_s2(int sig_no){
    cout << "Child Process 2 is Killed by Parent!" << endl;
    cout << "pro2 receive : "<< receive_num2 <<" times"<<endl;
    exit(1);
}