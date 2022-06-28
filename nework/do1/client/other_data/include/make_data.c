
#include "../mydef.h"
#include <time.h>
#include <stdlib.h>

int make_data(pack_t *pack) {
	uint32_t i;
	srand(time(NULL));

	i = 0;
	while (i < DATA_SIZ) {
		pack->data[i++] = rand() % ('z' - 'a') + 'a';
	}
	pack->head.length	= i;
	pack->head.sequence	= -1;
	return 1;
}

