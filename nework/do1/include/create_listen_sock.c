
#include "../mydef.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

int		create_listen_sock(void) {
	int		host_sock;
	int		ret;
	struct sockaddr_in	host_addr;
	int		opt_value;
	socklen_t	opt_siz; 
	host_addr.sin_family		= AF_INET;
	host_addr.sin_port			= htons(PORT_NUM);
	host_addr.sin_addr.s_addr	= htonl(INADDR_ANY);

	host_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (host_sock == -1) {
		return -1;
	}
	opt_value	= 1;
	opt_siz		= sizeof(opt_value);
	setsockopt(host_sock, SOL_SOCKET, SO_REUSEADDR,
			&opt_value, opt_siz);
	fcntl(host_sock, F_SETFL, O_NONBLOCK);
	ret = bind(host_sock, (struct sockaddr *)&host_addr, sizeof(host_addr));
	if (ret == -1) {
		return -1;
	}
	listen(host_sock, 1);
	return host_sock;
}

