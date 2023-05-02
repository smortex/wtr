#include <time.h>

#include "utils.h"

time_t
beginning_of_day(void)
{
	time_t now = time(0);
	struct tm *tm = localtime(&now);

	tm->tm_sec = 0;
	tm->tm_min = 0;
	tm->tm_hour = 0;

	return mktime(tm);
}
