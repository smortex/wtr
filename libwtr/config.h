#ifndef CONFIG_H
#define CONFIG_H

#include <glib.h>

int		 config_load(void);
void		 config_free(void);

extern gsize nroots;
struct root {
	int id;
	char *name;
	char *root;
	int active;
};

extern struct root *roots;

#endif
