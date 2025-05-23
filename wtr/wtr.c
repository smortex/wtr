#include <sys/param.h>
#if defined(__linux__)
#  include <sys/ioctl.h>
#endif
#include <sys/wait.h>

#include <err.h>
#include <getopt.h>
#include <glib.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

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
		wprintf(L"%3d day  %2d:%02d:%02d", duration, hrs, min, sec);
	else if (duration > 1)
		wprintf(L"%3d days %2d:%02d:%02d", duration, hrs, min, sec);
	else if (hrs > 0)
		wprintf(L"         %2d:%02d:%02d", hrs, min, sec);
	else
		wprintf(L"            %2d:%02d", min, sec);
}

static void
print_top_months_line(const time_t since, time_t until)
{
	time_t week_start = since;

	wprintf(L"    ");
	while (week_start <= until) {
		struct tm tm_week_start;
		localtime_r(&week_start, &tm_week_start);

		if (week_start == since || tm_week_start.tm_mday <= 7) {
			char buf[10];
			strftime(buf, sizeof(buf), "%b", &tm_week_start);

			if (buf[strlen(buf) - 1] == '.')
				buf[strlen(buf) - 1] = '\0';

			wchar_t wbuf[4];
			const char *p = buf;
			mbsrtowcs(wbuf, &p, 4, NULL);

			wprintf(L"%-4.4ls", wbuf);
		} else {
			wprintf(L"    ");
		}
		week_start = add_week(week_start, 1);
	}
	wprintf(L"\n");
}

static void
print_bottom_months_line(const time_t since, time_t until)
{
	time_t week_start = since;

	wprintf(L"    ");
	while (week_start <= until) {
		time_t week_stop = MIN(add_day(add_week(week_start, 1), -1), until);

		struct tm tm_week_start, tm_week_stop;
		localtime_r(&week_start, &tm_week_start);
		localtime_r(&week_stop, &tm_week_stop);

		if (week_start == since || tm_week_start.tm_mday == 1 || tm_week_stop.tm_mday < tm_week_start.tm_mday) {
			char buf[10];
			strftime(buf, sizeof(buf), "%b", &tm_week_stop);

			if (buf[strlen(buf) - 1] == '.')
				buf[strlen(buf) - 1] = '\0';

			wchar_t wbuf[4];
			const char *p = buf;
			mbsrtowcs(wbuf, &p, 4, NULL);

			wprintf(L"%-4.4ls", wbuf);
		} else {
			wprintf(L"    ");
		}
		week_start = add_week(week_start, 1);
	}
	wprintf(L"\n");
}

static void
print_years_line(const time_t since, time_t until)
{
	time_t week_start = since;

	wprintf(L"    ");
	while (week_start <= until) {
		time_t week_stop = MIN(add_day(add_week(week_start, 1), -1), until);

		struct tm tm_week_start, tm_week_stop;
		localtime_r(&week_start, &tm_week_start);
		localtime_r(&week_stop, &tm_week_stop);

		if (week_start == since || tm_week_start.tm_yday == 0 || (tm_week_stop.tm_year != tm_week_start.tm_year)) {

			char buf[10];
			strftime(buf, sizeof(buf), "%Y", &tm_week_stop);

			wprintf(L"%-4s", buf);
		} else {
			wprintf(L"    ");
		}
		week_start = add_week(week_start, 1);
	}
	wprintf(L"\n");
}

static void
usage(int exit_code)
{
	fprintf(stderr, "usage: wtr [-d] <command>\n");
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
	setlocale(LC_ALL, "");
	struct database *database;

	char *database_filename = database_path();
	if (!(database = database_open(database_filename))) {
		exit(1);
	}
	free(database_filename);

	if (config_load(database) < 0) {
		wprintf(L"Do you want to install a sample configuration file and open it with your favorite editor? [yN] ");
		char buf[BUFSIZ];
		if (!fgets(buf, sizeof(buf), stdin)) {
			err(EXIT_FAILURE, "fgets");
		}
		if (strncmp(buf, "y", 1) == 0) {
			FILE *f;

			if (!(f = fopen(config_file_path(), "w"))) {
				err(EXIT_FAILURE, "fopen");
			}

			fprintf(f, "# Define one section per project, and set the \"root\" key to the path or its\n"
				   "# root directory.\n"
				   "# Lines starting with a '#' are comments.\n"
				   "#\n"
				   "# Examples:\n"
				   "# [Acme Corporation]\n"
				   "# root = /home/wile/Projects/ACME\n"
				   "#\n"
				   "# [Fast and Furry-ous]\n"
				   "# root = /nfs/movies/E01\n");

			fclose(f);
			wtr_edit();
			/* NOTREACHED */
		}
		exit(1);
	}

	struct option longopts[] = {
		{ "debug", no_argument, NULL, 'd' },
		{ "help", no_argument, NULL, 'h' },
		{ NULL, 0, NULL, 0 },
	};

	int ch;
	while ((ch = getopt_long(argc, argv, "dh", longopts, NULL)) != -1) {
		switch (ch) {
		case 'd':
			yydebug = 1;
			break;
		case 'h':
			if (system("man wtr") != 0)
				exit(EXIT_SUCCESS);
			exit(EXIT_SUCCESS);
			break; /* NOTREACHED */
		default:
			usage(EXIT_FAILURE);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc) {
		global_argc = argc;
		global_argv = argv;
	} else {
		global_argc = default_argc;
		global_argv = default_argv;
	}

	yy_scan_string(global_argv[0]);
	if (yyparse(database)) {
		usage(EXIT_FAILURE);
		/* NOTREACHED */
	}

	config_free();
	database_close(database);

	exit(EXIT_SUCCESS);
}

void
wtr_active(void)
{
	each_user_process_working_directory(process_working_directory);

	for (size_t i = 0; i < nprojects; i++) {
		if (projects[i].active) {
			wprintf(L"%s\n", projects[i].name);
		}
	}
}

void
wtr_add_duration_to_project_on(struct database *database, int duration, const char *project, time_t date)
{
	int project_id = database_project_find_by_name(database, project);
	if (project_id < 0) {
		errx(EXIT_FAILURE, "unknown project: %s", project);
	}
	for (size_t i = 0; i < nprojects; i++) {
		if (project_id == projects[i].id) {
			database_project_add_duration(database, projects[i].id, date, duration);
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
callback_print_string(char *host)
{
	wprintf(L"%s\n", host);
}

void
wtr_list_hosts(struct database *database)
{
	database_list_hosts(database, callback_print_string);
}

void
wtr_list_projects(struct database *database)
{
	database_list_projects(database, callback_print_string);
}

struct report_project_duration_data {
	wchar_t *wformat_string;
	int current;
};

static void
report_project_duration(const char *project, int duration, void *user_data)
{
	struct report_project_duration_data *data = user_data;

	int active = 0;
	if (data->current) {
		for (size_t i = 0; i < nprojects; i++) {
			if (projects[i].active && strcmp((const char *)project, projects[i].name) == 0) {
				active += projects[i].active;
			}
		}
	}

	if (duration > 0 || active) {
		wprintf(data->wformat_string, project);
		print_duration(duration);
		if (active) {
			wprintf(L" ");
			for (int n = floor(log2(active + 1)); n > 0; n--) {
				wprintf(L"+");
			}
		}
		wprintf(L"\n");
	}
}

void
wtr_report(struct database *database, report_options_t options)
{
	time_t since = options.since;
	time_t until = options.until;

	time_t now = time(0);
	time_t tomorrow = add_day(today(), 1);

	each_user_process_working_directory(process_working_directory);

	if (!since)
		since = today();

	if (!until)
		until = tomorrow;

	int longest_name = database_longuest_project_name(database);
	char *format_string;
	if (asprintf(&format_string, "    %%-%ds ", longest_name) < 0)
		err(EXIT_FAILURE, "asprintf");

	wchar_t *wformat_string = malloc(sizeof(wchar_t) * (strlen(format_string) + 1));
	const char *p = format_string;
	mbsrtowcs(wformat_string, &p, BUFSIZ, NULL);

	GString *project_sql_filter = g_string_new(NULL);
	if (options.projects) {
		g_string_append(project_sql_filter, " WHERE projects.id IN (");
		for (id_list_t *item = options.projects; item; item = item->next) {
			g_string_append_printf(project_sql_filter, "%d", item->id);
			if (item->next)
				g_string_append(project_sql_filter, ", ");
		}
		g_string_append(project_sql_filter, ")");
	}

	GString *host_sql_filter = g_string_new(NULL);
	if (options.hosts) {
		g_string_append(host_sql_filter, " AND host_id IN (");
		for (id_list_t *item = options.hosts; item; item = item->next) {
			g_string_append_printf(host_sql_filter, "%d", item->id);
			if (item->next)
				g_string_append(host_sql_filter, ", ");
		}
		g_string_append(host_sql_filter, ")");
	}

	while (since < until) {
		time_t stop = until;

		if (options.next)
			stop = MIN(until, options.next(since, 1));

		char ssince[BUFSIZ], sstop[BUFSIZ];
		strftime(ssince, BUFSIZ, "%F", localtime(&since));
		strftime(sstop, BUFSIZ, "%F", localtime(&stop));
		wprintf(L"wtr since %s until %s\n\n", ssince, sstop);

		int current = since <= now && now < stop;

		struct report_project_duration_data data = {
			.wformat_string = wformat_string,
			.current = current,
		};
		int total_duration = 0;
		total_duration = database_get_duration_by_project(database, since, stop, project_sql_filter->str, host_sql_filter->str, report_project_duration, &data);

		wprintf(L"    ");
		for (int i = 0; i < longest_name + 18; i++) {
			wprintf(L"-");
		}
		wprintf(L"\n");
		wprintf(wformat_string, "Total");
		print_duration(total_duration);
		wprintf(L"\n");

		if (!options.next)
			return;

		since = options.next(since, 1);
		if (since < until)
			wprintf(L"\n");
	}

	free(wformat_string);
	g_string_free(project_sql_filter, TRUE);
	g_string_free(host_sql_filter, TRUE);
}

int
terminal_width(void)
{
#if defined(__linux__)
	struct winsize ws;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
	return ws.ws_col;
#elif defined(__FreeBSD__)
	struct winsize ws;
	tcgetwinsize(STDOUT_FILENO, &ws);
	return ws.ws_col;
#else
	return 80;
#endif
}

void
wtr_graph(struct database *database, report_options_t options)
{
	time_t since;
	if (options.since) {
		since = beginning_of_week(options.since);
	} else {
		int weeks = (terminal_width() - 4) / 4 - 1;
		/*                              |    |   `--- current week
		 *                              |    `------- width of a day
		 *                              `------------ length of header
		 */

		since = beginning_of_week(add_week(today(), -weeks));
	}
	time_t until = options.until;

	time_t tomorrow = add_day(today(), 1);

	GString *sql_filter = g_string_new(NULL);
	if (options.projects) {
		g_string_append(sql_filter, " AND project_id IN (");
		for (id_list_t *item = options.projects; item; item = item->next) {
			g_string_append_printf(sql_filter, "%d", item->id);
			if (item->next)
				g_string_append(sql_filter, ", ");
		}
		g_string_append(sql_filter, ")");
	}

	if (options.hosts) {
		g_string_append(sql_filter, " AND host_id IN (");
		for (id_list_t *item = options.hosts; item; item = item->next) {
			g_string_append_printf(sql_filter, "%d", item->id);
			if (item->next)
				g_string_append(sql_filter, ", ");
		}
		g_string_append(sql_filter, ")");
	}

	if (!until)
		until = tomorrow;

	time_t last_day = add_day(until, -1);

	int nweeks;
	for (nweeks = 0; add_week(since, nweeks) < until; nweeks++);

	int *durations;
	if (!(durations = malloc(7 * nweeks * sizeof(*durations)))) {
		err(EXIT_FAILURE, "malloc");
	}

	int min = INT_MAX;
	int max = 0;
	int total = 0;

	print_top_months_line(since, last_day);

	for (int day_of_week = 0; day_of_week < 7; day_of_week++) {
		for (int week = 0; week < nweeks ; week++) {
			time_t t = add_week(add_day(since, day_of_week), week);
			if (t < options.since || t >= until) {
				durations[week * 7 + day_of_week] = 0;
			} else {
				int duration = database_get_duration(database, t, add_day(t, 1), sql_filter->str);
				durations[week * 7 + day_of_week] = duration;
				total += duration;
				if (duration > max)
					max = duration;
				if (duration > 0 && duration < min)
					min = duration;
			}
		}
	}

	for (int day_of_week = 0; day_of_week < 7; day_of_week++) {
		if (day_of_week == 1 || day_of_week == 3 || day_of_week == 5) {
			time_t t = add_day(since, day_of_week);
			struct tm *tm = localtime(&t);
			char buf[10];
			strftime(buf, sizeof(buf), "%a", tm);

			if (buf[strlen(buf) - 1] == '.')
				buf[strlen(buf) - 1] = '\0';

			wchar_t wbuf[4];
			const char *p = buf;
			mbsrtowcs(wbuf, &p, 4, NULL);

			wprintf(L"%-4.4ls", wbuf);
		} else {
			wprintf(L"    ");
		}

		for (int week = 0; week < nweeks ; week++) {
			time_t t = add_week(add_day(since, day_of_week), week);
			if (t < options.since || t >= until) {
				wprintf(L"\033[48;2;235;237;240m");
				wprintf(L"    ");
			} else {
				struct tm* day = localtime(&t);
				int duration = durations[week * 7 + day_of_week];

				if (duration > 0) {
					float relative_ratio;

					if (min == max)
						relative_ratio = 0.0;
					else
						relative_ratio = 1.0 - (float) (duration - min) / (max - min);
					int red = relative_ratio * (155 - 33) + 33;
					int green = relative_ratio * (233 - 110) + 110;
					int blue = relative_ratio * (168 - 57) + 57;
					wprintf(L"\033[48;2;%d;%d;%dm", red, green, blue);
					if (red + green + blue > 255 * 1.5) {
						wprintf(L"\033[38;2;101;109;118m");
					} else {
						wprintf(L"\033[38;2;235;237;240m");
					}
				} else {
					wprintf(L"\033[48;2;235;237;240m");
					wprintf(L"\033[38;2;101;109;118m");
				}

				wprintf(L" %2d ", day->tm_mday);
			}
		}
		wprintf(L"\033[31;0m");
		wprintf(L"\n");
	}

	print_bottom_months_line(since, last_day);

	print_years_line(since, last_day);

	for (int i = 5; i < nweeks; i++) {
		wprintf(L"    ");
	}
	wprintf(L" \033[48;2;%d;%d;%dm\033[38;2;235;237;240m MAX \033[0m ", 33, 110, 57);
	print_duration(max);
	wprintf(L"\n");

	for (int i = 5; i < nweeks; i++) {
		wprintf(L"    ");
	}
	wprintf(L" \033[48;2;%d;%d;%dm\033[38;2;101;109;118m MIN \033[0m ", 155, 233, 168);
	print_duration(min == INT_MAX ? 0 : min);
	wprintf(L"\n");

	for (int i = 5; i < nweeks; i++) {
		wprintf(L"    ");
	}
	wprintf(L" TOTAL ");
	print_duration(total);
	wprintf(L"\n");

	g_string_free(sql_filter, TRUE);
	free(durations);
}

void
wtr_merge(struct database *database, char *filename)
{
	(void) database;

	struct database *import = database_open(filename);
	if (!import) {
		err(EXIT_FAILURE, "database_open");
	}

	database_merge(database, import);

	database_close(import);
}
