#include <sys/param.h>
#include <sys/queue.h>
#include <sys/sysctl.h>
#include <sys/user.h>

#include <err.h>
#include <libproc.h>
#include <libprocstat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <glib-unix.h>

#include "config.h"
#include "database.h"

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
each_process_working_directory(struct procstat *prstat, struct kinfo_proc *proc, void callback(const char *wd))
{
	struct filestat *fst;
	struct filestat_list *head;

	if (!(head = procstat_getfiles(prstat, proc, 0))) {
		warn("procstat_getfiles() returned NULL (ignored)");
		return;
	}

	STAILQ_FOREACH(fst, head, next) {
		if (fst->fs_uflags & PS_FST_UFLAG_CDIR) {
			callback(fst->fs_path);
		}
	}

	procstat_freefiles(prstat, head);
}

void
each_user_process(void callback(struct procstat *prstat, struct kinfo_proc *proc, void callback2(const char *wd)), void callback2(const char *wd))
{
	struct procstat *prstat;

	if (!(prstat = procstat_open_sysctl())) {
		err(EXIT_FAILURE, "procstat_open_kvm");
	}

	struct kinfo_proc *procs;
	unsigned int nproc;
	if (!(procs = procstat_getprocs(prstat, KERN_PROC_UID, getuid(), &nproc))) {
		err(EXIT_FAILURE, "procstat_getprocs");
	}

	for (uint i = 0; i < nproc; i++) {
		if (procs[i].ki_stat == PS_DEAD)
			continue;

		callback(prstat, &procs[i], callback2);
	}

	procstat_freeprocs(prstat, procs);

	procstat_close(prstat);
}

time_t
beginning_of_day(void)
{
	time_t now = time(0);
	struct tm *tm = localtime(&now);

	tm->tm_sec = 0;
	tm->tm_min = 0;
	tm->tm_hour = 0;

	return mktime(tm);
}

void
record_working_time(void)
{
	time_t date = beginning_of_day();

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
	each_user_process(each_process_working_directory, process_working_directory);
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
