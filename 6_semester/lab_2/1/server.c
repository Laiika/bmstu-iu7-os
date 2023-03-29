#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define SOCK_NAME "socket.soc"

static int fd;

void handler(int sig)
{
    close(fd);
    unlink(SOCK_NAME);
    exit(0);
}

int main()
{
	fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0)
	{
		perror("Create socket error");
		return 1;
	}

	struct sockaddr server; 
	server.sa_family = AF_UNIX;
	strcpy(server.sa_data, SOCK_NAME);

	// bind() - связывает сокет с заданным адресом.
	// После вызова bind() программа-сервер становится доступна для соединения по заданному адресу (имени файла)
	if (bind(fd, &server, sizeof(server)) < 0)
	{
		perror("Bind error");
		return 1;
	}

	signal(SIGINT, handler);
    signal(SIGTERM, handler);

	struct sockaddr client; 
	socklen_t namelen = sizeof(struct sockaddr);

	char buf[16];
	while (1)
	{
		int bytes = recvfrom(fd, buf, sizeof(buf), 0, &client, &namelen);
		if (bytes < 0)
		{
			perror("recvfrom failed\n");
			return 1;
		}

		printf("\nServer received message: %s\n", buf);
	}

	close(fd);
	unlink(SOCK_NAME);
	return 0;
}