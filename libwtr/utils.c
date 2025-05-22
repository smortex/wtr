#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "utils.h"

char _hostname[BUFSIZ];
char *
short_hostname(void)
{
	if (! *_hostname) {
		if (gethostname(_hostname, BUFSIZ) < 0) {
			err(EXIT_FAILURE, "gethostname");
			/* NOTREACHED */
		}

		for (char *c = _hostname; *c; c++) {
			if (*c == '.')
				*c = '\0';
		}
	}

	return _hostname;
}

int
scan_date(const char *str, time_t *date)
{
	time_t now = time(0);
	struct tm *tm = localtime(&now);

	if (!strptime(str, "%Y-%m-%d", tm))
		return -1;

	tm->tm_sec = 0;
	tm->tm_min = 0;
	tm->tm_hour = 0;

	*date = mktime(tm);
	return 0;
}

int
scan_duration(const char *str, int *duration)
{
	int hrs, min, sec;
	char rest;

	if (sscanf(str, "%d:%02d:%02d%c", &hrs, &min, &sec, &rest) == 3) {
		*duration = hrs * 3600 + min * 60 + sec;
		return 0;
	}
	if (sscanf(str, "%d:%02d%c", &hrs, &min, &rest) == 2) {
		*duration = hrs * 3600 + min * 60;
		return 0;
	}
	return -1;
}

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

	tm->tm_isdst = -1;

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

	tm->tm_isdst = -1;

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

	tm->tm_isdst = -1;

	return mktime(tm);
}

time_t
beginning_of_quarter(time_t date)
{
	struct tm *tm = localtime(&date);

	tm->tm_sec = 0;
	tm->tm_min = 0;
	tm->tm_hour = 0;
	tm->tm_mday = 1;
	tm->tm_mon = tm->tm_mon / 3 * 3;

	tm->tm_isdst = -1;

	return mktime(tm);
}

time_t
beginning_of_year(time_t date)
{
	struct tm *tm = localtime(&date);

	tm->tm_sec = 0;
	tm->tm_min = 0;
	tm->tm_hour = 0;
	tm->tm_mday = 1;
	tm->tm_mon = 0;

	tm->tm_isdst = -1;

	return mktime(tm);
}

time_t
add_day(time_t date, int n)
{
	struct tm *tm = localtime(&date);

	tm->tm_mday += n;

	tm->tm_isdst = -1;

	return mktime(tm);
}

time_t
add_week(time_t date, int n)
{
	struct tm *tm = localtime(&date);

	tm->tm_mday += n * 7;

	tm->tm_isdst = -1;

	return mktime(tm);
}

time_t
add_month(time_t date, int n)
{
	struct tm *tm = localtime(&date);

	tm->tm_mon += n;

	tm->tm_isdst = -1;

	return mktime(tm);
}

time_t
add_quarter(time_t date, int n)
{
	struct tm *tm = localtime(&date);

	tm->tm_mon += 3 * n;

	tm->tm_isdst = -1;

	return mktime(tm);
}

time_t
add_year(time_t date, int n)
{
	struct tm *tm = localtime(&date);

	tm->tm_year += n;

	tm->tm_isdst = -1;

	return mktime(tm);
}
