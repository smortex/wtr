#include <err.h>
#include <glib.h>
#include <sqlite3.h>

#include <stdio.h>

#include "database.h"

static void	 database_migrate(void);

sqlite3 *db;

struct migration {
	int applied;
	char *name;
	char *sql;
} migrations[] = {
	{ 0, "202305011057", "CREATE TABLE projects (id INTEGER PRIMARY KEY AUTOINCREMENT, name VARCHAR(255))" },
	{ 0, "202305011058", "CREATE TABLE activity (project_id INTEGER, date INTEGER, duration INTEGER, PRIMARY KEY (project_id, date))" },
};

int
database_open(void)
{
	g_auto (GPathBuf) path;

	g_path_buf_init (&path);

	g_path_buf_push(&path, g_get_user_data_dir());
	g_path_buf_push(&path, "wtr");

	g_autofree char *database_dir_path = g_path_buf_to_path (&path);

	if (g_mkdir_with_parents(database_dir_path, 0700) < 0) {
		warn("Could not create %s", database_dir_path);
		return -1;
	}

	g_path_buf_push(&path, "database.sqlite");

	g_autofree char *database_file_path = g_path_buf_to_path (&path);

	if (sqlite3_open(database_file_path, &db) != SQLITE_OK) {
		sqlite3_close(db);
		warn("Cannot open database");
		return -1;
	}

	database_migrate();

	return 0;
}

static int
find_applied_migrations(void *not_used, int argc, char **argv, char **column_name)
{
	for (int i = 0; i < argc; i++) {
		for (int j = 0; j < sizeof(migrations) / sizeof(*migrations); j++) {
			if (strcmp(argv[i], migrations[j].name) == 0) {
				migrations[j].applied = 1;
			}
		}
	}
	return 0;
}

static void
database_migrate(void)
{
	char *err = NULL;
	int rc;

	rc = sqlite3_exec(db, "SELECT migration FROM information_schema", find_applied_migrations, 0, &err);
	switch (rc) {
	case SQLITE_OK:
		break;
	case SQLITE_ERROR:
		rc = sqlite3_exec(db, "CREATE TABLE information_schema (migration VARCHAR(255) PRIMARY KEY)", NULL, 0, &err);
		if (rc != SQLITE_OK) {
			errx(EXIT_FAILURE, "Failed to create the information_schema table: %s", err);
			/* NOTREACHED */
		}
		break;
	default:
		errx(EXIT_FAILURE, "Cannot read the information_schema table: %s", err);
		/* NOTREACHED */
	}

	for (int i = 0; i < sizeof(migrations) / sizeof(*migrations); i++) {
		if (!migrations[i].applied) {
			if (sqlite3_exec(db, "BEGIN TRANSACTION", NULL, 0, &err) != SQLITE_OK) {
				errx(EXIT_FAILURE, "%s", err);
				/* NOTREACHED */
			}
			if (sqlite3_exec(db, migrations[i].sql, NULL, 0, &err) != SQLITE_OK) {
				errx(EXIT_FAILURE, "%s", err);
				/* NOTREACHED */
			}
			char *sql = NULL;
			asprintf(&sql, "INSERT INTO information_schema (migration) VALUES ('%s')", migrations[i].name); 
			if (sqlite3_exec(db, sql, NULL, 0, &err) != SQLITE_OK) {
				errx(EXIT_FAILURE, "%s", err);
				/* NOTREACHED */
			}
			free(sql);
			if (sqlite3_exec(db, "COMMIT", NULL, 0, &err) != SQLITE_OK) {
				errx(EXIT_FAILURE, "%s", err);
				/* NOTREACHED */
			}
		}

	}
}

static int
read_single_integer(void *result, int argc, char **argv, char **column_name)
{
	if (argc == 1 && argv[0]) {
		sscanf(argv[0], "%d", (int *) result);
	}
	return 0;
}

static int
find_project_by_name(char *project)
{
	int id = -1;
	char *sql = NULL;
	char *err;

	asprintf(&sql, "SELECT id FROM projects WHERE name = '%s'", project);
	if (sqlite3_exec(db, sql, read_single_integer, &id, &err) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", err);
		/* NOTREACHED */
	}
	free(sql);

	return id;
}

int
database_find_or_create_project_by_name(char *project)
{
	int id = -1;
	char *sql = NULL;

	id = find_project_by_name(project);
	char *err;

	if (id < 0) {
		asprintf(&sql, "INSERT INTO projects (name) VALUES ('%s')", project);
		if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
			errx(EXIT_FAILURE, "%s", err);
			/* NOTREACHED */
		}
		free(sql);

		id = find_project_by_name(project);
	}

	return id;
}

void
database_project_add_duration(int project_id, time_t date, int duration)
{
	char *sql;
	char *err;

	asprintf(&sql, "INSERT INTO activity (project_id, date, duration) VALUES (%d, %ld, %d) ON CONFLICT (project_id, date) DO UPDATE SET duration = duration + %d", project_id, date, duration, duration);
	if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", err);
		/* NOTREACHED */
	}
	free(sql);
}

int
database_project_get_duration(int project_id)
{
	int duration = 0;
	char *sql = NULL;
	char *err;
	asprintf(&sql, "SELECT SUM(duration) FROM activity WHERE project_id = %d", project_id);
	if (sqlite3_exec(db, sql, read_single_integer, &duration, &err) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", err);
		/* NOTREACHED */
	}
	free(sql);

	return duration;
}

void
database_close(void)
{
	sqlite3_close(db);
}
