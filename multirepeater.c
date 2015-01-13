#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>

enum {
	n_faces = 8,
};

struct sock {
	const char *iface;
	int fd;
	in_addr_t addr;
};

void
usage(char *message){
	fprintf(stderr, "Usage: multirepeater (interface) (interface)+\n");
	exit(2);
}

struct sockaddr_in mcastaddr;

void
listen_on(struct sock *sock){
	int ifindex;
	ifindex = if_nametoindex(sock->iface);
	if(ifindex == 0){
		perror(sock->iface);
		exit(1);
	}

	sock->fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock->fd < 0){
		perror("socket");
		exit(1);
	}

	// join the mulitcast group
	struct ip_mreqn mreq;
	bzero(&mreq, sizeof(mreq));
	mreq.imr_ifindex = ifindex;
	mreq.imr_address.s_addr = INADDR_ANY;
	mreq.imr_multiaddr.s_addr = mcastaddr.sin_addr.s_addr;
	if(setsockopt(sock->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0){
		perror("setsockopt: IP_ADD_MEMBERSHIP");
		exit(1);
	}

	// bind to this interface
	struct ifreq ifr;
	bzero(&ifr, sizeof(ifr));
	strncpy(ifr.ifr_name, sock->iface, IFNAMSIZ);
	if(setsockopt(sock->fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0){
		perror("setsockopt: SO_BINDTODEVICE");
		exit(1);
	} 

	// get the primary ip
	// FIXME YUCK!
	if(ioctl(sock->fd, SIOCGIFADDR, &ifr) == 0){
		sock->addr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
	} 

	// do the actual binding
	struct sockaddr_in addr;
	bzero(&addr, sizeof(mcastaddr));
	addr.sin_family = AF_INET;
	addr.sin_port = mcastaddr.sin_port;
	addr.sin_addr.s_addr = INADDR_ANY;
	if(bind(sock->fd, (struct sockaddr *)&addr, sizeof(mcastaddr)) < 0){
		perror("bind");
		exit(1);
	}
}

void
handle_packet(int fd, int sc, struct sock *s){
	char buf[65536];
	int size, i;

	struct sockaddr_in from;
	socklen_t from_size = sizeof(from);
	size = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&from, &from_size);

	// make sure this packet isn't ours
	for(i=0; i<sc; i++){
		if(from.sin_addr.s_addr == s[i].addr) return;
	}
	
	for(i=0; i<sc; i++){
		// don't send back out on the same interface we got it on
		if(fd == s[i].fd) continue;

		if(sendto(s[i].fd, buf, size, 0, (struct sockaddr *)&mcastaddr, sizeof(mcastaddr)) < size){
			perror("sendto too small");
		}
	}
}

int
main(int argc, char **argv){
	int i, sc;
	struct sock s[n_faces];
	struct epoll_event pollev;
	int pollfd;

	/* initialize multicast address and port */
	bzero(&mcastaddr, sizeof(mcastaddr));
	mcastaddr.sin_family = AF_INET;
	mcastaddr.sin_port = htons(5353);
	mcastaddr.sin_addr.s_addr = inet_addr("224.0.0.251");

	sc = argc-1;
	if(sc < 2)
		usage("not enough interfaces");
	if(sc > n_faces)
		usage("too many interfaces");

	pollfd = epoll_create(sc);
	if(pollfd < 0){
		perror("epoll_create");
		exit(1);
	}

	pollev.events = EPOLLIN;
	for(i=0;i<sc;i++){
		s[i].iface = argv[i+1];
		listen_on(&s[i]);
		pollev.data.fd = s[i].fd;
		if(epoll_ctl(pollfd, EPOLL_CTL_ADD, s[i].fd, &pollev) < 0){
			perror("epoll_ctl");
			exit(1);
		}
	}

	while(1){
		if(epoll_wait(pollfd, &pollev, 1, -1) < 1){
			perror("epoll_wait");
			exit(1);
		}
		handle_packet(pollev.data.fd, sc, s);
	}
}
