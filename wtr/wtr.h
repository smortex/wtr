#ifndef WTR_H
#define WTR_H

#include <time.h>

typedef struct {
    int day;
    int week;
    int month;
    int year;
} time_unit_t;

typedef struct {
    time_t since;
    time_t until;
    time_t (*next)(time_t, int);
    int rounding;
} duration_t;

int		 scan_date(const char *str, time_t *date);
int		 scan_duration(const char *str, int *duration);

void		 wtr_active(void);
void		 wtr_add_duration_to_project(int duration, char *project);
void		 wtr_edit(void);
void		 wtr_list(void);
void		 wtr_report(duration_t duration);

#endif
