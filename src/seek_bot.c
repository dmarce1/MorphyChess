
/*********************************************************************************
MorphyChess 1.0.5 Copyright (C) 2008 Dominic C. Marcello   (dmarcello@phys.lsu.edu) 

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see [http://www.gnu.org/licenses/].
**********************************************************************************/
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
