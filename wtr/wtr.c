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
	fprintf(stderr, "  <hrs>:<min>\n");
	fprintf(stderr, "  <hrs>:<min>:<sec>\n");
	fprintf(stderr, "  <hrs>hrs\n");
	fprintf(stderr, "  <min>min\n");
	fprintf(stderr, "  <sec>sec\n");
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

	for (size_t i = 0; i < nprojects; i++) {
		if (projects[i].active) {
			printf("%s\n", projects[i].name);
		}
	}
}

void
wtr_add_duration_to_project(int duration, const char *project)
{
	int project_id = database_project_find_by_name(project);
	if (project_id < 0) {
		errx(EXIT_FAILURE, "unknown project: %s", project);
	}
	for (size_t i = 0; i < nprojects; i++) {
		if (project_id == projects[i].id) {
			database_project_add_duration(projects[i].id, today(), duration);
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
	for (size_t i = 0; i < nprojects; i++) {
		printf("%s\n", projects[i].name);
	}
}

void
wtr_report(report_options_t options)
{
	time_t since = options.since;
	time_t until = options.until;

	time_t now = time(0);
	time_t tomorrow = add_day(today(), 1);

	each_user_process_working_directory(process_working_directory);

	if ((since && !until) || until > tomorrow)
		until = tomorrow;

	int longest_name = 5;
	for (size_t i = 0; i < nprojects; i++) {
		int name_length = strlen(projects[i].name);
		if (name_length > longest_name)
			longest_name = name_length;
	}
	char *format_string;
	if (asprintf(&format_string, "    %%-%ds ", longest_name) < 0)
		err(EXIT_FAILURE, "asprintf");

	while (since < until) {
		time_t stop = until;

		if (options.next)
			stop = MIN(until, options.next(since, 1));

		char ssince[BUFSIZ], suntil[BUFSIZ];
		strftime(ssince, BUFSIZ, "%F", localtime(&since));
		strftime(suntil, BUFSIZ, "%F", localtime(&stop));
		printf("wtr since %s until %s\n\n", ssince, suntil);

		int total_duration = 0;
		for (size_t i = 0; i < nprojects; i++) {
			int project_duration;
			int currently_active = projects[i].active && since <= now && now < stop;

			project_duration = database_project_get_duration(projects[i].id, since, stop);

			if (project_duration == 0 && !currently_active)
				continue;

			if (options.rounding && project_duration % options.rounding)
				project_duration = (project_duration / options.rounding + 1) * options.rounding;

			printf(format_string, projects[i].name);
			print_duration(project_duration);
			if (currently_active) {
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

		if (!options.next)
			return;

		since = options.next(since, 1);
		if (since < until)
			printf("\n");
	}

	free(format_string);
}
