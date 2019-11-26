#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#define BUF_SIZE 3
#define BUF_LENGTH 256
#define FULL_SEM "my full semaphore"
#define EMPTY_SEM "my empty semaphore"
#define FILE_NAME "my map file"
#define MUTEX_NAME "my mutex"
#define IFNULLPRTEXIT(VALUE, ERR_MSG)             \
    if (NULL == VALUE) {                          \
        printf(ERR_MSG " %d.\n", GetLastError()); \
        exit(EXIT_FAILURE);                       \
    }

struct shared_memory {
    int read_index, write_index;
    char buf[BUF_SIZE][BUF_LENGTH];
};

int randint(int maxint) {
    srand((unsigned int)time(NULL));
    return rand() % maxint;
}

void printsm(struct shared_memory *sm) {
    puts("┌─────────────────────────────────┐");
    for (int i = 0; i < BUF_SIZE; i++) {
        strcmp(sm->buf[i], "") ? printf("| %s |", sm->buf[i])
                               : printf("|                                 |");
        if (i == sm->write_index) {
            printf(" <-- write_index");
        }
        if (i == sm->read_index) {
            printf(" <-- read_index");
        }
        puts("");
    }
    puts("└─────────────────────────────────┘");
}

struct sync_signals {
    struct shared_memory *sm;
    HANDLE hMapFile, emptySemaphore, fullSemaphore, mutex;
} signals;

void open_signals() {
    // Shared Memeory
    signals.hMapFile = OpenFileMapping(FILE_MAP_WRITE, FALSE, FILE_NAME);
    IFNULLPRTEXIT(signals.hMapFile, "Could not open file mapping object")

    signals.sm = (struct shared_memory *)MapViewOfFile(
        signals.hMapFile, FILE_MAP_WRITE, 0, 0, sizeof(struct shared_memory));
    IFNULLPRTEXIT(signals.sm, "Could not map view of file")

    // Semaphore
    signals.emptySemaphore =
        OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, EMPTY_SEM);
    IFNULLPRTEXIT(signals.emptySemaphore, "OpenEmptySemaphore error:")

    signals.fullSemaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, FULL_SEM);
    IFNULLPRTEXIT(signals.fullSemaphore, "OpenFullSemaphore error:")

    // mutex
    signals.mutex = OpenMutex(MUTEX_ALL_ACCESS, TRUE, MUTEX_NAME);
    IFNULLPRTEXIT(signals.mutex, "OpenMutex error:")
}

void close_signals() {
    UnmapViewOfFile(signals.sm);
    CloseHandle(signals.hMapFile);
    CloseHandle(signals.emptySemaphore);
    CloseHandle(signals.fullSemaphore);
    CloseHandle(signals.mutex);
}

int producer(int no) {
    printf("producer %d created.\n", no);
    open_signals();

    SYSTEMTIME local_time;
    for (int i = 0; i < 6; i++) {
        Sleep(1000 * randint(5));
        WaitForSingleObject(signals.emptySemaphore, INFINITE);
        WaitForSingleObject(signals.mutex, INFINITE);
        GetLocalTime(&local_time);
        sprintf(signals.sm->buf[signals.sm->write_index],
                "生产者%d第%d次写: %2ld分%2ld秒%3ld毫秒", no, i + 1,
                local_time.wHour, local_time.wMinute, local_time.wMilliseconds,
                local_time.wSecond);
        signals.sm->write_index = (signals.sm->write_index + 1) % BUF_SIZE;
        printsm(signals.sm);
        ReleaseMutex(signals.mutex);
        ReleaseSemaphore(signals.fullSemaphore, 1, NULL);
    }
    close_signals();
    return 0;
}

int consumer(int no) {
    printf("consumer %d created.\n", no);
    open_signals();

    for (int i = 0; i < 4; i++) {
        Sleep(1000 * randint(5));
        WaitForSingleObject(signals.fullSemaphore, INFINITE);
        WaitForSingleObject(signals.mutex, INFINITE);
        printf("消费者%d第%d次读: %s\n", no, i + 1,
               signals.sm->buf[signals.sm->read_index]);
        memset(signals.sm->buf[signals.sm->read_index], 0,
               sizeof(signals.sm->buf[0]));
        signals.sm->read_index = (signals.sm->read_index + 1) % BUF_SIZE;
        printsm(signals.sm);
        ReleaseMutex(signals.mutex);
        ReleaseSemaphore(signals.emptySemaphore, 1, NULL);
    }
    close_signals();
    return 0;
}

int main(int argc, char *argv[]) {
    if (1 == argc) {
        //第一次进入
        system("chcp 65001");
        puts("main");
        // 创建共享内存
        HANDLE hMapFile;
        char *pBuf;

        hMapFile =
            CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                              sizeof(struct shared_memory), FILE_NAME);
        IFNULLPRTEXIT(hMapFile, "Could not create file mapping object")

        pBuf = (char *)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0,
                                     sizeof(struct shared_memory));
        IFNULLPRTEXIT(pBuf, "Could not map view of file")

        // 清空共享内存（主要是让index=0）
        ZeroMemory(pBuf, sizeof pBuf);

        // Semaphore
        HANDLE emptySemaphore = CreateSemaphore(NULL, 3, 3, EMPTY_SEM);
        IFNULLPRTEXIT(emptySemaphore, "CreateEmptySemaphore error:")
        HANDLE fullSemaphore = CreateSemaphore(NULL, 0, 3, FULL_SEM);
        IFNULLPRTEXIT(fullSemaphore, "CreateFullSemaphore error:")

        // MUTEX
        HANDLE mutex = CreateMutex(NULL, FALSE, MUTEX_NAME);
        IFNULLPRTEXIT(mutex, "CreateMutex error:")

        // 创建进程
        HANDLE processes[5];
        for (int i = 0; i < 2; i++) {
            // 两个生产者
            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            memset(&si, 0, sizeof si);
            memset(&pi, 0, sizeof pi);
            si.cb = sizeof si;
            char cmd[50];
            sprintf(cmd, "./pc producer %d", i);
            if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si,
                               &pi)) {
                printf("create producer process %d failed\n", i);
                return 1;
            }
            processes[i] = pi.hProcess;
        }

        for (int i = 0; i < 3; i++) {
            // 三个消费者
            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            memset(&si, 0, sizeof si);
            memset(&pi, 0, sizeof pi);
            si.cb = sizeof si;
            char cmd[50];
            sprintf(cmd, "./pc consumer %d", i);
            if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si,
                               &pi)) {
                printf("create consumer process %d failed\n", i);
                return 1;
            }
            processes[2 + i] = pi.hProcess;
        }

        // 等待所有进程结束
        WaitForMultipleObjects(5, processes, TRUE, INFINITE);
        for (int i = 0; i < 5; i++) {
            CloseHandle(processes[i]);
        }

        UnmapViewOfFile(pBuf);
        CloseHandle(hMapFile);
        CloseHandle(emptySemaphore);
        CloseHandle(fullSemaphore);
        CloseHandle(mutex);
        puts("This is the end.");
    } else if (argc > 2) {
        if (!strcmp("producer", argv[1])) {
            producer(1 + atoi(argv[2]));
        } else if (!strcmp("consumer", argv[1])) {
            consumer(1 + atoi(argv[2]));
        }
    }
    return 0;
}