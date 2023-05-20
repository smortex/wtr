#include <stdio.h>
#include <stdlib.h>

#include "../utils.h"

int res = 0;

void
test_scan_date(char *str, int expected_ret, time_t expected_date)
{
	printf("scan_date(\"%s\"):\n", str);

	time_t date;
	int ret = scan_date(str, &date);
	printf("  should return %d: %d (%s)\n", expected_ret, ret, expected_ret == ret ? "OK" : "FAIL");

	if (expected_ret != ret) {
		res = 1;
		return;
	}

	if (ret < 0)
		return;

	printf("  should scan %ld: %ld (%s)\n", expected_date, date, expected_date == date ? "OK" : "FAIL");
	if (expected_date != date) {
		res = 1;
		return;
	}
}

int
main(void)
{
	setenv("TZ", "Pacific/Tahiti", 1);
	test_scan_date("2023-05-20", 0, 1684576800);

	setenv("TZ", "UTC", 1);
	test_scan_date("2023-05-20", 0, 1684540800);

	test_scan_date("1", -1, -1);
	test_scan_date("random", -1, -1);

	exit(res);
}
