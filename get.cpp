#include "head.h"

int main(int argc,char *argv[]){
    if (argc<3){
        printf("wrong num args");
        return 1;
    }

    int shmtail;
    int semid;
    FILE *inFile;

    shmtail = atoi(argv[1]);
    semid = atoi(argv[2]);

//    printf("in get %d,%s\n",shmtail,argv[1]);
//    printf("in get %d,%s\n",semid,argv[2]);

    if ((inFile = fopen(argv[0], "rb")) == NULL) {
        perror("无法创建打开目标文件\n");
        return 1;
    }else{
        printf("打开文件:%s成功\n",argv[0]);
    }

    ShareBuffer *bufferTail = (ShareBuffer *)shmat(shmtail, NULL, SHM_W);
    if (bufferTail==(ShareBuffer*)(-1)){
        printf("error\n");
    }
    while (1) {
        printf("int get,signal in \n");
        P(semid, 2);
        P(semid, 1);
        printf("Read  hhhhhhhhhhhhhhhh\n");
        size_t bytesRead;
        if ((bytesRead = fread((void *)(bufferTail->data), 1, DATASIZE, inFile)) == 0) {
            printf("end of file\n");
            bufferTail->status = STATUS_ALL;
            bufferTail->size = bytesRead;
            V(semid, 0);
            V(semid, 2);
            fclose(inFile);
            return 0;
        }
        bufferTail->size = bytesRead;
        shmtail = bufferTail->nextshm;
        bufferTail = (ShareBuffer *)shmat(shmtail, NULL, 0);
        printf("release signal\n");
        V(semid, 0);
        V(semid, 2);
    }
}