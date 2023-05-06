#include <err.h>
#include <stdio.h>
#include <string.h>

#include "../libwtr/libwtr.h"

void
print_duration(int duration)
{
	int sec = duration % 60;
	duration /= 60;
	int min = duration % 60;
	duration /= 60;
	int hrs = duration % 24;
	duration /= 24;

	if (duration == 1)
		printf("%3d day  %2d:%02d:%02d", duration, hrs, min, sec);
	else if (duration > 1)
		printf("%3d days %2d:%02d:%02d", duration, hrs, min, sec);
	else if (hrs > 0)
		printf("         %2d:%02d:%02d", hrs, min, sec);
	else
		printf("            %2d:%02d", min, sec);
}

void
usage(int exit_code)
{
	fprintf(stderr, "usage: wtr <duration>\n");
	exit(exit_code);
}

void
status(int argc, char *argv[], time_t *from, time_t *to)
{
	int n;
	char c;

	time_t now = time(0);

	switch (argc) {
	case 1:
		if (strcmp(argv[0], "today") == 0) {
			*from = beginning_of_day(now);
			*to = add_day(*from, 1);
		} else if (strcmp(argv[0], "yesterday") == 0) {
			*from = add_day(beginning_of_day(now), -1);
			*to = add_day(*from, 1);
		} else if (sscanf(argv[0], "%d%c", &n, &c) == 1) {
			*from = add_day(beginning_of_day(now), n);
			*to = add_day(*from, 1);
		} else {
			usage(EXIT_FAILURE);
		}
		break;
	case 2:
		if (strcmp(argv[0], "this") == 0 && strcmp(argv[1], "week") == 0) {
			*from = beginning_of_week(now);
			*to = add_week(*from, 1);
		} else if (strcmp(argv[0], "last") == 0 && strcmp(argv[1], "week") == 0) {
			*from = add_week(beginning_of_week(now), -1);
			*to = add_week(*from, 1);
		} else if (strcmp(argv[0], "this") == 0 && strcmp(argv[1], "month") == 0) {
			*from = beginning_of_month(now);
			*to = add_month(*from, 1);
		} else if (strcmp(argv[0], "last") == 0 && strcmp(argv[1], "month") == 0) {
			*from = add_month(beginning_of_month(now), -1);
			*to = add_month(*from, 1);
		} else {
			usage(EXIT_FAILURE);
		}
		break;
	case 3:
		if (sscanf(argv[0], "%d%c", &n, &c) == 1 && strcmp(argv[2], "ago") == 0) {
			if (strcmp(argv[1], "day") == 0 || strcmp(argv[1], "days") == 0) {
				*from = add_day(beginning_of_day(now), -n);
				*to = add_day(*from, 1);
			} else if (strcmp(argv[1], "week") == 0 || strcmp(argv[1], "weeks") == 0) {
				*from = add_week(beginning_of_week(now), -n);
				*to = add_week(*from, 1);
			} else if (strcmp(argv[1], "month") == 0 || strcmp(argv[1], "months") == 0) {
				*from = add_month(beginning_of_month(now), -n);
				*to = add_month(*from, 1);
			} else {
				usage(EXIT_FAILURE);
			}
		} else {
			usage(EXIT_FAILURE);
		}
		break;
	case 0:
	default:
		usage(EXIT_FAILURE);
		/* NOTREACHED */
		break;
	}
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

	time_t from = 0, to = 0;
	if (argc > 1) {
		status(argc - 1, argv + 1, &from, &to);
	}

	each_user_process_working_directory(process_working_directory);

	int longest_name = 0;
	for (size_t i = 0; i < nroots; i++) {
		int name_length = strlen(roots[i].name);
		if (name_length > longest_name)
			longest_name = name_length;
	}
	char *format_string;
	if (asprintf(&format_string, "%%-%ds ", longest_name) < 0)
		err(EXIT_FAILURE, "asprintf");

	for (size_t i = 0; i < nroots; i++) {
		int duration;
		if (from && to)
			duration = database_project_get_duration3(roots[i].id, from, to);
		else
			duration = database_project_get_duration(roots[i].id);

		if (duration == 0)
			continue;

		printf(format_string, roots[i].name);
		print_duration(duration);
		if (roots[i].active) {
			printf(" +");
		}
		printf("\n");
	}

	free(format_string);

	config_free();
	database_close();

	exit(EXIT_SUCCESS);
}
