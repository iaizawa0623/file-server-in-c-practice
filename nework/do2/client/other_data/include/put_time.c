
#ifndef	PUT_TIME
#define	PUT_TIME
#ifndef _POSIX_C_SOURCE
#define	_POSIX_C_SOURCE 199309L
#endif	/* _POSIX_C_SOURCE */
#include <time.h>
#include <stdio.h>

int put_time() {
	static struct timespec ts;
	static struct tm tm;

	clock_gettime(CLOCK_REALTIME, &ts);
	localtime_r(&ts.tv_sec, &tm);
	printf("\n%02d:%02d:%02d.%03ld\t", tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000000);

	return 0;
}
#endif /* PUT_TIME */
