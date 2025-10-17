#include <notstd/core.h>
#include <notstd/delay.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <netdb.h>

long www_ping(const char* host){
	struct addrinfo* res = NULL;
	struct addrinfo  req;
	memset(&req, 0, sizeof req);
	req.ai_family   = AF_UNSPEC;
	req.ai_socktype = SOCK_DGRAM;
	if( getaddrinfo(host, NULL, &req, &res) < 0 ){
		dbg_error("fail getaddrinfo");
		return -1;
	}
	struct sockaddr* addr = res->ai_addr;
	size_t addrsize = 0;
	//inet_pton(AF_INET, host, &addr.sin_addr);
	//char packet[sizeof(struct icmphdr)] = {0};
	char packet[64] = {0};
	int sck;
	if( res->ai_family == AF_INET ){
		dbg_info("ipv4");
		addrsize = sizeof(struct sockaddr_in);
		struct icmphdr* icmp = (struct icmphdr*)packet;
		icmp->type = ICMP_ECHO;
		icmp->code = 0;
		icmp->un.echo.id = getpid() &0xFFFF;
		icmp->un.echo.sequence = 1;
		icmp->checksum = 0;
		sck = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
		if( sck < 0 ) return -1;
	}
	else if( res->ai_family == AF_INET6 ){
		dbg_info("ipv6");
		addrsize = sizeof(struct sockaddr_in6);
		struct icmp6_hdr* icmp = (struct icmp6_hdr*)packet;
		icmp->icmp6_type = ICMP6_ECHO_REQUEST;
		icmp->icmp6_id   = getpid() &0xFFFF;
		icmp->icmp6_seq  = 1;
		sck = socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6);
		if( sck < 0 ) return -1;
	}
	else{
		dbg_error("wrong family");
		errno = EPROTONOSUPPORT;
		if( res ) freeaddrinfo(res);
		return -1;
	}
	struct timeval tv = {
		.tv_sec  = 1,
		.tv_usec = 0
	};
	if( setsockopt(sck, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv) < 0 ) goto ONERR;
	
	char buf[1024];
	const delay_t start = time_us();
	if( sendto(sck, packet, sizeof(packet), 0, addr, addrsize) < 0 ) goto ONERR;
	if( recv(sck, buf, 1024, 0) < 0 ){
		if( errno == EAGAIN ) errno = ETIME;
		goto ONERR;
	}
	const delay_t end = time_us();
	close(sck);
	freeaddrinfo(res);
	return end-start;
ONERR:
	close(sck);
	if( res ) freeaddrinfo(res);
	return -1;
}
