#ifndef UTILS_H
#define UTILS_H

#include <time.h>

char		*short_hostname(void);
int		 scan_date(const char *str, time_t *date);
int		 scan_duration(const char *str, int *duration);
time_t		 today(void);
time_t		 beginning_of_day(time_t date);
time_t		 beginning_of_week(time_t date);
time_t		 beginning_of_month(time_t date);
time_t		 beginning_of_year(time_t date);
time_t		 add_day(time_t date, int n);
time_t		 add_week(time_t date, int n);
time_t		 add_month(time_t date, int n);
time_t		 add_year(time_t date, int n);

#endif
