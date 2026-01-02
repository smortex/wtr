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
read_single_integer(void *r, int argc, char **argv, char **column_name)
{
	(void) column_name;

	if (argc != 1 || argv[0] == NULL) {
		return 1;
	}

	int *result = r;
	char *rest;
	*result = strtol(argv[0], &rest, 10);

	if (*rest) {
		return 1;
	}

	return 0;
}

static int
read_single_string(void *r, int argc, char **argv, char **column_name)
{
	(void) column_name;

	if (argc != 1 || argv[0] == NULL) {
		return 1;
	}

	char **result = r;
	*result = strdup(argv[0]);

	return 0;
}

static int
read_single_time_t(void *r, int argc, char **argv, char **column_name)
{
	(void) column_name;

	if (argc != 1 || argv[0] == NULL) {
		return 1;
	}

	time_t *result = r;
	char *rest;

	*result = strtol(argv[0], &rest, 10);

	if (*rest) {
		return 1;
	}

	return 0;
}

struct merged_project_info {
	char *old_project_name;
	char *new_project_name;
	time_t created_at;
};

static int
read_merged_project_info(void *result, int argc, char **argv, char **column_name)
{
	(void) argc;
	(void) column_name;

	struct merged_project_info *info = result;

	info->old_project_name = strdup(argv[0]);
	info->new_project_name = strdup(argv[1]);
	info->created_at = strtol(argv[2], NULL, 10);

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
	{ 0, "202512311243", "CREATE TABLE merged_projects (old_project_name VARCHAR(255) PRIMARY KEY NOT NULL, new_project_name VARCHAR(255) NOT NULL, created_at INTEGER)", NULL },
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

char *
database_version(struct database *database)
{
	char *res;

	char *errmsg;
	if (sqlite3_exec(database->db, "SELECT MAX(migration) FROM information_schema", read_single_string, &res, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}

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
find_applied_migration(void *result, int argc, char **argv, char **column_name)
{
	(void) result;
	(void) column_name;

	if (argc != 1) {
		return 1;
	}

	for (size_t i = 0; i < sizeof(migrations) / sizeof(*migrations); i++) {
		if (strcmp(argv[0], migrations[i].name) == 0) {
			migrations[i].applied = 1;
		}
	}
	return 0;
}

static void
load_applied_migrations(struct database *database)
{
	char *errmsg = NULL;
	int rc;

	rc = sqlite3_exec(database->db, "SELECT migration FROM information_schema", find_applied_migration, 0, &errmsg);
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
}

static void
apply_migration(struct database *database, struct migration *migration)
{
	char *errmsg = NULL;

	if (sqlite3_exec(database->db, "BEGIN TRANSACTION", NULL, 0, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}
	if (migration->sql) {
		if (sqlite3_exec(database->db, migration->sql, NULL, 0, &errmsg) != SQLITE_OK) {
			errx(EXIT_FAILURE, "%s", errmsg);
			/* NOTREACHED */
		}
	}
	if (migration->callback) {
		migration->callback(database);
	}
	char *sql = NULL;
	if (asprintf(&sql, "INSERT INTO information_schema (migration) VALUES ('%s')", migration->name) < 0) {
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

static void
database_migrate(struct database *database)
{
	load_applied_migrations(database);

	for (size_t i = 0; i < sizeof(migrations) / sizeof(*migrations); i++) {
		if (!migrations[i].applied) {
			apply_migration(database, &migrations[i]);
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

	if (id < 0) {
		if (asprintf(&sql, "SELECT old_project_name, new_project_name, created_at FROM merged_projects WHERE old_project_name = '%s'", project) < 0) {
			err(EXIT_FAILURE, "asprintf");
			/* NOTREACHED */
		}
		struct merged_project_info info = {
			.old_project_name = NULL,
			.new_project_name = NULL,
			.created_at = 0,
		};

		if (sqlite3_exec(database->db, sql, read_merged_project_info, &info, &errmsg) != SQLITE_OK) {
			errx(EXIT_FAILURE, "%s", errmsg);
			/* NOTREACHED */
		}

		if (info.old_project_name) {
			char date[BUFSIZ];

			strftime(date, sizeof(date), "%FT%T%z", localtime(&info.created_at));
			warnx("Project %s was merged into %s on %s.  You should remove it from your configuration.", info.old_project_name, info.new_project_name, date);

			id = database_project_find_by_name(database, info.new_project_name);

			free(info.old_project_name);
			free(info.new_project_name);
		}
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
	if (asprintf(&sql, "SELECT COALESCE(SUM(duration), 0) FROM activity WHERE date >= %ld AND date < %ld%s", since, until, sql_filter) < 0) {
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
database_get_duration_by_project(struct database *database, time_t since, time_t until, char *project_sql_filter, char *host_sql_filter, void (*callback)(const char *project, int duration, void *data), void *data)
{
	int total_duration = 0;

	char *sql = NULL;

	if (asprintf(&sql, "SELECT name, (SELECT COALESCE(SUM(duration), 0) FROM activity WHERE projects.id = project_id AND date >= %ld AND date < %ld%s) FROM projects %s ORDER BY LOWER(projects.name)", since, until, host_sql_filter, project_sql_filter) < 0) {
		err(EXIT_FAILURE, "asprintf");
		/* NOTREACHED */
	}

	sqlite3_stmt *stmt;
	int res;
	if ((res = sqlite3_prepare_v2(database->db, sql, strlen(sql), &stmt, NULL)) != SQLITE_OK) {
		errx(EXIT_FAILURE, "sqlite3_prepare_v2: %s", sqlite3_errstr(res));
	}

	while ((res = sqlite3_step(stmt)) != SQLITE_DONE) {
		if (res == SQLITE_ROW) {
			callback((const char *)sqlite3_column_text(stmt, 0),
			         sqlite3_column_int(stmt, 1),
			         data);
		} else {
			errx(EXIT_FAILURE, "sqlite3_step: %s", sqlite3_errstr(res));
		}
		total_duration += sqlite3_column_int(stmt, 1);
	}

	if ((res = sqlite3_finalize(stmt)) != SQLITE_OK) {
		errx(EXIT_FAILURE, "sqlite3_finalize: %s", sqlite3_errstr(res));
	}

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

	if (asprintf(&sql, "INSERT INTO activity (project_id, host_id, date, duration) VALUES (%d, %d, %s, %s) ON CONFLICT (project_id, host_id, date) DO UPDATE SET duration = %s WHERE project_id = %d AND host_id = %d AND date = %s AND duration < %s", si->target_project_id, si->target_host_id, argv[1], argv[2], argv[2], si->target_project_id, si->target_host_id, argv[1], argv[2]) < 0) {
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
	char *errmsg;

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

static int
merge_merged_projects(void *result, int argc, char **argv, char **column_name)
{
	(void) argc;
	(void) column_name;

	struct import *si = result;

	char *sql;
	char *errmsg;

	if (asprintf(&sql, "SELECT created_at FROM merged_projects WHERE old_project_name = '%s'", argv[0]) < 0) {
		err(EXIT_FAILURE, "asprintf");
		/* NOTREACHED */
	}

	time_t created_at = -1;

	if (sqlite3_exec(si->target->db, sql, read_single_time_t, &created_at, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}

	if (created_at < 0) {
		if (asprintf(&sql, "INSERT INTO merged_projects (old_project_name, new_project_name, created_at) VALUES ('%s', '%s', %s)", argv[0], argv[1], argv[2]) < 0) {
			err(EXIT_FAILURE, "asprintf");
			/* NOTREACHED */
		}

		if (sqlite3_exec(si->target->db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
			errx(EXIT_FAILURE, "%s", errmsg);
			/* NOTREACHED */
		}
	} else if (created_at != strtol(argv[2], NULL, 10)) {
		errx(EXIT_FAILURE, "%s: conflicting merge date: %s (import database), %ld (target database)", argv[0], argv[2], created_at);
	}

	free(sql);

	return 0;
}

void
database_merge(struct database *database, struct database *import)
{
	char *target_version = database_version(database);
	char *import_version = database_version(import);

	if (strcmp(target_version, import_version) != 0) {
		errx(EXIT_FAILURE, "database version mismatch: %s (target database), %s (import database)", target_version, import_version);
	}

	free(target_version);
	free(import_version);

	struct import si = {
		.target = database,
		.import = import,
	};

	char *errmsg;
	if (sqlite3_exec(database->db, "BEGIN TRANSACTION", NULL, 0, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}

	if (sqlite3_exec(import->db, "SELECT id, name FROM hosts", merge_host, &si, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}

	if (sqlite3_exec(import->db, "SELECT old_project_name, new_project_name, created_at FROM merged_projects", merge_merged_projects, &si, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}

	if (sqlite3_exec(database->db, "COMMIT", NULL, 0, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}
}

void
database_merge_project(struct database *database, const char *old_project_name, const char *new_project_name)
{
	int old_project_id = database_project_find_by_name(database, old_project_name);
	if (old_project_id < 0) {
		errx(EXIT_FAILURE, "%s: no such project", old_project_name);
	}

	int new_project_id = database_project_find_by_name(database, new_project_name);
	if (new_project_id < 0) {
		errx(EXIT_FAILURE, "%s: no such project", new_project_name);
	}

	char *errmsg;
	if (sqlite3_exec(database->db, "BEGIN TRANSACTION", NULL, 0, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}

	char *sql;
	if (asprintf(&sql, "UPDATE activity SET project_id = %d WHERE project_id = %d", new_project_id, old_project_id) < 0) {
		err(EXIT_FAILURE, "asprintf");
		/* NOTREACHED */
	}

	if (sqlite3_exec(database->db, sql, NULL, 0, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}
	free(sql);

	if (asprintf(&sql, "DELETE FROM projects WHERE id = %d", old_project_id) < 0) {
		err(EXIT_FAILURE, "asprintf");
		/* NOTREACHED */
	}

	if (sqlite3_exec(database->db, sql, NULL, 0, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}
	free(sql);

	if (asprintf(&sql, "INSERT INTO merged_projects (old_project_name, new_project_name, created_at) VALUES ('%s', '%s', %ld)", old_project_name, new_project_name, time(NULL)) < 0) {
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
	if (sqlite3_exec(database->db, "SELECT name FROM hosts ORDER BY LOWER(name)", callback_single_string, callback, &errmsg) != SQLITE_OK) {
		errx(EXIT_FAILURE, "%s", errmsg);
		/* NOTREACHED */
	}
}

void
database_list_projects(struct database *database, void (*callback)(char *project))
{
	char *errmsg;
	if (sqlite3_exec(database->db, "SELECT name FROM projects ORDER BY LOWER(name)", callback_single_string, callback, &errmsg) != SQLITE_OK) {
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
