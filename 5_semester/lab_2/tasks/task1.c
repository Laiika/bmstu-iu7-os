#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

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
            printf("child before sleep: pid - %d, ppid - %d, gid - %d\n", getpid(), getppid(), getpgrp());
            sleep(2);
            printf("child after sleep: pid - %d, ppid - %d, gid - %d\n", getpid(), getppid(), getpgrp());
            return 0;
        }   
        else
            printf("parent: pid - %d, gid - %d, child - %d\n", getpid(), getpgrp(), childpid[i]); 
    }

    return 0;
}