#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* refer getaddrinfo man-page */
int setup_fd(const char *addr, const char *port)
{
	int fd = -1, ret = -1;
	struct addrinfo hints;
	struct addrinfo *results, *rp;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICHOST;
	hints.ai_protocol = 0;
		
	ret = getaddrinfo(addr, port, &hints, &results);
	if(0 != ret)
	{
		printf("getaddrinfo error %s\n", gai_strerror(ret));
		return -1;
	}
	
	/* result is a list, try every one until success */
	for(rp = results; rp != NULL; rp = rp->ai_next)
	{
		fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(-1 == fd)
			continue;
		ret = connect(fd, rp->ai_addr, rp->ai_addrlen);
		if(-1 != ret)
			break;		/* Success */
		close(fd);
	}

	if(NULL == rp)
	{
		printf("could not connect %s %s\n", addr, port);
		freeaddrinfo(results);
		return -1;
	}

	freeaddrinfo(results);
	return fd;
}

int doit(int fd)
{
#define BUFLEN 512
	int ret;
	unsigned char buf[BUFLEN];
	int len;
	char timestr[64];
	time_t t;
	struct tm *tmp;

	t = time(NULL);
	tmp = localtime(&t);
	if(NULL == tmp)
	{
		printf("localtime error\n");
		return -1;
	}

	ret = strftime(timestr, sizeof(timestr), "%Y%m%d %H:%M:%S", tmp);
	if(0 == ret)
	{
		printf("strftime error\n");
		return -1;
	}
	
	snprintf(buf, BUFLEN, "%s %s", timestr, "Hello there! Have a nice day!");
	len = strlen(buf);

	ret = send(fd, buf, len, 0);

	if(len != ret)
	{
		printf("%d of %d sent!\n", ret, len);
	}

	return 0;
}

int main(int argc, char **argv)
{
	int fd = -1;
		
	if(argc != 3)
	{
		printf("usage:\t%s ip port\n", argv[0]);
		return 0;
	}

	fd = setup_fd(argv[1], argv[2]);
	if(-1 == fd)
	{
		printf("setup fd error %s %s\n", argv[1], argv[2]);
		return -1;
	}

	while(1)
	{
		doit(fd);
		sleep(1);
	}

	return 0;
}
