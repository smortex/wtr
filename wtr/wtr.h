#ifndef WTR_H
#define WTR_H

#include <time.h>

typedef struct {
    int day;
    int week;
    int month;
    int year;
} time_unit_t;

typedef struct project_list {
    int id;
    struct project_list *next;
} project_list_t;

typedef struct {
    time_t since;
    time_t until;
    time_t (*next)(time_t, int);
    int rounding;
    project_list_t *projects;
} report_options_t;

project_list_t	*project_list_new(const char *name);
project_list_t	*project_list_add(project_list_t *head, const char *name);
void		 project_list_free(project_list_t *head);

void		 wtr_active(void);
void		 wtr_add_duration_to_project(int duration, const char *project);
void		 wtr_edit(void);
void		 wtr_list(void);
void		 wtr_report(report_options_t options);
void		 wtr_graph(report_options_t options);
void		 wtr_graph_auto(void);

#endif
