
#include "put_time.c"
#include "set_signal.c"

#include <unistd.h>
int st;

void stop(int sig) {
	st = 0;
}

int main(int argc, char *argv[])
{
	set_signal(SIGINT, stop);
	st = 1;
	while (st) {
		sleep(10);
	}
	put_time();
    return 0;
}

