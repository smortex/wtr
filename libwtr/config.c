#include <err.h>
#include <stdio.h>

#include <glib.h>

#include "config.h"
#include "database.h"

size_t nprojects;
struct project *projects;

char *
config_file_path(void)
{
	gchar *conf_dir_path = g_build_path(G_DIR_SEPARATOR_S, g_get_user_config_dir(), "wtr", NULL);
	if (g_mkdir_with_parents(conf_dir_path, 0700) < 0) {
		warn("Could not create %s", conf_dir_path);
		free(conf_dir_path);
		return NULL;
	}

	char *res = g_build_path(G_DIR_SEPARATOR_S, conf_dir_path, "roots.conf", NULL);

	free(conf_dir_path);
	return res;
}

int
config_load(struct database *database)
{
	char *conf_file_path = config_file_path();

	if (!conf_file_path) {
		return -1;
	}

	GKeyFile *conf_file = g_key_file_new();
	if (!g_key_file_load_from_file(conf_file, conf_file_path, G_KEY_FILE_NONE, NULL)) {
		warn("Could not load configuration from %s", conf_file_path);
		return -1;
	}

	gchar **groups = g_key_file_get_groups(conf_file, &nprojects);

	if (nprojects == 0) {
		warnx("Configuration file %s has no projects", conf_file_path);
		return -1;
	}

	free(conf_file_path);

	projects = malloc(sizeof(*projects) * nprojects);

	gsize valid_projects = 0;
	for (gsize i = 0; i < nprojects; i++) {
		gchar *root;
		if (!(root = g_key_file_get_string(conf_file, groups[i], "root", NULL))) {
			warnx("Ignoring project \"%s\": no root key", groups[i]);
			continue;
		}

		if (!(root = realpath(root, NULL))) {
			warn("Ignoring project \"%s\": realpath", groups[i]);
			continue;
		}

		projects[valid_projects].id = database_project_find_or_create_by_name(database, groups[i]);
		projects[valid_projects].name = g_strdup(groups[i]);
		projects[valid_projects].root = realpath(root, NULL);
		projects[valid_projects].active = 0;
		valid_projects++;

		free(root);
	}

	nprojects = valid_projects;

	g_strfreev(groups);

	g_key_file_free(conf_file);

	if (!(projects = realloc(projects, sizeof(*projects) * nprojects))) {
		warn("realloc");
		return -1;
	}

	return 0;
}

void
config_free(void)
{
	for (gsize i = 0; i < nprojects; i++) {
		free(projects[i].name);
		free(projects[i].root);
	}
	free(projects);
}
