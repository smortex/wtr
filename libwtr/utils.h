#ifndef UTILS_H
#define UTILS_H

#include <time.h>

time_t		 beginning_of_day(time_t date);
time_t		 beginning_of_week(time_t date);
time_t		 beginning_of_month(time_t date);
time_t		 add_day(time_t date, int n);
time_t		 add_week(time_t date, int n);
time_t		 add_month(time_t date, int n);

#endif
