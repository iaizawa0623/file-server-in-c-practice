#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif  /* _BSD_SOURCE */
#include "../mydef.h"
#include <stdio.h>  /* printf(), fgets() */
#include <stdlib.h> /* EXIT_SUCCESS, EXIT_FAILURE */
#include <stdint.h> /* int32_t, uint8_t */
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "../include/make_data.c"
#include "../include/get_pack.c"
#include "../include/post_pack.c"
#include "../include/set_signal.c"
#include "../include/keep_a_log.c"

#define SLEEP_TIM   1 /* ミリ秒 */
#define LOG_FILE	"log/logfile.txt"

int     ghost_sock;
int		logfile;

void	exit_close(int sig);

int main(int argc, char *argv[])
{
    int     ret;
	char	line[BUFSIZ];
    struct sockaddr_in  host_addr;
    pack_t  pack;
    pack_t  buf;
	fd_set	readfds;

	if (argc < 2) {
		fprintf(stderr, "引数が足りません\n");
		return 1;
	}
    host_addr.sin_family    = AF_INET;
    host_addr.sin_port      = htons(PORT_NUM);
    host_addr.sin_addr.s_addr   = htonl(INADDR_ANY);

	logfile = open(LOG_FILE, O_WRONLY | O_APPEND);
	if (logfile == -1) {
		if (errno == ENOENT) {
			logfile = creat(LOG_FILE, 0644);
		} else {
			perror("open");
		}
	}
	set_signal(SIGINT, exit_close);
    ghost_sock  = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    printf("\n接続を試みます...");
    ret = connect(ghost_sock, (struct sockaddr *)&host_addr, sizeof(host_addr));
    if (ret == -1) {
        if (errno == ECONNREFUSED) {
			fflush(stdout);
            perror("\n接続できませんでした");
        } else {
            perror("connect");
        }
        return 1;
    } else {
        printf("\n接続されました");
    }
    fcntl(ghost_sock, F_SETFL, O_NONBLOCK);
    /* 通信確立 */
    make_data(&pack);   /* 適当なデータを作る */
    while (true) {
		FD_ZERO(&readfds);
		FD_SET(ghost_sock, &readfds);

		pack.head.sequence += 2;
        ret = post_pack(ghost_sock, &pack); /* データを送信 */
        if (ret == -1) {
			fprintf(stderr, "\npost_pack error");
            break;
        } 
		ret = get_pack(ghost_sock, &buf);
		if (ret == -1) {
			fprintf(stderr, "\nget_pack error");
			break;
		} else if (ret == 0) {
			printf("\nサーバが終了しました");
			break;
		} else {	/* ret > 0 */
			sprintf(line, "-post- %s", argv[1]);
			keep_a_log(logfile, &pack, line);	/* 送信ログの記録 */
		}
		/* 比較 */
		if (pack.head.length != buf.head.length) {
			fprintf(stderr, "\nlengthが違う");
			break;
		} else if ((pack.head.sequence + 1) != buf.head.sequence) {
			fprintf(stderr, "\nseqenceが違う");
		} else if (strncmp((char *)pack.data, (char *)buf.data,
					pack.head.length) != 0) {
			for (ret = 0; (unsigned)ret < pack.head.length; ret++) {
				if (strncmp((char *)pack.data, (char *)buf.data,
							pack.head.length) != 0) {
					break;
				}
				fprintf(stderr, "\ndataの%dbyte目が違う", ret);
			}
		}

		usleep(SLEEP_TIM * 1000);
	}
	if (ghost_sock != -1) {
		close(ghost_sock);
	}
	printf("\n終了します\n");
	return 0;
}

void	exit_close(int sig) {
	printf("\nユーザ割込み");
	printf("\n終了します\n");
	if (ghost_sock != -1) {
		close(ghost_sock);
	}
	close(logfile);
	exit(EXIT_SUCCESS);
}

