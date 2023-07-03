#ifndef DATABASE_H
#define DATABASE_H

#include <time.h>

struct database;

struct database	*database_open(void);
void		 database_close(struct database *db);

int		 database_project_find_by_name(struct database *db, const char *project);
int		 database_project_find_or_create_by_name(struct database *db, const char *project);
void		 database_project_add_duration(struct database *db, int project_id, time_t date, int duration);
int		 database_get_duration(struct database *db, time_t since, time_t until, const char *and_project_in);
int		 database_project_get_duration(struct database *db, int project_id, time_t since, time_t until);

#endif
