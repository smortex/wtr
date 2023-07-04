#ifndef WTR_H
#define WTR_H

#include <time.h>

#include "../libwtr/database.h"

typedef struct {
    int day;
    int week;
    int month;
    int year;
} time_unit_t;

typedef struct project_list {
    int id;
    struct project_list *next;
} id_list_t;

typedef struct {
    time_t since;
    time_t until;
    time_t (*next)(time_t, int);
    int rounding;
    id_list_t *projects;
    id_list_t *hosts;
} report_options_t;

id_list_t	*id_list_new(struct database *database, char *what, int (*find_callback)(struct database *database, const char *name), const char *name);
id_list_t	*id_list_add(struct database *database, id_list_t *head, char *what, int (*find_callback)(struct database *database, const char *name), const char *name);
void		 id_list_free(id_list_t *head);

void		 wtr_active(void);
void		 wtr_add_duration_to_project_on(struct database *database, int duration, const char *project, time_t date);
void		 wtr_edit(void);
void		 wtr_list(void);
void		 wtr_report(struct database *database, report_options_t options);
void		 wtr_graph(struct database *database, report_options_t options);
void		 wtr_graph_auto(struct database *database);

#endif
