/*
 * cab202_timers.c
 *
 * Simple character-based "graphics" library for CAB202, 2016 semester 1.
 *
 *	Authors:
 *	 	Lawrence Buckingham
 *
 *	$Revision:Wed Mar  2 15:21:10 EAST 2016$
 */

#include "cab202_timers.h"
#include <assert.h>
#include <stdlib.h>


#ifdef WIN32
#include <windows.h>
#else
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#endif

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

/*
*	Creates a new timer and sets it up with the required interval.
*
*	Input:
*	-  milliseconds: the length of the desired interval between reset and expiry.
*
*	Output:
*		Returns a non-negative ID number that can be used to check the status
*		of the timer if fewer than TIMER_MAX timers have been created. Otherwise,
*		returns TIMER_ERROR.
*/

timer_id create_timer( long milliseconds ) {
	assert( milliseconds > 0 );

	timer_id timer = malloc( sizeof(cab202_timer_t) );

	timer->milliseconds = milliseconds;
	timer_reset( timer );

	return timer;
}


/*
 *	Deallocates resources associated with the timer.
 *
 *	Input:
 *		timer: the address of a timer to be deallocated.
 */
void destroy_timer( timer_id timer ) {
	assert( timer != NULL );

	free( timer );
}

/*
*	timer_reset:
*
*	Reset a timer to start a new interval.
*
*	Input:
*	-	timer: the address of a timer which is to be reset.
*
*	Output: void.
*/

void timer_reset( timer_id timer ) {
	assert( timer != NULL );

	timer->reset_time = get_current_time();
}


/*
*	timer_expired:
*
*	Checks a timer to see if the associated interval has passed. If the interval has
*	elapsed, the timer is reset automatically, ready to start again.
*
*	Input:
*	-	id: The ID of a timer to check and update.
*
*	Output:
*		Returns TRUE if and only if the interval had elapsed.
*/

bool timer_expired( timer_id timer ) {
	assert( timer != NULL );

	double current_time = get_current_time();
	double time_diff = current_time - timer->reset_time;
	int expired = time_diff * MILLISECONDS >= timer->milliseconds;

	if ( expired ) {
		timer_reset( timer );
	}

	return expired;
}


/*
*	timer_pause:
*
*	Pauses execution in a system-friendly way to allow other processes to work and
*	conserve clock cycles.
*
*	Input:
*		milliseconds:	The duration of the desired pause.
*
*	Output: void.
*/

void timer_pause( long milliseconds ) {
#ifdef WIN32
	Sleep( milliseconds );
#else
	/* usleep requires input in microseconds rather than milliseconds. */
	usleep( milliseconds * MILLISECONDS );
#endif
}

#ifdef WIN32
/*
	Implementation of clock_gettime sourced from StackOverflow:
	http://stackoverflow.com/questions/5404277/porting-clock-gettime-to-windows
	*/

LARGE_INTEGER getFILETIMEoffset() {
	SYSTEMTIME s;
	FILETIME f;
	LARGE_INTEGER t;

	s.wYear = 1970;
	s.wMonth = 1;
	s.wDay = 1;
	s.wHour = 0;
	s.wMinute = 0;
	s.wSecond = 0;
	s.wMilliseconds = 0;
	SystemTimeToFileTime( &s, &f );
	t.QuadPart = f.dwHighDateTime;
	t.QuadPart <<= 32;
	t.QuadPart |= f.dwLowDateTime;
	return ( t );
}

#pragma warning( disable: 4100 )

int clock_gettime( int X, struct timeval *tv ) {
	LARGE_INTEGER           t;
	FILETIME            f;
	double                  microseconds;
	static LARGE_INTEGER    offset;
	static double           frequencyToMicroseconds;
	static int              initialized = 0;
	static BOOL             usePerformanceCounter = 0;

	if ( !initialized ) {
		LARGE_INTEGER performanceFrequency;
		initialized = 1;
		usePerformanceCounter = QueryPerformanceFrequency( &performanceFrequency );
		if ( usePerformanceCounter ) {
			QueryPerformanceCounter( &offset );
			frequencyToMicroseconds = (double) performanceFrequency.QuadPart / 1000000.;
		}
		else {
			offset = getFILETIMEoffset();
			frequencyToMicroseconds = 10.;
		}
	}
	if ( usePerformanceCounter ) QueryPerformanceCounter( &t );
	else {
		GetSystemTimeAsFileTime( &f );
		t.QuadPart = f.dwHighDateTime;
		t.QuadPart <<= 32;
		t.QuadPart |= f.dwLowDateTime;
	}

	t.QuadPart -= offset.QuadPart;
	microseconds = (double) t.QuadPart / frequencyToMicroseconds;
	t.QuadPart = (LONGLONG) microseconds;
	tv->tv_sec = (long) ( t.QuadPart / 1000000 );
	tv->tv_usec = t.QuadPart % 1000000;
	return ( 0 );
}

double get_current_time() {
	struct timeval timeval;
	clock_gettime( 0, &timeval );
	return timeval.tv_sec + timeval.tv_usec / 1.0e+6;
}
#else 
double get_current_time () {
	struct timespec timeval;

#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
	clock_serv_t cclock;
	mach_timespec_t mts;
	host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
	clock_get_time(cclock, &mts);
	mach_port_deallocate(mach_task_self(), cclock);
	timeval.tv_sec = mts.tv_sec;
	timeval.tv_nsec = mts.tv_nsec;
#else
	/* http://linux.die.net/man/3/clock_gettime */
	clock_gettime( CLOCK_REALTIME, &timeval );
#endif

	return timeval.tv_sec + timeval.tv_nsec / 1.0e+9;
}
#endif
