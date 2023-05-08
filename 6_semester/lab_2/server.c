#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>

#define MAX_EVENTS_COUNT 100
#define SERVER_PORT 9877
#define MSG_LEN 64

static int sock_fd_global;

void sig_handler(int signum)
{
    close(sock_fd_global);
    exit(EXIT_SUCCESS);
}

int handle_event(int sock_fd)
{
    struct sockaddr_in client_addr; 
    socklen_t client_len;
    char input_msg[MSG_LEN];
  
    int bytes = read(sock_fd, input_msg, MSG_LEN);
    if (bytes == 0)
        return(EXIT_SUCCESS);

    if (bytes == -1)
    {
        perror("read error");
        return EXIT_FAILURE;
    }

    printf("Server received: %s \n", input_msg);

    char output_msg[MSG_LEN];
    sprintf(output_msg, "%s %d", input_msg, getpid());

    if (write(sock_fd, output_msg, MSG_LEN) == -1)
    {
        perror("write error");
        return EXIT_FAILURE;
    }

    printf("Server send: %s \n\n", output_msg);
    return EXIT_SUCCESS;
}


int main(void)
{
    setbuf(stdout, NULL);

    struct epoll_event ev, events[MAX_EVENTS_COUNT]; // epoll_event - структура для конфигурирования epoll
    int listen_sock, nfds, epoll_fd;

    // sockaddr_in описывает сокет для cетевого взаимодействия
    struct sockaddr_in serv_addr = 
    {
        .sin_family = AF_INET,         // сетевое взаимодействие (по протоколу TCP)
        .sin_addr.s_addr = INADDR_ANY, // IP адрес. INADDR_ANY - все адреса локального хоста (0.0.0.0)
        .sin_port = htons(SERVER_PORT) // номер порта, который намерен занять процесс (htons - перевод из Little Endian (Intel) -> Big Endian (Network)
    };
  
    // printf("%d\n", htons(SERVER_PORT));
  
    listen_sock = socket(AF_INET, SOCK_STREAM, 0); // получение файлового дескриптора
    if (listen_sock == -1)
    {
        perror("socket error");
        exit(EXIT_FAILURE);
    }
    sock_fd_global = listen_sock;

    if (bind(listen_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) // связываем сокет с сетевым адресом (порт + IP адрес)
    {
        perror("bind error");
        close(listen_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(listen_sock, 1) == -1) // информируем ОС, что сервер готов принимать соединения
    {
        perror("listen error");
        close(listen_sock);
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    epoll_fd = epoll_create1(0); // получаем дескриптор файла, указывающий на экземпляр epoll
    if (epoll_fd == -1)
    {
        perror("epoll_create1 error");
        close(listen_sock);
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN; // наблюдаем за событием: новые данные (для чтения) в файловом дескриптор
    ev.data.fd = listen_sock;
    // epoll_ctl выполняет операции управления epoll. Он запрашивает выполнение операции EPOLL_CTL_ADD для дескриптора listen_sock
    // EPOLL_CTL_ADD - регистрирует файловый дескриптор listen_sock в epoll и связать событие event с listen_sock
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) 
    {
        perror("epoll_ctl: listen_sock error");
        close(listen_sock);
        exit(EXIT_FAILURE);
    }

    for (;;)
    {
        nfds = epoll_wait(epoll_fd, events, MAX_EVENTS_COUNT, -1); // epoll_wait() ожидает события на экземпляре epoll. -1 - не истечёт время ожидания.
        // events - события, которые произошли на epoll. nfds - кол-во произошедших событий
        if (nfds == -1)
        {
            perror("epoll_wait error");
            close(listen_sock);
            exit(EXIT_FAILURE);
        }
    
        for (int n = 0; n < nfds; ++n)
        {
            // если сокет исходный - создаем копию
            if (events[n].data.fd == listen_sock)
            {
                struct sockaddr client_addr; 
                socklen_t client_len = sizeof(client_addr);
                // получает фд исходного сокета, возвращает фд копии исходного сокета (исходный сокет - в состоянии листен, копия - коннект)
                int conn_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_len); // сервер принимает соединение (получил запрос на него) (создается копия исходного сокета, чтобы сервер мог принимать соединения)
                if (conn_sock == -1) // проверяем файловый дескриптор исходного сокета
                {
                    perror("accept error");
                    close(listen_sock);
                    exit(EXIT_FAILURE);
                }
            
                // F_SETFL - Устанавливает часть флагов, относящихся к состоянию файла, согласно значению, указанному в аргументе arg
                // F_GETFL - Читает флаги файлового дескриптора.
                // O_NONBLOCK - неблокирующий ввод-вывод
                // fnctl - выполняет команду над conn_sock 
                int status = fcntl(conn_sock, F_SETFL, fcntl(conn_sock, F_GETFL, 0) | O_NONBLOCK);
                if (status == -1)
                {
                    perror("fcntl error");
                    close(listen_sock);
                    exit(EXIT_FAILURE);
                }

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn_sock;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &ev) == -1) 
                {
                    perror("epoll_ctl: conn_sock error");
                    close(listen_sock);
                    exit(EXIT_FAILURE);
                }
            }
            else if (handle_event(events[n].data.fd) != EXIT_SUCCESS)
            {
                close(listen_sock);
                exit(EXIT_FAILURE);
            }
        }
    }

    close(listen_sock);
    return EXIT_SUCCESS;
}
