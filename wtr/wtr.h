#ifndef WTR_H
#define WTR_H

#include <glib.h>

#include <time.h>

#include "../libwtr/database.h"

typedef struct {
	int day;
	int week;
	int month;
	int quarter;
	int year;
} time_unit_t;

typedef struct {
	time_t since;
	time_t until;
	time_t (*next)(time_t, int);
	int rounding;
	GList *projects;
	GList *hosts;
} report_options_t;

void		 wtr_active(void);
void		 wtr_add_duration_to_project_on(struct database *database, int duration, const char *project, time_t date);
void		 wtr_edit(void);
void		 wtr_list_hosts(struct database *database);
void		 wtr_list_projects(struct database *database);
void		 wtr_report(struct database *database, report_options_t options);
void		 wtr_graph(struct database *database, report_options_t options);
void		 wtr_graph_auto(struct database *database);
void		 wtr_merge(struct database *database, char *filename);
void		 wtr_merge_project(struct database *database, const char *old_project_name, const char *new_project_name);

#endif
