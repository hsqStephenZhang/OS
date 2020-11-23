#include<stdio.h>
#include<stdint.h>
#include <stdlib.h>

#include<sys/sem.h>//信号灯
#include<sys/shm.h>//共享内存
#include<sys/wait.h>

#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<signal.h>


#define BUFFERNUM 1024
#define DATASIZE 1000
#define STATUS_PENDING  0x01
#define STATUS_READ     0x02
#define STATUS_WRITTEN  0x04
#define STATUS_ALL      0x08
#define STATUS_HEAD     0x10
#define STATUS_TAIL     0x20
#define SIZE_HEADER 5


union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO */
};

void P(int semid, int index) {
    struct sembuf sem;
    sem.sem_num = index;
    sem.sem_op = -1;
    sem.sem_flg = SEM_UNDO;
    semop(semid, &sem, 1);
    return ;
}

void V(int semid, int index) {
    struct sembuf sem;
    sem.sem_num = index;
    sem.sem_op = 1;
    sem.sem_flg = SEM_UNDO;
    semop(semid, &sem, 1);
    return ;
}

typedef struct _my_share_memory{
    unsigned char status;
    uint32_t size;
    int  nextshm;
    char data[DATASIZE];
} MyShm;