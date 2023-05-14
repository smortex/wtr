#include <sys/wait.h>

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
	fprintf(stderr, "  add -P <project> <duration>     Add work time to a project\n");
	fprintf(stderr, "  remove -P <project> <duration>  Remove work time from a project\n");
	fprintf(stderr, "  edit                            Edit wtrd(1) configuration file\n");
	fprintf(stderr, "  list                            List known projects\n");
	fprintf(stderr, "  <report>                        Report time spent on projects\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Durations:\n");
	fprintf(stderr, "  <sec>\n");
	fprintf(stderr, "  <min>:<sec>\n");
	fprintf(stderr, "  <hrs>:<min>:<sec>\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Reports:\n");
	fprintf(stderr, "  today\n");
	fprintf(stderr, "  yesterday\n");
	fprintf(stderr, "  <n> days ago | -<n>\n");
	fprintf(stderr, "  last <n> days\n");
	fprintf(stderr, "  this week\n");
	fprintf(stderr, "  last week\n");
	fprintf(stderr, "  <n> weeks ago\n");
	fprintf(stderr, "  last <n> weeks\n");
	fprintf(stderr, "  this month\n");
	fprintf(stderr, "  last month\n");
	fprintf(stderr, "  <n> months ago\n");
	fprintf(stderr, "  last <n> months\n");
	fprintf(stderr, "  since <date>\n");
	fprintf(stderr, "  until <date>\n");
	exit(exit_code);
}

void
scan_project_and_duration(int argc, char *argv[], int *project, int *duration)
{
	char ch;
	int found = 0;
	while ((ch = getopt(argc, argv, "P:")) > 0) {
		switch(ch) {
		case 'P':
			for (size_t i = 0; i < nroots; i++) {
				if (strcmp(roots[i].name, optarg) == 0) {
					*project = roots[i].id;
					found = 1;
				}
			}
			if (!found) {
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

	if (!found) {
		warnx("You must specify a project");
		usage(EXIT_FAILURE);
		/* NOTREACHED */
	}

	if (argc != 1) {
		usage(EXIT_FAILURE);
		/* NOTREACHED */
	}

	if (scan_duration(argv[0], duration) < 0)
		errx(EXIT_FAILURE, "malformed duration: %s", argv[0]);
}

void
add_duration(int project, int duration)
{
	for (size_t i = 0; i < nroots; i++) {
		if (project == roots[i].id) {
			database_project_add_duration(roots[i].id, today(), duration);
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

	while (argc) {
		if (argc >= 3 && !*from && !*to && sscanf(argv[0], "%d%c", &n, &c) == 1 && strcmp(argv[2], "ago") == 0) {
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
			argc -= 3;
			argv += 3;
			continue;
		}
		if (argc >= 3 && !*from && !*to && strcmp(argv[0], "last") == 0 && sscanf(argv[1], "%d%c", &n, &c) == 1) {
			if (strcmp(argv[2], "day") == 0 || strcmp(argv[2], "days") == 0) {
				*from = add_day(today(), -n);
				*to = today();
			} else if (strcmp(argv[2], "week") == 0 || strcmp(argv[2], "weeks") == 0) {
				*from = add_week(beginning_of_week(now), -n);
				*to = beginning_of_week(now);
			} else if (strcmp(argv[2], "month") == 0 || strcmp(argv[2], "months") == 0) {
				*from = add_month(beginning_of_month(now), -n);
				*to = beginning_of_month(now);
			} else {
				usage(EXIT_FAILURE);
			}
			argc -= 3;
			argv += 3;
			continue;
		}
		if (argc >= 2 && !*from && !*to && strcmp(argv[0], "this") == 0 && strcmp(argv[1], "week") == 0) {
			*from = beginning_of_week(now);
			*to = add_week(*from, 1);
			argc -= 2;
			argv += 2;
			continue;
		}
		if (argc >= 2 && !*from && !*to && strcmp(argv[0], "last") == 0 && strcmp(argv[1], "week") == 0) {
			*from = add_week(beginning_of_week(now), -1);
			*to = add_week(*from, 1);
			argc -= 2;
			argv += 2;
			continue;
		}
		if (argc >= 2 && !*from && !*to && strcmp(argv[0], "this") == 0 && strcmp(argv[1], "month") == 0) {
			*from = beginning_of_month(now);
			*to = add_month(*from, 1);
			argc -= 2;
			argv += 2;
			continue;
		}
		if (argc >= 2 && !*from && !*to && strcmp(argv[0], "last") == 0 && strcmp(argv[1], "month") == 0) {
			*from = add_month(beginning_of_month(now), -1);
			*to = add_month(*from, 1);
			argc -= 2;
			argv += 2;
			continue;
		}
		if (argc >= 2 && !*from && strcmp(argv[0], "since") == 0 && scan_date(argv[1], from) >= 0) {
			argc -= 2;
			argv += 2;
			continue;
		}
		if (argc >= 2 && !*to && strcmp(argv[0], "until") == 0 && scan_date(argv[1], to) >= 0) {
			argc -= 2;
			argv += 2;
			continue;
		}
		if (argc >= 1 && !*from && !*to && strcmp(argv[0], "today") == 0) {
			*from = beginning_of_day(now);
			*to = add_day(*from, 1);
			argc -= 1;
			argv += 1;
			continue;
		}
		if (argc >= 1 && !*from && !*to && strcmp(argv[0], "yesterday") == 0) {
			*from = add_day(beginning_of_day(now), -1);
			*to = add_day(*from, 1);
			argc -= 1;
			argv += 1;
			continue;
		}
		if (argc >= 1 && !*from && !*to && sscanf(argv[0], "%d%c", &n, &c) == 1) {
			*from = add_day(beginning_of_day(now), n);
			*to = add_day(*from, 1);
			argc -= 1;
			argv += 1;
			continue;
		}
		fprintf(stderr, "runaway argument:");
		for (int i = 0; i < argc; i++) {
			fprintf(stderr, " %s", argv[i]);
		}
		fprintf(stderr, "\n");
		usage(EXIT_FAILURE);
		/* NOTREACHED */
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

	if (argc > 1 && (strcmp(argv[1], "add") == 0 || strcmp(argv[1], "remove") == 0)) {
		int project, duration;
		scan_project_and_duration(argc - 1, argv + 1, &project, &duration);

		if (strcmp(argv[1], "add") == 0) {
			add_duration(project, duration);
			/* NOTREACHED */
		} else {
			add_duration(project, -duration);
			/* NOTREACHED */
		}
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

		exit(WEXITSTATUS(ret));
	}

	if (argc > 1 && strcmp(argv[1], "list") == 0) {
		for (size_t i = 0; i < nroots; i++) {
			printf("%s\n", roots[i].name);
		}

		exit(EXIT_SUCCESS);
	}

	each_user_process_working_directory(process_working_directory);

	if (argc == 2 && strcmp(argv[1], "active") == 0) {
		for (size_t i = 0; i < nroots; i++) {
			if (roots[i].active) {
				printf("%s\n", roots[i].name);
			}
		}

		exit(EXIT_SUCCESS);
	}

	time_t from = 0, to = 0;
	if (argc > 1) {
		status(argc - 1, argv + 1, &from, &to);
	}

	int longest_name = 5;
	for (size_t i = 0; i < nroots; i++) {
		int name_length = strlen(roots[i].name);
		if (name_length > longest_name)
			longest_name = name_length;
	}
	char *format_string;
	if (asprintf(&format_string, "%%-%ds ", longest_name) < 0)
		err(EXIT_FAILURE, "asprintf");

	int total_duration = 0;
	for (size_t i = 0; i < nroots; i++) {
		int duration;
		duration = database_project_get_duration(roots[i].id, from, to);

		if (duration == 0)
			continue;

		printf(format_string, roots[i].name);
		print_duration(duration);
		if (roots[i].active) {
			printf(" +");
		}
		printf("\n");

		total_duration += duration;
	}

	for (int i = 0; i < longest_name + 18; i++) {
		printf("-");
	}
	printf("\n");
	printf(format_string, "Total");
	print_duration(total_duration);
	printf("\n");

	free(format_string);

	config_free();
	database_close();

	exit(EXIT_SUCCESS);
}
