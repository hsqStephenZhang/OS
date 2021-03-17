#include<stdio.h>
#include <iostream>

#include<sys/sem.h>//信号灯
#include<sys/shm.h>//共享内存
#include<sys/wait.h>

#include<unistd.h>


#define BUFFERNUM 10
#define SHMKEY 0x222

using namespace std;

union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO */
};//信号灯集

char buffer[BUFFERNUM]={0};

void P(int semid,int index){
    struct sembuf sem{};
    sem.sem_num=index;
    sem.sem_op=-1;
    sem.sem_flg=0;
    semop(semid,&sem,1);
}

void V(int semid,int index){
    struct sembuf sem{};
    sem.sem_num=index;
    sem.sem_op=1;
    sem.sem_flg=0;
    semop(semid,&sem,1);
}

void readProcess(int semid,int shmid,int pid){
    char* s=(char*)shmat(shmid,0,0);
    int out=0;
    while(1){
        printf("pid%d read",pid);
        P(semid,1);
        P(semid,2);
        printf("-->%c\n",*(s+out));
        out=(out+1)%BUFFERNUM;
        V(semid,2);
        V(semid,0);

        sleep(1);
    }
    if (shmdt(shmat(shmid, 0, 0))==-1){
        printf("delete failed");
    }
}


void writeProcess(int semid,int shmid){
    char* s=(char*)shmat(shmid,0,0);
    int in=0;
    while(1){
        fflush(stdout);
        P(semid,0);
        *(s+in)='0'+in;
        cout<<"write---->"<<*(s+in)<<"\n";
        in=(in+1)%BUFFERNUM;
        V(semid,1);
        sleep(1);
    }
    if (shmdt(shmat(shmid, 0, 0))==-1){
        printf("delete failed");
    }
}

int main(){
    printf("in main\n");
    int semid;
    semid = semget(IPC_PRIVATE, 3, IPC_CREAT | 0666);//创建三个信号灯.
    union semun wp; // write process's size
    union semun rp;  // read process's size
    union semun mutex;  // read/write mutex signal
    wp.val = BUFFERNUM;
    rp.val = 0;
    mutex.val = 1;
    if (semctl(semid, 0, SETVAL, wp) == -1 ||
        semctl(semid, 1, SETVAL, rp) == -1 ||
        semctl(semid, 2, SETVAL, mutex)  == -1) {
        perror("IPC error 1: semctl");
        exit(1);
    }

    int shmid;
    if((shmid=shmget(SHMKEY, sizeof(buffer), IPC_CREAT | 0666)) < 0){
        return  -1;
    }

    int pid1,pid2,pid3;
    if ((pid3=fork())==0){
        writeProcess(semid, shmid);
        return 0;
    }
    if ((pid1=fork())==0){
        readProcess(semid, shmid, 1);
        return 0;
    }
    if ((pid2=fork())==0){
        readProcess(semid, shmid, 2);
        return 0;
    }

    wait(&pid1);
    wait(&pid2);
    wait(&pid3);

//    shmctl(shmid,IPC_RMID,0);

    return 0;

}
