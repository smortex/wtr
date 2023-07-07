#include <err.h>
#include <glib.h>
#include <sqlite3.h>

#include <stdio.h>

#include "database.h"
#include "utils.h"

struct database {
	sqlite3 *db;
};

static void	 database_migrate(struct database *database);

int _host_id;

int
host_id(struct database *database)
{
	if (!_host_id) {
		_host_id = database_host_find_or_create_by_name(database, short_hostname());
	}

	return _host_id;
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

void
insert_current_host(struct database *database)
{
	char *sql = NULL;
	if (asprintf(&sql, "INSERT INTO hosts (name) VALUES ('%s')", short_hostname()) < 0) {
		err(EXIT_FAILURE, "asprintf");
		/* NOTREACHED */
	}
	char *errmsg = NULL;
	if (sqlite3_exec(database->db, sql, NULL, 0, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}
	free(sql);
}

struct migration {
	int applied;
	char *name;
	char *sql;
	void (*callback)(struct database *database);
} migrations[] = {
	{ 0, "202305011057", "CREATE TABLE projects (id INTEGER PRIMARY KEY AUTOINCREMENT, name VARCHAR(255))", NULL },
	{ 0, "202305011058", "CREATE TABLE activity (project_id INTEGER, date INTEGER, duration INTEGER, PRIMARY KEY (project_id, date))", NULL },
	{ 0, "202307031327", "CREATE TABLE hosts (id INTEGER PRIMARY KEY AUTOINCREMENT, name VARCHAR(255) NOT NULL)", NULL },
	{ 0, "202307031328", NULL, insert_current_host },
	{ 0, "202307031335", "CREATE TABLE new_activity (project_id INTEGER NOT NULL REFERENCES projects(id), date INTEGER NOT NULL, duration INTEGER NOT NULL, host_id INTEGER NOT NULL REFERENCES hosts(id), PRIMARY KEY (project_id, host_id, date))", NULL },
	{ 0, "202307031336", "INSERT INTO new_activity SELECT *, 1 FROM activity", NULL },
	{ 0, "202307031337", "DROP TABLE activity", NULL },
	{ 0, "202307031338", "ALTER TABLE new_activity RENAME TO activity", NULL },

};

gchar *
database_path(void)
{
	gchar *database_dir_path = g_build_path(G_DIR_SEPARATOR_S, g_get_user_data_dir(), "wtr", NULL);
	if (g_mkdir_with_parents(database_dir_path, 0700) < 0) {
		warn("Could not create %s", database_dir_path);
		return NULL;
	}

	gchar *database_file_path = g_build_path(G_DIR_SEPARATOR_S, database_dir_path, "database.sqlite", NULL);

	free(database_dir_path);

	return database_file_path;
}

struct database *
database_open(char *filename)
{
	struct database *res;
	if (!(res = malloc(sizeof(*res)))) {
	    warn("Cannot allocate memory");
	    return NULL;
	}

	if (sqlite3_open(filename, &res->db) != SQLITE_OK) {
		warn("Cannot open database");
		sqlite3_close(res->db);
		free(res);
		return NULL;
	}

	if (sqlite3_busy_timeout(res->db, 1000) != SQLITE_OK) {
		warn("Connet set database busy timeout");
		sqlite3_close(res->db);
		free(res);
		return NULL;
	}

	char *errmsg = NULL;
	if (sqlite3_exec(res->db, "PRAGMA foreign_keys = ON", NULL, NULL, &errmsg) != SQLITE_OK) {
		warn("Cannot enforce foreign keys constraints: %s", errmsg);
		sqlite3_close(res->db);
		free(res);
		return NULL;
	}

	database_migrate(res);

	return res;
}

int
database_longuest_project_name(struct database *database)
{
	int res;

	char *errmsg;
	if (sqlite3_exec(database->db, "SELECT MAX(LENGTH(name)) FROM projects", read_single_integer, &res, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}

	return res;
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
database_migrate(struct database *database)
{
	char *errmsg = NULL;
	int rc;

	rc = sqlite3_exec(database->db, "SELECT migration FROM information_schema", find_applied_migrations, 0, &errmsg);
	switch (rc) {
	case SQLITE_OK:
		break;
	case SQLITE_ERROR:
		rc = sqlite3_exec(database->db, "CREATE TABLE information_schema (migration VARCHAR(255) PRIMARY KEY)", NULL, 0, &errmsg);
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
			if (sqlite3_exec(database->db, "BEGIN TRANSACTION", NULL, 0, &errmsg) != SQLITE_OK) {
				errx(EXIT_FAILURE, "%s", errmsg);
				/* NOTREACHED */
			}
			if (migrations[i].sql) {
			if (sqlite3_exec(database->db, migrations[i].sql, NULL, 0, &errmsg) != SQLITE_OK) {
				errx(EXIT_FAILURE, "%s", errmsg);
				/* NOTREACHED */
			}
			}
			if (migrations[i].callback) {
				migrations[i].callback(database);
			}
			char *sql = NULL;
			if (asprintf(&sql, "INSERT INTO information_schema (migration) VALUES ('%s')", migrations[i].name) < 0) {
				err(EXIT_FAILURE, "asprintf");
				/* NOTREACHED */
			}
			if (sqlite3_exec(database->db, sql, NULL, 0, &errmsg) != SQLITE_OK) {
				errx(EXIT_FAILURE, "%s", errmsg);
				/* NOTREACHED */
			}
			free(sql);
			if (sqlite3_exec(database->db, "COMMIT", NULL, 0, &errmsg) != SQLITE_OK) {
				errx(EXIT_FAILURE, "%s", errmsg);
				/* NOTREACHED */
			}
		}

	}
}

int
database_host_find_by_name(struct database *database, const char *host)
{
	int id = -1;
	char *sql = NULL;
	char *errmsg;

	if (asprintf(&sql, "SELECT id FROM hosts WHERE name = '%s'", host) < 0) {
		err(EXIT_FAILURE, "asprintf");
		/* NOTREACHED */
	}
	if (sqlite3_exec(database->db, sql, read_single_integer, &id, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}
	free(sql);

	return id;
}

int
database_host_find_or_create_by_name(struct database *database, const char *host)
{
	int id = -1;
	char *sql = NULL;

	id = database_host_find_by_name(database, host);
	char *errmsg;

	if (id < 0) {
		if (asprintf(&sql, "INSERT INTO hosts (name) VALUES ('%s')", host) < 0) {
			err(EXIT_FAILURE, "asprintf");
			/* NOTREACHED */
		}
		if (sqlite3_exec(database->db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
			errx(EXIT_FAILURE, "%s", errmsg);
			/* NOTREACHED */
		}
		free(sql);

		id = database_host_find_by_name(database, host);
	}

	return id;
}

int
database_project_find_by_name(struct database *database, const char *project)
{
	int id = -1;
	char *sql = NULL;
	char *errmsg;

	if (asprintf(&sql, "SELECT id FROM projects WHERE name = '%s'", project) < 0) {
		err(EXIT_FAILURE, "asprintf");
		/* NOTREACHED */
	}
	if (sqlite3_exec(database->db, sql, read_single_integer, &id, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}
	free(sql);

	return id;
}

int
database_project_find_or_create_by_name(struct database *database, const char *project)
{
	int id = -1;
	char *sql = NULL;

	id = database_project_find_by_name(database, project);
	char *errmsg;

	if (id < 0) {
		if (asprintf(&sql, "INSERT INTO projects (name) VALUES ('%s')", project) < 0) {
			err(EXIT_FAILURE, "asprintf");
			/* NOTREACHED */
		}
		if (sqlite3_exec(database->db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
			errx(EXIT_FAILURE, "%s", errmsg);
			/* NOTREACHED */
		}
		free(sql);

		id = database_project_find_by_name(database, project);
	}

	return id;
}

void
database_project_add_duration(struct database *database, int project_id, time_t date, int duration)
{
	char *sql;
	char *errmsg;

	if (asprintf(&sql, "INSERT INTO activity (project_id, host_id, date, duration) VALUES (%d, %d, %ld, %d) ON CONFLICT (project_id, host_id, date) DO UPDATE SET duration = duration + %d", project_id, host_id(database), date, duration, duration) < 0) {
		err(EXIT_FAILURE, "asprintf");
		/* NOTREACHED */
	}
	if (sqlite3_exec(database->db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}
	free(sql);
}

int
database_get_duration(struct database *database, time_t since, time_t until, const char *sql_filter)
{
	int duration = 0;
	char *sql = NULL;
	char *errmsg;
	if (asprintf(&sql, "SELECT SUM(duration) FROM activity WHERE date >= %ld AND date < %ld%s", since, until, sql_filter) < 0) {
		err(EXIT_FAILURE, "asprintf");
		/* NOTREACHED */
	}
	if (sqlite3_exec(database->db, sql, read_single_integer, &duration, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}
	free(sql);

	return duration;
}

int
database_get_duration_by_project(struct database *database, time_t since, time_t until, char *sql_filter, void (*callback)(const char *project, int duration, void *data), void *data)
{
	int total_duration = 0;

	char *sql = NULL;

	if (asprintf(&sql, "SELECT projects.name, SUM(activity.duration) FROM projects LEFT JOIN activity on projects.id = project_id WHERE date >= %ld AND date < %ld%s GROUP BY activity.project_id ORDER BY projects.name", since, until, sql_filter) < 0) {
		err(EXIT_FAILURE, "asprintf");
		/* NOTREACHED */
	}

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(database->db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK) {
		err(EXIT_FAILURE, "sqlite3_prepare_v2");
	}

	int res;
	while ((res = sqlite3_step(stmt)) != SQLITE_DONE) {
		if (res == SQLITE_ROW) {
			callback((const char *)sqlite3_column_text(stmt, 0),
			         sqlite3_column_int(stmt, 1),
			         data);
		} else {
			err(EXIT_FAILURE, "sqlite3_step");
		}
		total_duration += sqlite3_column_int(stmt, 1);
	}

	sqlite3_finalize(stmt);

	free(sql);

	return total_duration;
}

struct import {
	struct database *target;
	struct database *import;
	int target_host_id;
	int target_project_id;
	int import_project_id;
};

static int
find_project_id(void *result, int argc, char **argv, char **column_name)
{
	struct import *si = result;

	(void) argc;
	(void) column_name;

	si->target_project_id = database_project_find_or_create_by_name(si->target, argv[0]);

	return 0;
}

static int
merge_host_activity(void *result, int argc, char **argv, char **column_name)
{
	struct import *si = result;
	char *sql;

	(void) argc;
	(void) column_name;

	char *errmsg;
	if (atoi(argv[0]) != si->import_project_id) {
		si->import_project_id = strtol(argv[0], NULL, 10);

		char *sql;
		if (asprintf(&sql, "SELECT name FROM projects WHERE id = %d", si->import_project_id) < 0) {
			err(EXIT_FAILURE, "asprintf");
			/* NOTREACHED */
		}

		if (sqlite3_exec(si->import->db, sql, find_project_id, si, &errmsg) != SQLITE_OK) {
			errx(EXIT_FAILURE, "%s", errmsg);
			/* NOTREACHED */
		}

		free(sql);
	}

	if (asprintf(&sql, "INSERT INTO activity (project_id, host_id, date, duration) VALUES (%d, %d, %s, %s)", si->target_project_id, si->target_host_id, argv[1], argv[2]) < 0) {
		err(EXIT_FAILURE, "asprintf");
		/* NOTREACHED */
	}

	if (sqlite3_exec(si->target->db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}

	free(sql);

	return 0;
}

static int
merge_host(void *result, int argc, char **argv, char **column_name)
{
	(void) argc;
	(void) column_name;

	struct import *si = result;
	// Skip self
	if (strcmp(argv[1], short_hostname()) == 0) {
		return 0;
	}

	si->target_host_id = database_host_find_or_create_by_name(si->target, argv[1]);
	si->target_project_id = -1;
	si->import_project_id = -1;

	char *sql;
	if (asprintf(&sql, "DELETE FROM activity WHERE host_id = %d", si->target_host_id) < 0) {
		err(EXIT_FAILURE, "asprintf");
		/* NOTREACHED */
	}

	char *errmsg;
	if (sqlite3_exec(si->target->db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}

	free(sql);

	if (asprintf(&sql, "SELECT project_id, date, duration FROM activity WHERE host_id = %s ORDER BY project_id", argv[0]) < 0) {
		err(EXIT_FAILURE, "asprintf");
		/* NOTREACHED */
	}

	if (sqlite3_exec(si->import->db, sql, merge_host_activity, si, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}

	free(sql);

	return 0;
}

void
database_merge(struct database *database, struct database *import)
{
	struct import si = {
		.target = database,
		.import = import,
	};

	char *errmsg;
	if (sqlite3_exec(import->db, "SELECT id, name FROM hosts", merge_host, &si, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}
}

static int
callback_single_string(void *result, int argc, char **argv, char **column_name)
{
	(void) column_name;

	void (*callback)(char *host) = result;

	if (argc == 1 && argv[0]) {
		callback(argv[0]);
	}
	return 0;
}

void
database_list_hosts(struct database *database, void (*callback)(char *host))
{
	char *errmsg;
	if (sqlite3_exec(database->db, "SELECT name FROM hosts ORDER BY NAME", callback_single_string, callback, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}
}

void
database_list_projects(struct database *database, void (*callback)(char *project))
{
	char *errmsg;
	if (sqlite3_exec(database->db, "SELECT name FROM projects ORDER BY NAME", callback_single_string, callback, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}
}

void
database_close(struct database *database)
{
	sqlite3_close(database->db);
	free(database);
}
