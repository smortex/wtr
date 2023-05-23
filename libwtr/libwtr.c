#include <string.h>

#include "libwtr.h"

void
process_working_directory(const char *working_directory)
{
	if (!working_directory) {
		return;
	}

	for (size_t i = 0; i < nprojects; i++) {
		if (strnstr(working_directory, projects[i].root, strlen(projects[i].root)) == working_directory &&
		    (working_directory[strlen(projects[i].root)] == '/' ||
		     working_directory[strlen(projects[i].root)] == '\0')) {
			projects[i].active = 1;
			break;
		}
	}
}
