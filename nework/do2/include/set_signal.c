
#include <signal.h>

int set_signal(int sig, void (*func)(int)) {
	int ret;
	struct sigaction handler;

	handler.sa_handler = func;
	handler.sa_flags = 0;
	ret = sigfillset(&handler.sa_mask);
	if (ret == -1) {
		return -1;
	}
	ret = sigaction(sig, &handler, 0);
	if (ret == -1) {
		return -1;
	}
	return 0;
}
