#include "head.h"

int main(int argc,char *argv[]){
    if (argc<3){
        printf("wrong num args");
        return 1;
    }


    int shmhead;
    int semid;
    FILE *outFile;

    shmhead = atoi(argv[1]);
    semid = atoi(argv[2]);

//    printf("in put %d,%s\n",shmhead,argv[1]);
//    printf("in put %d,%s\n",semid,argv[2]);

    if ((outFile = fopen(argv[0], "wb")) == NULL) {
        perror("无法创建打开目标文件");
        return 1;
    } else{
        printf("打开文件:%s成功\n",argv[0]);
    }

    ShareBuffer *bufferhead = (ShareBuffer *)shmat(shmhead, NULL, SHM_R);
    while (1) {
        printf("int put,signal in \n");
        P(semid, 2);
        P(semid, 0);
        printf("write hhhhhhhh\n");
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