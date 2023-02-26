#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/resource.h>

#define LOCKFILE "/var/run/daemon.pid"

sigset_t mask;


int lockfile(const int fd)
{
    struct flock fl;  
    fl.l_type = F_WRLCK;  
    fl.l_start = 0; 
    fl.l_whence = SEEK_SET;  
    fl.l_len = 0; 
    return fcntl(fd, F_SETLK, &fl);
}

int already_running(void)
{
    int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int fd = open(LOCKFILE, O_RDWR | O_CREAT, perms);
    char buf[16];
    if (fd == -1)
    {
        syslog(LOG_ERR, "Невозможно открыть %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }
    if (lockfile(fd) == -1)
    {
        if (errno == EACCES || errno == EAGAIN)
        {
            close(fd);
            return 1;
        }
        syslog(LOG_ERR, "Невозможно установить блокировку на %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }
    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);
    return 0;
}

void daemonize(const char *cmd)
{
    int i, fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;
    /*
    * Сбросить маску режима создания файлов
    */
    umask(0);
    /*
	* Получить максимально возможный номер дескриптора файла
	*/
	if (getrlimit(RLIMIT_NOFILE, &rl) == -1)
    {
		syslog(LOG_ERR, "%s: невозможно получить максимальный номер дескриптора", cmd);
        exit(1);
    }
    /*
	* Стать лидером новой сессии, чтобы утратить управляющий терминал
	*/
	if ((pid = fork()) == -1)
    {
		syslog(LOG_ERR, "%s: ошибка вызова функции fork", cmd);
        exit(1);
    }
	else if (pid > 0) /* родительский процесс */
		exit(0);
	/*
	* Обеспечить невозможность обретения управляющего терминала в будущем
	*/
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGHUP, &sa, NULL) < 0)
    {
		syslog(LOG_ERR, "%s: невозможно игнорировать сигнал SIGHUP", cmd);
        exit(1);
    }

	if (setsid() == -1)
    {
		syslog(LOG_ERR, "%s: ошибка вызова setsid", cmd);
        exit(1);
    }
	/*
	* Назначить корневой каталог текущим рабочим каталогом,
	* чтобы впоследствии можно было отмонтировать файловую систему
	*/
	if (chdir("/") == -1)
    {
		syslog(LOG_ERR, "%s: невозможно сделать текущим рабочим каталогом /", cmd);
        exit(1);
    }
	/*
	* Закрыть все открытые файловые дескрипторы
	*/
	if (rl.rlim_max == RLIM_INFINITY)
		rl.rlim_max = 1024;
	for (i = 0; i < rl.rlim_max; i++)
		close(i);
	/*
	* Присоединить файловые дескрипторы 0, 1 и 2 к /dev/null
	*/
	fd0 = open("/dev/null", O_RDWR);
	fd1 = dup(0);
	fd2 = dup(0);
	/*
	* Инициализировать файл журнала
	*/
	openlog(cmd, LOG_CONS, LOG_DAEMON);
	if (fd0 != 0 || fd1 != 1 || fd2 != 2) 
	{
		syslog(LOG_ERR, "Ошибочные файловые дескрипторы %d %d %d", fd0, fd1, fd2);
		exit(1);
	}
}

void *thr_fn(void *arg)
{
    int err, signo;
    for (;;)
    {
        err = sigwait(&mask, &signo);
        if (err != 0) 
        {
            syslog(LOG_ERR, "Ошибка вызова функции sigwait");
            exit(1);
        }
        switch (signo)
        {
            case SIGHUP:
                syslog(LOG_INFO, "Чтение конфигурационного файла");
                break;
            case SIGTERM:
                syslog(LOG_INFO, "Получен сигнла SIGTERM; выход");
                exit(0);
            default:
                syslog(LOG_INFO, "Получен непредвиденный сигнал %d\n", signo);
        }
    }
    return (void*)0;
}

int main(void)
{
    int err;
    struct sigaction sa;
    pthread_t tid;

    daemonize("mydaemon");

    if (already_running())
    {
        syslog(LOG_ERR, "Демон уже запущен\n");
        exit(1);
    }
    /*
    * Восстановить действие по умолчанию для сигнала SIGHUP и заблокировать все сигналы
    */
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) == -1)
    {
        syslog(LOG_ERR, "Невозможно восстановить действие SIG_DFL для SIGHUP\n");
    }
    // Блокируем все сигналы
    sigfillset(&mask);
    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0)
        syslog(LOG_ERR, "Ошибка выполнения операции SIG_BLOCK\n");
    /*
	* Создать поток
	*/
    err = pthread_create(&tid, NULL, thr_fn, 0);
    if (err != 0)
        syslog(LOG_ERR, "Невозможно создать поток\n");

    time_t cur_time;
    struct tm *timeinfo;
    for (;;)
    {
        time(&cur_time);
        timeinfo = localtime(&cur_time);
        syslog(LOG_INFO, "Текущее календарное время: %s", asctime(timeinfo));
        sleep(5);
    }
    return 0;
}