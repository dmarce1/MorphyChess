
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
