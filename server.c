#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>

struct thread_info
{
	pthread_t tid;
	struct sockaddr_in addr;
	int fd;
};

#if 0
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
#endif

#if 1
/* simple way */
int setup_fd(const char *port)
{
	int ret;
	int fd;
	struct sockaddr_in addr;

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(port));
	//addr.sin_addr.s_addr = inet_addr(addr_str);
	addr.sin_addr.s_addr = INADDR_ANY;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == fd)
		return -1;

	ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	if(-1 == ret)
	{
		close(fd);
		return -1;
	}

	ret = listen(fd, 100);
	if(0 != ret)
	{
		close(fd);
		return -1;
	}

	return fd;
}
#endif

static void *thread_start(void *arg)
{
	int ret;
	char hbuf[NI_MAXHOST];
	char sbuf[NI_MAXSERV];
	struct thread_info *tinfo = arg;

#define BUFLEN	512
	char buf[BUFLEN + 1];
	int len;

	ret = getnameinfo((struct sockaddr *)&tinfo->addr, sizeof(tinfo->addr), hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
	if(0 != ret)
		printf("getnameinfo error %s\n", gai_strerror(ret));
	else
	{
		printf("connect from %s %s! \n", hbuf, sbuf);
	}

	while(1)
	{
		ret = recv(tinfo->fd, buf, BUFLEN, 0);
		if(0 == ret)
		{
			printf("%s %s bye!\n", hbuf, sbuf);
			break;
		}
		else if(-1 == ret)
		{
			printf("recv from %s %s error \n", hbuf, sbuf);
			break;
		}

		buf[ret] = '\0';	/* NULL terminated?, necessary? */
		printf("\tfrom %s %s [%s]\n", hbuf, sbuf, buf);
	}

	return NULL;
}

int main(int argc, char **argv)
{
	int fd = -1, nfd = -1;
#define MAX_TNUM 1000
	struct thread_info *tinfo = NULL;
	socklen_t peer_addr_size;
	int t_num = 0;

	if(argc < 2)
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

	tinfo = calloc(MAX_TNUM, sizeof(struct thread_info));
	if(NULL == tinfo)
	{
		printf("calloc error...\n");
		return -1;
	}

	peer_addr_size = sizeof(struct sockaddr_in);
	while(1)
	{
		nfd = accept(fd, (struct sockaddr *)&tinfo[t_num].addr, &peer_addr_size);
		if(-1 != nfd)
		{
			/*
			while(-1 != doit(nfd, peer_addr))
				sleep(1);
			*/
			tinfo[t_num].fd = nfd;
			pthread_create(&tinfo[t_num].tid, NULL, thread_start, &tinfo[t_num]);
			t_num++;
		}
	}

	return 0;
}

