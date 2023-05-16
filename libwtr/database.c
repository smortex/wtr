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
	gchar *database_dir_path = g_build_path(G_DIR_SEPARATOR_S, g_get_user_data_dir(), "wtr", NULL);
	if (g_mkdir_with_parents(database_dir_path, 0700) < 0) {
		warn("Could not create %s", database_dir_path);
		return -1;
	}

	gchar *database_file_path = g_build_path(G_DIR_SEPARATOR_S, database_dir_path, "database.sqlite", NULL);
	if (sqlite3_open(database_file_path, &db) != SQLITE_OK) {
		sqlite3_close(db);
		warn("Cannot open database");
		return -1;
	}

	if (sqlite3_busy_timeout(db, 1000) != SQLITE_OK) {
		warn("Connet set database busy timeout");
		sqlite3_close(db);
		return -1;
	}

	free(database_file_path);
	free(database_dir_path);

	database_migrate();

	return 0;
}

static int
find_applied_migrations(void *not_used, int argc, char **argv, char **column_name)
{
	(void) not_used;
	(void) column_name;

	for (int i = 0; i < argc; i++) {
		for (size_t j = 0; j < sizeof(migrations) / sizeof(*migrations); j++) {
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
	char *errmsg = NULL;
	int rc;

	rc = sqlite3_exec(db, "SELECT migration FROM information_schema", find_applied_migrations, 0, &errmsg);
	switch (rc) {
	case SQLITE_OK:
		break;
	case SQLITE_ERROR:
		rc = sqlite3_exec(db, "CREATE TABLE information_schema (migration VARCHAR(255) PRIMARY KEY)", NULL, 0, &errmsg);
		if (rc != SQLITE_OK) {
			errx(EXIT_FAILURE, "Failed to create the information_schema table: %s", errmsg);
			/* NOTREACHED */
		}
		break;
	default:
		errx(EXIT_FAILURE, "Cannot read the information_schema table: %s", errmsg);
		/* NOTREACHED */
	}

	for (size_t i = 0; i < sizeof(migrations) / sizeof(*migrations); i++) {
		if (!migrations[i].applied) {
			if (sqlite3_exec(db, "BEGIN TRANSACTION", NULL, 0, &errmsg) != SQLITE_OK) {
				errx(EXIT_FAILURE, "%s", errmsg);
				/* NOTREACHED */
			}
			if (sqlite3_exec(db, migrations[i].sql, NULL, 0, &errmsg) != SQLITE_OK) {
				errx(EXIT_FAILURE, "%s", errmsg);
				/* NOTREACHED */
			}
			char *sql = NULL;
			if (asprintf(&sql, "INSERT INTO information_schema (migration) VALUES ('%s')", migrations[i].name) < 0) {
				err(EXIT_FAILURE, "asprintf");
				/* NOTREACHED */
			}
			if (sqlite3_exec(db, sql, NULL, 0, &errmsg) != SQLITE_OK) {
				errx(EXIT_FAILURE, "%s", errmsg);
				/* NOTREACHED */
			}
			free(sql);
			if (sqlite3_exec(db, "COMMIT", NULL, 0, &errmsg) != SQLITE_OK) {
				errx(EXIT_FAILURE, "%s", errmsg);
				/* NOTREACHED */
			}
		}

	}
}

static int
read_single_integer(void *result, int argc, char **argv, char **column_name)
{
	(void) column_name;

	if (argc == 1 && argv[0]) {
		sscanf(argv[0], "%d", (int *) result);
	}
	return 0;
}

int
database_project_find_by_name(char *project)
{
	int id = -1;
	char *sql = NULL;
	char *errmsg;

	if (asprintf(&sql, "SELECT id FROM projects WHERE name = '%s'", project) < 0) {
		err(EXIT_FAILURE, "asprintf");
		/* NOTREACHED */
	}
	if (sqlite3_exec(db, sql, read_single_integer, &id, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}
	free(sql);

	return id;
}

int
database_project_find_or_create_by_name(char *project)
{
	int id = -1;
	char *sql = NULL;

	id = database_project_find_by_name(project);
	char *errmsg;

	if (id < 0) {
		if (asprintf(&sql, "INSERT INTO projects (name) VALUES ('%s')", project) < 0) {
			err(EXIT_FAILURE, "asprintf");
			/* NOTREACHED */
		}
		if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
			errx(EXIT_FAILURE, "%s", errmsg);
			/* NOTREACHED */
		}
		free(sql);

		id = database_project_find_by_name(project);
	}

	return id;
}

void
database_project_add_duration(int project_id, time_t date, int duration)
{
	char *sql;
	char *errmsg;

	if (asprintf(&sql, "INSERT INTO activity (project_id, date, duration) VALUES (%d, %ld, %d) ON CONFLICT (project_id, date) DO UPDATE SET duration = duration + %d", project_id, date, duration, duration) < 0) {
		err(EXIT_FAILURE, "asprintf");
		/* NOTREACHED */
	}
	if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}
	free(sql);
}

int
database_project_get_duration(int project_id, time_t from, time_t to)
{
	int duration = 0;
	char *sql = NULL;
	char *errmsg;
	char *empty = "";
	char *since_sql = empty, *until_sql = empty;
	if (from) {
		if (asprintf(&since_sql, " AND date >= %ld", from) < 0) {
			err(EXIT_FAILURE, "asprintf");
			/* NOTREACHED */
		}
	}
	if (to) {
		if (asprintf(&until_sql, " AND date < %ld", to) < 0) {
			err(EXIT_FAILURE, "asprintf");
			/* NOTREACHED */
		}
	}
	if (asprintf(&sql, "SELECT SUM(duration) FROM activity WHERE project_id = %d%s%s", project_id, since_sql, until_sql) < 0) {
		err(EXIT_FAILURE, "asprintf");
		/* NOTREACHED */
	}
	if (sqlite3_exec(db, sql, read_single_integer, &duration, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}
	free(sql);
	if (from)
		free(since_sql);
	if (to)
		free(until_sql);

	return duration;
}

void
database_close(void)
{
	sqlite3_close(db);
}
