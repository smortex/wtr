#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>

char		*config_file_path(void);
int		 config_load(void);
void		 config_free(void);

extern size_t nroots;
struct root {
	int id;
	char *name;
	char *root;
	int active;
};

extern struct root *roots;

#endif
