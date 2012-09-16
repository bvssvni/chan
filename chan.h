/*
	Chan - Concurrency in C
	BSD license.
	by Sven Nilsen, 2012
	http://www.cutoutpro.com

	Version: 0.001 in angular degrees version notation
	http://isprogrammingeasy.blogspot.no/2012/08/angular-degrees-versioning-notation.html
*/
/*
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.
*/
#ifndef CHAN_GUARD
#define CHAN_GUARD
#include <unistd.h>

/* Channels for concurrency communication. */
#define CHAN_ERROR_DISCONNECTED 1
#define CHAN_ERROR_CAN_ONLY_HAVE_ONE_WRITER 2
#define CHAN_ERROR_CAN_ONLY_HAVE_ONE_READER 3
#define CHAN_ERROR_NO_MESSAGE 4
#define CHAN_TYPE_DECLARE(type)\
typedef struct chan_ ## type {\
	int received, unopened, disconnected, reading, writing, mux;\
	type val;\
} chan_ ## type;\
int chan_##type##_write(chan_##type *c, type val);\
int chan_##type##_read(chan_##type *c, type *val);\
int chan_##type##_readAny(int cn, chan_##type *c[], type *val);
#define CHAN_TYPE(type)\
int chan_##type##_write(chan_##type *c, type val) {\
	if (c->mux) {while (c->unopened || c->writing) { \
	if (c->disconnected) return CHAN_ERROR_DISCONNECTED; usleep(0);}}\
	else if (c->unopened || c->writing)\
		return CHAN_ERROR_CAN_ONLY_HAVE_ONE_WRITER;\
	c->writing = 1; c->val = val; c->unopened = 1;\
	while (!c->received && !c->disconnected) usleep(0);\
	c->received = 0; c->writing = 0;\
	if (c->disconnected) return CHAN_ERROR_DISCONNECTED;\
	return 0;\
}\
int chan_##type##_read(chan_##type *c, type *val) {\
	if (c->mux) {while (c->reading) usleep(0);}\
	else if (c->reading) return CHAN_ERROR_CAN_ONLY_HAVE_ONE_READER;\
	c->reading = 1;\
	if (c->disconnected) {c->reading = 0; return CHAN_ERROR_DISCONNECTED;}\
	while (c->received || !c->unopened) usleep(0);\
	*val = c->val; c->unopened = 0;	c->received = 1; c->reading = 0;\
	return 0;\
}\
int chan_##type##_check(chan_##type *c, type *val) {\
	if (c->reading) return CHAN_ERROR_NO_MESSAGE; \
	c->reading = 1;\
	if (c->disconnected) {c->reading = 0; return CHAN_ERROR_DISCONNECTED;}\
	if (!c->unopened) {c->reading = 0; return CHAN_ERROR_NO_MESSAGE;} \
	*val = c->val; c->unopened = 0;	c->received = 1; c->reading = 0;\
	return 0;\
} \
int chan_##type##_read_any(int cn, chan_##type c[], type *val, int *index) {\
	int i, active, err;\
	while (1) {\
		for (active = cn, i = 0; i < cn; i++) {\
			if (c[i].disconnected) {active--; continue;}\
			if (!c[i].received && c[i].unopened) {\
				err = chan_##type##_check(&c[i], val);\
				if (err == CHAN_ERROR_NO_MESSAGE) continue; \
				*index = i; return 0;\
			}\
		}\
		if (active == 0) return CHAN_ERROR_DISCONNECTED;\
		usleep(0);\
	}\
	return -1;\
}

#endif
