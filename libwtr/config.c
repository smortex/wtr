#include <err.h>
#include <stdio.h>

#include <glib.h>

#include "config.h"
#include "database.h"

gsize nroots;
struct root *roots;

int
config_load(void)
{
	gchar *conf_dir_path = g_build_path(G_DIR_SEPARATOR_S, g_get_user_config_dir(), "wtr", NULL);
	if (g_mkdir_with_parents(conf_dir_path, 0700) < 0) {
		warn("Could not create %s", conf_dir_path);
		return -1;
	}

	gchar *conf_file_path = g_build_path(G_DIR_SEPARATOR_S, conf_dir_path, "roots.conf", NULL);

	GKeyFile *conf_file = g_key_file_new();
	if (!g_key_file_load_from_file(conf_file, conf_file_path, G_KEY_FILE_NONE, NULL)) {
		warn("Could not load configuration from %s", conf_file_path);
		// TODO: Create a sample configuration with comments to guide the user
		return -1;
	}

	gchar **groups = g_key_file_get_groups(conf_file, &nroots);

	if (nroots == 0) {
		warnx("Configuration file %s has no projects", conf_file_path);
		return -1;
	}

	free(conf_file_path);
	free(conf_dir_path);

	roots = malloc(sizeof(*roots) * nroots);

	for (gsize i = 0; i < nroots; i++) {
		gchar *root;
		if (!(root = g_key_file_get_string(conf_file, groups[i], "root", NULL))) {
			warnx("Project %s has no root", groups[i]);
			return -1;
		}

		roots[i].id = database_find_or_create_project_by_name(groups[i]);
		roots[i].name = g_strdup(groups[i]);
		roots[i].root = root;
		roots[i].active = 0;
	}

	g_strfreev(groups);

	g_key_file_free(conf_file);

	return 0;
}

void
config_free(void)
{
	for (gsize i = 0; i < nroots; i++) {
		free(roots[i].name);
		free(roots[i].root);
	}
	free(roots);
}
