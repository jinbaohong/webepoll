#define _GNU_SOURCE
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>
#include <errno.h>

#define EPOLLCREATE 10
#define MAX_EVENTS 10
#define MAX_BUF 256
#define oops(x) {perror(x); exit(1);}

int make_server_socket_nb(int portnum);
int connet_rqst_handler(int sock_id, int epfd);
void read_til_crnl(FILE *fp);
void process_rq(char *rq, int fd);
void cannot_do(int fd);
int not_exist(char *f);
void do_404(char *item, int fd);
void do_cat(char *item, int fd);

static int fifo_fd;
static FILE *fifo_fp;

int main(int ac, char const *av[])
{
	struct epoll_event ev, evlist[MAX_EVENTS];
	int epfd, ready, listen_fd, fd_tmp;
	char request[BUFSIZ], buf[MAX_BUF];
	FILE *fp;

	if (ac != 2) {
		printf("%s port-num\n", *av);
		exit(1);
	}

	/* epoll handler */
	if ((epfd = epoll_create(EPOLLCREATE)) == -1)
		oops("epoll_create");

	/* Message queue: used to notify main thread(by fd)
	 * that it is ok to close a socket.
	 * However, mq couldn't send msg then exit.
	 */
	/* FIFO */
	if (mkfifo("myfifo", S_IRUSR | S_IWUSR) == -1 && errno != EEXIST)
		oops("mkfifo");
	if ((fifo_fd = open("myfifo", O_RDWR)) == -1)
		oops("open");
	fifo_fp = fdopen(fifo_fd, "rw");
	ev.data.fd = fifo_fd;
	ev.events = EPOLLIN;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fifo_fd, &ev) == -1)
		oops("epoll_ctl");

	/* Create listener socket and add it into event list. */
	listen_fd = make_server_socket_nb(atoi(av[1]));
	ev.data.fd = listen_fd;
	ev.events = EPOLLIN;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) == -1)
		oops("epoll_ctl");

	while (1) {
		printf("[main]Waiting events...\n");
		ready = epoll_wait(epfd, evlist, MAX_EVENTS, -1);
		printf("[main]ready: %d\n", ready);

		for (int i = 0; i < ready; i++) {
			printf("[main]event %d\n", i);
			if (evlist[i].data.fd == listen_fd) { // Create connection request
				printf("[fd %d]Listener get a connection...\n", evlist[i].data.fd);
				connet_rqst_handler(listen_fd, epfd);
			} else if (evlist[i].data.fd == fifo_fd) { // Close socket(child has finished)
				if (fscanf(fifo_fp, "%d", &fd_tmp) != 1)
					oops("fscanf");
				close(fd_tmp);
				printf("[main]closed fd %d\n", fd_tmp);	
			} else { // Transmission request
				if (evlist[i].events & EPOLLIN) {
					printf("[fd %d]Receive request for data...\n", evlist[i].data.fd);
					if ((fp = fdopen(evlist[i].data.fd, "r")) == NULL)
						oops("fdopen");
					fgets(request, BUFSIZ, fp);
					printf("[fd %d]request = %s\n", evlist[i].data.fd, request);
					read_til_crnl(fp);
					process_rq(request, evlist[i].data.fd);
					fclose(fp);
					// close(evlist[i].data.fd);
					// printf("[main]closed fd %d\n", evlist[i].data.fd);
				}
			}
		}
	}
	return 0;
}

// void http_handler()
void process_rq(char *rq, int fd)
{
	char cmd[BUFSIZ], arg[BUFSIZ];
	int pid;
	pid = fork();
	if ( pid != 0 ){
		// waitpid(pid, NULL, 0);
		return;
	}
	
	strcpy(arg, ".");
	if ( sscanf(rq, "%s %s", cmd, arg+1) != 2)
		return;
	printf("[fd %d(forked)]cmd = %s ; arg = %s\n", fd, cmd, arg);
	
	if ( strcmp(cmd, "GET") != 0 )
		cannot_do(fd);
	else if ( not_exist(arg) )
		do_404(arg, fd);
	else
		do_cat(arg, fd);
	// printf("%s\n", rq);
	printf("[fd %d(forked)]Exit mission...\n", fd);
	fprintf(fifo_fp, "%d", fd);
	exit(0);
}


void read_til_crnl(FILE *fp)
{
	char buf[BUFSIZ];
	while ( fgets(buf, BUFSIZ, fp) != NULL &&
			strcmp(buf, "\r\n") != 0);
		// printf("%s\n", buf);
}

void cannot_do(int fd)
{
	printf("cannot_do\n");
	FILE *fp = fdopen(fd, "w");
	fprintf(fp, "HTTP/1.0 501 Not implemented\r\n");
	fclose(fp);
}

int not_exist(char *f)
{
	struct stat info;
	return (stat(f, &info) == -1);
}

void do_404(char *item, int fd)
{
	FILE *fp;

	printf("[fd %d(forked)]do_404\n", fd);
	if ((fp = fdopen(fd, "w")) == NULL)
		oops("fdopen");
	fprintf(fp, "HTTP/1.0 404 Not Found\r\n");
	fclose(fp);
}

void do_cat(char *item, int fd)
{
	FILE *fp_sock = fdopen(fd, "w");
	FILE *fp_file = fopen(item, "r");
	int c;

	if ( fp_sock != NULL && fp_file != NULL) {
		fprintf(fp_sock, "HTTP/1.0 200 OK\r\n");
		fprintf(fp_sock, "Content-Type: text/html\r\n");
		fprintf(fp_sock, "\r\n");
		while ( (c = getc(fp_file)) != EOF)
			fprintf(fp_sock, "%c", c);
		fclose(fp_sock);
		fclose(fp_file);
	}
}

int connet_rqst_handler(int listen_fd, int epfd)
{
	int fd;
	struct epoll_event ev;

	if ((fd = accept4(listen_fd, NULL, NULL, SOCK_NONBLOCK)) == -1)
		oops("accept4");
	ev.data.fd = fd;
	ev.events = EPOLLIN;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
		oops("epoll_ctl");
	printf("[fd %d]Add (fd %d) to event list\n", listen_fd, fd);
	return fd;
}
