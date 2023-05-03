#include <err.h>
#include <glib.h>

#include "wtrd.h"

const char *proc = "/proc";

void
each_user_process_working_directory(void callback(const char *working_directory))
{
	GDir* proc_dir;
	if (!(proc_dir = g_dir_open(proc, 0, NULL))) {
		err(EXIT_FAILURE, "Cannot list user process");
	}

	const gchar *pid;
	while ((pid = g_dir_read_name(proc_dir))) {
		gchar *path = g_build_path(G_DIR_SEPARATOR_S, proc, pid, "cwd", NULL);
		gchar *cwd;

		if ((cwd = g_file_read_link(path, NULL))) {
			callback(cwd);
		}
		free(cwd);
		free(path);

	}

	g_dir_close(proc_dir);
}
