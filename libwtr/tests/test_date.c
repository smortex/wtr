#include <stdio.h>
#include <stdlib.h>

#include "../utils.h"

#include <time.h>

int res = 0;

/*
 * In these tests, we mock the current date and time.
 *
 * The code is time zone dependant, this mocked current date is:
 *
 * Sun May 28 04:05:48 UTC 2023
 * Sat May 27 18:05:48 -10 2023
 * Sun May 28 06:05:48 CEST 2023
 */
const time_t now = 1685246748;

enum {
	UTC,
	Tahiti,
	Paris,
};

const time_t time_at_beginning_of_day[] = {
	1685232000, // Sun May 28 00:00:00 UTC 2023
	1685181600, // Sat May 27 00:00:00 -10 2023
	1685224800, // Sun May 28 00:00:00 CEST 2023
};

const time_t time_at_beginning_of_week[] = {
	1685232000, // Sun May 28 00:00:00 UTC 2023
	1684663200, // Sun May 21 00:00:00 -10 2023
	1685224800, // Sun May 28 00:00:00 CEST 2023
};

const time_t time_at_beginning_of_month[] = {
	1682899200, // Mon May  1 00:00:00 UTC 2023
	1682935200, // Mon May  1 00:00:00 -10 2023
	1682892000, // Mon May  1 00:00:00 CEST 2023
};

const time_t time_at_beginning_of_quarter[] = {
	1680307200, // Sat Apr  1 00:00:00 UTC 2023
	1680343200, // Sat Apr  1 00:00:00 -10 2023
	1680300000, // Sat Apr  1 00:00:00 CEST 2023
};

const time_t time_at_beginning_of_year[] = {
	1672531200, // Sun Jan  1 00:00:00 UTC 2023
	1672567200, // Sun Jan  1 00:00:00 -10 2023
	1672527600, // Sun Jan  1 00:00:00 CET 2023
};

time_t
__wrap_time(time_t *tloc)
{
	if (tloc)
		*tloc = now;

	return now;
}

void
test_time(time_t *tloc, time_t expected)
{
	printf("time(%p)\n", tloc);

	time_t value = time(tloc);
	printf("  should return %ld: %ld (%s)\n", expected, value, expected == value ? "OK" : "FAIL");

	if (expected != value) {
		res = 1;
		return;
	}
}

void
test_today(time_t expected)
{
	printf("today()\n");

	time_t value = today();
	printf("  should return %ld: %ld (%s)\n", expected, value, expected == value ? "OK" : "FAIL");

	if (expected != value) {
		res = 1;
		return;
	}
}

void
test_beginning_of_day(time_t date, time_t expected)
{
	printf("beginning_of_day(%ld)\n", date);

	time_t value = beginning_of_day(date);
	printf("  should return %ld: %ld (%s)\n", expected, value, expected == value ? "OK" : "FAIL");

	if (expected != value) {
		res = 1;
		return;
	}
}

void
test_beginning_of_week(time_t date, time_t expected)
{
	printf("beginning_of_week(%ld)\n", date);

	time_t value = beginning_of_week(date);
	printf("  should return %ld: %ld (%s)\n", expected, value, expected == value ? "OK" : "FAIL");

	if (expected != value) {
		res = 1;
		return;
	}
}

void
test_beginning_of_month(time_t date, time_t expected)
{
	printf("beginning_of_month(%ld)\n", date);

	time_t value = beginning_of_month(date);
	printf("  should return %ld: %ld (%s)\n", expected, value, expected == value ? "OK" : "FAIL");

	if (expected != value) {
		res = 1;
		return;
	}
}

void
test_beginning_of_quarter(time_t date, time_t expected)
{
	printf("beginning_of_quarter(%ld)\n", date);

	time_t value = beginning_of_quarter(date);
	printf("  should return %ld: %ld (%s)\n", expected, value, expected == value ? "OK" : "FAIL");

	if (expected != value) {
		res = 1;
		return;
	}
}

void
test_beginning_of_year(time_t date, time_t expected)
{
	printf("beginning_of_year(%ld)\n", date);

	time_t value = beginning_of_year(date);
	printf("  should return %ld: %ld (%s)\n", expected, value, expected == value ? "OK" : "FAIL");

	if (expected != value) {
		res = 1;
		return;
	}
}

int
main(void)
{
	setenv("TZ", "Europe/Paris", 1);
	test_time(0, 1685246748);
	setenv("TZ", "Pacific/Tahiti", 1);
	test_time(0, 1685246748);
	setenv("TZ", "UTC", 1);
	test_time(0, 1685246748);

	setenv("TZ", "Europe/Paris", 1);
	test_today(time_at_beginning_of_day[Paris]);
	setenv("TZ", "Pacific/Tahiti", 1);
	test_today(time_at_beginning_of_day[Tahiti]);
	setenv("TZ", "UTC", 1);
	test_today(time_at_beginning_of_day[UTC]);

	setenv("TZ", "Europe/Paris", 1);
	test_beginning_of_day(now, time_at_beginning_of_day[Paris]);
	setenv("TZ", "Pacific/Tahiti", 1);
	test_beginning_of_day(now, time_at_beginning_of_day[Tahiti]);
	setenv("TZ", "UTC", 1);
	test_beginning_of_day(now, time_at_beginning_of_day[UTC]);

	setenv("TZ", "Europe/Paris", 1);
	test_beginning_of_week(now, time_at_beginning_of_week[Paris]);
	setenv("TZ", "Pacific/Tahiti", 1);
	test_beginning_of_week(now, time_at_beginning_of_week[Tahiti]);
	setenv("TZ", "UTC", 1);
	test_beginning_of_week(now, time_at_beginning_of_week[UTC]);

	setenv("TZ", "Europe/Paris", 1);
	test_beginning_of_month(now, time_at_beginning_of_month[Paris]);
	setenv("TZ", "Pacific/Tahiti", 1);
	test_beginning_of_month(now, time_at_beginning_of_month[Tahiti]);
	setenv("TZ", "UTC", 1);
	test_beginning_of_month(now, time_at_beginning_of_month[UTC]);

	setenv("TZ", "Europe/Paris", 1);
	test_beginning_of_quarter(now, time_at_beginning_of_quarter[Paris]);
	setenv("TZ", "Pacific/Tahiti", 1);
	test_beginning_of_quarter(now, time_at_beginning_of_quarter[Tahiti]);
	setenv("TZ", "UTC", 1);
	test_beginning_of_quarter(now, time_at_beginning_of_quarter[UTC]);

	setenv("TZ", "Europe/Paris", 1);
	test_beginning_of_year(now, time_at_beginning_of_year[Paris]);
	setenv("TZ", "Pacific/Tahiti", 1);
	test_beginning_of_year(now, time_at_beginning_of_year[Tahiti]);
	setenv("TZ", "UTC", 1);
	test_beginning_of_year(now, time_at_beginning_of_year[UTC]);

	exit(res);
}
