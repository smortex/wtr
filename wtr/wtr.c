#include <err.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../libwtr/libwtr.h"

int
scan_duration(const char *str, int *duration)
{
	int hrs, min, sec;
	char rest;

	if (sscanf(str, "%d%c", &sec, &rest) == 1) {
		*duration = sec;
		return 0;
	}
	if (sscanf(str, "%d:%02d%c", &min, &sec, &rest) == 2) {
		*duration = min * 60 + sec;
		return 0;
	}
	if (sscanf(str, "%d:%02d:%02d%c", &hrs, &min, &sec, &rest) == 3) {
		*duration = hrs * 3600 + min * 60 + sec;
		return 0;
	}
	return -1;
}

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
	fprintf(stderr, "usage: wtr <command>\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Commands:\n");
	fprintf(stderr, "  add -P <project> <duration>  Add work time to a project\n");
	fprintf(stderr, "  edit                         Edit wtrd(1) configuration file\n");
	fprintf(stderr, "  <report>                     Report time spent on projects\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Reports:\n");
	fprintf(stderr, "  today\n");
	fprintf(stderr, "  yesterday\n");
	fprintf(stderr, "  <n> days ago | -<n>\n");
	fprintf(stderr, "  this week\n");
	fprintf(stderr, "  last week\n");
	fprintf(stderr, "  <n> weeks ago\n");
	fprintf(stderr, "  this month\n");
	fprintf(stderr, "  last month\n");
	fprintf(stderr, "  <n> months ago\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Durations:\n");
	fprintf(stderr, "  <sec>\n");
	fprintf(stderr, "  <min>:<sec>\n");
	fprintf(stderr, "  <hrs>:<min>:<sec>\n");
	exit(exit_code);
}

void
add_duration(int argc, char *argv[])
{
	int project = -1;

	char ch;
	while ((ch = getopt(argc, argv, "P:")) > 0) {
		switch(ch) {
		case 'P':
			for (size_t i = 0; i < nroots; i++) {
				if (strcmp(roots[i].name, optarg) == 0) {
					project = roots[i].id;
				}
			}
			if (project < 0) {
				errx(EXIT_FAILURE, "No such project: %s", optarg);
			}
			break;
		default:
			usage(EXIT_FAILURE);
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (project < 0) {
		warnx("You must specify a project");
		usage(EXIT_FAILURE);
		/* NOTREACHED */
	}

	if (argc != 1) {
		usage(EXIT_FAILURE);
		/* NOTREACHED */
	}

	int n;
	if (scan_duration(argv[0], &n) < 0)
		errx(EXIT_FAILURE, "malformed duration: %s", argv[0]);

	for (size_t i = 0; i < nroots; i++) {
		if (project == roots[i].id) {
			database_project_add_duration(roots[i].id, today(), n);
		}
	}

	exit(EXIT_SUCCESS);
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

	if (argc > 1 && strcmp(argv[1], "add") == 0) {
		add_duration(argc - 1, argv + 1);
		/* NOTREACHED */
	}

	if (argc > 1 && strcmp(argv[1], "edit") == 0) {
		char *editor = getenv("EDITOR");
		if (!editor)
			editor = "vi";
		char *config = config_file_path();
		char *cmd;
		if (asprintf(&cmd, "%s %s", editor, config) < 0) {
			err(EXIT_FAILURE, "asprintf");
		}
		free(config);
		int ret;
		if ((ret = system(cmd)) < 0) {
			err(EXIT_FAILURE, "system");
		}
		free(cmd);

		exit(ret);
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
