%{
#include <err.h>
#include <stdio.h>
#include <time.h>

#include "wtr.h"
#include "../libwtr/libwtr.h"

#include "cmd_lexer.h"
#include "cmd_parser.h"

void		 yyerror(const char *msg);

struct {
    time_t (*beginning_of)(time_t);
    time_t (*add)(time_t, int);
} time_unit_functions[] = {
    { beginning_of_day, add_day },
    { beginning_of_week, add_week },
    { beginning_of_month, add_month },
    { beginning_of_year, add_year },
};

duration_t combine_report_parts(duration_t a, duration_t b);
%}

%define parse.trace
%define parse.error verbose

%union {
    int integer;
    int project_id;
    time_t date;
    char *string;
    time_unit_t time_unit;
    duration_t duration;
}

%start command

%token ADD REMOVE
%token TO FROM
%token <string> PROJECT
%token ACTIVE EDIT LIST
%token TODAY YESTERDAY
%token SINCE UNTIL
%token <integer> DURATION
%token <time_unit> DAY WEEK MONTH YEAR
%token THIS LAST AGO
%token <date> DATE
%token <integer> INTEGER

%type <duration> duration report_part report
%type <integer> time_unit

%%

command: ACTIVE { wtr_active(); }
       | EDIT { wtr_edit(); }
       | LIST { wtr_list(); }
       | ADD DURATION TO PROJECT { wtr_add_duration_to_project($2, $4); }
       | REMOVE DURATION FROM PROJECT { wtr_add_duration_to_project(- $2, $4); }
       | report {  wtr_report($1.since, $1.until); }
       ;

report: report report_part { $$ = combine_report_parts($1, $2); }
      | report_part { $$ = $1; }
      ;

report_part: duration { $$ = $1; }
	   | SINCE DATE { $$.since = $2; $$.until = 0; }
	   | UNTIL DATE { $$.since = 0; $$.until = $2; }
	   ;

duration: INTEGER { $$.since = add_day(today(), $1); $$.until = add_day($$.since, 1); }
	| TODAY { $$.since = today(); $$.until = add_day($$.since, 1); }
	| YESTERDAY { $$.since = add_day(today(), -1); $$.until = today(); }
	| THIS time_unit { $$.since = time_unit_functions[$2].beginning_of(today()); $$.until = time_unit_functions[$2].add($$.since, 1); }
	| INTEGER time_unit AGO { $$.since = time_unit_functions[$2].add(time_unit_functions[$2].beginning_of(today()), -$1); $$.until = time_unit_functions[$2].add($$.since, 1); }
	| LAST time_unit { $$.since = time_unit_functions[$2].add(time_unit_functions[$2].beginning_of(today()), -1); $$.until = time_unit_functions[$2].add($$.since, 1); }
	| LAST INTEGER time_unit { $$.since = time_unit_functions[$3].add(time_unit_functions[$3].beginning_of(today()), -$2); $$.until = time_unit_functions[$3].beginning_of(today()); }
	;

time_unit: DAY { $$ = 0; }
	 | WEEK { $$ = 1; }
	 | MONTH { $$ = 2; }
	 | YEAR { $$ = 3; }
	 ;

%%

duration_t
combine_report_parts(duration_t a, duration_t b)
{
    duration_t res = a;
    if (a.since && b.since)
        errx(EXIT_FAILURE, "multiple since date");
    if (a.until && b.until)
        errx(EXIT_FAILURE, "multiple until date");

    res.since = a.since | b.since;
    res.until = a.until | b.until;

    return res;
}


void
yyerror(const char *msg)
{
    fprintf(stderr, "yyerror: %s\n", msg);
}