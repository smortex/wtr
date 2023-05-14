#ifndef DATABASE_H
#define DATABASE_H

#include <time.h>

int		 database_open(void);
void		 database_close(void);

int		 database_find_or_create_project_by_name(char *project);
void		 database_project_add_duration(int project_id, time_t date, int duration);
int		 database_project_get_duration(int project_id, time_t from, time_t to);

#endif
