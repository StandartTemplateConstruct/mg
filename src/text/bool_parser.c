
/*  A Bison parser, made from bool_parser.y
 by  GNU Bison version 1.27
  */

#define YYBISON 1  /* Identify Bison output.  */

#define	TERM	257

#line 24 "bool_parser.y"

 
#include "sysfuncs.h"

#include "messages.h"
 
#include "memlib.h"
#include "words.h"
#include "stemmer.h"
#include "term_lists.h"
#include "bool_tree.h"
/* [RPAP - Jan 97: Stem Index Change] */
#include "backend.h"     /* for stemmed_dict def */
#include "stem_search.h"

#include "query_term_list.h"  /* [RPAP - Feb 97: Term Frequency] */

/* --- routines --- */
static int query_lex();
static int yyerror(char *);
#define yylex() query_lex(&ch_buf, end_buf)
 
/* --- module variables --- */
static char *ch_buf; /* ptr to the character query line buffer */
static char *end_buf; /* ptr to the last character of the line buffer */
static bool_tree_node *tree_base = NULL;
static TermList **term_list;
static int stemmer_num;
static int stem_method;
/* [RPAP - Jan 97: Stem Index Change] */
stemmed_dict *p__sd;
static int indexed;
/* [RPAP - Feb 97: Term Frequency] */
static QueryTermList **query_term_list;
static int word_num;
static u_long count;
static u_long doc_count;
static u_long invf_ptr;
static u_long invf_len;

#line 66 "bool_parser.y"
typedef union {
  char *text;
  bool_tree_node *node;
} YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		20
#define	YYFLAG		-32768
#define	YYNTBASE	11

#define YYTRANSLATE(x) ((unsigned)(x) <= 257 ? yytranslate[x] : 16)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     8,     2,     2,     2,     2,     9,     2,     4,
     5,     6,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     7,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,    10,     2,     2,     2,     2,     2,     2,
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
     2,     2,     2,     2,     2,     1,     3
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     2,     4,     8,    10,    12,    14,    17,    21,    24,
    26,    30
};

static const short yyrhs[] = {    15,
     0,     3,     0,     4,    15,     5,     0,     6,     0,     7,
     0,    12,     0,     8,    13,     0,    14,     9,    13,     0,
    14,    13,     0,    13,     0,    15,    10,    14,     0,    14,
     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
    76,    80,    81,    82,    83,    86,    87,    90,    91,    92,
    95,    96
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","TERM","'('",
"')'","'*'","'_'","'!'","'&'","'|'","query","term","not","and","or", NULL
};
#endif

static const short yyr1[] = {     0,
    11,    12,    12,    12,    12,    13,    13,    14,    14,    14,
    15,    15
};

static const short yyr2[] = {     0,
     1,     1,     3,     1,     1,     1,     2,     3,     2,     1,
     3,     1
};

static const short yydefact[] = {     0,
     2,     0,     4,     5,     0,     6,    10,    12,     1,     0,
     7,     0,     9,     0,     3,     8,    11,     0,     0,     0
};

static const short yydefgoto[] = {    18,
     6,     7,     8,     9
};

static const short yypact[] = {    10,
-32768,    10,-32768,-32768,    10,-32768,-32768,     2,    -9,    14,
-32768,    10,-32768,    10,-32768,-32768,     2,     4,    15,-32768
};

static const short yypgoto[] = {-32768,
-32768,    -5,   -12,    18
};


#define	YYLAST		24


static const short yytable[] = {    11,
    14,    17,    13,    19,     1,     2,    16,     3,     4,     5,
    12,    13,     1,     2,    20,     3,     4,     5,    15,    10,
     0,     0,     0,    14
};

static const short yycheck[] = {     5,
    10,    14,     8,     0,     3,     4,    12,     6,     7,     8,
     9,    17,     3,     4,     0,     6,     7,     8,     5,     2,
    -1,    -1,    -1,    10
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/lib/bison.simple"
/* This file comes from bison-1.27.  */

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

#ifndef YYSTACK_USE_ALLOCA
#ifdef alloca
#define YYSTACK_USE_ALLOCA
#else /* alloca not defined */
#ifdef __GNUC__
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi) || (defined (__sun) && defined (__i386))
#define YYSTACK_USE_ALLOCA
#include <alloca.h>
#else /* not sparc */
/* We think this test detects Watcom and Microsoft C.  */
/* This used to test MSDOS, but that is a bad idea
   since that symbol is in the user namespace.  */
#if (defined (_MSDOS) || defined (_MSDOS_)) && !defined (__TURBOC__)
#if 0 /* No need for malloc.h, which pollutes the namespace;
	 instead, just don't use alloca.  */
#include <malloc.h>
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
/* I don't know what this was needed for, but it pollutes the namespace.
   So I turned it off.   rms, 2 May 1997.  */
/* #include <malloc.h>  */
 #pragma alloca
#define YYSTACK_USE_ALLOCA
#else /* not MSDOS, or __TURBOC__, or _AIX */
#if 0
#ifdef __hpux /* haible@ilog.fr says this works for HPUX 9.05 and up,
		 and on HPUX 10.  Eventually we can turn this on.  */
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#endif /* __hpux */
#endif
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc */
#endif /* not GNU C */
#endif /* alloca not defined */
#endif /* YYSTACK_USE_ALLOCA not defined */

#ifdef YYSTACK_USE_ALLOCA
#define YYSTACK_ALLOC alloca
#else
#define YYSTACK_ALLOC malloc
#endif

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Define __yy_memcpy.  Note that the size argument
   should be passed with type unsigned int, because that is what the non-GCC
   definitions require.  With GCC, __builtin_memcpy takes an arg
   of type size_t, but it can handle unsigned int.  */

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     unsigned int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, unsigned int count)
{
  register char *t = to;
  register char *f = from;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 216 "/usr/lib/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
#ifdef YYPARSE_PARAM
int yyparse (void *);
#else
int yyparse (void);
#endif
#endif

int
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;
  int yyfree_stacks = 0;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  if (yyfree_stacks)
	    {
	      free (yyss);
	      free (yyvs);
#ifdef YYLSP_NEEDED
	      free (yyls);
#endif
	    }
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
#ifndef YYSTACK_USE_ALLOCA
      yyfree_stacks = 1;
#endif
      yyss = (short *) YYSTACK_ALLOC (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1,
		   size * (unsigned int) sizeof (*yyssp));
      yyvs = (YYSTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1,
		   size * (unsigned int) sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1,
		   size * (unsigned int) sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 1:
#line 76 "bool_parser.y"
{ tree_base = yyvsp[0].node;;
    break;}
case 2:
#line 80 "bool_parser.y"
{ yyval.node = CreateBoolTermNode(term_list, yyvsp[0].text, 1, word_num, count, doc_count, invf_ptr, invf_len, stemmer_num); ;
    break;}
case 3:
#line 81 "bool_parser.y"
{ yyval.node = yyvsp[-1].node; ;
    break;}
case 4:
#line 82 "bool_parser.y"
{ yyval.node = CreateBoolTreeNode(N_all, NULL, NULL); ;
    break;}
case 5:
#line 83 "bool_parser.y"
{ yyval.node = CreateBoolTreeNode(N_none, NULL, NULL); ;
    break;}
case 7:
#line 87 "bool_parser.y"
{ yyval.node = CreateBoolTreeNode(N_not, yyvsp[0].node, NULL); ;
    break;}
case 8:
#line 90 "bool_parser.y"
{ yyval.node = CreateBoolTreeNode(N_and, yyvsp[-2].node, yyvsp[0].node); ;
    break;}
case 9:
#line 91 "bool_parser.y"
{ yyval.node = CreateBoolTreeNode(N_and, yyvsp[-1].node, yyvsp[0].node); ;
    break;}
case 11:
#line 95 "bool_parser.y"
{ yyval.node = CreateBoolTreeNode(N_or, yyvsp[-2].node, yyvsp[0].node); ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 542 "/usr/lib/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;

 yyacceptlab:
  /* YYACCEPT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 0;

 yyabortlab:
  /* YYABORT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 1;
}
#line 99 "bool_parser.y"

 
/* Bison on one mips machine defined "const" to be nothing but
   then did not undef it */
#ifdef const
#undef const
#endif

/**************************************************************************/ 


/* =========================================================================
 * Function: query_lex
 * Description:
 *      Hand written lexical analyser for the parser.
 * Input:
 *      ptr = ptr to a ptr into character query-line buffer
 *      end = ptr to last char in buffer
 * Output:
 *      yylval.text = the token's text
 * Notes:
 *      does NOT produce WILD tokens at the moment
 * ========================================================================= */
 
/* [RPAP - Jan 97: Stem Index Change]
   state mode:
      0 = Read next token
      1 = Output word
      2 = Output '|' or ')'
 */
static int query_lex(char **ptr, const char *end)
{
  char *buf_ptr = *ptr;
  static int mode = 0;
  static int termnum = 0;
  static TermList *Terms = NULL;

  if (mode == 0)
    {
      /* jump over whitespace */
      buf_ptr = skipspace(buf_ptr, end);
 
      if (inaword(buf_ptr, end))
	{
	  static char word[MAXSTEMLEN + 1]; /* [RJM 07/98: Memory Leak] */
	  char *sWord = Xmalloc(MAXSTEMLEN + 1);
	  int stem_to_apply, method_using = -1;

	  PARSE_STEM_WORD(word, buf_ptr, end);

	  /* Extract any parameters */
	  stem_to_apply = stem_method;
	  while (buf_ptr <= end)
	    {
	      int stem_param, param_type;
	      char param[MAXPARAMLEN + 1];

	      param_type = 0;
	      PARSE_OPT_TERM_PARAM (param, param_type, buf_ptr, end);
	      if (!param_type)
		break;

	      if (param_type == STEMPARAM)
		{
		  stem_param = atoi (param);
		  if (errno != ERANGE && indexed && stem_param >= 0 && stem_param <= 3)
		    method_using = stem_to_apply = stem_param;
		}
	    }

	  bcopy ((char *) word, (char *) sWord, *word + 1);
	  stemmer (stem_to_apply, stemmer_num, sWord);

	  if (stem_to_apply == 0 || !indexed || p__sd == NULL)
	    {
	      /* [RPAP - Feb 97: Term Frequency] */
	      word_num = FindWord (p__sd, sWord, &count, &doc_count, &invf_ptr, &invf_len);
	      if (word_num == -1)
		count = doc_count = invf_ptr = invf_len = 0;
	      AddQueryTerm (query_term_list, (u_char *) word, count, method_using);

	      yylval.text = word;
	      *ptr = buf_ptr; /* fix up ptr */
	      Xfree (sWord);
	      return TERM;
	    }
	  else
	    {
	      *ptr = buf_ptr; /* fix up ptr */
	      termnum = 0;
	      ResetTermList (&Terms);
	      if (FindWords (p__sd, (u_char *) sWord, stem_to_apply, &Terms) > 0)
		{
		  /* [RPAP - Feb 97: Term Frequency] */ 
		  int i, freq = 0;
		  for (i = 0; i < Terms->num; i++)
		    freq += Terms->TE[i].WE.count;
		  AddQueryTerm (query_term_list, word, freq, method_using);

		  Xfree (sWord);
		  mode = 1;
		  return '(';
		}
	      else
		{
		  /* Word does not exists - include in tree anyway */
		  Xfree (sWord);

		  /* [RPAP - Feb 97: Term Frequency] */
		  word_num = -1;
		  count = doc_count = invf_ptr = invf_len = 0;
		  AddQueryTerm (query_term_list, (u_char *) word, count, method_using);

		  yylval.text = word;
		  return TERM;
		}
	    }
	}
      else /* NON-WORD */
	{
	  if (*buf_ptr == '\0')
	    {
	      /* return null-char if it is one */
	      *ptr = buf_ptr; /* fix up ptr */
	      return 0;
	    }
	  else
	    {
	      /* return 1st char, and delete from buffer */
	      char c = *buf_ptr++;
	      *ptr = buf_ptr; /* fix up ptr */
	      return c;
	    }
	}
    }
  else if (mode == 1)
    {
      yylval.text = Terms->TE[termnum].Word;
      
      /* [RPAP - Feb 97: Term Frequency] */
      word_num = Terms->TE[termnum].WE.word_num;
      count = Terms->TE[termnum].WE.count;
      doc_count = Terms->TE[termnum].WE.doc_count;
      invf_ptr = Terms->TE[termnum].WE.invf_ptr;
      invf_len = Terms->TE[termnum].WE.invf_len;

      termnum++;
      mode = 2;
      return TERM;
    }
  else  /* mode == 2 */
    {
      if (termnum >= Terms->num)
	{
	  mode = 0;
	  return ')';
	}
      else
	{
	  mode = 1;
	  return '|';
	}
    }
}/*query_lex*/

/* =========================================================================
 * Function: yyerror
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */ 
static int yyerror(char *s)
{
  Message("%s", s);
  return(1);
}

 
/* =========================================================================
 * Function: ParseBool
 * Description:
 *      Parse a boolean query string into a term-list and a boolean parse tree
 * Input:
 *      query_line = query line string
 *      query_len = query line length
 *      the_stem_method = stem method id used for stemming
 * Output:
 *      the_term_list = the list of terms
 *      res = parser result code
 * ========================================================================= */
 
bool_tree_node *
ParseBool(char *query_line, int query_len,
          TermList **the_term_list, int the_stemmer_num, int the_stem_method, int *res,
	  stemmed_dict * the_sd, int is_indexed,   /* [RPAP - Jan 97: Stem Index Change] */
	  QueryTermList **the_query_term_list)  /* [RPAP - Feb 97: Term Frequency] */
{
  /* global variables to be accessed by bison/yacc created parser */
  term_list = the_term_list;
  stemmer_num = the_stemmer_num;
  stem_method = the_stem_method;
  ch_buf = query_line;
  end_buf = query_line + query_len;
  p__sd = the_sd;   /* [RPAP - Jan 97: Stem Index Change] */
  indexed = is_indexed;  /* [RPAP - Jan 97: Stem Index Change] */
  query_term_list = the_query_term_list; /* [RPAP - Feb 97: Term Frequency] */

  FreeBoolTree(&(tree_base));
 
  ResetTermList(term_list);
  ResetQueryTermList(query_term_list);  /* [RPAP - Feb 97: Term Frequency] */

  *res = yyparse();
 
  return tree_base;
}
 
