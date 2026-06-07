/*
 * Sweetspot -- an OSI layer 3 network access controller.
 * Written by Ilya Etingof <ilya@glas.net>, 2006.
 *
 * Partially based on http://chillispot.org.
 * Copyright (C) 2003, 2004, 2005 Mondru AB.
 * The contents of this file may be used under the terms of the GNU
 * General Public License Version 2, provided that the above copyright
 * notice and this permission notice is included in all copies or
 * substantial portions of the software.
 */

/* Scanner for Sweetspot packet filter parser */

%{
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "log.h"
#include "filter_parser.h"
extern YYSTYPE yylval;
void llerror(char *);
%}

%option noyywrap
%x comment

%%

<comment>\n      { 
  BEGIN(INITIAL);
}
<comment><<EOF>> {
  BEGIN(INITIAL);
}
#                  { 
  BEGIN(comment);
}
<comment>.

[ \t\n]+

block        return SW_YY_BLOCK;
pass         return SW_YY_PASS;
dnat         return SW_YY_DNAT;
shape        return SW_YY_SHAPE;
rate         return SW_YY_RATE;
flags        return SW_YY_FLAGS;
redir        return SW_YY_REDIR;
in           return SW_YY_IN;
out          return SW_YY_OUT;
proto        return SW_YY_PROTO;
from         return SW_YY_FROM;
to           return SW_YY_TO;
port         return SW_YY_PORT;
eq           return SW_YY_EQ;
ne           return SW_YY_NE;
lt           return SW_YY_LT;
le           return SW_YY_LE;
gt           return SW_YY_GT;
ge           return SW_YY_GE;
mask         return SW_YY_MASK;
&            return SW_YY_ERROR;

\:           return ':';
\/           return '/';
\,           return ',';

[[:digit:]]*       {
  yylval.number = atol(yytext); return NUMBER; 
}

[[:alnum:]\.]* {
  yylval.string = strdup(yytext); return STRING; 
}

<<EOF>> {
  yyterminate();
}

<*>. {
  llerror("unexpected character");
  return SW_YY_ERROR;
}

%%

void llerror(char *msg) {
  sw_log("%s:%d lexer error at \"%s\": %s", __FILE__, __LINE__, yytext, msg);
}
