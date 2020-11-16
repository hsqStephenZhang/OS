#include<stdio.h>
#include<stdint.h>
#include<stdio.h>

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
};//信号灯集

void P(int semid, int index) {
    struct sembuf sem;
    sem.sem_num = index;
    sem.sem_op = -1;
    sem.sem_flg = SEM_UNDO;
    semop(semid, &sem, 1);
    return ;
}//p操作

void V(int semid, int index) {
    struct sembuf sem;
    sem.sem_num = index;
    sem.sem_op = 1;
    sem.sem_flg = SEM_UNDO;
    semop(semid, &sem, 1);
    return ;
}//v操作


static int read_proc = 0, write_proc = 0;

typedef struct _ShareBuffer {
    unsigned char status;
    uint32_t size;
    int  nextshm;
    char data[DATASIZE];
} ShareBuffer;

int read_pro(FILE *inFile, int shmhead, int shmtail, int semid) {
    ShareBuffer *bufferTail = (ShareBuffer *)shmat(shmtail, NULL, 0);
    while (1) {
        P(semid, 2);
        P(semid, 1);
        printf("Read\n");
        size_t bytesRead;
        if ((bytesRead = fread((void *)(bufferTail->data), 1, DATASIZE, inFile)) == 0) {
            bufferTail->status = STATUS_ALL;
            bufferTail->size = bytesRead;
            fclose(inFile);
            V(semid, 0);
            V(semid, 2);
            return 0;
        }
        bufferTail->size = bytesRead;
        shmtail = bufferTail->nextshm;
        bufferTail = (ShareBuffer *)shmat(shmtail, NULL, 0);
        V(semid, 0);//读入文件加1
        V(semid, 2);//释放权限
    }
}

int write_pro(FILE *outFile, int shmhead, int shmtail, int semid) {
    ShareBuffer *bufferhead = (ShareBuffer *)shmat(shmhead, NULL, 0);
    while (1) {
        P(semid, 2);
        P(semid, 0);
        printf("write\n");
        if (bufferhead->status == STATUS_ALL) {
            fwrite((void *)(bufferhead->data), bufferhead->size, 1, outFile);
            fclose(outFile);
            V(semid, 1);
            V(semid, 2);
            return 0;
        }
        fwrite((void *)(bufferhead->data), bufferhead->size, 1, outFile);
        shmhead = bufferhead->nextshm;
        bufferhead = (ShareBuffer *)shmat(shmhead, NULL, 0);
        V(semid, 1);
        V(semid, 2);
    }
}

int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("请在 ./main 后依次输入源文件与目标文件\n");
        return 1;
    }

    FILE *fileIn;
    FILE *fileOut;
    if ((fileIn = fopen(argv[1], "rb")) == NULL) {
        perror("无法读取源文件");
        return 1;
    }
    if ((fileOut = fopen(argv[2], "wb")) == NULL) {
        perror("无法创建打开目标文件");
        return 1;
    }

    key_t shmkey;
    if ((shmkey = ftok("./key", 'b')) == (key_t)(-1)) {
        perror("无法获取key值");
        exit(1);
    }//获取共享内存key值

    int shm_head;
    if ((shm_head = shmget(shmkey, sizeof(ShareBuffer), IPC_CREAT | 0666)) <= 0) {
        perror("无法创建共享缓冲区");
        exit(1);
    }//创建第一个head共享缓冲区

    ShareBuffer *shareBuffer = (ShareBuffer *)shmat(shm_head, NULL, 0);
    if ((int64_t)(shareBuffer) == -1) {
        perror("无法获取共享缓冲区");
        exit(1);
    }

    shareBuffer->status = STATUS_PENDING | STATUS_HEAD | STATUS_TAIL;

    for (int i = 0; i < BUFFERNUM; ++i) {
        key_t shmkey;
        if ((shmkey = ftok("./key", i)) == (key_t)(-1)) {
            perror("无法获取key值");
            exit(1);
        }
        int idShm;
        if ((idShm = shmget(shmkey, sizeof(ShareBuffer), IPC_CREAT | 0666)) <= 0) {
            perror("无法创建共享缓冲区");
            exit(1);
        }
        shareBuffer->nextshm = idShm;
        shareBuffer->status = STATUS_PENDING;
        shareBuffer = (ShareBuffer *)shmat(idShm, NULL, 0);
        if ((int64_t)(shareBuffer) == -1) {
            perror("无法获取共享缓冲区");
            exit(1);
        }
    }

    shareBuffer->nextshm = shm_head;//将最后一个缓冲区链接到头缓冲区构成循环

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

    //创建读文件子进程
    if ((read_proc = fork()) == -1) {
        perror("Failed to create process.");
        return 1;
    }
    if (read_proc == 0){
        //调用read_pro函数读取文件,并放入缓冲区
        return read_pro(fileIn, shm_head, shm_head, semid);
    }

    //创建写文件子进程
    if ((write_proc = fork()) == -1) {
        perror("Failed to create process.");
        kill(read_proc, SIGKILL);//当read进程结束时,销毁write进程
        return 1;
    }
    if (write_proc == 0){
        //调用write_pro函数将文件写入.
        return write_pro(fileOut, shm_head, shm_head, semid);
    }


    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("销毁信号灯失败\n");
    }//销毁信号灯
    if (shmctl(shm_head, IPC_RMID, NULL) == -1) {
        perror("销毁共享内存失败\n");
    }//销毁共享内存
    return 0;
}

