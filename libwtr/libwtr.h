#ifndef LIBWTR_H
#define LIBWTR_H

#include "config.h"
#include "database.h"
#include "utils.h"


void		 process_working_directory(const char *working_directory);
void		 each_user_process_working_directory(void callback(const char *working_directory));

#endif
