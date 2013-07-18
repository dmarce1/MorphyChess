#include <math.h>
#include <stdio.h>


int rating_change_win( int engine_rating, int opponent_rating )
{
	double r;
	r = pow( 10.0, (engine_rating - opponent_rating) / 400.0 );
	r = 1.0 / (1.0 + r);
	r *= 32.0;
	return (int) (r + 0.5);
}


int rating_change_loss( int engine_rating, int opponent_rating )
{
	return -(32 - rating_change_win( engine_rating, opponent_rating ));
}


int rating_change_draw( int engine_rating, int opponent_rating )
{
	double r;
	r = rating_change_win( engine_rating, opponent_rating ) / 2.0;
	r += rating_change_loss( engine_rating, opponent_rating ) / 2.0;
	return r;
}


int rating_compute_new_rating( int old_rating, int opponent_rating, int result, int num_games )
{
	int rating;
	printf( "er %i or %i r %i n %i\n", old_rating, opponent_rating, result, num_games );
	if( num_games <= 20 )
	{
		rating = (num_games - 1) * old_rating;
		rating += opponent_rating + result * 400;
		rating /= num_games;
	}
	else
	{
		rating = old_rating;
		if( result > 0 )
			rating += rating_change_win( old_rating, opponent_rating );
		else if( result < 0 )
			rating += rating_change_loss( old_rating, opponent_rating );
		else
			rating += rating_change_draw( old_rating, opponent_rating );
	}
	return rating;
}


