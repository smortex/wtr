%{
#include <err.h>
#include <stdio.h>
#include <time.h>

#include "wtr.h"
#include "../libwtr/libwtr.h"

#include "cmd_lexer.h"
#include "cmd_parser.h"

void		 yyerror(struct database *database, const char *msg);

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

id_list_t *
project_list_new(struct database *database, const char *name)
{
    id_list_t *head;
    if (!(head = malloc(sizeof(*head))))
	return head;

    head->id = database_project_find_by_name(database, name);
    head->next = NULL;

    if (head->id < 0)
	errx(EXIT_FAILURE, "unknown project: %s", name);

    return head;
}

id_list_t *
project_list_add(struct database *database, id_list_t *head, const char *name)
{
    id_list_t *tail = head;

    while (tail->next)
	tail = tail->next;

    if (!(tail->next = malloc(sizeof(*head))))
	return NULL;

    tail = tail->next;
    tail->id = database_project_find_by_name(database, name);
    tail->next = NULL;

    if (tail->id < 0)
	errx(EXIT_FAILURE, "unknown project: %s", name);

    return tail;
}

void
project_list_free(id_list_t *head)
{
    id_list_t *next;
    while (head) {
	next = head->next;
	free(head);
	head = next;
    }
}

report_options_t empty_options;

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
    id_list_t *projects;
}

%start command

%token ADD REMOVE
%token TO FROM
%token <string> IDENTIFIER
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
%token ON
%token GRAPH

%type <report_options> moment report_part report recursive_time_span time_span
%type <integer> time_unit
%type <integer> duration
%type <projects> projects

%parse-param {struct database *database}

%%

command: ACTIVE YYEOF { wtr_active(); }
       | EDIT YYEOF { wtr_edit(); }
       | LIST YYEOF { wtr_list(); }
       | ADD duration TO IDENTIFIER YYEOF { wtr_add_duration_to_project_on(database, $2, $4, today()); }
       | REMOVE duration FROM IDENTIFIER YYEOF { wtr_add_duration_to_project_on(database, - $2, $4, today()); }
       | ADD duration TO IDENTIFIER moment YYEOF { wtr_add_duration_to_project_on(database, $2, $4, $5.since); }
       | REMOVE duration FROM IDENTIFIER moment YYEOF { wtr_add_duration_to_project_on(database, - $2, $4, $5.since); }
       | report YYEOF {  wtr_report(database, $1); }
       | GRAPH YYEOF { wtr_graph_auto(database, NULL); }
       | GRAPH ON projects YYEOF { wtr_graph_auto(database, $3); }
       | GRAPH recursive_time_span YYEOF { wtr_graph(database, $2); }
       | GRAPH recursive_time_span ON projects YYEOF { $2.projects = $4; wtr_graph(database, $2); }
       ;

report: report report_part { $$ = combine_report_parts($1, $2); }
      | report_part { $$ = $1; }
      ;

report_part: time_span { $$ = $1; }
	   | BY time_unit { $$ = empty_options; $$.next = time_unit_functions[$2].add; }
	   | ROUNDING DURATION { $$ = empty_options; $$.rounding = $2; }
	   | ON projects { $$ = empty_options; $$.projects = $2; }
	   ;

recursive_time_span: recursive_time_span time_span { $$ = combine_report_parts($1, $2); }
		   | time_span { $$ = $1; }
		   ;

time_span: moment { $$ = empty_options; $$.since = $1.since; $$.until = $1.until; }
	 | SINCE DATE { $$ = empty_options; $$.since = $2; }
	 | UNTIL DATE { $$ = empty_options; $$.until = $2; }
	 | SINCE moment { $$ = empty_options; $$.since = $2.since; }
	 | UNTIL moment { $$ = empty_options; $$.until = $2.since; }
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

projects: projects IDENTIFIER { project_list_add(database, $1, $2); $$ = $1; }
	| IDENTIFIER { $$ = project_list_new(database, $1); }
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
    if (a.projects && b.projects)
	errx(EXIT_FAILURE, "multiple project filters");

    res.since = a.since | b.since;
    res.until = a.until | b.until;
    if (a.next)
	res.next = a.next;
    else
	res.next = b.next;
    res.rounding = a.rounding | b.rounding;
    if (a.projects)
	res.projects = a.projects;
    else
	res.projects = b.projects;

    return res;
}


void
yyerror(struct database *database, const char *msg)
{
    (void) database;

    fprintf(stderr, "yyerror: %s\n", msg);
}
