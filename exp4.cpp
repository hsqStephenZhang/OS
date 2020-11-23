#include<dirent.h>
#include<time.h>
#include<sys/stat.h>
#include<string.h>
#include<grp.h>
#include<pwd.h>
#include<libgen.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<locale.h>
#include <locale>

#define LS_BLOCK_SIZE 1024


int printdir(char *dir, int depth); // print recursively
int printdetail(struct dirent *entry, int fmtLinkWidth, int fmtSizeWidth); // print details
int sortdir(const void *a, const void *b); // sort



int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("please input the name of dir after ./main \n");
        return 1;
    }
    char *path = (char*)malloc(strlen(argv[1]) + 1);
    strcpy(path, argv[1]);
    printdir(path, 0);
    free(path);
}

static time_t now;//时间

//递归全部目录
int printdir(char *dir, int depth) {
    now = time(NULL);
    char *oldir = NULL;
    long currentPathSize = pathconf(".", _PC_PATH_MAX);
    oldir = getcwd(oldir, currentPathSize);

    char *dirTemp = (char*)malloc(strlen(dir) + 1);
    strcpy(dirTemp, dir);
    char *dirBase = basename(dirTemp);

    struct stat temp_stat;
    struct stat link_stat;
    stat(dirBase, &temp_stat);
    lstat(dirBase, &link_stat);
    long blockTotalCount = 0;
    DIR *nowdir = NULL;

    if (dir[0] == '/') {
        if ((nowdir = opendir(dir)) == NULL) {
            perror("Failed to open directory");
            return 1;
        }
        chdir(dir);//cd
    } else {
        if ((nowdir = opendir(dirBase)) == NULL) {
            perror("Failed to open directory");
            return 1;
        }
        chdir(dirBase);//cd
    }

    printf("%s(depth:%d):\n", dir,depth);

    struct dirent *entryItems;
    struct dirent *entryTemp;
    size_t entryCount = 0;
    while ((entryTemp = readdir(nowdir)))
        entryCount++;

    // allocate space of each items in this directory
    entryItems = (struct dirent *)malloc(entryCount * sizeof(struct dirent));
    rewinddir(nowdir);

    //目录排序
    for (int i = 0; (entryTemp = readdir(nowdir)); ++i)
        memcpy(entryItems + i, entryTemp, sizeof(struct dirent));
    qsort(entryItems, entryCount, sizeof(struct dirent), sortdir);

    int sizeWidth = 0, linkWidth = 0;
    struct stat status;

    // 统计块大小信息
    for (int i = 0; i < entryCount; ++i) {
        entryTemp = entryItems + i;
        stat(entryTemp->d_name, &status);

        if (entryTemp->d_name[0] != '.' &&
            strcmp(entryTemp->d_name, ".") &&
            strcmp(entryTemp->d_name, "..") ) {
            blockTotalCount += status.st_blocks;
        }
    }


    printf("total %lu\n", blockTotalCount * 512 / LS_BLOCK_SIZE );
    rewinddir(nowdir);

    // 输出文件details
    for (int i = 0; i < entryCount; ++i) {
        entryTemp = entryItems + i;
        // skip current and upper directory
        if (strcmp(entryTemp->d_name, ".") &&           // pass current file
            strcmp(entryTemp->d_name, "..") &&          // pass upper file
            entryTemp->d_name[0] != '.')                // pass hidden file
            printdetail(entryTemp, linkWidth, sizeWidth);
    }
    rewinddir(nowdir);

    // 递归输出文件夹的子结构
    for (int i = 0; i < entryCount; ++i) {
        entryTemp = entryItems + i;
        if (entryTemp->d_type == DT_DIR &&
            entryTemp->d_name[0] != '.' &&
            strcmp(entryTemp->d_name, ".") &&
            strcmp(entryTemp->d_name, "..") ) {

            // construct path
            char *dirTemp = (char *)malloc(strlen(dir) + strlen(entryTemp->d_name) + 2);
            strcpy(dirTemp, dir);
            if (dir[strlen(dir) - 1] != '/')
                strcat(dirTemp, "/");
            strcat(dirTemp, entryTemp->d_name);
            putchar('\n');

            printdir(dirTemp, depth + 1);

            free(dirTemp);
        }
    }
    closedir(nowdir);
    chdir(oldir);
    free(oldir);
    free(entryItems);
    return 0;
}

// print details
int printdetail(struct dirent *entry, int fmtLinkWidth, int fmtSizeWidth) {
    struct stat status;
    if (lstat(entry->d_name, &status) == 0) {
        struct passwd *passwdTemp;
        struct group *groupTemp;
        passwdTemp = getpwuid(status.st_uid);
        groupTemp = getgrgid(status.st_gid);
        struct tm *timeTemp = localtime(&(status.st_mtime));
        char strTime[20];
        // 最后修改内容的时间
        if (now - status.st_mtime > (365 * 24 * 60 * 60 / 2) || now < status.st_mtime) {
            strftime(strTime, 20, "%b %_d  %Y", timeTemp);
        } else {
            strftime(strTime, 20, "%b %_d %H:%M", timeTemp);
        }
        // 文件的类型 (普通/链接文件)
        char mode='-';
        if(S_ISDIR(status.st_mode)) mode='d';
        if(S_ISLNK(status.st_mode)) mode='l';

        printf("%c",mode);
        printf( (status.st_mode & S_IRUSR) ? "r" : "-");
        printf( (status.st_mode & S_IWUSR) ? "w" : "-");
        printf( (status.st_mode & S_IXUSR) ? "x" : "-");
        printf( (status.st_mode & S_IRGRP) ? "r" : "-");
        printf( (status.st_mode & S_IWGRP) ? "w" : "-");
        printf( (status.st_mode & S_IXGRP) ? "x" : "-");
        printf( (status.st_mode & S_IROTH) ? "r" : "-");
        printf( (status.st_mode & S_IWOTH) ? "w" : "-");
        printf( (status.st_mode & S_IXOTH) ? "x" : "-");
        printf(" %-1ld %-5s %-5s %-4ld %s %s",
               status.st_nlink,
               passwdTemp->pw_name,
               groupTemp->gr_name,
                status.st_size,
               strTime,
               entry->d_name);
        if(mode=='l'){
            stat(entry->d_name, &status);
            // unable to get the linked file
            printf("\n");
        }else{
            printf("\n");
        }
        return 0;
    }
    return 1;
}

// sort the files by their names
int sortdir(const void *a, const void *b) {
    struct dirent *dirA = (struct dirent *) a,
            *dirB = (struct dirent *)b;
    return strcasecmp((dirA->d_name), dirB->d_name)>0;
}