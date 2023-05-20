#include <stdio.h>
#include <stdlib.h>

#include "../utils.h"

int res = 0;

void
test_scan_duration(char *str, int expected_ret, int expected_duration)
{
	printf("scan_duration(\"%s\"):\n", str);

	int duration;
	int ret = scan_duration(str, &duration);
	printf("  should return %d: %d (%s)\n", expected_ret, ret, expected_ret == ret ? "OK" : "FAIL");

	if (expected_ret != ret) {
		res = 1;
		return;
	}

	if (ret < 0)
		return;

	printf("  should scan %d: %d (%s)\n", expected_duration, duration, expected_duration == duration ? "OK" : "FAIL");
	if (expected_duration != duration) {
		res = 1;
		return;
	}
}

int
main(void)
{
	test_scan_duration("1", 0, 1);
	test_scan_duration("3600", 0, 3600);
	test_scan_duration("1:1", 0, 3660);
	test_scan_duration("1:15", 0, 4500);
	test_scan_duration("1:101", -1, 0);
	test_scan_duration("2:20:15", 0, 8415);
	test_scan_duration("1:1:1", 0, 3661);
	test_scan_duration("1:01 extra", -1, -1);
	test_scan_duration("random", -1, -1);

	exit(res);
}
