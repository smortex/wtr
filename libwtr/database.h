#ifndef DATABASE_H
#define DATABASE_H

#include <time.h>

int		 database_open(void);
void		 database_close(void);

int		 database_project_find_by_name(const char *project);
int		 database_project_find_or_create_by_name(const char *project);
void		 database_project_add_duration(int project_id, time_t date, int duration);
int		 database_project_get_duration(int project_id, time_t from, time_t to);

#endif
