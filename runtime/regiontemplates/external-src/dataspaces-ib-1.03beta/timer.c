/*
 * Timer functions implementation, used for performance analysis
 * 
 * Ciprian Docan (2006-2009) TASSL Rutgers University
 *
 * The redistribution of the source code is subject to the terms of version 
 * 2 of the GNU General Public License: http://www.gnu.org/licenses/gpl.html.
 *
 */

#include "timer.h"

#ifdef XT3
static double tv_diff(double start, double end)
{
	return end - start;
}
#else
static double tv_diff(struct timeval *tv_start, struct timeval *tv_end)
{
	// returns the time difference in seconds 
	long sec, usec;

	sec = tv_end->tv_sec - tv_start->tv_sec;
	usec = tv_end->tv_usec - tv_start->tv_usec;
	if(usec < 0) {
		sec = sec - 1;
		usec += 1000000;
	}

	return (double) (sec + (double) usec / 1.e6);
}
#endif // XT3

static void __timer_update(mtimer_t * timer)
{
#ifdef XT3
	double now;

	now = dclock();
	timer->elapsed_time += tv_diff(timer->stoptime, now);
	timer->stoptime = now;
#else
	struct timeval now;

	gettimeofday(&now, 0);
	timer->elapsed_time += tv_diff(&timer->stop_time, &now);
	timer->stop_time = now;
	// gettimeofday( &timer->stop_time, 0 );
#endif
}

static void __timer_reset(mtimer_t * timer)
{
#ifdef XT3
	timer->starttime = timer->stoptime = dclock();
#else
	gettimeofday(&timer->start_time, 0);
	timer->stop_time = timer->start_time;
#endif
}

void timer_init(mtimer_t * timer, unsigned int alarm_time)
{
	// input parameter "alarm_time" is expressed in mili-seconds
	timer->elapsed_time = 0.0;

	timer->alarm_time = (double) alarm_time / 1.e3;
	timer->stopped = 0;
	timer->started = 0;

	__timer_reset(timer);
}

void timer_start(mtimer_t * timer)
{
	if(!timer->started) {
		timer->started = 1;
		if(!timer->stopped)
			__timer_reset(timer);
		else
#ifdef XT3
			timer->stoptime = dclock();
#else
			gettimeofday(&timer->stop_time, 0);
#endif
	}
	timer->stopped = 0;
}

void timer_stop(mtimer_t * timer)
{
	if(!timer->stopped) {
		__timer_update(timer);
		timer->stopped = 1;
	}
	timer->started = 0;
}

void timer_reset(mtimer_t * timer)
{
	timer->elapsed_time = 0.0;

	__timer_reset(timer);
}

/* Function to read the elapsed time value. It does not stop the
   internal timer. */
double timer_read(mtimer_t * timer)
{
	if(timer->started && !timer->stopped)
		__timer_update(timer);

	return timer->elapsed_time;
}

int timer_expired(mtimer_t * timer)
{
	if(!timer->stopped)
		__timer_update(timer);

	return timer->alarm_time <= timer->elapsed_time;
}

/* Function to get and return the current (wall clock) time. */
double timer_timestamp(void)
{
	double ret;

#ifdef XT3
	ret = dclock();
#else
	struct timeval tv;

	gettimeofday(&tv, 0);
	ret = (double) tv.tv_usec + tv.tv_sec * 1.e6;
#endif
	return ret;
}
