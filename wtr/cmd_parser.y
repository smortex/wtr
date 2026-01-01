%{
#include <err.h>
#include <stdio.h>
#include <stdbool.h>
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
    { beginning_of_quarter, add_quarter },
    { beginning_of_year, add_year },
};

report_options_t combine_report_parts(report_options_t a, report_options_t b);

GList *
add_host_by_name(struct database *database, GList *list, const char *name)
{
    bool found = false;

    int id = database_host_find_by_name(database, name);
    if (id >= 0) {
	list = g_list_append(list, GINT_TO_POINTER(id));
	found = true;
    }

    if (!found) {
	errx(EXIT_FAILURE, "%s: no such host", name);
    }

    return list;
}

GList *
add_project_by_name(struct database *database, GList *list, const char *name)
{
    bool found = false;

    if (name[0] == '+') {
	for (size_t i = 0; i < nprojects; i++) {
	    if (projects[i].tags && g_strv_contains((const gchar *const*)projects[i].tags, name + 1)) {
		list = g_list_append(list, GINT_TO_POINTER(projects[i].id));
		found = true;
	    }
	}
    } else {
	int id = database_project_find_by_name(database, name);
	if (id >= 0) {
		list = g_list_append(list, GINT_TO_POINTER(id));
		found = true;
	}
    }

    if (!found) {
	errx(EXIT_FAILURE, "%s: no such project", name);
    }

    return list;
}

void
g_list_print(FILE *io, GList *list)
{
    fprintf(io, "(");
    while (list) {
	fprintf(io, "%d", GPOINTER_TO_INT(list->data));
	if (list->next) {
	    fprintf(io, ", ");
	}
	list = list->next;
    }
    fprintf(io, ")");
}

void
time_print(FILE *io, time_t time)
{
    struct tm *t = localtime(&time);
    char buf[BUFSIZ];
    strftime(buf, sizeof(buf), "%c", t);
    fprintf(io, "%ld (%s)", time, buf);
}

report_options_t empty_options;

%}

%define parse.trace
%define parse.error verbose

%printer { fprintf(yyo, "%d", $$); } <integer>;
%printer { time_print(yyo, $$); } <date>;
%printer { fprintf(yyo, "%s", $$); } <string>;
%printer {
    fprintf(yyo, "since=");
    time_print(yyo, $$.since);
    fprintf(yyo, " until=");
    time_print(yyo, $$.until);
    fprintf(yyo, " rounding=%d projects=", $$.rounding);
    g_list_print(yyo, $$.projects);
    fprintf(yyo, " hosts=");
    g_list_print(yyo, $$.hosts);
} <report_options>;
%printer {
    fprintf(yyo, "<%p> ", $$);
    g_list_print(yyo, $$);
} <projects> <hosts>;

%union {
    int integer;
    time_t date;
    char *string;
    time_unit_t time_unit;
    report_options_t report_options;
    GList *projects;
    GList *hosts;
}

%start command

%token ADD REMOVE
%token TO FROM
%token <string> IDENTIFIER
%token ACTIVE EDIT LIST
%token HOSTS PROJECTS
%token TODAY YESTERDAY TOMORROW
%token SINCE UNTIL
%token <integer> DURATION
%token <time_unit> DAY WEEK MONTH QUARTER YEAR
%token THIS LAST NEXT AGO IN
%token BY
%token <date> DATE
%token <integer> INTEGER
%token ROUNDING
%token ON HOST
%token GRAPH MERGE INTO

%type <report_options> moment report_part report graph_options graph_part time_span
%type <integer> time_unit
%type <integer> duration
%type <projects> projects
%type <hosts> hosts

%parse-param {struct database *database}

%destructor { free($$); } <string>
%destructor { g_list_free($$); } <projects> <hosts>
%destructor { g_list_free($$.hosts); g_list_free($$.projects); } <report_options>

%%

command: ACTIVE YYEOF { wtr_active(); }
       | EDIT YYEOF { wtr_edit(); }
       | LIST PROJECTS YYEOF { wtr_list_projects(database); }
       | LIST HOSTS YYEOF { wtr_list_hosts(database); }
       | ADD duration TO IDENTIFIER YYEOF { wtr_add_duration_to_project_on(database, $2, $4, today()); free($4); }
       | REMOVE duration FROM IDENTIFIER YYEOF { wtr_add_duration_to_project_on(database, - $2, $4, today()); free($4); }
       | ADD duration TO IDENTIFIER moment YYEOF { wtr_add_duration_to_project_on(database, $2, $4, $5.since); free($4); }
       | REMOVE duration FROM IDENTIFIER moment YYEOF { wtr_add_duration_to_project_on(database, - $2, $4, $5.since); free($4); }
       | report YYEOF {  wtr_report(database, $1); g_list_free($1.projects); g_list_free($1.hosts); }
       | GRAPH YYEOF { wtr_graph(database, empty_options); }
       | GRAPH graph_options YYEOF { wtr_graph(database, $2); g_list_free($2.projects); g_list_free($2.hosts); }
       | MERGE IDENTIFIER YYEOF { wtr_merge(database, $2); free($2); }
       | MERGE IDENTIFIER INTO IDENTIFIER YYEOF { wtr_merge_project(database, $2, $4); free($2); free($4); }
       ;

report: report report_part { $$ = combine_report_parts($1, $2); }
      | report_part { $$ = $1; }
      ;

report_part: time_span { $$ = $1; }
	   | BY time_unit { $$ = empty_options; $$.next = time_unit_functions[$2].add; }
	   | ROUNDING DURATION { $$ = empty_options; $$.rounding = $2; }
	   | ON projects { $$ = empty_options; $$.projects = $2; }
	   | ON host hosts { $$ = empty_options; $$.hosts = $3; }
	   | ON THIS HOST { $$ = empty_options; $$.hosts = add_host_by_name(database, NULL, short_hostname()); }
	   ;

graph_options: graph_options graph_part { $$ = combine_report_parts($1, $2); }
	     | graph_part { $$ = $1; }
	     ;

graph_part: time_span { $$ = $1; }
	  | BY time_unit { $$ = empty_options; $$.next = time_unit_functions[$2].add; }
	  | ON projects { $$ = empty_options; $$.projects = $2; }
	  | ON host hosts { $$ = empty_options; $$.hosts = $3; }
	  | ON THIS HOST { $$ = empty_options; $$.hosts = add_host_by_name(database, NULL, short_hostname()); }
	  ;

host: HOSTS
    | HOST
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
      | YESTERDAY { $$.since = add_day(today(), -1); $$.until = add_day($$.since, 1);; }
      | TOMORROW { $$.since = add_day(today(), 1); $$.until = add_day($$.since, 1); }
      | ON DATE { $$.since = $2; $$.until = add_day($$.since, 1); }
      | THIS time_unit { $$.since = time_unit_functions[$2].beginning_of(today()); $$.until = time_unit_functions[$2].add($$.since, 1); }
      | INTEGER time_unit AGO { $$.since = time_unit_functions[$2].add(time_unit_functions[$2].beginning_of(today()), -$1); $$.until = time_unit_functions[$2].add($$.since, 1); }
      | IN INTEGER time_unit { $$.since = time_unit_functions[$3].add(time_unit_functions[$3].beginning_of(today()), $2); $$.until = time_unit_functions[$3].add($$.since, 1); }
      | LAST time_unit { $$.since = time_unit_functions[$2].add(time_unit_functions[$2].beginning_of(today()), -1); $$.until = time_unit_functions[$2].add($$.since, 1); }
      | LAST INTEGER time_unit { $$.since = time_unit_functions[$3].add(time_unit_functions[$3].beginning_of(today()), -$2); $$.until = time_unit_functions[$3].add($$.since, $2); }
      | NEXT time_unit { $$.since = time_unit_functions[$2].add(time_unit_functions[$2].beginning_of(today()), 1); $$.until = time_unit_functions[$2].add($$.since, 1); }
      | NEXT INTEGER time_unit { $$.since = time_unit_functions[$3].add(time_unit_functions[$3].beginning_of(today()), 1); $$.until = time_unit_functions[$3].add($$.since, $2); }
      ;

time_unit: DAY { $$ = 0; }
	 | WEEK { $$ = 1; }
	 | MONTH { $$ = 2; }
	 | QUARTER { $$ = 3; }
	 | YEAR { $$ = 4; }
	 ;

projects: projects IDENTIFIER { $$ = add_project_by_name(database, $1, $2); free($2); }
	| IDENTIFIER { $$ = add_project_by_name(database, NULL, $1); free($1); }
	;

hosts: hosts IDENTIFIER { $$ = add_host_by_name(database, $1, $2); free($2); }
     | IDENTIFIER { $$ = add_host_by_name(database, NULL, $1); free($1); }
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
    if (a.hosts && b.hosts)
	errx(EXIT_FAILURE, "multiple host filters");

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
    if (a.hosts)
	res.hosts = a.hosts;
    else
	res.hosts = b.hosts;

    return res;
}


void
yyerror(struct database *database, const char *msg)
{
    (void) database;

    fprintf(stderr, "yyerror: %s\n", msg);
}
