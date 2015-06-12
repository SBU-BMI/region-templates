/*
 * util.cpp
 *
 *  Created on: Feb 15, 2012
 *      Author: george
 */

#include "Util.h"

#define PROFILE
long long Util::ClockGetTime()
{
        struct timeval ts;
        gettimeofday(&ts, NULL);
        return (ts.tv_sec*1000000 + (ts.tv_usec))/1000LL;
}

long long Util::ClockGetTimeProfile() {
#ifdef	PROFILE
	return ClockGetTime();
#else
	return 0;
#endif
}

void Util::PrintElapsedTime(std::string phase, uint64_t &t_init) {
#ifdef	PROFILE
	uint64_t t_end = Util::ClockGetTime();
	std::cout <<"PROFILE: "<< phase <<" : " << t_end-t_init << std::endl;
	t_init=t_end;
#endif
}

