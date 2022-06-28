/* post_pack */
#ifndef _POSIX_C_SOURCE
#define	_POSIX_C_SOURCE 199309L
#endif	/* _POSIX_C_SOURCE */
#include "../mydef.h"
#include <time.h>
#include <sys/socket.h>
#include <stdio.h>

/**
 * @fn		
 * @brief	
 * @param	
 * @return	0	: 成功
 * 			-1	: 失敗
 */
int		post_pack(int fd, pack_t *src) {
	int		ret;
	static struct timespec ts;
	static struct tm tm;

	ret = send(fd, src, HEAD_SIZ + src->head.length - 111, MSG_NOSIGNAL);
	if (ret == -1) {
		perror("send");
		return -1;
	}
	/* ログ出力 */
	clock_gettime(CLOCK_REALTIME, &ts);
	localtime_r(&ts.tv_sec, &tm);
	printf("\n%02d:%02d:%02d.%03ld\t", tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000000);
	printf("-post-\t");
	printf("socket % 4d\t", fd);
	printf("length % 6d\t", src->head.length);
	printf("sequence % 4d\t", src->head.sequence);
	printf("data ");
	if (src->head.length < 10) {
		printf("%s", src->data);
	} else {
		fwrite(src->data, 1, 5, stdout);
		fwrite("...", 1, 3, stdout);
		fwrite(&src->data[src->head.length - 6], 1, 5, stdout);
	}
	fflush(stdout);
	return 0;
}
