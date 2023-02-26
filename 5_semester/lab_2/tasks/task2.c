#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    pid_t childpid[2]; 

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
    return 0;
}