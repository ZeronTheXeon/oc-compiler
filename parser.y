%{

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "lyutils.h"
#include "astree.h"

%}

%debug
%defines
%error-verbose
%token-table
%verbose

// %destructor { destroy ($$); } <>
// %printer { astree::dump (yyoutput, $$); } <>

%initial-action {
    if (parser::root == nullptr) {
        parser::root = new astree (TOK_ROOT, {0, 0, 0}, "");
    } else {
        errprintf("Found a duplicate root!\n");
    }
}

%token TOK_VOID TOK_INT TOK_STRING
%token TOK_IF TOK_ELSE TOK_WHILE TOK_RETURN TOK_STRUCT
%token TOK_NULL TOK_NEW TOK_ARRAY TOK_ARROW TOK_INDEX
%token TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE TOK_NOT

%token TOK_ROOT TOK_BLOCK TOK_CALL TOK_IFELSE TOK_VARDECL TOK_DECLID
%token TOK_POS TOK_NEG TOK_NEWARRAY TOK_TYPEID TOK_FIELD TOK_FUNCTION
%token TOK_IDENT TOK_INTCON TOK_CHARCON TOK_STRINGCON
%token TOK_PROTO TOK_PARAM TOK_NEWSTR

%right TOK_IF TOK_ELSE
%right '='
%left TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%left '+' '-'
%left '*' '/' '%'
%right TOK_POS TOK_NEG TOK_NOT TOK_NEW
%left TOK_ARROW TOK_ARRAY TOK_FIELD TOK_FUNCTION '['

// %right TOK_VOID TOK_INT TOK_STRING

// %left '(' ')' '[' ']'
//%nonassoc '('
// %nonassoc '['

%start start

%%
start       : program                           { $$ = $1 = nullptr; }
            ;

program     : program structdef                 { $$ = $1->adopt($2); }
            | program function                  { $$ = $1->adopt($2); }
            | program globaldecl                { $$ = $1->adopt($2); }
            | program error '}'                 {
                    $$ = $1;
                    destroy($3);
                }
            | program error ';'                 {
                    $$ = $1;
                    destroy($3);
                }
            |                                   { $$ = parser::root; }
            ;

structdef   : TOK_STRUCT TOK_IDENT '{' '}'      {
                    $2 -> adopt_sym(nullptr, TOK_TYPEID);
                    $$ = $1 -> adopt($2);
                    destroy($3, $4);
                }
            | TOK_STRUCT TOK_IDENT fielddecls '}'   {
                    $2 -> adopt_sym(nullptr, TOK_TYPEID);
                    $$ = $1 -> adopt($2, $3);
                    destroy($4);
                }
            ;

fielddecls  : '{' fielddecl                     {
                    $$ = $1 -> adopt($2);
                }
            | fielddecls fielddecl              {
                    $$ = $1 -> adopt($2);
                }
            ;


fielddecl   : basetype TOK_ARRAY TOK_IDENT ';'  {
                    $$ = $2->adopt($1);
                    $$ = $2->adopt_sym($3, TOK_FIELD);
                    destroy($4);
                }
            | basetype TOK_IDENT ';'            {
                    $$ = $1->adopt_sym($2, TOK_FIELD);
                    destroy($3);
                }
            ;

basetype    : TOK_VOID                          { $$ = $1; }
            | TOK_INT                           { $$ = $1; }
            | TOK_STRING                        { $$ = $1; }
            | TOK_IDENT                         {
                    $$ = $1 -> adopt_sym(nullptr, TOK_TYPEID);
                }
            ;

globaldecl  : identdecl '=' constant ';'        {
                    $$ = $2 -> adopt_sym($1, TOK_VARDECL, $3);
                    destroy($4);
                }
            ;

function    : identdecl '(' ')' ';'             {
                    astree *as = new astree(TOK_PROTO, $1 -> lloc, "");
                    $2 -> adopt_sym(nullptr, TOK_PARAM);
                    $$ = as -> adopt($1, $2);
                    destroy($3, $4);
                }
            | identdecl params ')' ';'          {
                    astree *as = new astree(TOK_PROTO, $1 -> lloc, "");
                    $$ = as->adopt($1, $2);
                    destroy($3, $4);
              }
            | identdecl '(' ')' fnbody '}'      {
                    astree *as = new astree(TOK_FUNCTION, $1 -> lloc,
                                            "");
                    $2 -> adopt_sym(nullptr, TOK_PARAM);
                    $$ = as -> adopt($1, $2, $4);
                    destroy($3, $5);
                }
            | identdecl params ')' fnbody '}'   {
                    astree *as = new astree(TOK_FUNCTION, $1 -> lloc,
                                            "");
                    $$ = as -> adopt($1, $2, $4);
                    destroy($3, $5);
                }
            | identdecl '(' ')' '{' '}'         {
                    astree *as = new astree(TOK_FUNCTION, $1 -> lloc,
                                            "");
                    $2 -> adopt_sym(nullptr, TOK_PARAM);
                    $4 -> adopt_sym(nullptr, TOK_BLOCK);
                    $$ = as -> adopt($1, $2, $4);
                    destroy($3, $5);
                }
            | identdecl params ')' '{' '}'      {
                    astree *as = new astree(TOK_FUNCTION, $1 -> lloc,
                                            "");
                    $4 -> adopt_sym(nullptr, TOK_BLOCK);
                    $$ = as -> adopt($1, $2, $4);
                    destroy($3, $5);
                }
            ;

identdecl   : basetype TOK_ARRAY TOK_IDENT      {
                    $3 -> adopt_sym(nullptr, TOK_DECLID);
                    $$ = $2 -> adopt($1, $3);
                }
            | basetype TOK_IDENT                {
                    $2 -> adopt_sym(nullptr, TOK_DECLID);
                    $$ = $1 -> adopt($2);
                }
            ;

fnbody      : '{' fnstmt                        {
                    $$ = $1 -> adopt_sym($2, TOK_BLOCK);
                }
            | fnbody fnstmt                     {
                    $$ = $1 -> adopt($2);
                }
            ;

fnstmt      : statement                         { $$ = $1; }
            | localdecl                         { $$ = $1; }
            ;

localdecl   : identdecl '=' expr ';'            {
                    $$ = $2 -> adopt_sym($1, TOK_VARDECL, $3);
                    destroy($4);
                }
            ;

// left recursion again
params      : '(' identdecl                     {
                    $$ = $1 -> adopt_sym($2, TOK_PARAM);
                }
            | params ',' identdecl              {
                    $$ = $1 -> adopt($3);
                    destroy($2);
                }
            ;

statements  : '{' statement                     {
                    $$ = $1 -> adopt_sym($2, TOK_BLOCK);
                }
            | statements statement              {
                    $$ = $1 -> adopt($2);
                }
            ;

statement   : block                             { $$ = $1; }
            | while                             { $$ = $1; }
            | ifelse                            { $$ = $1; }
            | return                            { $$ = $1; }
            | expr ';'                          {
                    $$ = $1;
                    destroy($2);
                }
            | ';'                               { $$ = $1; }
            ;

block       : '{' '}'                           {
                    $$ = $1 -> adopt_sym(nullptr, TOK_BLOCK);
                    destroy($2);
                }
            | statements '}'                    {
                    $$ = $1;
                    destroy($2);
                }
            ;

while       : TOK_WHILE '(' expr ')' statement  {
                    // nice and simple, we only want $3 and $5
                    $$ = $1 -> adopt($3, $5);
                    destroy($2, $4);
                }
            ;

ifelse      : TOK_IF '(' expr ')' statement TOK_ELSE statement {
                    $$ = $1 -> adopt($3, $5, $7);
                    destroy($2, $4, $6);
                }
                /* See https://www.gnu.org/software/bison/manual/
                html_node/Contextual-Precedence.html */
            | TOK_IF '(' expr ')' statement %prec TOK_ELSE {
                    $$ = $1 -> adopt($3, $5);
                    destroy($2, $4);
                }
            ;

return      : TOK_RETURN expr ';'               {
                    $$ = $1 -> adopt($2);
                    destroy($3);
                }
            | TOK_RETURN ';'                    {
                    $$ = $1;
                    destroy($2);
                }
            ;


expr        : expr binop expr                   {
                    $$ = $2 -> adopt($1, $3);
                }
            | unop                              { $$ = $1; }
            | allocation                        {
                    $$ = $1;
                }
            | call                              { $$ = $1; }
            | '(' expr ')'                      {
                    $$ = $2;
                    destroy($1, $3);
                }
            | expr TOK_ARROW TOK_IDENT          {
                                $3 -> adopt_sym(nullptr, TOK_FIELD);
                                $$ = $2 -> adopt($1, $3);
                            }
            | variable                          { $$ = $1; }
            | constant                          { $$ = $1; }
            ;

binop       : '='                               { $$ = $1; }
            | TOK_EQ                            { $$ = $1; }
            | TOK_NE                            { $$ = $1; }
            | TOK_LT                            { $$ = $1; }
            | TOK_LE                            { $$ = $1; }
            | TOK_GT                            { $$ = $1; }
            | TOK_GE                            { $$ = $1; }
            | '+'                               { $$ = $1; }
            | '-'                               { $$ = $1; }
            | '*'                               { $$ = $1; }
            | '/'                               { $$ = $1; }
            | '%'                               { $$ = $1; }
            ;

unop        : '+' expr %prec TOK_POS            {
                    $$ = $1 -> adopt_sym($2, TOK_POS);
                }
            | '-' expr %prec TOK_NEG            {
                    $$ = $1 -> adopt_sym($2, TOK_NEG);
                }
            | TOK_NOT expr                      {
                    $$ = $1 -> adopt($2);
                }
            ;

// We have to have TOK_NEW in here to set symbol
allocation  : TOK_NEW TOK_IDENT                 {
                    $2 -> adopt_sym(nullptr, TOK_TYPEID);
                    $$ = $1 -> adopt($2);
                }
            | TOK_NEW TOK_STRING '(' expr ')'   {
                    $$ = $1 -> adopt_sym($4, TOK_NEWSTR);
                    destroy($2, $3, $5);
                }
            | TOK_NEW basetype '[' expr ']'     {
                    $$ = $1 -> adopt_sym($2, TOK_NEWARRAY, $4);
                    destroy($3, $5);
                }
            ;

// Clean way with left recursion
callexprs   : TOK_IDENT '(' expr                {
                    $$ = $2 -> adopt_sym($1, TOK_CALL, $3);
                }
            | callexprs ',' expr                {
                    $$ = $1 -> adopt($3);
                    destroy($2);
                }
            ;

call        : TOK_IDENT '(' ')'                 {
                    $$ = $2 -> adopt_sym($1, TOK_CALL);
                    destroy($3);
                }
            | callexprs ')'                     {
                    $$ = $1;
                    destroy($2);
                }
            ;

variable    : TOK_IDENT                         { $$ = $1; }
            | expr '[' expr ']'                 {
                    $$ = $2 -> adopt_sym($1, TOK_INDEX, $3);
                    destroy($4);
                }

            ;

constant    : TOK_INTCON                        { $$ = $1; }
            | TOK_CHARCON                       { $$ = $1; }
            | TOK_STRINGCON                     { $$ = $1; }
            | TOK_NULL                          { $$ = $1; }
            ;

%%

const char *parser::get_tname (int symbol) {
   return yytname [YYTRANSLATE (symbol)];
}


bool is_defined_token (int symbol) {
   return YYTRANSLATE (symbol) > YYUNDEFTOK;
}

