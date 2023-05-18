%{

#include <time.h>

#include "wtr.h"
#include "../libwtr/libwtr.h"

#include "cmd_parser.h"

%}

%option nounput noinput

%%

add				{ return ADD; }
remove				{ return REMOVE; }
to				{ return TO; }
from				{ return FROM; }
today				{ return TODAY; }
yesterday			{ return YESTERDAY; }
active				{ return ACTIVE; }
edit				{ return EDIT; }
list				{ return LIST; }
since				{ return SINCE; }
until				{ return UNTIL; }
this				{ return THIS; }
last				{ return LAST; }
ago				{ return AGO; }
by				{ return BY; }
rounding			{ return ROUNDING; }
days?				{ yylval.time_unit = (time_unit_t){1, 0, 0, 0}; return DAY; }
weeks?				{ yylval.time_unit = (time_unit_t){0, 1, 0, 0}; return WEEK; }
months?				{ yylval.time_unit = (time_unit_t){0, 0, 1, 0}; return MONTH; }
years?				{ yylval.time_unit = (time_unit_t){0, 0, 0, 1}; return YEAR; }

[0-9]+(s|sec)			{ yylval.integer = strtol(yytext, NULL, 10); return DURATION; }
[0-9]+(m|mn|min)		{ yylval.integer = strtol(yytext, NULL, 10) * 60; return DURATION; }
[0-9]+(h|hr|hrs)		{ yylval.integer = strtol(yytext, NULL, 10) * 3600; return DURATION; }
[0-9]{1,2}:[0-9]{2}		{ scan_duration(yytext, &yylval.integer); return DURATION; }
[0-9]+:[0-9]{2}:[0-9]{2}	{ scan_duration(yytext, &yylval.integer); return DURATION; }

[0-9]{4}-[0-9]{2}-[0-9]{2}	{ scan_date(yytext,&yylval.date); return DATE; }
-?[0-9]+			{ yylval.integer = strtol(yytext, NULL, 10); return INTEGER; }
.+				{ yylval.string = yytext; return PROJECT; }

%%

extern int global_argc;
extern char **global_argv;
int
yywrap(void)
{
	global_argc -= 1;
	global_argv += 1;
	if (global_argc > 0) {
		if (YY_CURRENT_BUFFER)
			yy_delete_buffer(YY_CURRENT_BUFFER);

		yy_scan_string(global_argv[0]);
		return 0;
	} else {
		return 1;
	}
}

