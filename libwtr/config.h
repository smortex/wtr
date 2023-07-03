#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>

#include "database.h"

char		*config_file_path(void);
int		 config_load(struct database *database);
void		 config_free(void);

extern size_t nprojects;
struct project {
	int id;
	char *name;
	char *root;
	int active;
};

extern struct project *projects;

#endif
