#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#define oops(x) {perror(x); exit(1);}
#define HOSTLEN 256
#define PORTNUM 13000

int make_server_socket(int portnum)
{
	int sock_id;
	struct sockaddr_in saddr;
	struct hostent *hp;
	char hostname[HOSTLEN];

	sock_id = socket(PF_INET, SOCK_STREAM, 0);
	bzero( (void *)&saddr, sizeof(saddr));
	gethostname( hostname, HOSTLEN);
	hp = gethostbyname( hostname );
	bcopy( (void*)hp->h_addr,
		   (void*)&saddr.sin_addr,
		   hp->h_length);
	saddr.sin_port = htons(portnum);
	saddr.sin_family = AF_INET;
	bind(sock_id,
		(struct sockaddr*)&saddr,
		sizeof(saddr));

	listen(sock_id, 1);
	
	return sock_id;
}

int make_server_socket_nb(int portnum)
{
	int sock_id;
	struct sockaddr_in saddr;
	struct hostent *hp;
	char hostname[HOSTLEN];

	if ((sock_id = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
		oops("make server socket");
	bzero( (void *)&saddr, sizeof(saddr));
	gethostname( hostname, HOSTLEN);
	hp = gethostbyname( hostname );
	bcopy( (void*)hp->h_addr,
		   (void*)&saddr.sin_addr,
		   hp->h_length);
	saddr.sin_port = htons(portnum);
	saddr.sin_family = AF_INET;
	bind(sock_id,
		(struct sockaddr*)&saddr,
		sizeof(saddr));

	listen(sock_id, 10);
	
	return sock_id;
}
