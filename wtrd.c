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

#include "config.h"

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

void
print_wd(void)
{
	printf("----\n");
	for (int i = 0; i < nroots; i++) {
		if (roots[i].active) {
			printf("%s\n", roots[i].name);
		}
	}
}

void
reset_wd(void)
{
	for (int i = 0; i < nroots; i++) {
		roots[i].active = 0;
	}
}

int
main(void)
{
	if (config_load() < 0) {
		exit(1);
	}

	for (;;) {
		reset_wd();
		each_user_process(each_process_working_directory, process_working_directory);
		print_wd();
		sleep(1);
	}

	config_free();

	exit(EXIT_SUCCESS);
}
