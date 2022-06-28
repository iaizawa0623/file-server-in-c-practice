/**
 * 	エコーサーバ
 */
#include "../mydef.h"
#include <stdio.h>	/* printf(), fgets() */
#include <stdlib.h>	/* EXIT_SUCCESS, EXIT_FAILURE */
#include <stdbool.h>
#include <stdint.h>	/* int32_t, uint8_t */
#include <sys/select.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include "../include/create_listen_sock.c"
#include "../include/get_pack.c"
#include "../include/post_pack.c"
#include "../include/set_signal.c"
#include "../include/keep_a_log.c"

static int		ghost_sock = -1;
static int		gclit_sock[MAX_CLIT];		/* クライアントソケット */
static int		logfile[MAX_CLIT];	/* ログファイル */

void	exit_close(int sig);

int		main(int argc, char *argv[])
{
	int			ret;	/* 戻り値 */
	uint8_t		line[128];
	uint16_t	clit_num;	/* 接続中のクライアント数 */
	uint16_t	max_sock;	/* ディスクリプタの最大値 */
	uint16_t	iter;
	struct sockaddr_in clit_addr[MAX_CLIT];
	uint32_t	addr_siz;
	fd_set		read_fds;
	struct timeval tv;	/* select */
	pack_t		packet;

	system("tabs -4");
	clit_num = 0;
	set_signal(SIGINT, exit_close);
	ghost_sock = create_listen_sock();
	addr_siz = sizeof(clit_addr);
	for (iter = 0; iter < MAX_CLIT; iter++) {
		static uint16_t	iiter = 0;
		/* 初期化 */
		gclit_sock[iter] = -1;

		/* ログファイル */
		sprintf((char *)line, "log/logfile_of_%d.txt", ++iiter);
		logfile[iter] = open((char *)line, O_WRONLY | O_APPEND);
		if (logfile[iter] == -1) {
			logfile[iter] = creat((char *)line, 0644);	/* 新規作成 */
			printf("\nログファイル「%s」を作成しました", line);
		} else {
			printf("\nログファイル「%s」を開きました", line);
		}
		fflush(stdout);
	}
	printf("\n接続を待っています.");
	fflush(stdout);
	while (true) {
		max_sock = -1;
		FD_ZERO(&read_fds);
		if (ghost_sock != -1) {	/* ホストソケットを追加 */
			FD_SET(ghost_sock, &read_fds);
			max_sock = ghost_sock;
		}
		for (iter = 0; iter < MAX_CLIT; iter++) {
			if(gclit_sock[iter] != -1) {
				FD_SET(gclit_sock[iter], &read_fds);
				max_sock = LARGER(gclit_sock[iter], max_sock);
			}
		}
		tv.tv_sec	= SELECT_SEC;
		tv.tv_usec	= SELECT_USEC;
		ret = select(max_sock + 1, &read_fds, NULL, NULL, &tv);
		if (ret == -1) {
			if (errno != EINTR) {
				perror("\nselect");
			}	
		} else if (ret > 0) {
			/* 新規接続 */
			if (FD_ISSET(ghost_sock, &read_fds)) {
				iter = 0;
				while (gclit_sock[iter] != -1) {	/* 使用可能なソケットを探す */
					iter++;
				}
				gclit_sock[iter] = accept(ghost_sock,
						(struct sockaddr *)&clit_addr[iter], &addr_siz);
				if (gclit_sock[iter] == -1) {
					perror("accept");
				}
				printf("\n新たなクライアントが接続しました(sock:%d ip:%s port:%d)",
						gclit_sock[iter], inet_ntoa(clit_addr[iter].sin_addr),
						clit_addr[iter].sin_port);
				fflush(stdout);
				/* ノンブロッキング */
				fcntl(gclit_sock[iter], F_SETFL, O_NONBLOCK);
				if (++clit_num >= MAX_CLIT) {	/* 受け入れ停止 */
					close(ghost_sock);
					ghost_sock = -1;
					printf("\nクライアント数が上限に達しました");
					printf("\n受け入れを一時中断します");
					fflush(stdout);
				}
			}
			for (iter = 0; iter < MAX_CLIT; iter++) {
				if (gclit_sock[iter] != -1) {
					if (FD_ISSET(gclit_sock[iter], &read_fds)) {
						ret = get_pack(gclit_sock[iter], &packet);
						if (ret == 0) {
							printf("\nクライアントが切断しました(sock:%d ip:%s port:%d)",
									gclit_sock[iter], inet_ntoa(clit_addr[iter].sin_addr),
									clit_addr[iter].sin_port);
							fflush(stdout);
							close(gclit_sock[iter]);
							gclit_sock[iter] = -1;
							clit_num--;
							if (ghost_sock == -1) {
								ghost_sock = create_listen_sock();
								printf("\n新規接続の受け入れを再開します");
								fflush(stdout);
							}
						} else if (ret == -1 ||  ret == -2) {
							fprintf(stderr, "\nクライアントが不正なデータを送信");
							fprintf(stderr, "\nクライアントを切断します(sock: %d ip:%s port:%d)",
									gclit_sock[iter], inet_ntoa(clit_addr[iter].sin_addr),
									clit_addr[iter].sin_port);
							close(gclit_sock[iter]);
							gclit_sock[iter] = -1;
							clit_num--;
						} else {	/* ret > 0 */
							keep_a_log(logfile[iter], &packet, "-get -");	/* 受信ログの記録 */
							packet.head.sequence++;
							post_pack(gclit_sock[iter], &packet);	/* 送り返す */
						}
					}
				}
			}
		} else {
			printf(".");
			fflush(stdout);
		}
	}
/* ここには来ない */
}

void	exit_close(int sig) {
	uint16_t iter;

	printf("\nユーザ割込み");
	printf("\n終了します\n");
	if (ghost_sock != -1) {
		close(ghost_sock);
	}
	for (iter = 0; iter < MAX_CLIT; iter++) {
		if (gclit_sock[iter] != -1) {
			close(gclit_sock[iter]);
		}
		close(logfile[iter]);
	}
	exit (EXIT_SUCCESS);
}

