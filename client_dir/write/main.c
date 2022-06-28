
#include <unistd.h>

int main() {
	write(STDIN_FILENO, "@", 1);
	return 0;
}
