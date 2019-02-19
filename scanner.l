/* $Id: lexer.l,v 1.7 2016-10-27 19:45:59-07 - - $ */

%{

#include "lyutils.h"

#define YY_USER_ACTION  { lexer::advance(); }

%}

%option 8bit
%option debug
%option nodefault
%option noinput
%option nounput
%option noyywrap
%option warn
/*%option verbose*/

LETTER          [A-Za-z_]
DECIMALDIGIT    [0-9]
OCTALDIGIT      [0-7]
HEXDIGIT        ({DECIMALDIGIT}|[A-Fa-f])
MANTISSA        ({DECIMALDIGIT}+\.?{DECIMALDIGIT}*|\.{DECIMALDIGIT}+)
EXPONENT        ([Ee][+-]?{DECIMALDIGIT}+)
HEXNUMBER       (0[Xx]{HEXDIGIT}+)
OCTALNUMBER     (0{OCTALDIGIT}+)
DECIMALNUMBER   ({MANTISSA}{EXPONENT}?)
NOTNUMBER       ({MANTISSA}[Ee][+-]?)

CHAR            ([^\\'\n]|\\[\\'"0nt])
NOTCHAR         (\\[^\\'"0nt])
STRING          (([^\\"\n]|\\[\\'"0nt])*)
CHARCONSTANT    ('{CHAR}')
STRINGCONSTANT  (\"{STRING}\")

INTCONSTANT     ({HEXNUMBER}|{OCTALNUMBER}|{DECIMALNUMBER})

CHARORNOT       ({CHAR}|{NOTCHAR})
UNCLOSEDCHAR    ('({CHARORNOT})*)
WEIRDESCAPE     ({CHAR}*{NOTCHAR}({CHARORNOT})*)
WEIRDESCAPECHAR ('{WEIRDESCAPE}')
LONGCHAR        ('{CHAR}{2,}')

UNCLOSEDSTR     (\"({CHAR}|{NOTCHAR})*)

IDENT           ({LETTER}({LETTER}|{DECIMALDIGIT})*)
NOTIDENT        ({DECIMALDIGIT}({LETTER}|{DECIMALDIGIT})*)
NOTCHARCONSTANT (''|{LONGCHAR}|{WEIRDESCAPECHAR}|{UNCLOSEDCHAR})
NOTSTRINGCONSTANT   (\"{STRING}|"{WEIRDESCAPE}")

%%

"#".*           { lexer::include(); }
[ \t]+          { }
[\r\n|\n]       { lexer::newline(); }

"return"        { return yylval_token(TOK_RETURN); }
"string"        { return yylval_token(TOK_STRING); }
"struct"        { return yylval_token(TOK_STRUCT); }
"while"         { return yylval_token(TOK_WHILE); }
"else"          { return yylval_token(TOK_ELSE); }
"null"          { return yylval_token(TOK_NULL); }
"void"          { return yylval_token(TOK_VOID); }
"int"           { return yylval_token(TOK_INT); }
"new"           { return yylval_token(TOK_NEW); }
"not"           { return yylval_token(TOK_NOT); }
"if"            { return yylval_token(TOK_IF); }

"=="            { return yylval_token(TOK_EQ); }
"!="            { return yylval_token(TOK_NE); }
"<"             { return yylval_token(TOK_LT); }
">"             { return yylval_token(TOK_GT); }
"<="            { return yylval_token(TOK_LE); }
">="            { return yylval_token(TOK_GE); }
"->"            { return yylval_token(TOK_ARROW); }
"[]"            { return yylval_token(TOK_ARRAY); }

"+"             { return yylval_token('+'); }
"-"             { return yylval_token('-'); }
"*"             { return yylval_token('*'); }
"/"             { return yylval_token('/'); }
"%"             { return yylval_token('%'); }
"="             { return yylval_token('='); }
","             { return yylval_token(','); }
";"             { return yylval_token(';'); }
"("             { return yylval_token('('); }
")"             { return yylval_token(')'); }
"["             { return yylval_token('['); }
"]"             { return yylval_token(']'); }
"{"             { return yylval_token('{'); }
"}"             { return yylval_token('}'); }

{IDENT}         { return yylval_token(TOK_IDENT); }
{INTCONSTANT}   { return yylval_token(TOK_INTCON); }
{CHARCONSTANT}  { return yylval_token(TOK_CHARCON); }
{STRINGCONSTANT} { return yylval_token (TOK_STRINGCON); }

{NOTIDENT}      { lexer::badtoken(yytext);
                  return yylval_token (TOK_IDENT); }
{NOTCHARCONSTANT}    { lexer::badtoken(yytext);
                  return yylval_token (TOK_CHARCON); }
{NOTSTRINGCONSTANT}  { lexer::badtoken(yytext);
                  return yylval_token (TOK_STRINGCON); }

.               { lexer::badchar (*yytext); }
%%
