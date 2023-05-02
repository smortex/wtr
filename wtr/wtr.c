#include <stdio.h>

#include "config.h"
#include "database.h"

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
main(void)
{
	if (database_open() < 0) {
		exit(1);
	}

	if (config_load() < 0) {
		exit(1);
	}

	for (int i = 0; i < nroots; i++) {
		int duration = database_project_get_duration(roots[i].id);

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
