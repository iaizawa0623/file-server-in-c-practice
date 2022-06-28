#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/uio.h>

#define	PORT		5000
#define DATA_SIZ	256 * 1024
#define MAX(a,b)	(a > b ? a : b)
#define WAIT		500000

typedef struct {
	uint32_t	length;
	uint32_t	sequence;
} head_t;

#define HEAD_SIZ	sizeof(head_t)

typedef struct {
	head_t	head;
	uint8_t	data[DATA_SIZ];
} packet_t;

int	g_host_sock;
int	g_host_sock = -1;
time_t gtimer;
struct tm *gtm;
struct timeval tv;
struct timespec ts;
struct tm tm;

void	exit_err(char *msg);	/* エラー終了 */
void	sigint_h(int sig);	/* シグナルハンドラ(exit_errを呼び出す) */
void	set_signal(int sig, void (*func)(int));	/* シグナルハンドラ指定 */
void	send_pack(int fd, packet_t *p_data);	/* パケットの送信 */
int 	get_pack(int fd, packet_t *dst, packet_t *src);	/* パケット受信 */

int main(int argc, char *argv[])
{
	int ret;
	struct sockaddr_in host_addr;
	packet_t packet;	/* 送信 */
	packet_t buf;		/* 受信buf */
	struct timeval tv;

	srand((unsigned)time(NULL));
	/* シグナルハンドラ */
	set_signal(SIGINT, sigint_h);	/* <C-c>で終了 */
	/* アドレス構造体 */
	host_addr.sin_family = AF_INET;
	host_addr.sin_port	= htons(PORT);
	host_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	/* ソケット */
	g_host_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	/* コネクト */
	printf("接続を試みます...\n");
	ret = connect(g_host_sock, (struct sockaddr *)&host_addr, sizeof(host_addr));
	fcntl(g_host_sock, F_SETFL, O_NONBLOCK);
	if (ret == -1) {
		if (errno == ECONNREFUSED) {
			exit_err("接続をできませんでした...");
		} else {
			exit_err("connect");
		}
	} else {
		printf("接続されました\n");
	}
	/* 通信確立 */
	packet.head.sequence = 0;
	while (true) {
		/* data生成 */
		memset(packet.data, '\0', DATA_SIZ);
		for (ret = 0; ret < DATA_SIZ; ret++) {
			packet.data[ret] = (rand() % ('z' - 'a')) + 'a';
		}
		packet.head.length = ret;
		packet.head.sequence++;
		do {
			send_pack(g_host_sock, &packet);	/* パケットを送信 */
			memset(buf.data, '\0', DATA_SIZ);
			ret = get_pack(g_host_sock, &buf, &packet);	/* パケット受信 */
			if (ret == -1) {
				exit_err(NULL);
			}
			packet.head.sequence += 2;
			tv.tv_usec = WAIT;
			select(0, NULL, NULL, NULL, &tv);
		} while (true);
	}
	return EXIT_SUCCESS;
}

void	exit_err(char *msg) {
	int err_num;
	err_num = errno;

	fprintf(stderr, "\n");
	if (msg != NULL) {
		perror(msg);
	}
	if (g_host_sock > 0) {
		close(g_host_sock);
	}
	printf("終了します\n");
	exit(err_num);
}

void	sigint_h(int sig) {
	exit_err("\rsigintを受け取りました\n");
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

int 	get_pack(int fd, packet_t *dst, packet_t *src) {
	int ret;
	int retval;
	int32_t read_ren;
	uint32_t count;

	retval = 0;
	count = 0;
	read_ren = HEAD_SIZ;


	while (true) {
		ret = recv(fd, &dst->head, HEAD_SIZ, 0);
		if (ret == -1) {
			if (errno != EAGAIN) {
				perror("recb head");
				break;
			} else {
			}
		} else {
			break;
		}
	}
	read_ren = dst->head.length;
	for (count = 0; read_ren > 0; count++) {
		ret = recv(fd, &dst->data[dst->head.length - read_ren], read_ren, 0);
		if (ret == -1) {
			if (errno == EAGAIN) {
			} else {
				exit_err("recv data");
			}
		} else {
			read_ren -= ret;
		}
		if (count > 1000) {
			retval = -1;
			fprintf(stderr, "\ndataが来ない");
			break;
		}
	}
	/* ログ出力 */
	clock_gettime(CLOCK_REALTIME, &ts);
	localtime_r(&ts.tv_sec, &tm);
	printf("\n%02d:%02d:%02d.%03ld\t",
			tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000000);
	printf("-get -\t");
	printf("length	: %d\t", dst->head.length);
	printf("sequence: %d\t", dst->head.sequence);
	printf("data	: ");
	fflush(stdout);
	if (dst->head.length < 5) {
		printf("%s", dst->data);
	} else {
		write(STDOUT_FILENO, dst->data, 5);
		write(STDOUT_FILENO, "...", 3);
		write(STDOUT_FILENO, &dst->data[dst->head.length - 6], 5);
	}
	if (retval == 0) {
		if (dst->head.length != src->head.length) {
			printf("lengthが誤り\n");
			retval = -1;	/*  */
		}
		if (dst->head.sequence - 1 != src->head.sequence)  {
			printf("sequenceが誤り\n");
			retval = -1;	/*  */
		}
		ret = strncmp((char *)dst->data, (char *)src->data, src->head.length);
		if (ret != 0) {
			uint32_t i;
			printf("dataが誤り\t");
			for (i = 0; i < src->head.length; i++) {
				ret = strncmp((char *)dst->data, (char *)src->data, i);
				if (ret != 0) {
					break;
				}
			}
			printf("%d文字目\n", i);
			retval = -1;	/*  */
		}
	}
	return retval;
}

void	send_pack(int fd, packet_t *p_data) {
	int ret;

	ret = send(fd, p_data, HEAD_SIZ + (p_data->head.length), 0);
	if (ret == -1) {
		exit_err("send");
	}
	/* ログ出力 */
	clock_gettime(CLOCK_REALTIME, &ts);
	localtime_r(&ts.tv_sec, &tm);
	printf("\n%02d:%02d:%02d.%03ld\t",
			tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000000);
	printf("-post-\t");
	printf("length	: %d\t", p_data->head.length);
	printf("sequence: %d\t", p_data->head.sequence);
	printf("data : ");
	fflush(stdout);
	write(STDIN_FILENO, p_data->data, 5);
	write(STDIN_FILENO, "...", 3);
	write(STDIN_FILENO, &p_data->data[p_data->head.length - 6], 5);
	fflush(stdout);
}
