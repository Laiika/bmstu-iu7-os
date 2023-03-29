#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/socket.h>

#define N 4

int main(void)
{
    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == -1)
    {
        perror("socketpair error");
        exit(1);
    }

    pid_t child_pid[N];
    char buf[256];
    for (size_t i = 0; i < N; ++i)
    {
        if ((child_pid[i] = fork()) == -1)
        {
            perror("fork error");
            exit(1);
        }
        else if (child_pid[i] == 0)
        {
            // close(sockets[1]);
            char message[20];
            sprintf(message, "%d", getpid()); 
            printf("Child %d sent: %s\n", getpid(), message);
            write(sockets[0], message, sizeof(message));
            read(sockets[0], buf, sizeof(buf));
            printf("Child %d recieved: %s\n", getpid(), buf);
            // close(sockets[0]);

            return EXIT_SUCCESS;
        }
        else 
        {
            // close(sockets[0]);
            char message[20];
            sprintf(message, "parent %d", getpid()); 
            read(sockets[1], buf, sizeof(buf));
            printf("Parent recieved: %s\n", buf);
            write(sockets[1], message, sizeof(message));
            printf("Parent sent: %s\n", message);
            // close(sockets[1]);
        }
    }

    for (size_t i = 0; i < N; ++i)
    {
        int status;
        if (waitpid(child_pid[i], &status, 0) == -1)
        {
            perror("waitpid error");
            exit(1);
        }
    }
    close(sockets[1]);

    return EXIT_SUCCESS;
}