#ifndef SEEK_BOT_H_
#define SEEK_BOT_H_

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "integers.h"
#include "bool.h"

typedef struct
{
	pthread_mutex_t mutex;
	bool kill;
	pthread_t id;
} seek_bot_t;

void start_seek_bot( seek_bot_t* );

#endif /*SEEK_BOT_H_*/
