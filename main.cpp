#include <iostream>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

using namespace std;

const char WRITE_OPERATION[] ="write\0";
const char READ_OPERATION[] = "read\0";

void pathToFile(char *path, char *file)
{
    strcpy(path, file);
    for(int i = (int) strlen(file); i!=0; i--)
    {
        if (path[i] != '/')
        {
            path[i]='\0';
        }
        else
        {
            return;
        }
    }
}

void getFileName(char *fullPath,char *fileName)
{
    char path[255];
    pathToFile(path, fullPath);
    strcpy(fileName, fullPath + strlen(path));
}
bool findFile(char *dirPath, char *fileName)
{
    DIR *dir = opendir(dirPath);
    if (!dir) {
        perror("diropen");
        exit(1);
    };
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, fileName) == 0) {
            closedir(dir);
            return true;
        }
    }
    closedir(dir);
    return false;
}

void printLockFile(FILE *lockFile,const char *operationType, char *blockNumber)
{
    fprintf(lockFile, "%d ", getpid());
    fprintf(lockFile, "%s ", operationType);
    fprintf(lockFile, "%s\n", blockNumber);
}

struct Line
{
    int pid;
    char operationType[255];
    char blockNumber[255];
};
void parseLine(FILE *file, struct Line *line)
{
    fscanf(file, "%d %s %s", &line->pid, line->operationType, line->blockNumber);
}

int createLockFile(char *path, char *operationType, char *blockNumber) {
    char absoluteLockLockFilePath[strlen(path) + 4];
    strcpy(absoluteLockLockFilePath, path);
    strcpy(absoluteLockLockFilePath + strlen(absoluteLockLockFilePath), ".lck");
    char dirPath[255];
    pathToFile(dirPath, path);
    char lockLockFileName[255];
    getFileName(absoluteLockLockFilePath, lockLockFileName);
    while (findFile(dirPath, lockLockFileName))
    {
        printf("Кто-то залочил файл, Ждем..\n");
        sleep(1);
    }

    FILE *lockLockFile = fopen(absoluteLockLockFilePath, "w");
    printLockFile(lockLockFile, WRITE_OPERATION, blockNumber);
    FILE *lockFile = fopen(path, "r+");
    if (lockFile == NULL)
    {
        lockFile = fopen(path, "w");
    }
    else{
        return -2;
    }
    printLockFile(lockFile, operationType, blockNumber);
    fclose(lockFile);
    sleep(20);
    remove((char const *) absoluteLockLockFilePath);
    //sleep(20);
    return 1;
}

int checkLock(char *file, char *operationType, char *block) {
    char absoluteLockFileName[strlen(file) + 4];
    strcpy(absoluteLockFileName, file);
    strcpy(absoluteLockFileName + strlen(file), ".lck");
    printf("%s\n", absoluteLockFileName);
    char path[255];
    pathToFile(path,file);
    char lockFileName[255];
    getFileName(absoluteLockFileName, lockFileName);
    switch (createLockFile(absoluteLockFileName, operationType, block) ) {
        case 1: {
            printf("Успешно залочено\n");
            return 1;
        }
        case -1: {
            printf("Кто то лочит сейчас\n");
            return -1;
        }
        case -2: {
            printf("В данный момент файл кем то редактируется\n");
            return -1;
        }
        default:
            break;
    }
    return 1;
}

int fileOperation(char *path, char *numberOfBlock, char * operationType, char *name, char *password)
{
    int lineNumber = atoi(numberOfBlock);
    if(strcmp(operationType, READ_OPERATION) == 0)
    {
        FILE * file = fopen(path, "r");
        for(int i=0; i < lineNumber; i++)
        {
            if(feof(file))
            {
                return -1;
            }
            fscanf(file, "%s %s\n", name, password);
        }
        return 1;
     }
    if(strcmp(operationType, WRITE_OPERATION) == 0)
    {
        FILE *file = fopen(path, "r+");
        FILE *tempFile = fopen("/tmp/tempFile", "w+");
        char tempName[255];
        char tempPasswd[255];
        for(int i=0; i< lineNumber; i++)
        {
            fscanf(file, "%s %s\n", tempName, tempPasswd);
            fprintf(tempFile, "%s %s\n", tempName, tempPasswd);
        }
        fscanf(file, "%s %s\n", tempName, tempPasswd);
        fprintf(tempFile, "%s %s\n", name, password);
        while(!feof(file))
        {
            fscanf(file, "%s %s\n", tempName, tempPasswd);
            fprintf(tempFile, "%s %s\n", tempName, tempPasswd);
        }
        fclose(file);
        fclose(tempFile);
        file = fopen(path, "w");
        tempFile = fopen("/tmp/tempFile", "r");
        while(!feof(tempFile))
        {
            fscanf(tempFile, "%s %s\n", tempName, tempPasswd);
            fprintf(file, "%s %s\n", tempName, tempPasswd);
        }
        return 1;
    }
}
void unlock(char *path)
{
    char absoluteLockFileName[strlen(path) + 4];
    strcpy(absoluteLockFileName, path);
    strcpy(absoluteLockFileName + strlen(path), ".lck");
    FILE *lockFile = fopen(absoluteLockFileName, "r");
    FILE *tempFile = fopen("/tmp/tempLockFile", "w");
    int pid=getpid();
    int tempPid;
    char tempOperationType[255];
    char tempBlockNumber[255];
    while(!feof(lockFile)){
        fscanf(lockFile, "%d %s %s\n", &tempPid, tempOperationType, tempBlockNumber);
        if(tempPid != pid)
        {
            fprintf(tempFile, "%d %s %s\n", tempPid, tempOperationType, tempBlockNumber);
        }
    }
    fclose(lockFile);
    fseek(tempFile, 0, SEEK_END);
    long pos=ftell(tempFile);
    if(pos == 0)
    {
        remove((char const *) absoluteLockFileName);
        return;
    }
    fclose(tempFile);
    tempFile = fopen("/tmp/tempLockFile", "r");
    lockFile = fopen(absoluteLockFileName, "w+");
    while(!feof(tempFile))
    {
        fscanf(tempFile, "%d %s %s\n", &tempPid, tempOperationType, tempBlockNumber);
        fprintf(lockFile, "%d %s %s\n", tempPid, tempOperationType, tempBlockNumber);
    }
}
int main(int argc, char **argv) {
    switch (checkLock(argv[1], argv[3], argv[2])) {
        case -1 : {
            printf("%s", "Залочено");
            break;
        }
        case 1: {
            printf("%s\n", "Выполняем операцию с файлом");
            switch (fileOperation(argv[1], argv[2], argv[3], argv[4], argv[5])){
                case -1: {
                    printf("%s\n", "Не успех");
                    break;
                };
                case 1:{
                    break;
                }
                default:
                {
                    printf("%s\n", "Что то пошло не так");
                    break;
                }
            };
            break;
        }

        default:
            break;
    }
    unlock(argv[1]);
    return 0;
}