/**
 * 	ファイルサーバ
 */
#define _POSIX_C_SOURCE 199309L
#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>

#define PORT 5000
#define DETA_SIZ 256 * 1024
#define WAIT_TIM 3
#define MAX(a,b) (a > b ? a : b)
#define MAX_CLIT 3

typedef struct {
	uint32_t	length;	/* dataの大きさ(byte) */
	uint32_t	sequence;	/* シーケンス番号 */
} head_t;

#define HEAD_SIZ sizeof(head_t)

typedef struct {
	head_t	head;
	uint8_t	data[DETA_SIZ];
} packet_t;

int	g_host_sock;	/* サーバソケット */
int	g_host_sock = -1;
int g_clit_sock[MAX_CLIT];	/* クライアントソケット */
int i;
struct timeval tv;	/* select */
struct timespec ts;
struct tm tm;

void	exit_err(char *msg);	/* エラー終了 */
void	sigint_h(int sig);	/* 割込みシグナルのハンドラ */
int		create_listen_socker();	/* 新しいlisten状態のソケット */
void	set_signal(int sig, void (*func)(int));	/* シグナルハンドラを指定 */
void	send_pack(int fd, packet_t *p_data);	/* パケットの送信 */
int 	get_pack(int fd, packet_t *dst);	/* パケット受信 */

int main(int argc, char *argv[])
{
	int ret;
	uint32_t count;
	uint32_t clit_num;
	int max_sock;		/* 使用するディスクリプタの最大値 */
	fd_set read_fds;	/* 読み取り用のディスク0リプタ群 */
	struct sockaddr_in clint_addr[MAX_CLIT];
	packet_t packet;	/* 通信の単位 */

	clit_num = 0;
	g_host_sock = create_listen_socker();
	set_signal(SIGINT, sigint_h);
	for (count = 0; count < MAX_CLIT; count++) {
		g_clit_sock[count] = -1;
	}

	system("tabs -4");
	printf("接続を待っています.");
	fflush(stdout);
	while (true) {
		FD_ZERO(&read_fds);
		max_sock = -1;
		if (g_host_sock != -1) {
			FD_SET(g_host_sock, &read_fds);
			max_sock = g_host_sock;
		}
		for (count = 0; count < MAX_CLIT; count++) {
			if (g_clit_sock[count] != -1) {
				FD_SET(g_clit_sock[count], &read_fds);
				max_sock = MAX(max_sock, g_clit_sock[count]);
			}
		}
		tv.tv_sec = WAIT_TIM;
		tv.tv_usec = 0;
		ret = select(max_sock + 1, &read_fds, NULL, NULL, &tv);
		if (ret == -1) {
			exit_err("select");
		} else if (ret > 0) {
			if (FD_ISSET(g_host_sock, &read_fds)) { /* 新規接続 */
				static uint32_t caddr_siz = sizeof(clint_addr);
				for (count = 0; count < MAX_CLIT; count++) {
					if (g_clit_sock[count] == -1) {
						break;
					}
				}
				g_clit_sock[count] = accept(g_host_sock,
						(struct sockaddr *)&clint_addr, &caddr_siz);
				printf("\n新たなクライアントが接続しました(sock:%d ip:%s port:%d)",
						g_clit_sock[count], inet_ntoa(clint_addr[count].sin_addr),
						clint_addr[count].sin_port);
				fflush(stdout);
				/* ノンブロッキング */
				fcntl(g_clit_sock[count], F_SETFL, O_NONBLOCK);
				if (++clit_num >= MAX_CLIT) {
					close(g_host_sock);
					g_host_sock = -1;
				}
			}
			for (count = 0; count < MAX_CLIT; count++) {
				if (FD_ISSET(g_clit_sock[count], &read_fds)) {
					ret = get_pack(g_clit_sock[count], &packet);
					if (ret == 0) {
						printf("\nクライアントが切断しました(sock:%d ip:%s port:%d)",
								g_clit_sock[count], inet_ntoa(clint_addr[count].sin_addr),
								clint_addr[count].sin_port);
						fflush(stdout);
						close(g_clit_sock[count]);
						g_clit_sock[count] = -1;
						clit_num--;
						if (g_host_sock == -1) {
							g_host_sock = create_listen_socker();	/* 受け入れ再開 */
						}
					} else if (ret == -1) {
						fprintf(stderr, "\nクライアントが不正なデータを送信");
						fprintf(stderr, "\nクライアントを切断します(sock:%d ip:%s port:%d)",
								g_clit_sock[count], inet_ntoa(clint_addr[count].sin_addr),
								clint_addr[count].sin_port);
						close(g_clit_sock[count]);
						g_clit_sock[count] = -1;
						clit_num--;
					} else {	/* ret > 0 */
						packet.head.sequence++;
						send_pack(g_clit_sock[count], &packet);	/* 送り返す */
					}
				}
			}
		} else {
			write(STDOUT_FILENO, ".", 1);
		}
	}
	return EXIT_SUCCESS;
}

int		create_listen_socker() {
	int32_t host_sock;
	int32_t ret;	/* 関数の返り値。主にエラーチェック */
	struct sockaddr_in host_addr;	/* ホストアドレス構造体 */
	int32_t sock_opt_value;		/* setsockopt()で使用 */
	socklen_t	sock_opt_size;	/* setsockopt()で使用 */

	/* アドレス構造体を設定 */
	host_addr.sin_family = AF_INET;
	host_addr.sin_port = htons(PORT);
	host_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	/* ソケット作る */
	host_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (host_sock == -1) {
		exit_err("socket host_sock" );
	}
	/* 使用中のアドレスポートへのバインドを許可 */
	sock_opt_value = 1;
	sock_opt_size = sizeof(sock_opt_value);
	setsockopt(host_sock, SOL_SOCKET, SO_REUSEADDR,
			(void *)&sock_opt_value, sock_opt_size);
	/* ノンブロッキング */
	fcntl(host_sock, F_SETFL, O_NONBLOCK);
	/* 名前を付ける */
	ret = bind(host_sock, (struct sockaddr *)&host_addr, sizeof(host_addr));
	if (ret == -1) {
		exit_err("bind to host_sock");
	}
	listen(host_sock, 1);
	return host_sock;
}

void	set_signal(int sig, void (*func)(int)) {
	int32_t ret;	/* 関数の返り値。主にエラーチェック */
	struct sigaction handler;	/* シグナルハンドラを指定する構造体 */

	/* シグナルハンドラ指定 */
	handler.sa_handler = func;
	/* 全シグナルをマスク */
	ret = sigfillset(&handler.sa_mask);
	if (ret == -1) {
		exit_err("sigfillset");
	}
	/* フラグなし */
	handler.sa_flags = 0;
	/* 割り込みシグナルに対する処理を設定 */
	ret = sigaction(sig, &handler, 0);
	if (ret == -1) {
		exit_err("sigaction");
	}
}

void	sigint_h(int sig) {
	exit_err("\rsigintを受け取りました");
}

void	exit_err(char *msg) {
	int err_num;
	uint32_t num;
	err_num = errno;

	fprintf(stderr, "\n");
	if (msg != NULL) {
		perror(msg);
	}
	if (g_host_sock > 0) {
		close(g_host_sock);
	}
	for (num = 0; num < MAX_CLIT; num++) {
		if (g_clit_sock[num] > 0) {
			close(g_clit_sock[num]);
		}
	}
	printf("\n終了します\n");
	exit(err_num);
}

int 	get_pack(int fd, packet_t *dst) {
	int ret;
	uint32_t count;
	int64_t read_len;
	int retval;

	retval = 1;
	ret = recv(fd, &dst->head, HEAD_SIZ, 0);
	if (ret == -1) {
		exit_err("recv head");
	} else if (ret == 0) {
		retval = 0;
		return retval;	/* クライアントが切断 */
	}
	read_len = dst->head.length;
	for (count = 0; read_len > 0; count++) {
		errno = 0;
		ret = recv(fd, &dst->data[dst->head.length - read_len], read_len, 0);
		if (ret == -1) {
			if (errno == EAGAIN) {
			} else {
				exit_err("recv data");
			}
		} else {
			read_len -= ret;
		}
		if (count > 1000) {
			retval = -1;
			fprintf(stderr, "...最後まで来ない count is %d", count);
			break;
		}
	}
	/* ログ出力 */
	clock_gettime(CLOCK_REALTIME, &ts);
	localtime_r(&ts.tv_sec, &tm);
	printf("\n%02d:%02d:%02d.%03ld\t", tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000000);
	printf("-get -\t");
	printf("socket % 4d\t", fd);
	printf("length % 6d\t", dst->head.length);
	printf("sequence % 4d\t", dst->head.sequence);
	printf("data ");
	fflush(stdout);
	if (dst->head.length < 5) {
		printf("%s", dst->data);
	} else {
		write(STDOUT_FILENO, dst->data, 5);
		write(STDOUT_FILENO, "...", 3);
		write(STDOUT_FILENO, &dst->data[dst->head.length - 6], 5);
	}

	return retval;
}

void	send_pack(int fd, packet_t *p_data) {
	int ret;

	ret = send(fd, p_data, HEAD_SIZ + (p_data->head.length), 0);
	if (ret == -1) {
		perror("send");
		return;
	}
	/* ログ出力 */
	clock_gettime(CLOCK_REALTIME, &ts);
	localtime_r(&ts.tv_sec, &tm);
	printf("\n%02d:%02d:%02d.%03ld\t", tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000000);
	printf("-post-\t");
	printf("socket % 4d\t", fd);
	printf("length % 6d\t", p_data->head.length);
	printf("sequence % 4d\t", p_data->head.sequence);
	printf("data ");
	fflush(stdout);
	if (p_data->head.length < 5) {
		printf("%s", p_data->data);
	} else {
		write(STDOUT_FILENO, p_data->data, 5);
		write(STDOUT_FILENO, "...", 3);
		write(STDOUT_FILENO, &p_data->data[p_data->head.length - 6], 5);
	}
}
