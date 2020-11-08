#include <iostream>
#include <sys/sem.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
using namespace std;

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short int *array;
    struct seminfo *__buf;
};

int semid;
union semun arg0,arg1,arg_empty,arg_full;
int a = 0;
pthread_t  s0,s1,s_producer;
struct sembuf p0,v0,p1,v1,p_empty,v_empty,p_full,v_full;

void* subp1(void*i);
void* subp2(void*i);
void* producer(void *i);
void initial();

int main() {
    initial();

    arg0.val = 1;  // odd
    arg1.val = 0;  // even
    arg_empty.val = 1;   // when program start, buffer is empty
    arg_full.val = 0;
    semid = semget(IPC_PRIVATE,4,IPC_CREAT|0666);

    semctl(semid,0,SETVAL,arg0);
    semctl(semid,1,SETVAL,arg1);
    semctl(semid,2,SETVAL,arg_empty);
    semctl(semid,3,SETVAL,arg_full);

    pthread_create(&s_producer, NULL,producer,NULL);
    pthread_create(&s0, NULL,subp1,NULL);
    pthread_create(&s1, NULL,subp2,NULL);

    pthread_join(s_producer, NULL);
    pthread_join(s0, NULL);
    pthread_join(s1, NULL);
    pthread_exit(NULL);
}

void initial(){
    p0.sem_num = 0;
    p0.sem_op = -1;
    p0.sem_flg = SEM_UNDO;

    p1.sem_num = 1;
    p1.sem_op = -1;
    p1.sem_flg = SEM_UNDO;

    p_empty.sem_num = 2;
    p_empty.sem_op = -1;
    p_empty.sem_flg = SEM_UNDO;

    p_full.sem_num = 3;
    p_full.sem_op = -1;
    p_full.sem_flg = SEM_UNDO;

    v0.sem_num = 0;
    v0.sem_op = 1;
    v0.sem_flg = SEM_UNDO;

    v1.sem_num = 1;
    v1.sem_op = 1;
    v1.sem_flg = SEM_UNDO;

    v_empty.sem_num = 2;
    v_empty.sem_op = 1;
    v_empty.sem_flg = SEM_UNDO;

    v_full.sem_num = 3;
    v_full.sem_op = 1;
    v_full.sem_flg = SEM_UNDO;
}
void* subp1(void *i){
    for(int j=0;j<=100;j++){
        sleep(1);

        semop(semid,&p0,1);
        semop(semid,&p_full,1);

        cout<< "thread1:"<<a << endl;

        semop(semid,&v_empty,1);
        semop(semid,&v1,1);
    }
    semctl(semid,IPC_RMID,0);
    return 0;
}

void* subp2(void *i){
    for(int n=0;n<=100;n++){
        sleep(1);

        semop(semid,&p1,1);
        semop(semid,&p_full,1);

        cout<< "thread2:"<<a<<endl;

        semop(semid,&v_empty,1);
        semop(semid,&v0,1);
    }
    return 0;
}

void* producer(void *i){
    for(int n=0;n<=100;n++){
        sleep(1);
        semop(semid,&p_empty,1);            /*占有临界资源*/
        a++;
        cout<< "producer produce a:"<<a<<endl;
        semop(semid,&v_full,1);            /*释放临界资源*/
    }
    return 0;
}