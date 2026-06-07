/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_FILTER_PARSER_H_INCLUDED
# define YY_YY_FILTER_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    STRING = 258,                  /* STRING  */
    NUMBER = 259,                  /* NUMBER  */
    SW_YY_BLOCK = 260,             /* SW_YY_BLOCK  */
    SW_YY_PASS = 261,              /* SW_YY_PASS  */
    SW_YY_DNAT = 262,              /* SW_YY_DNAT  */
    SW_YY_SHAPE = 263,             /* SW_YY_SHAPE  */
    SW_YY_RATE = 264,              /* SW_YY_RATE  */
    SW_YY_IN = 265,                /* SW_YY_IN  */
    SW_YY_OUT = 266,               /* SW_YY_OUT  */
    SW_YY_FROM = 267,              /* SW_YY_FROM  */
    SW_YY_TO = 268,                /* SW_YY_TO  */
    SW_YY_PORT = 269,              /* SW_YY_PORT  */
    SW_YY_EQ = 270,                /* SW_YY_EQ  */
    SW_YY_NE = 271,                /* SW_YY_NE  */
    SW_YY_LT = 272,                /* SW_YY_LT  */
    SW_YY_LE = 273,                /* SW_YY_LE  */
    SW_YY_GT = 274,                /* SW_YY_GT  */
    SW_YY_GE = 275,                /* SW_YY_GE  */
    SW_YY_MASK = 276,              /* SW_YY_MASK  */
    SW_YY_PROTO = 277,             /* SW_YY_PROTO  */
    SW_YY_FLAGS = 278,             /* SW_YY_FLAGS  */
    SW_YY_REDIR = 279,             /* SW_YY_REDIR  */
    SW_YY_ERROR = 280              /* SW_YY_ERROR  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define STRING 258
#define NUMBER 259
#define SW_YY_BLOCK 260
#define SW_YY_PASS 261
#define SW_YY_DNAT 262
#define SW_YY_SHAPE 263
#define SW_YY_RATE 264
#define SW_YY_IN 265
#define SW_YY_OUT 266
#define SW_YY_FROM 267
#define SW_YY_TO 268
#define SW_YY_PORT 269
#define SW_YY_EQ 270
#define SW_YY_NE 271
#define SW_YY_LT 272
#define SW_YY_LE 273
#define SW_YY_GT 274
#define SW_YY_GE 275
#define SW_YY_MASK 276
#define SW_YY_PROTO 277
#define SW_YY_FLAGS 278
#define SW_YY_REDIR 279
#define SW_YY_ERROR 280

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 68 "filter_parser.y"

  char *string;
  int number;

#line 122 "filter_parser.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_FILTER_PARSER_H_INCLUDED  */
