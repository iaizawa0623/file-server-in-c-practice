
#ifndef _POSIX_C_SOURCE
#define	_POSIX_C_SOURCE 199309L
#endif	/* _POSIX_C_SOURCE */

#ifndef	GET_PACK
#define GET_PACK

#include "../mydef.h"
#include <sys/socket.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>

bool	more;

void	timeout() {
	more = false;
}

/**
 * return	1:正常
 * 			0:相手が切断
 * 			-1:何らかのエラー(recv(), )
 * 			-2:タイムアウト
 */
int		get_pack(int fd, pack_t *dst) {
	int			ret;
	int64_t		read_len;
	struct sigaction	ss_alarm;
	static struct timespec ts;
	static struct tm tm;

	ss_alarm.sa_handler = timeout;
	ret = sigfillset(&ss_alarm.sa_mask);
	if (ret == -1) {
		perror("sigfillset");
		return -1;
	}
	ss_alarm.sa_flags = 0;
	ret = sigaction(SIGALRM, &ss_alarm, 0);
	if (ret == -1) {
		perror("sigaction");
		return -1;
	}

	more = true;
	alarm(ALARM_SEC);	/* 1秒 */
	read_len = HEAD_SIZ;
	while (true) {	/* head部を受け取り */
		ret = recv(fd, &dst->head + (HEAD_SIZ - read_len), read_len, 0);
		if (ret == -1) {
			if (errno != EAGAIN) {
				perror("\nrecv on get_pack head");
				return -1;
			}
		} else if (ret == 0) {
			return 0;	/* 相手が終了 */
		} else if (ret > 0){
			read_len -= ret;
			if (read_len <= 0) {
				alarm(0);	/* アラームをキャンセル */
				break;
			}
		}
		if (more == 0) {
			fprintf(stderr, "\nタイムアウト");
			return -2;	/* タイムアウト */
		}
	}
	more = true;
	alarm(ALARM_SEC);
	read_len = dst->head.length;
	if (read_len > DATA_SIZ) {
		fprintf(stderr, "\nバッファサイズを超えています");
		return -1;
	}
	while (true) {	/* data部の受け取り */
		ret = recv(fd, &dst->data[dst->head.length - read_len], read_len, 0);
		if (ret == -1) {
			if (errno != EAGAIN) {
				fprintf(stderr, "\ndst->head.length - read_len : %ld\n", dst->head.length - read_len);
				perror("\nrecv on get_pack data");
				return -1;
			}
		} else if (ret == 0) {
			return 0;	/* 相手が終了 */
		} else if (ret > 0){
			read_len -= ret;
			if (read_len <= 0) {
				alarm(0);	/* アラームをキャンセル */
				break;
			}
		}
		if (more == 0) {
			fprintf(stderr, "\nタイムアウト");
			return -2;	/* タイムアウト */
		}
	}
	/* ログの出力 */
	clock_gettime(CLOCK_REALTIME, &ts);
	localtime_r(&ts.tv_sec, &tm);
	printf("\n%02d:%02d:%02d.%03ld\t", tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000000);
	printf("-get -\t");
	printf("socket % 4d\t", fd);
	printf("length % 6d\t", dst->head.length);
	printf("sequence % 4d\t", dst->head.sequence);
	printf("data ");
	fflush(stdout);
	if (dst->head.length < 10) {
		printf("%s", dst->data);
	} else {
		fwrite(dst->data, 1, 5, stdout);
		fwrite("...", 1, 3, stdout);
		fwrite(&dst->data[dst->head.length - 6], 1, 5, stdout);
	}
	return 1;
}

#endif /* GET_PACK */
