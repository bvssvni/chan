
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

void *writeLoop(void *arg)
{
	chan_int *ch = (chan_int*)arg;
	
	while (1) {
		int r = random(100);
		int err = chan_int_write(ch, r);
		if (err == CHAN_ERROR_DISCONNECTED) return NULL;
		usleep(r);
	}

	return NULL;
}

typedef struct reader_args {
	int readerId;
	int readAny;
	int n;
	chan_int *ch;
} reader_args;

void *readLoop(void *arg)
{
	reader_args *args = (reader_args*)arg;
	int readAny = args->readAny;
	int verbose = 0;

	int i;
	int res, index, err;
	for (i = 0; i < 100; i++) {
		if (readAny) {
			err = chan_int_read_any(args->n, args->ch, &res, &index);
			if (err > 0) {
				printf("Err %i\n", err);
				i--;
			} else if (verbose) {
				printf("%i Reader %i Thread %i: %i\r\n", 
				i, args->readerId, index, res);
			}
		} else {
			err = chan_int_read(&args->ch[args->readerId], &res);
			if (err > 0) {
				printf("%i Err %i\n", args->readerId, err);
				i--;
			}
			else if (verbose) {
				printf("%i Reader %i Thread %i: %i\r\n", 
				i, args->readerId, index, res);
			}
		}

	}
	return NULL;
}

void test_multiple_channels(int readAny, int threads) {
	const int n = threads;
	chan_int ch[n];
	memset(ch, 0, sizeof(chan_int)*n);
	pthread_t th[n];

	// Create threads for writing random numbers.
	int i;
	for (i = 0; i < n; i++) {
		pthread_create(&th[i], NULL, writeLoop, &ch[i]);
	}
	
	clock_t start, end;
	start = clock();

	// Create threads for reading random numbers.
	pthread_t reader_threads[n];
	reader_args args[n];
	for (i = 0; i < n; i++) {
		args[i] = (reader_args){.n = n, .ch = ch, .readerId = i,
		.readAny = readAny};
		pthread_create(&reader_threads[i], NULL, readLoop,
		&args[i]);
	}
	for (i = 0; i < n; i++) pthread_join(reader_threads[i], NULL);

	end = clock();
	
	double seconds = (end - start) / (double)CLOCKS_PER_SEC;
	printf("Seconds: %g\r\n", seconds);
	
	// Disconnect all channels and join the threads.
	for (i = 0; i < n; i++) ch[i].disconnected = 1;
	for (i = 0; i < n; i++) pthread_join(th[i], NULL);
	
}

// Tests what happens when all threads reads and writes to same channel.
void test_multiplex(int readAny, int threads) {
	const int n = threads;
	chan_int ch = {.mux = 1};
	pthread_t th[n];

	// Create threads for writing random numbers.
	int i;
	for (i = 0; i < n; i++) {
		pthread_create(&th[i], NULL, writeLoop, &ch);
	}
	
	clock_t start, end;
	start = clock();

	// Create threads for reading random numbers.
	pthread_t reader_threads[n];
	reader_args args[n];
	for (i = 0; i < n; i++) {
		args[i] = (reader_args){.n = n, .ch = &ch, .readerId = 0,
		.readAny = readAny};
		pthread_create(&reader_threads[i], NULL, readLoop,
		&args[i]);
	}
	for (i = 0; i < n; i++) pthread_join(reader_threads[i], NULL);

	end = clock();
	
	double seconds = (end - start) / (double)CLOCKS_PER_SEC;
	printf("Seconds: %g\r\n", seconds);
	
	// Disconnect all channels and join the threads.
	ch.disconnected = 1;
	for (i = 0; i < n; i++) pthread_join(th[i], NULL);
	
}

int main(int argc, char *argv[])
{
	randomize();
	// readAny = 1 means any reader reads from any channel.
	// This is usually faster because of higher chance to read message.
	int readAny;
	int threads = 4;
	test_multiple_channels(readAny = 0, threads);
	test_multiple_channels(readAny = 1, threads);
	test_multiplex(readAny = 0, threads);
	test_multiplex(readAny = 1, threads);
	return 0;
}
