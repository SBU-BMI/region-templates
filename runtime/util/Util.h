/*
 * Util.h
 *
 *  Created on: Feb 15, 2012
 *      Author: george
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <cstddef>
#include <sys/time.h>
#include <cstddef>
#include <stdint.h>
#include <string>
#include <iostream>

class Util {
private:
	Util();

public:
	static long long ClockGetTime();
	static long long ClockGetTimeProfile();
	static void PrintElapsedTime(std::string phase, uint64_t &t_initd);

};

#endif /* UTIL_H_ */
