#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/sem.h>

#define SB 0
#define BUF_FULL  1
#define BUF_EMPTY 2

struct sembuf consume_start[2]  = {{BUF_FULL, -1, 0},  {SB, -1, 0}};
struct sembuf consume_end[2] =    {{SB, 1, 0},         {BUF_EMPTY, 1, 0}};
struct sembuf produce_start[2]  = {{BUF_EMPTY, -1, 0}, {SB, -1, 0}};
struct sembuf produce_end[2] =    {{SB, 1, 0},         {BUF_FULL, 1, 0}};

struct buf
{
    char *paddr;  
    char *caddr; 
    char symbol;
};

void consume(int shmid, int semid, int id)
{
    struct buf *buffer = shmat(shmid, NULL, 0);
    if (buffer ==(void *)-1)
    {
        perror("shmat error\n");
        exit(1);
    }

    srand(getpid());
    for (int i = 0; i < 5; i++)
    {
        sleep(rand() % 4 + 1);
        if (semop(semid, consume_start, 2) == -1)
        {
	        perror("semop consumer error\n");
	        exit(1);
	    }
        printf("consumer %d read symbol:  %c\n", id + 1, *(buffer->caddr));
        buffer->caddr++;
        if (semop(semid, consume_end, 2) == -1)
        {
            perror("semop consumer error\n");
            exit(1);
	    }
    }

    if (shmdt(buffer) == -1)
    {
        perror("shmdt error\n");
        exit(1);
    }
}

void produce(int shmid, int semid, int id)
{
    struct buf *buffer = shmat(shmid, NULL, 0);
    if (buffer == (void *)-1)
    {
        perror("shmat error\n");
        exit(1);
    }

    srand(getpid());
    for (int i = 0; i < 5; i++)
    {
        sleep(rand() % 3 + 1);
	    if (semop(semid, produce_start, 2) == -1)
	    {
	        perror("semop producer error\n");
	        exit(1);
	    }
	    *(buffer->paddr) = buffer->symbol;
	    printf("producer %d wrote symbol: %c\n", id + 1, buffer->symbol);
        buffer->paddr++;
        buffer->symbol++;
	    if (semop(semid, produce_end, 2) == -1)
        {
	        perror("semop producer error\n");
	        exit(1);
	    }
    }
    
    if (shmdt(buffer) == -1)
    {
        perror("shmdt error\n");
        exit(1);
    }
}

int main(void)
{
    key_t key = ftok("text.txt", 1);  
    if (key == -1)
    {
        perror("ftok error\n");
        exit(1);
    }
    // создание нового сегмента разделяемой памяти
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;
    int shmid = shmget(key, sizeof(struct buf) + 64 * sizeof(char), IPC_CREAT | perms);  
    if (shmid == -1)
    {
        perror("shmget error\n");
        exit(1);
    }
    // подключаем сегмент разделяемой памяти shmid к адресному пространству вызывающего процесса
    struct buf *buffer = shmat(shmid, NULL, 0);
    if (buffer == (void *)-1)
    {
        perror("shmat error\n");
        exit(1);
    }
    buffer->paddr = (char *)buffer + sizeof(struct buf);
    buffer->caddr = (char *)buffer + sizeof(struct buf);
    buffer->symbol = 'a';
    // создаем новый набор семафоров
    int semid = semget(key, 3, IPC_CREAT | perms);
    if (semid == -1)
    {
        perror("semget error\n");
        exit(1);
    }
    // инициализируем семафоры
    if (semctl(semid, SB, SETVAL, 1) == -1)
    {
        perror("semctl error\n");
        exit(1);
    }
    if (semctl(semid, BUF_FULL, SETVAL, 0) == -1)
    {
        perror("semctl error\n");
        exit(1);
    }
    if (semctl(semid, BUF_EMPTY, SETVAL, 64) == -1)
    {
        perror("semctl error\n");
        exit(1);
    }

    for (int i = 0; i < 2; i++)
    {
        int child_pid;
        if ((child_pid = fork()) == -1)
        {
            perror("fork producer error\n");
            exit(1);
        }
        else if (child_pid == 0)
        {
            produce(shmid, semid, i);
            return 0;
        }
    }
    for (int i = 0; i < 3; i++)
    {
        int child_pid;
        if ((child_pid = fork()) == -1)
        {
            perror("fork consumer error\n");
            exit(1);
        }
        else if (child_pid == 0)
        {
            consume(shmid, semid, i);
            return 0;
        }
    }
    // производитель (parent)
    srand(getpid());
    for (int i = 0; i < 5; i++)
    {
        sleep(rand() % 3 + 1);
	    if (semop(semid, producer_in_cr, 2) == -1)
	    {
	        perror("semop producer error\n");
	        exit(1);
	    }
	    *(buffer->paddr) = buffer->symbol;
	    printf("producer %d wrote symbol: %c\n", 3, buffer->symbol);
        buffer->paddr++;
        buffer->symbol++;
	    if (semop(semid, producer_out_cr, 2) == -1)
        {
	        perror("semop producer error\n");
	        exit(1);
	    }
    }

    for (int i = 0; i < 5; i++)
    {
        int status;
        if (wait(&status) == -1)
            perror("wait error\n");

        if (!WIFEXITED(status))
			printf("one of children terminated abnormally\n");
    }
    if (shmdt(buffer) == -1)
    {
        perror("shmdt error\n");
        exit(1);
    }
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
    {
        perror("shmctl error\n");
        exit(1);
    }
    if (semctl(semid, IPC_RMID, 0) == -1)
    {
        perror("semctl error\n");
        exit(1);
    }
    return 0;
}