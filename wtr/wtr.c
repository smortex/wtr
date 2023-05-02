#include <err.h>
#include <stdio.h>

#include "config.h"
#include "database.h"
#include "utils.h"

void
print_duration(duration)
{
	int sec = duration % 60;
	duration /= 60;
	int min = duration % 60;
	duration /= 60;
	int hrs = duration % 24;
	duration /= 24;

	printf("%d days %2d:%02d:%02d", duration, hrs, min, sec);
}

int
main(int argc, char *argv[])
{
	if (database_open() < 0) {
		exit(1);
	}

	if (config_load() < 0) {
		exit(1);
	}

	time_t from, to;
	if (argc > 1) {
		if (strcmp(argv[1], "today") == 0) {
			from = beginning_of_day();
			to = beginning_of_day() + 24 * 60 * 60;
		} else if (strcmp(argv[1], "yesterday") == 0) {
			from = beginning_of_day() - 24 * 60 * 60;
			to = beginning_of_day();
		} else {
			errx(EXIT_FAILURE, "I don't know how to report \"%s\"", argv[1]);
		}
	}

	for (int i = 0; i < nroots; i++) {
		int duration;
		if (from && to)
			duration = database_project_get_duration3(roots[i].id, from, to);
		else
			duration = database_project_get_duration(roots[i].id);

		if (duration == 0)
			continue;

		printf("%-20s", roots[i].name);
		print_duration(duration);
		printf("\n");
	}

	config_free();
	database_close();

	exit(EXIT_SUCCESS);
}
