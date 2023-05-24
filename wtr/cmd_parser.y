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

report_options_t combine_report_parts(report_options_t a, report_options_t b);
%}

%define parse.trace
%define parse.error verbose

%union {
    int integer;
    int project_id;
    time_t date;
    char *string;
    time_unit_t time_unit;
    report_options_t report_options;
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
%token BY
%token <date> DATE
%token <integer> INTEGER
%token ROUNDING

%type <report_options> moment report_part report
%type <integer> time_unit
%type <integer> duration

%%

command: ACTIVE YYEOF { wtr_active(); }
       | EDIT YYEOF { wtr_edit(); }
       | LIST YYEOF { wtr_list(); }
       | ADD duration TO PROJECT YYEOF { wtr_add_duration_to_project($2, $4); }
       | REMOVE duration FROM PROJECT YYEOF { wtr_add_duration_to_project(- $2, $4); }
       | report YYEOF {  wtr_report($1); }
       ;

report: report report_part { $$ = combine_report_parts($1, $2); }
      | report_part { $$ = $1; }
      ;

report_part: moment { $$ = $1; $$.next = NULL; $$.rounding = 0; }
	   | SINCE DATE { $$.since = $2; $$.until = 0; $$.next = NULL; $$.rounding = 0; }
	   | UNTIL DATE { $$.since = 0; $$.until = $2; $$.next = NULL; $$.rounding = 0; }
	   | SINCE moment { $$.since = $2.since; $$.until = 0; $$.next = NULL; $$.rounding = 0; }
	   | UNTIL moment { $$.since = 0; $$.until = $2.since; $$.next = NULL; $$.rounding = 0; }
	   | BY time_unit { $$.since = 0; $$.until = 0; $$.next = time_unit_functions[$2].add; $$.rounding = 0; }
	   | ROUNDING DURATION { $$.since = 0; $$.until = 0; $$.next = NULL; $$.rounding = $2; }
	   ;

duration: INTEGER
	| DURATION
	;

moment: INTEGER { $$.since = add_day(today(), $1); $$.until = add_day($$.since, 1); }
      | TODAY { $$.since = today(); $$.until = add_day($$.since, 1); }
      | YESTERDAY { $$.since = add_day(today(), -1); $$.until = today(); }
      | THIS time_unit { $$.since = time_unit_functions[$2].beginning_of(today()); $$.until = time_unit_functions[$2].add($$.since, 1); }
      | INTEGER time_unit AGO { $$.since = time_unit_functions[$2].add(today(), -$1); $$.until = time_unit_functions[$2].add($$.since, 1); }
      | LAST time_unit { $$.since = time_unit_functions[$2].add(time_unit_functions[$2].beginning_of(today()), -1); $$.until = time_unit_functions[$2].add($$.since, 1); }
      | LAST INTEGER time_unit { $$.since = time_unit_functions[$3].add(time_unit_functions[$3].beginning_of(today()), -$2); $$.until = time_unit_functions[$3].beginning_of(today()); }
      ;

time_unit: DAY { $$ = 0; }
	 | WEEK { $$ = 1; }
	 | MONTH { $$ = 2; }
	 | YEAR { $$ = 3; }
	 ;

%%

report_options_t
combine_report_parts(report_options_t a, report_options_t b)
{
    report_options_t res = a;
    if (a.since && b.since)
        errx(EXIT_FAILURE, "multiple since date");
    if (a.until && b.until)
        errx(EXIT_FAILURE, "multiple until date");
    if (a.next && b.next)
        errx(EXIT_FAILURE, "multiple next functions");
    if (a.rounding && b.rounding)
        errx(EXIT_FAILURE, "multiple rounding functions");

    res.since = a.since | b.since;
    res.until = a.until | b.until;
    if (a.next)
	res.next = a.next;
    else
	res.next = b.next;
    res.rounding = a.rounding | b.rounding;

    return res;
}


void
yyerror(const char *msg)
{
    fprintf(stderr, "yyerror: %s\n", msg);
}
