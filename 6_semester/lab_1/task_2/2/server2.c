#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
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
	fd = socket(AF_UNIX, SOCK_DGRAM, 0); // возвращает файловый дескриптор
	if (fd < 0)
	{
		perror("Create socket error");
		return 1;
	}

	struct sockaddr server; // заполняю поля структуры sockaddr
	server.sa_family = AF_UNIX;
	strcpy(server.sa_data, SOCK_NAME); // в поле дата - имя файла

	if (bind(fd, &server, sizeof(server)) < 0) // связываю сокет (файловый дескриптор) и адрес (имя файла в данном случае)
	{
		perror("Bind error");
		return 1;
	}

	// ctrl c
	signal(SIGINT, handler);
    signal(SIGTERM, handler);

	struct sockaddr client; 
	socklen_t clientlen = sizeof(struct sockaddr);

	char buf[16];
	printf("Server pid: ");
	printf("%d\n\n", getpid());
	while(1)
	{
		int bytes = recvfrom(fd, buf, sizeof(buf), 0, &client, &clientlen); // &client - указатель на структуру (ее заполняет recvfrom), клиенту адрес автоматически назначается ядром
		if (bytes < 0)
		{
			perror("recvfrom error");
			return 1;
		}
		printf("Server recieved from client: %s \n", buf);

        char send[16];
        sprintf(send, "%s %d", buf, getpid());
        if (sendto(fd, send, strlen(send) + 1, 0, &client, sizeof(client)) < 0) // в sendto передаем уже заполненную структуру
        {
            perror("sendto failed");
            return 1;
        }
		printf("Server send to client: %s \n\n", send);
		clientlen = sizeof(struct sockaddr);
	}

    close(fd);
	unlink(SOCK_NAME);
	return 0;
}