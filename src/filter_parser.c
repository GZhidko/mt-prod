/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 35 "filter_parser.y"

  #ifdef HAVE_CONFIG_H
  #include "config.h"
  #endif
  #include <stdio.h>
  #include <string.h>
  #include <netdb.h>
  #include <errno.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include "filter.h"
  #include "pretty.h"
  #include "log.h"
  int yylex (void);
  void yyerror (char const *);
  typedef struct yy_buffer_state * YY_BUFFER_STATE;

  YY_BUFFER_STATE yy_scan_string (const char * yystr );
  void yy_delete_buffer (YY_BUFFER_STATE  b );

#define YYERROR_VERBOSE 0  /* Compatibility hack for old Bison */

#define YYDEBUG 1

  static sw_filter_rule_t *ruleset_in, *ruleset_out, *rule;
  static sw_filter_target_t *target = NULL;
  static char tcp_flags;

#line 101 "filter_parser.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
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

#line 209 "filter_parser.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_FILTER_PARSER_H_INCLUDED  */
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_STRING = 3,                     /* STRING  */
  YYSYMBOL_NUMBER = 4,                     /* NUMBER  */
  YYSYMBOL_SW_YY_BLOCK = 5,                /* SW_YY_BLOCK  */
  YYSYMBOL_SW_YY_PASS = 6,                 /* SW_YY_PASS  */
  YYSYMBOL_SW_YY_DNAT = 7,                 /* SW_YY_DNAT  */
  YYSYMBOL_SW_YY_SHAPE = 8,                /* SW_YY_SHAPE  */
  YYSYMBOL_SW_YY_RATE = 9,                 /* SW_YY_RATE  */
  YYSYMBOL_SW_YY_IN = 10,                  /* SW_YY_IN  */
  YYSYMBOL_SW_YY_OUT = 11,                 /* SW_YY_OUT  */
  YYSYMBOL_SW_YY_FROM = 12,                /* SW_YY_FROM  */
  YYSYMBOL_SW_YY_TO = 13,                  /* SW_YY_TO  */
  YYSYMBOL_SW_YY_PORT = 14,                /* SW_YY_PORT  */
  YYSYMBOL_SW_YY_EQ = 15,                  /* SW_YY_EQ  */
  YYSYMBOL_SW_YY_NE = 16,                  /* SW_YY_NE  */
  YYSYMBOL_SW_YY_LT = 17,                  /* SW_YY_LT  */
  YYSYMBOL_SW_YY_LE = 18,                  /* SW_YY_LE  */
  YYSYMBOL_SW_YY_GT = 19,                  /* SW_YY_GT  */
  YYSYMBOL_SW_YY_GE = 20,                  /* SW_YY_GE  */
  YYSYMBOL_SW_YY_MASK = 21,                /* SW_YY_MASK  */
  YYSYMBOL_SW_YY_PROTO = 22,               /* SW_YY_PROTO  */
  YYSYMBOL_SW_YY_FLAGS = 23,               /* SW_YY_FLAGS  */
  YYSYMBOL_SW_YY_REDIR = 24,               /* SW_YY_REDIR  */
  YYSYMBOL_SW_YY_ERROR = 25,               /* SW_YY_ERROR  */
  YYSYMBOL_26_ = 26,                       /* '/'  */
  YYSYMBOL_27_ = 27,                       /* ':'  */
  YYSYMBOL_28_ = 28,                       /* ','  */
  YYSYMBOL_YYACCEPT = 29,                  /* $accept  */
  YYSYMBOL_ruleset = 30,                   /* ruleset  */
  YYSYMBOL_31_1 = 31,                      /* $@1  */
  YYSYMBOL_rule = 32,                      /* rule  */
  YYSYMBOL_inrule = 33,                    /* inrule  */
  YYSYMBOL_outrule = 34,                   /* outrule  */
  YYSYMBOL_markin = 35,                    /* markin  */
  YYSYMBOL_markout = 36,                   /* markout  */
  YYSYMBOL_rulemain = 37,                  /* rulemain  */
  YYSYMBOL_action = 38,                    /* action  */
  YYSYMBOL_proto = 39,                     /* proto  */
  YYSYMBOL_srcdst = 40,                    /* srcdst  */
  YYSYMBOL_from = 41,                      /* from  */
  YYSYMBOL_42_2 = 42,                      /* $@2  */
  YYSYMBOL_to = 43,                        /* to  */
  YYSYMBOL_44_3 = 44,                      /* $@3  */
  YYSYMBOL_flags = 45,                     /* flags  */
  YYSYMBOL_flagset = 46,                   /* flagset  */
  YYSYMBOL_47_4 = 47,                      /* $@4  */
  YYSYMBOL_flagmask = 48,                  /* flagmask  */
  YYSYMBOL_49_5 = 49,                      /* $@5  */
  YYSYMBOL_flag = 50,                      /* flag  */
  YYSYMBOL_redir = 51,                     /* redir  */
  YYSYMBOL_rate = 52,                      /* rate  */
  YYSYMBOL_object = 53,                    /* object  */
  YYSYMBOL_port = 54,                      /* port  */
  YYSYMBOL_portcomp = 55,                  /* portcomp  */
  YYSYMBOL_portrange = 56,                 /* portrange  */
  YYSYMBOL_portnums = 57,                  /* portnums  */
  YYSYMBOL_singleport = 58,                /* singleport  */
  YYSYMBOL_portnum = 59,                   /* portnum  */
  YYSYMBOL_compare_eq_ne = 60,             /* compare_eq_ne  */
  YYSYMBOL_compare_other = 61,             /* compare_other  */
  YYSYMBOL_network = 62,                   /* network  */
  YYSYMBOL_hostname = 63,                  /* hostname  */
  YYSYMBOL_ipv4mask = 64                   /* ipv4mask  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   75

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  29
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  36
/* YYNRULES -- Number of rules.  */
#define YYNRULES  71
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  96

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   280


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    28,     2,     2,    26,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    27,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,    88,    88,    89,    89,   100,   114,   130,   131,   132,
     133,   134,   135,   136,   137,   140,   141,   142,   143,   146,
     149,   152,   153,   154,   155,   156,   159,   160,   161,   162,
     168,   171,   182,   183,   184,   187,   187,   190,   190,   193,
     194,   197,   197,   207,   207,   212,   245,   251,   257,   258,
     259,   262,   263,   267,   276,   286,   298,   299,   302,   312,
     318,   327,   328,   331,   332,   333,   334,   337,   341,   347,
     353,   357
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "STRING", "NUMBER",
  "SW_YY_BLOCK", "SW_YY_PASS", "SW_YY_DNAT", "SW_YY_SHAPE", "SW_YY_RATE",
  "SW_YY_IN", "SW_YY_OUT", "SW_YY_FROM", "SW_YY_TO", "SW_YY_PORT",
  "SW_YY_EQ", "SW_YY_NE", "SW_YY_LT", "SW_YY_LE", "SW_YY_GT", "SW_YY_GE",
  "SW_YY_MASK", "SW_YY_PROTO", "SW_YY_FLAGS", "SW_YY_REDIR", "SW_YY_ERROR",
  "'/'", "':'", "','", "$accept", "ruleset", "$@1", "rule", "inrule",
  "outrule", "markin", "markout", "rulemain", "action", "proto", "srcdst",
  "from", "$@2", "to", "$@3", "flags", "flagset", "$@4", "flagmask", "$@5",
  "flag", "redir", "rate", "object", "port", "portcomp", "portrange",
  "portnums", "singleport", "portnum", "compare_eq_ne", "compare_other",
  "network", "hostname", "ipv4mask", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-69)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-3)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int8 yypact[] =
{
       5,    28,    39,   -69,   -69,   -69,   -69,     2,     5,   -69,
     -69,    30,    38,   -69,   -69,   -69,    -6,    17,   -69,    46,
     -69,   -69,    45,    19,    -5,    -3,   -69,    40,   -69,    42,
     -69,    42,   -69,   -69,    11,    11,   -69,   -69,    49,    42,
     -69,   -69,    31,   -69,   -69,   -69,   -69,   -69,    18,   -69,
     -69,   -69,   -69,    41,    -9,   -69,    43,   -69,    32,    53,
     -69,   -69,   -69,   -69,   -69,   -69,   -69,   -69,    33,    55,
      55,   -69,    58,    59,   -69,    55,   -69,   -69,   -69,    55,
      34,   -69,    36,   -69,   -69,   -69,   -69,   -69,   -69,    53,
     -69,    55,    55,   -69,   -69,   -69
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       3,     0,     0,     1,    26,    27,    28,     0,     3,     5,
       6,     0,     0,     4,    19,    20,    14,    18,    29,     0,
      35,    37,     0,     0,    10,    24,    25,    33,    34,    12,
      13,    16,    17,    47,     0,     0,    31,    30,     0,     8,
       9,    41,    22,    23,    32,    11,    15,    69,     0,    36,
      50,    51,    52,    49,    67,    38,     0,     7,    39,     0,
      21,    60,    61,    62,    63,    64,    65,    66,     0,     0,
       0,    48,     0,     0,    68,     0,    43,    45,    42,     0,
      53,    56,    59,    54,    59,    70,    71,    46,    40,     0,
      55,     0,     0,    44,    58,    57
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -69,    57,   -69,   -69,   -69,   -69,   -69,   -69,    50,   -69,
     -69,    44,   -69,   -69,    47,   -69,    24,   -69,   -69,   -69,
     -69,   -21,    48,   -16,    35,    20,   -69,   -69,   -69,   -68,
     -48,   -69,   -69,   -69,    37,   -69
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,     1,     2,     8,     9,    10,    16,    17,    24,    11,
      25,    26,    27,    34,    28,    35,    43,    58,    59,    88,
      89,    78,    29,    30,    49,    50,    51,    52,    80,    81,
      84,    69,    70,    53,    54,    74
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      68,    32,    83,    19,    19,    -2,    20,    21,    40,    20,
      21,    12,    72,    45,    47,    46,    22,    73,    23,    23,
      41,    82,    61,    57,    95,    48,    19,    87,     3,    20,
      21,    90,    38,    62,    63,    64,    65,    66,    67,    22,
      14,    15,    18,    94,     4,     5,     6,     7,    36,    37,
      33,    19,    47,    21,    41,    48,    77,    75,    76,    61,
      79,    85,    91,    86,    92,    13,    60,    31,    93,    42,
      55,     0,    39,    71,    44,    56
};

static const yytype_int8 yycheck[] =
{
      48,    17,    70,     9,     9,     0,    12,    13,    24,    12,
      13,     9,    21,    29,     3,    31,    22,    26,    24,    24,
      23,    69,     4,    39,    92,    14,     9,    75,     0,    12,
      13,    79,    13,    15,    16,    17,    18,    19,    20,    22,
      10,    11,     4,    91,     5,     6,     7,     8,     3,     4,
       4,     9,     3,    13,    23,    14,     3,    14,    26,     4,
      27,     3,    28,     4,    28,     8,    42,    17,    89,    25,
      35,    -1,    24,    53,    27,    38
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    30,    31,     0,     5,     6,     7,     8,    32,    33,
      34,    38,     9,    30,    10,    11,    35,    36,     4,     9,
      12,    13,    22,    24,    37,    39,    40,    41,    43,    51,
      52,    37,    52,     4,    42,    44,     3,     4,    13,    51,
      52,    23,    40,    45,    43,    52,    52,     3,    14,    53,
      54,    55,    56,    62,    63,    53,    63,    52,    46,    47,
      45,     4,    15,    16,    17,    18,    19,    20,    59,    60,
      61,    54,    21,    26,    64,    14,    26,     3,    50,    27,
      57,    58,    59,    58,    59,     3,     4,    59,    48,    49,
      59,    28,    28,    50,    59,    58
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    29,    30,    31,    30,    32,    32,    33,    33,    33,
      33,    33,    33,    33,    33,    34,    34,    34,    34,    35,
      36,    37,    37,    37,    37,    37,    38,    38,    38,    38,
      39,    39,    40,    40,    40,    42,    41,    44,    43,    45,
      45,    47,    46,    49,    48,    50,    51,    52,    53,    53,
      53,    54,    54,    55,    55,    56,    57,    57,    57,    58,
      59,    60,    60,    61,    61,    61,    61,    62,    62,    63,
      64,    64
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     0,     3,     1,     1,     5,     4,     4,
       3,     4,     3,     3,     2,     4,     3,     3,     2,     1,
       1,     3,     2,     2,     1,     1,     1,     1,     1,     3,
       2,     2,     2,     1,     1,     0,     3,     0,     3,     2,
       4,     0,     2,     0,     2,     1,     5,     2,     2,     1,
       1,     1,     1,     3,     3,     4,     1,     3,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     1,
       2,     2
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 3: /* $@1: %empty  */
#line 89 "filter_parser.y"
              {
  rule = (sw_filter_rule_t *)malloc(sizeof(sw_filter_rule_t));
  if (!rule) {
    sw_log("%s:%d malloc failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }
  target = NULL;
  memset(rule, 0, sizeof(sw_filter_rule_t));
}
#line 1326 "filter_parser.c"
    break;

  case 5: /* rule: inrule  */
#line 100 "filter_parser.y"
                      {
  sw_filter_rule_t *prev = ruleset_in;
  if (prev) {
    while(prev->next) prev = prev->next;
    rule->num = prev->num+1;
    prev->next = rule;
  } else {
    if (sw_debug_flags & SW_DEBUG_FILTER)
      sw_log("%s:%d inbound ruleset created", __FILE__, __LINE__);
    ruleset_in = rule;
  }
  if (sw_debug_flags & SW_DEBUG_FILTER)
    sw_log("%s:%d inbound rule #%d parsed", __FILE__, __LINE__, rule->num);
}
#line 1345 "filter_parser.c"
    break;

  case 6: /* rule: outrule  */
#line 114 "filter_parser.y"
                       {
  sw_filter_rule_t *prev = ruleset_out;
  if (prev) {
    while(prev->next) prev = prev->next;
    rule->num = prev->num+1;
    prev->next = rule;
  } else {
    if (sw_debug_flags & SW_DEBUG_FILTER)
      sw_log("%s:%d outbound ruleset created", __FILE__, __LINE__);
    ruleset_out = rule;
  }
  if (sw_debug_flags & SW_DEBUG_FILTER)
    sw_log("%s:%d outbound rule #%d parsed", __FILE__, __LINE__, rule->num);
}
#line 1364 "filter_parser.c"
    break;

  case 19: /* markin: SW_YY_IN  */
#line 146 "filter_parser.y"
                        { rule->dir = SW_FILTER_DIR_IN; }
#line 1370 "filter_parser.c"
    break;

  case 20: /* markout: SW_YY_OUT  */
#line 149 "filter_parser.y"
                         { rule->dir = SW_FILTER_DIR_OUT; }
#line 1376 "filter_parser.c"
    break;

  case 26: /* action: SW_YY_BLOCK  */
#line 159 "filter_parser.y"
                           { rule->action = SW_FILTER_ACTION_BLOCK; }
#line 1382 "filter_parser.c"
    break;

  case 27: /* action: SW_YY_PASS  */
#line 160 "filter_parser.y"
                          { rule->action = SW_FILTER_ACTION_PASS; }
#line 1388 "filter_parser.c"
    break;

  case 28: /* action: SW_YY_DNAT  */
#line 161 "filter_parser.y"
                          { rule->action = SW_FILTER_ACTION_DNAT; }
#line 1394 "filter_parser.c"
    break;

  case 29: /* action: SW_YY_SHAPE SW_YY_RATE NUMBER  */
#line 162 "filter_parser.y"
                                             { 
                 rule->action = SW_FILTER_ACTION_SHAPE;
                 rule->shape.rate = (yyvsp[0].number);
                }
#line 1403 "filter_parser.c"
    break;

  case 30: /* proto: SW_YY_PROTO NUMBER  */
#line 168 "filter_parser.y"
                                  { 
               rule->proto = (yyvsp[0].number);
            }
#line 1411 "filter_parser.c"
    break;

  case 31: /* proto: SW_YY_PROTO STRING  */
#line 171 "filter_parser.y"
                                  {
               struct protoent *pe;
               pe = getprotobyname((yyvsp[0].string));
               if (!pe) {
                 sw_log("%s:%d unknown protocol %s", __FILE__, __LINE__, (yyvsp[0].string));
               }
               rule->proto = pe->p_proto;
               free((yyvsp[0].string));
            }
#line 1425 "filter_parser.c"
    break;

  case 35: /* $@2: %empty  */
#line 187 "filter_parser.y"
                          { target = &rule->src; }
#line 1431 "filter_parser.c"
    break;

  case 37: /* $@3: %empty  */
#line 190 "filter_parser.y"
                        { target = &rule->dst; }
#line 1437 "filter_parser.c"
    break;

  case 41: /* $@4: %empty  */
#line 197 "filter_parser.y"
               { tcp_flags = 0; }
#line 1443 "filter_parser.c"
    break;

  case 42: /* flagset: $@4 flag  */
#line 197 "filter_parser.y"
                                       {
  if (!rule->proto) {
    rule->proto = IPPROTO_TCP;
    sw_log("%s:%d assuming protocol %s at rule %d",
           __FILE__, __LINE__, sw_pretty_proto(rule->proto), rule->num);
  }
  rule->flags.set = tcp_flags;
}
#line 1456 "filter_parser.c"
    break;

  case 43: /* $@5: %empty  */
#line 207 "filter_parser.y"
               { tcp_flags = 0; }
#line 1462 "filter_parser.c"
    break;

  case 44: /* flagmask: $@5 flag  */
#line 207 "filter_parser.y"
                                       {
  rule->flags.mask = tcp_flags;
}
#line 1470 "filter_parser.c"
    break;

  case 45: /* flag: STRING  */
#line 212 "filter_parser.y"
                      {
  char *p = (yyvsp[0].string);
  while(*p) {
    switch(*p++) {
    case 'F':
      tcp_flags |= SW_FILTER_FLAGS_FIN;
      break;
    case 'S':
      tcp_flags |= SW_FILTER_FLAGS_SYN;
      break;
    case 'R':
      tcp_flags |= SW_FILTER_FLAGS_RST;
      break;
    case 'P':
      tcp_flags |= SW_FILTER_FLAGS_PUSH;
      break;
    case 'A':
      tcp_flags |= SW_FILTER_FLAGS_ACK;
      break;
    case 'U':
      tcp_flags |= SW_FILTER_FLAGS_URG;
      break;
    default:
      sw_log("%s:%d invalid tcp flag %c at rule %d",
             __FILE__, __LINE__, *p, rule->num);
      free((yyvsp[0].string));
      return -1;
    }
  }
  free((yyvsp[0].string));
}
#line 1506 "filter_parser.c"
    break;

  case 46: /* redir: SW_YY_REDIR SW_YY_TO hostname SW_YY_PORT portnum  */
#line 245 "filter_parser.y"
                                                                {
  rule->dnat.ip = (yyvsp[-2].number);
  rule->dnat.port = (yyvsp[0].number);
}
#line 1515 "filter_parser.c"
    break;

  case 47: /* rate: SW_YY_RATE NUMBER  */
#line 251 "filter_parser.y"
                                {
  rule->action |= SW_FILTER_ACTION_SHAPE;
  rule->shape.rate = (yyvsp[0].number);
}
#line 1524 "filter_parser.c"
    break;

  case 53: /* portcomp: SW_YY_PORT compare_eq_ne portnums  */
#line 267 "filter_parser.y"
                                                 {
  if (!rule->proto) {
    sw_log("%s:%d ambiguous protocol at rule %d", 
           __FILE__, __LINE__, rule->num);
    // Assuming $3 is the semantic value of portnums

  }
  target->op = (yyvsp[-1].number);
}
#line 1538 "filter_parser.c"
    break;

  case 54: /* portcomp: SW_YY_PORT compare_other singleport  */
#line 276 "filter_parser.y"
                                                  {
  if (!rule->proto) {
    sw_log("%s:%d ambiguous protocol at rule %d",
           __FILE__, __LINE__, rule->num);
    return -1;
  }
  target->op = (yyvsp[-1].number);
}
#line 1551 "filter_parser.c"
    break;

  case 55: /* portrange: SW_YY_PORT portnum ':' portnum  */
#line 286 "filter_parser.y"
                                              {
  if (!rule->proto) {
    sw_log("%s:%d ambiguous protocol at rule %d",
           __FILE__, __LINE__, rule->num);
    return -1;
  }
  target->op = SW_FILTER_TARGET_RG;
  target->port_lo = (yyvsp[-2].number);
  target->port_hi = (yyvsp[0].number);
}
#line 1566 "filter_parser.c"
    break;

  case 57: /* portnums: portnum ',' singleport  */
#line 299 "filter_parser.y"
                                      {
  target->ports[target->port_cnt++] = (uint16_t) (yyvsp[-2].number);
  }
#line 1574 "filter_parser.c"
    break;

  case 58: /* portnums: portnums ',' portnum  */
#line 302 "filter_parser.y"
                                    {
  if (target->port_cnt < SW_FILTER_PORTS_MAX) {
    target->ports[target->port_cnt++] = (uint16_t) (yyvsp[0].number); 
  } else {
    sw_log("%s:%d rule %d port set too large (max %d)",
            __FILE__, __LINE__, rule->num, SW_FILTER_PORTS_MAX);
  }
}
#line 1587 "filter_parser.c"
    break;

  case 59: /* singleport: portnum  */
#line 312 "filter_parser.y"
                          {
  target->port_cnt = 1;
  target->ports[0] = (uint16_t) (yyvsp[0].number);
}
#line 1596 "filter_parser.c"
    break;

  case 60: /* portnum: NUMBER  */
#line 318 "filter_parser.y"
                      {
  if ((yyvsp[0].number) > 65535) {       /* Unsigned */
    sw_log("%s:%d invalid port number %d at rule %d", 
           __FILE__, __LINE__, (yyvsp[0].number), rule->num);
    return -1;
  }
  (yyval.number) = (yyvsp[0].number);
}
#line 1609 "filter_parser.c"
    break;

  case 61: /* compare_eq_ne: SW_YY_EQ  */
#line 327 "filter_parser.y"
                          { (yyval.number) = SW_FILTER_TARGET_EQ; }
#line 1615 "filter_parser.c"
    break;

  case 62: /* compare_eq_ne: SW_YY_NE  */
#line 328 "filter_parser.y"
                          { (yyval.number) = SW_FILTER_TARGET_NE; }
#line 1621 "filter_parser.c"
    break;

  case 63: /* compare_other: SW_YY_LT  */
#line 331 "filter_parser.y"
                          { (yyval.number) = SW_FILTER_TARGET_LT; }
#line 1627 "filter_parser.c"
    break;

  case 64: /* compare_other: SW_YY_LE  */
#line 332 "filter_parser.y"
                          { (yyval.number) = SW_FILTER_TARGET_LE; }
#line 1633 "filter_parser.c"
    break;

  case 65: /* compare_other: SW_YY_GT  */
#line 333 "filter_parser.y"
                          { (yyval.number) = SW_FILTER_TARGET_GT; }
#line 1639 "filter_parser.c"
    break;

  case 66: /* compare_other: SW_YY_GE  */
#line 334 "filter_parser.y"
                          { (yyval.number) = SW_FILTER_TARGET_GE; }
#line 1645 "filter_parser.c"
    break;

  case 67: /* network: hostname  */
#line 337 "filter_parser.y"
                        {
              target->ip = (yyvsp[0].number);
              target->mask = 0xffffffffl; 
}
#line 1654 "filter_parser.c"
    break;

  case 68: /* network: hostname ipv4mask  */
#line 341 "filter_parser.y"
                                 {
  target->ip = (yyvsp[-1].number);
  target->mask = (yyvsp[0].number);
}
#line 1663 "filter_parser.c"
    break;

  case 69: /* hostname: STRING  */
#line 347 "filter_parser.y"
                      { 
              (yyval.number) = ntohl(inet_addr((yyvsp[0].string)));
              free((yyvsp[0].string));
}
#line 1672 "filter_parser.c"
    break;

  case 70: /* ipv4mask: SW_YY_MASK STRING  */
#line 353 "filter_parser.y"
                                 { 
              (yyval.number) = ntohl(inet_addr((yyvsp[0].string)));
              free((yyvsp[0].string));
}
#line 1681 "filter_parser.c"
    break;

  case 71: /* ipv4mask: '/' NUMBER  */
#line 357 "filter_parser.y"
                          {
              if ((yyvsp[0].number) < 0 || (yyvsp[0].number) > 32) {
                sw_log("%s:%d invalid CIDR %d at rule %d",
                       __FILE__, __LINE__, (yyvsp[0].number), rule->num);
                return -1;
              }
              (yyval.number) = 0xffffffff << (32-(yyvsp[0].number));
}
#line 1694 "filter_parser.c"
    break;


#line 1698 "filter_parser.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 368 "filter_parser.y"


void yyerror(char const *msg) {
  sw_log("%s:%d %s", __FILE__, __LINE__, msg);
}

int sw_filter_parse_ruleset(sw_filter_rule_t **rs_in, 
                            sw_filter_rule_t **rs_out,
                            char *text) {
  YY_BUFFER_STATE b = 0;

  if (sw_debug_flags & SW_DEBUG_FILTER)
    sw_log("%s:%d scanning ruleset", __FILE__, __LINE__);

  b = yy_scan_string(text);
  if (!b) {
    sw_log("%s:%d yy_scan_buffer() failed", __FILE__, __LINE__);
    return -1;
  }

  yydebug = 0; /* raise this to see AST build */

  if (sw_debug_flags & SW_DEBUG_FILTER)
    sw_log("%s:%d parsing ruleset", __FILE__, __LINE__);

  ruleset_in = ruleset_out = NULL;

  if (yyparse()) {
    yy_delete_buffer(b);
    return -1;
  }

  yy_delete_buffer(b);

  if (sw_debug_flags & SW_DEBUG_FILTER)
    sw_log("%s:%d ruleset finished", __FILE__, __LINE__);

  *rs_in = ruleset_in;
  *rs_out = ruleset_out;

  return 0;
}
