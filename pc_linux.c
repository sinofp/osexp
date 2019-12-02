#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define BUF_SIZE 3
#define BUF_LENGTH 256
#define SHM_NAME "/mysharedmemory2"
#define FULL_SEM "/myfullsemaphore"
#define EMPTY_SEM "/myemptysemaphore"
#define MUTEX "/mymutex"
#define handle_error(msg)   \
    do {                    \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

struct shared_memory {
    int read_index, write_index, color;  // have a colorful day!
    char buf[BUF_SIZE][BUF_LENGTH];
};

struct sync_signals {
    struct shared_memory *sm;
    sem_t *emptySemaphore, *fullSemaphore, *mutex;
} signals;

void printsm(struct shared_memory *sm) {
    puts("┌───────────────────────────────────────────┐");
    for (int i = 0; i < BUF_SIZE; i++) {
        strcmp(sm->buf[i], "")
            ? printf("| %s |", sm->buf[i])
            : printf("|                                           |");
        if (i == sm->write_index) {
            printf(" <-- write_index");
        }
        if (i == sm->read_index) {
            printf(" <-- read_index");
        }
        puts("");
    }
    puts("└───────────────────────────────────────────┘");
}

void open_signals() {
    // shared memory
    int shmfd = shm_open(SHM_NAME, O_RDWR, S_IRUSR | S_IWUSR);
    if (-1 == shmfd) handle_error("shm_open");
    signals.sm = mmap(NULL, sizeof(struct shared_memory),
                      PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (MAP_FAILED == signals.sm) handle_error("mmap");
    close(shmfd);

    // semaphore & mutex
    signals.emptySemaphore = sem_open(EMPTY_SEM, O_RDWR);
    if (SEM_FAILED == signals.emptySemaphore) handle_error("pc empty");
    signals.fullSemaphore = sem_open(FULL_SEM, O_RDWR);
    if (SEM_FAILED == signals.fullSemaphore) handle_error("pc full");
    signals.mutex = sem_open(MUTEX, O_RDWR);
    if (SEM_FAILED == signals.mutex) handle_error("pc mutex");
}

void close_signals() {
    sem_unlink(MUTEX);
    sem_unlink(FULL_SEM);
    sem_unlink(EMPTY_SEM);
    shm_unlink(SHM_NAME);
    munmap((void *)signals.sm, sizeof(struct shared_memory));
}

int randint(int maxint) {
    srand((unsigned int)time(NULL));
    return rand() % maxint;
}

int producer(int no) {
    printf("\033[0;31mproducer %d created.\033[0m\n", no);
    open_signals();

    struct timespec tp;
    for (int i = 0; i < 6; ++i) {
        sleep(randint(5));
        sem_wait(signals.emptySemaphore);
        sem_wait(signals.mutex);
        clock_gettime(CLOCK_REALTIME, &tp);
        signals.sm->color = 30 + (signals.sm->color + 1) % 8; // [30, 37]
        sprintf(signals.sm->buf[signals.sm->write_index],
                "\033[1;%dm生产者%d第%d次写: %10ld秒%9ld微秒\033[0m",
                signals.sm->color, no, i + 1, tp.tv_sec, tp.tv_nsec);
        signals.sm->write_index = (signals.sm->write_index + 1) % BUF_SIZE;
        printsm(signals.sm);
        sem_post(signals.mutex);
        sem_post(signals.fullSemaphore);
    }
    close_signals();
    return 0;
}

int consumer(int no) {
    printf("\033[0;31mconsumer %d created.\033[0m\n", no);

    open_signals();
    for (int i = 0; i < 4; ++i) {
        sleep(randint(5));
        sem_wait(signals.fullSemaphore);
        sem_wait(signals.mutex);
        printf("\033[0;31m消费者%d第%d次读:\033[0m %s\n", no, i + 1,
               signals.sm->buf[signals.sm->read_index]);
        memset(signals.sm->buf[signals.sm->read_index], 0,
               sizeof(signals.sm->buf[0]));
        signals.sm->read_index = (signals.sm->read_index + 1) % BUF_SIZE;
        printsm(signals.sm);
        sem_post(signals.mutex);
        sem_post(signals.emptySemaphore);
    }
    close_signals();
    return 0;
}

int main() {
    // semaphore & mutex
    signals.fullSemaphore =
        sem_open(FULL_SEM, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 0);
    if (SEM_FAILED == signals.fullSemaphore) handle_error("sem_open for full");
    signals.emptySemaphore =
        sem_open(EMPTY_SEM, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 3);
    if (SEM_FAILED == signals.emptySemaphore)
        handle_error("sem_open for empty");
    signals.mutex = sem_open(MUTEX, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 1);
    if (SEM_FAILED == signals.mutex) handle_error("sem_oepn for mutex");

    // shared memory
    int shmfd = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (-1 == shmfd) handle_error("main shm_open");
    ftruncate(shmfd, sizeof(struct shared_memory));
    signals.sm = mmap(NULL, sizeof(struct shared_memory),
                      PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (MAP_FAILED == signals.sm) handle_error("main mmap");
    close(shmfd);
    memset(signals.sm, 0, sizeof(struct shared_memory));

    for (int i = 0; i < 3; ++i) {
        // 三个消费者
        pid_t pid = fork();
        if (0 == pid) {
            consumer(i + 1);
            return 0;
        }
    }
    for (int i = 0; i < 2; ++i) {
        // 两个生产者
        pid_t pid = fork();
        if (0 == pid) {
            producer(i + 1);
            return 0;
        }
    }

    int wstatus;
    while (-1 != wait(&wstatus))
        ;
    close_signals();
    return 0;
}