#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main(void)
{
    char str[2][100] = { "aaa\n", "rrrrrrrrrrrrrrrrrrrrrrrrrrrrrr\n" };
    pid_t childpid[2]; 
    int fd[2];
    if (pipe(fd) == -1)
    {
        perror("Pipe failed");
        exit(1);
    }
    for (int i = 0; i < 2; i++) 
    {
        if ((childpid[i] = fork()) == -1) 
        {
            perror("Can`t not fork"); 
            exit(1);                       
        }    
        else if (childpid[i] == 0)
        {
            printf("child: pid - %d, ppid - %d, gid - %d\n", getpid(), getppid(), getpgrp());
            close(fd[0]);
            write(fd[1], str[i], strlen(str[i]));
            return 0;
        }    
        else
            printf("parent: pid - %d, gid - %d, child - %d\n", getpid(), getpgrp(), childpid[i]);
    }
    for (int i = 0; i < 2; i++) 
    {
        int status;
        pid_t child_pid;
        if ((child_pid = wait(&status)) == -1)
        {
            perror("Wait failed\n");
            exit(1);
        }
        printf("child: PID = %d\n", child_pid);
        if (WIFEXITED(status))
            printf("exited with code %d\n", WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("terminated, signal %d\n", WTERMSIG(status));
        else if (WIFSTOPPED(status))
            printf("stopped, signal %d\n", WSTOPSIG(status));  
    }

    close(fd[1]);
    read(fd[0], str[0], sizeof(str[0]));
    printf("Messages %s \n", str[0]);
    return 0;
}