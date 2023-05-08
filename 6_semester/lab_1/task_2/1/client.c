#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define SOCK_NAME "socket.soc"


int main()
{
	// нет bind (не связываем файловый дескриптор сокета с адресом клиента) в отличие от второй проги
	int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (fd < 0)
	{
		perror("Create socket error");
		return 1;
	}

    struct sockaddr server;
	server.sa_family = AF_UNIX;
	strcpy(server.sa_data, SOCK_NAME);

    char buf[16];
    sprintf(buf, "%d", getpid()); 

	if (sendto(fd, buf, strlen(buf) + 1, 0, &server, sizeof(server)) < 0)
	{
		perror("sendto failed");
		return 1;
	}
	printf("Client send message: %d\n", getpid());

	close(fd);
	return 0;
}