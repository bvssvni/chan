
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <windows.h>

#include "chan.h"

CHAN_TYPE_DECLARE(int)
CHAN_TYPE(int)

static long seed = 120932;
int random(int lim)
{
	seed = (seed * 125) % 2796203;
	return (int)(seed % lim);
}

void randomize()
{
	seed = time(NULL) % 86400;
}

void *crack(void *arg)
{
	chan_int *ch = (chan_int*)arg;
	
	while (1) {
		int r = random(100);
		int err = chan_int_write(ch, r);
		if (err == CHAN_ERROR_DISCONNECTED) return NULL;
		Sleep(r);
	}

	return NULL;
}

int main(int argc, char *argv[])
{
	randomize();

	const int n = 4;
	chan_int ch[n];
	memset(ch, 0, sizeof(chan_int)*n);
	pthread_t th[n];

	int i;
	for (i = 0; i < n; i++) {
		pthread_create(&th[i], NULL, crack, &ch[i]);
	}
	
	for (i = 0; i < 1000; i++) {
		int res;
		int index;
		chan_int_read_any(n, ch, &res, &index);
		printf("%i Thread %i: %i\r\n", i, index, res);
		Sleep(10);
	}
	
	for (i = 0; i < n; i++) {
		ch[i].disconnected = 1;
		pthread_join(th[i], NULL);
	}
	
	return 0;
}
