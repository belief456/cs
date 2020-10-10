#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>


int setup_fd(const char *port)
{
	int ret;
	int fd;
	struct addrinfo hints;
	struct addrinfo *results, *rp;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	ret = getaddrinfo(NULL, port, &hints, &results);
	if(0 != ret)
	{
		printf("getaddrinfo error %s\n", gai_strerror(ret));
		return -1;
	}

	for(rp = results; rp != NULL; rp = rp->ai_next)
	{
		fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(-1 == fd)
			continue;

		ret = bind(fd, rp->ai_addr, rp->ai_addrlen);
		if(-1 == ret)
		{
			close(fd);
			continue;
		}

		ret = listen(fd, 100);
		if(0 == ret)
			break;		/* Success */

		close(fd);
	}

	if(NULL == rp)
	{
		printf("setup socket error \n");
		freeaddrinfo(results);
		return -1;
	}

	freeaddrinfo(results);
	return fd;
}

int doit(int fd, struct sockaddr_un peer_addr, socklen_t peer_addr_size)
{
	int ret;
	char hbuf[NI_MAXHOST];
	char sbuf[NI_MAXSERV];

#define BUFLEN	512
	char buf[BUFLEN];
	int len;

	ret = getnameinfo((struct sockaddr *)&peer_addr, peer_addr_size, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf),\
			NI_NUMERICHOST | NI_NUMERICSERV);
	if(0 != ret)
		printf("getnameinfo error %s\n", gai_strerror(ret));

	ret = recv(fd, buf, BUFLEN, 0);
	if(-1 == ret || 0 == ret)
	{
		printf("recv from %s %s error \n", hbuf, sbuf);
		return -1;
	}

	buf[ret] = '\0';	/* NULL terminated?, necessary? */
	printf("recv from %s %s [%s]\n", hbuf, sbuf, buf);
	return 0;
}

int main(int argc, char **argv)
{
	int fd = -1;
	int nfd = -1;
	struct sockaddr_un peer_addr;
	socklen_t peer_addr_size;

	if(argc != 2)
	{
		printf("Usage:\t%s port\n", argv[0]);
		return -1;
	}

	fd = setup_fd(argv[1]);
	if(-1 == fd)
	{
		printf("setup port %s error\n", argv[1]);
		return -1;
	}

	peer_addr_size = sizeof(struct sockaddr_un);
	while(1)
	{
		nfd = accept(fd, (struct sockaddr *)&peer_addr, &peer_addr_size);
		if(-1 != nfd)
		{
			while(-1 != doit(nfd, peer_addr, peer_addr_size))
				sleep(1);
		}
		sleep(1);
	}

	return 0;
}


