#include <time.h>

#include "utils.h"

time_t
today(void)
{
	return beginning_of_day(time(0));
}

time_t
beginning_of_day(time_t date)
{
	struct tm *tm = localtime(&date);

	tm->tm_sec = 0;
	tm->tm_min = 0;
	tm->tm_hour = 0;

	return mktime(tm);
}

time_t
beginning_of_week(time_t date)
{
	struct tm *tm = localtime(&date);

	tm->tm_sec = 0;
	tm->tm_min = 0;
	tm->tm_hour = 0;
	tm->tm_mday -= tm->tm_wday;

	return mktime(tm);
}

time_t
beginning_of_month(time_t date)
{
	struct tm *tm = localtime(&date);

	tm->tm_sec = 0;
	tm->tm_min = 0;
	tm->tm_hour = 0;
	tm->tm_mday = 1;

	return mktime(tm);
}

time_t
add_day(time_t date, int n)
{
	struct tm *tm = localtime(&date);

	tm->tm_mday += n;

	return mktime(tm);
}

time_t
add_week(time_t date, int n)
{
	struct tm *tm = localtime(&date);

	tm->tm_mday += n * 7;

	return mktime(tm);
}

time_t
add_month(time_t date, int n)
{
	struct tm *tm = localtime(&date);

	tm->tm_mon += n;

	return mktime(tm);
}
