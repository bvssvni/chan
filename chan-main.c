
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

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
		usleep(r*1e3);
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
	
	clock_t start, end;
	start = clock();
	for (i = 0; i < 1000; i++) {
		int res;
		int index;
		chan_int_read_any(n, ch, &res, &index);
		printf("%i Thread %i: %i\r\n", i, index, res);
	}
	end = clock();
	
	// Dual-core Macbook Pro laptop with Windows XP:
	// threads 1:10
	// run 1 [57.375,31.0,19.765,14.36,12.016,9.843,8.828,7.875,7.625,7.047]
	// run 2 [58.921,29.218,21.75,15.094,12.125,10.359,8.843,8.515,7.218,6.875]
	// Notation: http://www.cutoutpro.com/calc
	double seconds = (end - start) / (double)CLOCKS_PER_SEC;
	printf("Seconds: %g\r\n", seconds);
	
	for (i = 0; i < n; i++) ch[i].disconnected = 1;
	for (i = 0; i < n; i++) pthread_join(th[i], NULL);
	
	return 0;
}
