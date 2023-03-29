#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#define SOCK_NAME "socket.soc"


int main()
{
	int fd = socket(AF_UNIX, SOCK_DGRAM, 0); // возвращает файловый дескриптор
	if (fd < 0)
	{
		perror("Create socket error");
		return 1;
	}

	struct sockaddr client; // заполняю поля структуры sockaddr
	client.sa_family = AF_UNIX; // семейство сокетов юникс
	char buf[16];
	sprintf(buf, "%d", getpid()); 
	strcpy(client.sa_data, buf); // в качестве имени временного файла указала ид процесса

	if (bind(fd, &client, sizeof(client)) < 0) // передаю адрес клиента (связываю клиента с адресом)
	{
		perror("Bind error");
		return 1;
	}

	struct sockaddr server; // заполняю поля структуры sockaddr
	server.sa_family = AF_UNIX;
	strcpy(server.sa_data, SOCK_NAME);
	socklen_t serverlen = sizeof(struct sockaddr);

	// клиент посылает сообщение серверу
	if (sendto(fd, buf, strlen(buf) + 1, 0, &server, serverlen) < 0) 
	{
		perror("sendto failed");
		return 1;
	}
	printf("Client send message: %d\n", getpid());

	int bytes = recvfrom(fd, buf, sizeof(buf), 0, &server, &serverlen);
	if (bytes < 0)
	{
		perror("recvfrom error");
		return 1;
	}
	printf("Client recieved message from server: %s \n", buf);

	sprintf(buf, "%d", getpid()); 
	unlink(buf);
	close(fd);
	return 0;
}