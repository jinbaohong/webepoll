#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#define oops(x) {perror(x); exit(1)}

int make_server_socket();
void process_rq();
void read_til_crnl();
void cannot_do();
void do_404();
int not_exist();
void do_cat();

int main(int ac, char const *av[])
{
	int fd, sock_id;
	FILE *fp;
	char request[BUFSIZ];

	sock_id = make_server_socket(atoi(av[1]));
	while (1) {
		fd = accept(sock_id, NULL, NULL);
		fp = fdopen(fd, "r");
		fgets(request, BUFSIZ, fp);
		printf("request = %s\n", request);
		read_til_crnl(fp);
		process_rq(request, fd);
		fclose(fp);
	}
	
	return 0;
}

void process_rq(char *rq, int fd)
{
	char cmd[BUFSIZ], arg[BUFSIZ];
	int pid;
	pid = fork();
	if ( pid != 0 )
		return;
	
	strcpy(arg, ".");
	if ( sscanf(rq, "%s %s", cmd, arg+1) != 2)
		return;
	printf("cmd = %s\narg = %s\n", cmd, arg);
	
	if ( strcmp(cmd, "GET") != 0 )
		cannot_do(fd);
	else if ( not_exist(arg) )
		do_404(arg, fd);
	else
		do_cat(arg, fd);
	// printf("%s\n", rq);
}


void read_til_crnl(FILE *fp)
{
	char buf[BUFSIZ];
	while ( fgets(buf, BUFSIZ, fp) != NULL &&
			strcmp(buf, "\r\n") != 0);
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
	printf("do_404\n");
	FILE *fp = fdopen(fd, "w");
	fprintf(fp, "HTTP/1.0 404 Not Found\r\n");
	fclose(fp);
}

void do_cat(char *item, int fd)
{
	FILE *fp_sock = fdopen(fd, "w");
	FILE *fp_file = fopen(item, "r");
	int c;

	if ( fp_sock != NULL && fp_file != NULL) {
		printf("Enter if\n");
		fprintf(fp_sock, "HTTP/1.0 200 OK\r\n");
		fprintf(fp_sock, "Content-type: text/html\r\n");
		fprintf(fp_sock, "\r\n");
		while ( (c = getc(fp_file)) != EOF)
			fprintf(fp_sock, "%c\n", c);
		fclose(fp_sock);
		fclose(fp_file);
	}
}