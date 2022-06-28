#include <stdio.h>	/* printf(), fgets() */
#include <stdlib.h>	/* EXIT_SUCCESS, EXIT_FAILURE */
#include <stdint.h>	/* int32_t, uint8_t */
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define CREAT_FILENAME	"src"
#define CREAT_LEN	1048 * 1048	/* 1Gbyte位 */

int main(int argc, char *argv[])
{
	uint32_t file_len;
	uint32_t i;
	int fd;
	int ret;
	uint8_t buf[BUFSIZ + 1];
	uint8_t *p_buf;

	fd = creat(CREAT_FILENAME, 0644);
	if (fd == -1) {
		perror("cread");
		exit(errno);
	}
	file_len = 0;
	while (file_len <= CREAT_LEN) {
		p_buf = buf;
		i = 0;
		while ((i < BUFSIZ) && (file_len++ <= CREAT_LEN)) {
			*p_buf++ = rand() % ('z' - 'a') + 'a';
			i++;
		}
		file_len--;
		i--;
		ret = write(fd, buf, i);
		if (ret == -1) {
			perror("write");
			exit(errno);
		}
	}
	if (ret == -1) {
		perror("write");
		exit(errno);
	}
	close(fd);
    return EXIT_SUCCESS;
}

