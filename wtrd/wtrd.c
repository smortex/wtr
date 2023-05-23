#include <err.h>

#include <glib.h>
#include <glib-unix.h>

#include "../libwtr/libwtr.h"

int check_interval = 10;

GMainLoop *main_loop;
GMutex roots_mutex;

void
record_working_time(void)
{
	time_t now = time(0);
	time_t date = beginning_of_day(now);

	for (gsize i = 0; i < nprojects; i++) {
		if (projects[i].active) {
			database_project_add_duration(projects[i].id, date, check_interval);
		}
	}
}

void
reset_working_time(void)
{
	for (gsize i = 0; i < nprojects; i++) {
		projects[i].active = 0;
	}
}

gboolean
tick(gpointer user_data)
{
	(void) user_data;

	g_mutex_lock(&roots_mutex);
	reset_working_time();
	each_user_process_working_directory(process_working_directory);
	record_working_time();
	g_mutex_unlock(&roots_mutex);

	return G_SOURCE_CONTINUE;
}

gboolean
terminate(gpointer user_data)
{
	(void) user_data;

	g_main_loop_quit(main_loop);

	return G_SOURCE_CONTINUE;
}

gboolean
reload_config(gpointer user_data)
{
	(void) user_data;

	g_mutex_lock(&roots_mutex);
	config_free();
	if (config_load() < 0) {
		exit(1);
	}
	g_mutex_unlock(&roots_mutex);

	return G_SOURCE_CONTINUE;
}

void
child_watch(GPid pid, gint status, gpointer user_data)
{
	(void) pid;
	(void) status;
	(void) user_data;

	g_main_loop_quit(main_loop);
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

	context = g_option_context_new("[COMMAND] - Work Time Recorder Daemon");
	g_option_context_add_main_entries(context, entries, NULL);
	GError *error = NULL;
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		errx(EXIT_FAILURE, "%s", error->message);
	}
	g_option_context_free(context);

	main_loop = g_main_loop_new(NULL, FALSE);

	if (argc > 1) {
		int child_pid;
		if (!(g_spawn_async_with_pipes(NULL, &argv[1], NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_CHILD_INHERITS_STDIN | G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &child_pid, NULL, NULL, NULL, &error))) {
			errx(EXIT_FAILURE, "%s", error->message);
		}

		g_child_watch_add (child_pid, child_watch, main_loop);

	}

	g_timeout_add(check_interval * 1000, tick, NULL);
	g_unix_signal_add(SIGINT, terminate, NULL);
	g_unix_signal_add(SIGHUP, reload_config, NULL);

	g_main_loop_run(main_loop);

	config_free();
	database_close();

	exit(EXIT_SUCCESS);
}
