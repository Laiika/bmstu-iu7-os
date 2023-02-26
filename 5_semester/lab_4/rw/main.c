#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define ACT_READERS 0 
#define ACT_WRITER 1    
#define QUEUE_WRITERS 2  
#define QUEUE_READERS 3  
#define SB 4

struct sembuf start_read[5] = {
    {QUEUE_READERS, 1, 0}, 
    {ACT_WRITER, 0, 0}, 
    {QUEUE_WRITERS, 0, 0}, 
    {ACT_READERS, 1, 0}, 
    {QUEUE_READERS, -1, 0}
};
struct sembuf stop_read[1] = {
    {ACT_READERS, -1, 0}
};
struct sembuf start_write[6] = {
    {QUEUE_WRITERS, 1, 0}, 
    {ACT_READERS, 0, 0}, 
    {ACT_WRITER, 0, 0},   
    {ACT_WRITER, 1, 0}, 
    {SB, -1, 0}, 
    {QUEUE_WRITERS, -1, 0}
};
struct sembuf stop_write[2] = {
    {ACT_WRITER, -1, 0},  
    {SB, 1, 0}
};

int reader(int shmid, int semid, int id)
{
    int *addr = shmat(shmid, NULL, 0);
    if (addr == (void *)-1)
    {
        perror("shmat error\n");
        exit(1);
    }

    srand(time(NULL) + id);
    for (int i = 0; i < 5; i++)
    {
        sleep(rand() % 5 + 1);
        if (semop(semid, start_read, 5) == -1)
            return 1;
        printf("reader %d read:  %d.\n", id + 1, *addr);
        if (semop(semid, stop_read, 1) == -1)
            return 1;
    }

    if (shmdt(addr) == -1)
    {
        perror("shmdt error\n");
        exit(1);
    }
    return 0;
}

int writer(int shmid, int semid, int id)
{
    int *addr = shmat(shmid, NULL, 0);
    if (addr == (void *)-1)
    {
        perror("shmat error\n");
        exit(1);
    }

    for (int i = 0; i < 5; i++)
    {
        sleep(rand() % 3 + 1);
        if (semop(semid, start_write, 6) == -1)
            return 1;
        printf("writer %d wrote: %d.\n", id + 1, ++(*addr));
        if (semop(semid, stop_write, 2) == -1)
            return 1;
    }

    if (shmdt(addr) == -1)
    {
        perror("shmdt error\n");
        exit(1);
    }
    return 0;
}

int main(void)
{
    key_t key = ftok("text.txt", 2);  
    if (key == -1)
    {
        perror("ftok error\n");
        exit(1);
    }
    // создание нового сегмента разделяемой памяти
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;
    int shmid = shmget(key, sizeof(int), IPC_CREAT | perms);
    if (shmid == -1)
    {
        perror("shmget error\n");
        exit(1);
    }
    // подключаем сегмент разделяемой памяти shmid к адресному пространству вызывающего процесса
    int *addr = shmat(shmid, NULL, 0);
    if (addr == (void *)-1)
    {
        perror("shmat error\n");
        exit(1);
    }
    *addr = 0;
    // создаем новый набор семафоров
    int semid = semget(key, 5, IPC_CREAT | perms);
    if (semid == -1)
    {
        perror("semget error\n");
        exit(1);
    }
    // инициализируем семафоры
    if (semctl(semid, ACT_READERS, SETVAL, 0) == -1)
    {
        perror("semctl error\n");
        exit(1);
    }
    if (semctl(semid, ACT_WRITER, SETVAL, 0) == -1)
    {
        perror("semctl error\n");
        exit(1);
    }
    if (semctl(semid, QUEUE_WRITERS, SETVAL, 0) == -1)
    {
        perror("semctl error\n");
        exit(1);
    }
    if (semctl(semid, QUEUE_READERS, SETVAL, 0) == -1)
    {
        perror("semctl error\n");
        exit(1);
    }
    if (semctl(semid, SB, SETVAL, 1) == -1)
    {
        perror("semctl error\n");
        exit(1);
    }

    for (int i = 0; i < 3; i++)
    {
        int child_pid;
        if ((child_pid = fork()) == -1)
        {
            perror("fork writer error\n");
            exit(1);
        }
        else if (child_pid == 0)
        {
            writer(shmid, semid, i);
            return 0;
        }
    }
    for (int i = 0; i < 4; i++)
    {
        int child_pid;
        if ((child_pid = fork()) == -1)
        {
            perror("fork reader error\n");
            exit(1);
        }
        else if (child_pid == 0)
        {
            reader(shmid, semid, i);
            return 0;
        }
    }

    for (int i = 0; i < 7; i++)
    {
        int status;
        if (wait(&status) == -1)
            perror("wait error\n");

        if (!WIFEXITED(status))
			printf("one of children terminated abnormally\n");
    }
    if (shmdt(addr) == -1)
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