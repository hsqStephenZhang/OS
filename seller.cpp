#include <iostream>
#include <sys/sem.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
using namespace std;

#define num_seller 6

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short int *array;
    struct seminfo *__buf;
};


int semid;
union semun arg0;
int a = 0;
pthread_t  sellers[num_seller],buyer;
struct sembuf p0,v0,p1,v1;

int total=20;

void* subp1(void*i);
void* subp2(void*i);
void initial();

int main() {
    initial();

    arg0.val=1;
    semid = semget(IPC_PRIVATE,2,IPC_CREAT|0666);
    semctl(semid,0,SETVAL,arg0);

    int indexs[num_seller]={0,1,2,3,4,5};
    for (int i = 0; i < num_seller; ++i) {
        pthread_create(sellers+i, NULL,subp1,(void*)(indexs+i));
    }
//    pthread_create(&buyer, NULL,subp1,NULL);

    for (int i = 0; i < num_seller; ++i) {
        pthread_join(sellers[i], NULL);
    }
//    pthread_join(buyer, NULL);

    pthread_exit(NULL);
}

void initial(){
    p0.sem_num = 0;
    p0.sem_op = -1;
    p0.sem_flg = SEM_UNDO;

    v0.sem_num = 0;
    v0.sem_op = 1;
    v0.sem_flg = SEM_UNDO;

    p1.sem_num = 1;
    p1.sem_op = -1;
    p1.sem_flg = SEM_UNDO;

    v1.sem_num = 1;
    v1.sem_op = 1;
    v1.sem_flg = SEM_UNDO;

}
void* subp1(void *i){
    int total_sell=0;
    int thread_num=*(int *)i;
    while (total!=0){
        sleep(1);
        semop(semid,&p0,1);            /*占有临界资源*/
        if (total!=0){
            total--;
            total_sell++;
            printf("in thread %d,ticket left: %d\n",thread_num,total);
        }
        semop(semid,&v0,1);            /*释放临界资源*/
    }
    semctl(semid,IPC_RMID,0);//撤销信号量集
    printf("thread %d,total sell:%d\n",thread_num,total_sell);
    return 0;
}