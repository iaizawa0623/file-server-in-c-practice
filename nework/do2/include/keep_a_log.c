#ifndef	_POSIX_SOURCE
#define	_POSIX_SOURCE
#define S_IFMT   0170000                /* type of file mask */
#define S_IFIFO  0010000                /* named pipe (fifo) */
#define S_IFCHR  0020000                /* character special */
#define S_IFDIR  0040000                /* directory */
#define S_IFBLK  0060000                /* block special */
#define S_IFREG  0100000                /* regular */
#define S_IFLNK  0120000                /* symbolic link */
#define S_IFSOCK 0140000                /* socket */
#define S_IFWHT  0160000                /* whiteout */
#define S_ISVTX  0001000                /* save swapped text even after use */
#endif	/* _POSIX_SOURCE */

#include "../mydef.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

int		keep_a_log(int	fd, pack_t *src, char *str) {
	static struct timespec	ts;
	static struct tm		tm;
	uint32_t	buf_len;
	uint32_t	inx;
	char		line_buf[BUFSIZ];
	char		add_buf[512];
	clock_gettime(CLOCK_REALTIME, &ts);
	localtime_r(&ts.tv_sec, &tm);

	buf_len = strlen(line_buf);
	if (buf_len > BUFSIZ) {
		return 1;
	}
	sprintf(line_buf, "\n%02d:%02d:%02d.%03ld\t", tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000000);

	sprintf(add_buf, "-%s-\t", str);
	buf_len += strlen(add_buf);
	if (buf_len > BUFSIZ) {
		return 1;
	}
	strcat(line_buf, add_buf);

	sprintf(add_buf, "length % 6d\t", src->head.length);
	buf_len += strlen(add_buf);
	if (buf_len > BUFSIZ) {
		return 1;
	}
	strcat(line_buf, add_buf);

	sprintf(add_buf, "sequence % 4d\t", src->head.sequence);
	buf_len += strlen(add_buf);
	if (buf_len > BUFSIZ) {
		return 1;
	}
	strcat(line_buf, add_buf);

	sprintf(add_buf, "data ");
	buf_len += strlen(add_buf);
	if (buf_len > BUFSIZ) {
		return 1;
	}
	strcat(line_buf, add_buf);

	buf_len += 13;
	if (buf_len > BUFSIZ) {
		return 1;
	}
	if (src->head.length < 10) {
		sprintf(add_buf, "%s", src->data);
		buf_len += strlen(add_buf);
	} else {
		for (inx = 0; inx < 5; inx++) {
			strncat(line_buf, (char *)&src->data[inx], 1);
		}
		strcat(line_buf, "...");
		for (inx = 0; inx < 5; inx++) {
			strncat(line_buf, (char *)&src->data[inx + (src->head.length - 6)], 1);
		}
	}
	strcat(line_buf, add_buf);
	write(fd, line_buf, strlen(line_buf));	/* 最後に一括 */
	return 0;
}

int		exist_file(char *path) {
	struct stat	st;
	int		ret;

	ret = stat(path, &st);
	if (ret == 0) {
		return 0;
	}
	return (st.st_mode & S_IFMT) == S_IFREG;
}

