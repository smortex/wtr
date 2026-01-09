#ifndef DATABASE_H
#define DATABASE_H

#include <time.h>

struct database;

char		*database_path(void);
struct database	*database_open(char *filename);
void		 database_close(struct database *database);

int		 database_longest_project_name(struct database *database);
void		 database_merge(struct database *database, struct database *import);
void		 database_merge_project(struct database *database, const char *old_project, const char *new_project);
int		 database_host_find_by_name(struct database *database, const char *host);
int		 database_host_find_or_create_by_name(struct database *database, const char *host);
void		 database_list_hosts(struct database *database, void (*callback)(char *host));
void		 database_list_projects(struct database *database, void (*callback)(char *project));
int		 database_project_find_by_name(struct database *database, const char *project);
int		 database_project_find_or_create_by_name(struct database *database, const char *project);
void		 database_project_add_duration(struct database *database, int project_id, time_t date, int duration);
int		 database_get_duration(struct database *database, time_t since, time_t until, const char *sql_filter);
int		 database_get_duration_by_project(struct database *database, time_t since, time_t until, char *project_sql_filter, char *host_sql_filter, void (*callback)(const char *project, int duration, void *data), void *data);

#endif
