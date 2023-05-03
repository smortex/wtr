#include <err.h>
#include <string.h>

#include <glib.h>
#include <glib-unix.h>

#include "wtrd.h"

#include "../libwtr/config.h"
#include "../libwtr/database.h"
#include "../libwtr/utils.h"

int check_interval = 10;

GMainLoop *main_loop;

void
process_working_directory(const char *working_directory)
{
	for (gsize i = 0; i < nroots; i++) {
		if (strnstr(working_directory, roots[i].root, strlen(roots[i].root)) == working_directory &&
		    (working_directory[strlen(roots[i].root)] == '/' ||
		     working_directory[strlen(roots[i].root)] == '\0')) {
			roots[i].active = 1;
			break;
		}
	}

}

void
record_working_time(void)
{
	time_t now = time(0);
	time_t date = beginning_of_day(now);

	for (gsize i = 0; i < nroots; i++) {
		if (roots[i].active) {
			database_project_add_duration(roots[i].id, date, check_interval);
		}
	}
}

void
reset_working_time(void)
{
	for (gsize i = 0; i < nroots; i++) {
		roots[i].active = 0;
	}
}

gboolean
tick(gpointer user_data)
{
	(void) user_data;

	reset_working_time();
	each_user_process_working_directory(process_working_directory);
	record_working_time();

	return TRUE;
}

gboolean
terminate(gpointer user_data)
{
	(void) user_data;

	g_main_loop_quit(main_loop);

	return TRUE;
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

	static GOptionEntry entries[] = {
		{ "interval", 'i', 0, G_OPTION_ARG_INT, &check_interval, "Wait SECONDS seconds between checks", "SECONDS" },
		{},
	};

	GOptionContext *context;

	context = g_option_context_new("- Work Time Recorder Daemon");
	g_option_context_add_main_entries(context, entries, NULL);
	GError *error = NULL;
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		errx(EXIT_FAILURE, "%s", error->message);
	}
	g_option_context_free(context);

	main_loop = g_main_loop_new(NULL, FALSE);

	g_timeout_add(check_interval * 1000, tick, NULL);
	g_unix_signal_add(SIGINT, terminate, NULL);

	g_main_loop_run(main_loop);

	config_free();
	database_close();

	exit(EXIT_SUCCESS);
}
