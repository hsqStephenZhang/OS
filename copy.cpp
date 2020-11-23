#include "head.h"

static int read_proc = 0, write_proc = 0;

int main(int argc,char* argv[]){
    int shm_head;
    if ((shm_head = shmget(IPC_PRIVATE, sizeof(MyShm), IPC_CREAT | 0666)) <= 0) {
        perror("无法创建共享缓冲区");
        exit(1);
    }

    MyShm *shareBuffer = (MyShm *)shmat(shm_head, NULL, SHM_W);
    if ((int64_t)(shareBuffer) == -1) {
        perror("无法获取共享缓冲区");
        exit(1);
    }

    shareBuffer->status = STATUS_PENDING | STATUS_HEAD | STATUS_TAIL;

    for (int i = 0; i < BUFFERNUM; ++i) {
        int idShm;
        if ((idShm = shmget(IPC_PRIVATE, sizeof(MyShm), IPC_CREAT | 0666)) <= 0) {
            perror("无法创建共享缓冲区");
            exit(1);
        }
        shareBuffer->nextshm = idShm;
        shareBuffer->status = STATUS_PENDING;
        shareBuffer = (MyShm *)shmat(idShm, NULL, 0);
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
    wp.val = BUFFERNUM+1;
    rp.val = 0;
    mutex.val = 1;
    if (semctl(semid, 0, SETVAL, wp) == -1 ||
        semctl(semid, 1, SETVAL, rp) == -1 ||
        semctl(semid, 2, SETVAL, mutex)  == -1) {
        perror("IPC error 1: semctl");
        exit(1);
    }

    char*argvs1[4]={0};
    char*argvs2[4]={0};

    char semid_char[30]={0};
    char shm_head_char[30]={0};

    // fileNames
    argvs1[0]=argv[1];
    argvs2[0]=argv[2];
    sprintf(shm_head_char,"%d",shm_head);
    sprintf(semid_char,"%d",semid);
    argvs1[1]=shm_head_char;
    argvs2[1]=shm_head_char;
    argvs1[2]=semid_char;
    argvs2[2]=semid_char;

    if ((read_proc = fork()) == -1) {
        perror("Failed to create process.");
        return 1;
    }
    if (read_proc == 0){
        //调用read_pro函数读取文件,并放入缓冲区
        printf("read\n");
        execv("./get",argvs1);
        return 0;
    }
    if ((write_proc = fork()) == -1) {
        perror("Failed to create process.");
        kill(read_proc, SIGKILL);//当read进程结束时,销毁write进程
        return 1;
    }
    if (write_proc == 0){
        printf("put\n");
        //调用write_pro函数将文件写入.
        execv("./put",argvs2);
        return 0;
    }

//    int nextshm=shm_head;
//    for (int i = 0; i < BUFFERNUM; ++i) {
//        shareBuffer = (MyShm *)shmat(nextshm, NULL, SHM_W);
//        shmctl(nextshm,IPC_RMID,0);
//        if ((int64_t)(shareBuffer) == -1) {
//            perror("无法获取共享缓冲区");
//            exit(1);
//        }
//        nextshm=shareBuffer->nextshm;
//    }

    semctl(semid,IPC_RMID,0);//撤销信号量集

}