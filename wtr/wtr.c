#include <sys/param.h>
#include <sys/wait.h>

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "wtr.h"
#include "../libwtr/libwtr.h"

#include "cmd_lexer.h"
#include "cmd_parser.h"

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
	if (sscanf(str, "%d:%02d%c", &min, &sec, &rest) == 2) {
		*duration = min * 60 + sec;
		return 0;
	}
	if (sscanf(str, "%d%c", &sec, &rest) == 1) {
		*duration = sec;
		return 0;
	}
	return -1;
}

static void
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

static void
usage(int exit_code)
{
	fprintf(stderr, "usage: wtr <command>\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Commands:\n");
	fprintf(stderr, "  add <duration> to <project>        Add work time to a project\n");
	fprintf(stderr, "  remove <duration> from <project>   Remove work time from a project\n");
	fprintf(stderr, "  edit                               Edit wtrd(1) configuration file\n");
	fprintf(stderr, "  active                             List currently active projects\n");
	fprintf(stderr, "  list                               List known projects\n");
	fprintf(stderr, "  <report>                           Report time spent on projects\n");
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
	fprintf(stderr, "  rounding <duration>\n");
	exit(exit_code);
}

int default_argc = 1;
char *default_argv[] = {
	"today"
};

int global_argc;
char **global_argv;

int
main(int argc, char *argv[])
{
	if (database_open() < 0) {
		exit(1);
	}

	if (config_load() < 0) {
		exit(1);
	}

	if (argc > 1) {
		global_argc = argc - 1;
		global_argv = argv + 1;
	} else {
		global_argc = default_argc;
		global_argv = default_argv;
	}

	yy_scan_string(global_argv[0]);
	if (yyparse()) {
		usage(EXIT_FAILURE);
		/* NOTREACHED */
	}

	config_free();
	database_close();

	exit(EXIT_SUCCESS);
}

void
wtr_active(void)
{
	each_user_process_working_directory(process_working_directory);

	for (size_t i = 0; i < nroots; i++) {
		if (roots[i].active) {
			printf("%s\n", roots[i].name);
		}
	}
}

void
wtr_add_duration_to_project(int duration, char *project)
{
	int project_id = database_project_find_by_name(project);
	if (project_id < 0) {
		errx(EXIT_FAILURE, "unknown project: %s", project);
	}
	for (size_t i = 0; i < nroots; i++) {
		if (project_id == roots[i].id) {
			database_project_add_duration(roots[i].id, today(), duration);
		}
	}
}

void
wtr_edit(void)
{
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

void
wtr_list(void)
{
	for (size_t i = 0; i < nroots; i++) {
		printf("%s\n", roots[i].name);
	}
}

void
wtr_report(duration_t duration)
{
	time_t since = duration.since;
	time_t until = duration.until;

	each_user_process_working_directory(process_working_directory);

	int longest_name = 5;
	for (size_t i = 0; i < nroots; i++) {
		int name_length = strlen(roots[i].name);
		if (name_length > longest_name)
			longest_name = name_length;
	}
	char *format_string;
	if (asprintf(&format_string, "    %%-%ds ", longest_name) < 0)
		err(EXIT_FAILURE, "asprintf");

	while (since < until) {
		time_t stop = until;

		if (duration.next)
			stop = MIN(until, duration.next(since, 1));

		char ssince[BUFSIZ], suntil[BUFSIZ];
		strftime(ssince, BUFSIZ, "%F", localtime(&since));
		strftime(suntil, BUFSIZ, "%F", localtime(&stop));
		printf("since %s until %s\n\n", ssince, suntil);

		int total_duration = 0;
		for (size_t i = 0; i < nroots; i++) {
			int project_duration;
			project_duration = database_project_get_duration(roots[i].id, since, stop);

			if (project_duration == 0)
				continue;

			if (duration.rounding && project_duration % duration.rounding)
				project_duration = (project_duration / duration.rounding + 1) * duration.rounding;

			printf(format_string, roots[i].name);
			print_duration(project_duration);
			if (roots[i].active) {
				printf(" +");
			}
			printf("\n");

			total_duration += project_duration;
		}

		printf("    ");
		for (int i = 0; i < longest_name + 18; i++) {
			printf("-");
		}
		printf("\n");
		printf(format_string, "Total");
		print_duration(total_duration);
		printf("\n");

		if (!duration.next)
			return;

		since = duration.next(since, 1);
		if (since < until)
			printf("\n");
	}

	free(format_string);
}
