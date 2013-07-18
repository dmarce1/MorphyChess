#include "seek_bot.h"
#include <stdlib.h>

static void* start_routine( void* _bot )
{
	seek_bot_t* bot;
	int inc, tc, sleep_time;
	bot = (seek_bot_t*) _bot;
	while( 1 )
	{
		tc = rand() % 18 + 1;
		inc = rand() % 18 + 1;
		pthread_mutex_lock( &(bot->mutex) );
		printf( "tellics seek %i %i\n", tc, inc );
		pthread_mutex_unlock( &(bot->mutex) );
		sleep_time = rand() % 10 + 1;
		sleep( sleep_time );
		pthread_mutex_lock( &(bot->mutex) );
		if( bot->kill == true )
			break;
		pthread_mutex_unlock( &(bot->mutex) );
	}
	pthread_mutex_unlock( &(bot->mutex) );
	return NULL;
}


void start_seek_bot( seek_bot_t* bot )
{
	pthread_mutex_init( &(bot->mutex), NULL );
	pthread_mutex_lock( &(bot->mutex) );
	bot->kill = false;
	pthread_create( &(bot->id), NULL, start_routine, bot );
	pthread_mutex_unlock( &(bot->mutex) );
}


void kill_seek_bot( seek_bot_t* bot )
{
	pthread_mutex_lock( &(bot->mutex) );
	bot->kill = true;
	pthread_mutex_unlock( &(bot->mutex) );
}
