#include <string.h>

#include <glib.h>
#include <glib-unix.h>

#include "wtrd.h"

#include "config.h"
#include "database.h"
#include "utils.h"

const int tick_interval = 1;

GMainLoop *main_loop;

void
process_working_directory(const char *working_directory)
{
	for (int i = 0; i < nroots; i++) {
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

	for (int i = 0; i < nroots; i++) {
		if (roots[i].active) {
			database_project_add_duration(roots[i].id, date, tick_interval);
		}
	}
}

void
reset_working_time(void)
{
	for (int i = 0; i < nroots; i++) {
		roots[i].active = 0;
	}
}

gboolean
tick(gpointer user_data)
{
	reset_working_time();
	each_user_process_working_directory(process_working_directory);
	record_working_time();

	return TRUE;
}

gboolean
terminate(gpointer user_data)
{
	g_main_loop_quit(main_loop);

	return TRUE;
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

	main_loop = g_main_loop_new(NULL, FALSE);

	g_timeout_add(tick_interval * 1000, tick, NULL);
	g_unix_signal_add(SIGINT, terminate, NULL);

	g_main_loop_run(main_loop);

	config_free();
	database_close();

	exit(EXIT_SUCCESS);
}
