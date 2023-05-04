#include <string.h>

#include "libwtr.h"

void
process_working_directory(const char *working_directory)
{
	for (size_t i = 0; i < nroots; i++) {
		if (strnstr(working_directory, roots[i].root, strlen(roots[i].root)) == working_directory &&
		    (working_directory[strlen(roots[i].root)] == '/' ||
		     working_directory[strlen(roots[i].root)] == '\0')) {
			roots[i].active = 1;
			break;
		}
	}
}
