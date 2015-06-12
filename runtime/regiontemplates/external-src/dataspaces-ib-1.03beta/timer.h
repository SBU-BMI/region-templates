/* 
 * Ciprian Docan (2006-2009) TASSL Rutgers University
 *
 * The redistribution of the source code is subject to the terms of version 
 * 2 of the GNU General Public License: http://www.gnu.org/licenses/gpl.html.
 *
 */

#ifndef __TIMER_H_
#define __TIMER_H_

#ifdef XT3
#  include <catamount/dclock.h>
#else
#  include <sys/time.h>
#endif

struct timer {
#ifdef XT3
	double starttime;
	double stoptime;
#else
	struct timeval start_time;
	struct timeval stop_time;
#endif
	double elapsed_time;
	double alarm_time;

	unsigned char stopped:1;
	unsigned char started:1;
	unsigned char reset:1;
};
typedef struct timer mtimer_t;

void timer_init(mtimer_t *, unsigned int);
void timer_start(mtimer_t *);
void timer_stop(mtimer_t *);
void timer_reset(mtimer_t *);
double timer_read(mtimer_t *);
int timer_expired(mtimer_t *);
double timer_timestamp(void);

#endif
